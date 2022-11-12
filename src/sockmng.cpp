#include"sockmng.h"
#include"datatype.h"
#include"lock.h"
#include"sockcenter.h"
#include"sockdealer.h"
#include"sockpoll.h"
#include"msgcenter.h"
#include"sockutil.h"
#include"msgtype.h"
#include"ticktimer.h"
#include"cthread.h"


SockMng::SockMng(int capacity) : m_capacity(capacity) {
    m_obj = NULL;
    m_lock = NULL;
    m_poll = NULL;
    m_dealer = NULL; 
}

SockMng::~SockMng() {
}

Int32 SockMng::init() { 
    Int32 ret = 0;

    ret = TaskPool::init();
    if (0 != ret) {
        return ret;
    }
    
    I_NEW_1(GrpSpinLock, m_lock, DEF_LOCK_ORDER);
    ret = m_lock->init();
    if (0 != ret) {
        return ret;
    }

    I_NEW_1(SockPoll, m_poll, m_capacity);
    ret = m_poll->init();
    if (0 != ret) {
        return ret;
    }

    I_NEW(SockDealer, m_dealer);
    ret = m_dealer->init();
    if (0 != ret) {
        return ret;
    }

    m_poll->set(this); 
    m_dealer->set(this); 
    return ret;
}

Void SockMng::finish() { 
    if (NULL != m_dealer) {
        m_dealer->finish();
        I_FREE(m_dealer);
    }

    if (NULL != m_poll) {
        m_poll->finish();
        I_FREE(m_poll);
    }
    
    if (NULL != m_lock) {
        m_lock->finish();
        I_FREE(m_lock);
    } 

    TaskPool::finish();
}

Void SockMng::set(I_FdObj* obj) {
    m_obj = obj;
}

int SockMng::start(const char* name) {
    int ret = 0; 

    ret = TaskPool::start(name);
    if (0 != ret) {
        return -1;
    }

    ret = m_dealer->start("dealer");
    if (0 != ret) {
        TaskPool::stop();
        return -1;
    }

    ret = m_poll->start("poll");
    if (0 != ret) {
        m_dealer->stop();
        TaskPool::stop();
        return -1;
    } 

    return 0;
}

void SockMng::join() {
    TaskPool::join();

    m_poll->stop();
    m_dealer->stop();
    
    m_dealer->join();
    m_poll->join();
}

void SockMng::stop() {
    m_poll->stop();
    m_dealer->stop();
    TaskPool::stop();
}

unsigned int SockMng::procTask(struct Task* task) {
    FdInfo* info = NULL;

    info = list_entry(task, FdInfo, m_mng_task);
    
    return BIT_EVENT_NORM;
}

void SockMng::procTaskEnd(struct Task* task) {
    FdInfo* info = NULL;
    
    info = list_entry(task, FdInfo, m_mng_task); 
    
    closePoll(info); 
    return;
}

void SockMng::closePoll(struct FdInfo* info) {
    m_poll->endTask(&info->m_wr_task); 
}

void SockMng::closeDealer(struct FdInfo* info) {
    m_dealer->endTask(&info->m_deal_task); 
}

void SockMng::closeMng(struct FdInfo* info) {
    endTask(&info->m_mng_task); 
}

Void SockMng::addEvent(FdInfo* info, NodeBase* base) {
    info->m_fd_type = base->m_node_type;
    info->m_data = base;
    
    m_poll->addTask(&info->m_wr_task, BIT_EVENT_NORM);

    LOG_DEBUG("add_event| fd=%d| type=%d|", info->m_fd, info->m_fd_type);
}

FdInfo* SockMng::creatBase(Int32 fd, Bool testRd, Bool testWr) {
    FdInfo* info = NULL;
    
    info = m_poll->creatFd(fd, testRd, testWr); 
    return info;
}

Int32 SockMng::sendMsg(FdInfo* info, MsgHdr* msg) {
    Bool bOk = TRUE;

    if (!info->m_wr_err) {
        bOk = lock(info);
        if (bOk) {
            MsgCenter::notify(msg, &info->m_send_que);
            unlock(info);

            m_poll->addTask(&info->m_wr_task, BIT_EVENT_NORM);

            return 0;
        } else {
            MsgCenter::free(msg);
            return -1;
        }
    } else {
        MsgCenter::free(msg);
        return -1;
    }
}

Int32 SockMng::writeMsg(FdInfo* info) {
    Int32 ret = 0;
    Bool bOk = TRUE;
    
    bOk = lock(info);
    if (bOk) {
        if (!list_empty(&info->m_send_que)) {
            list_splice_back(&info->m_send_que, &info->m_wr_que);
        }

        unlock(info);
    }

    ret = m_obj->writeFd(info); 
    
    return ret;
}

Int32 SockMng::readMsg(FdInfo* info) {
    Int32 ret = 0;
    
    ret = m_obj->readFd(info); 
    
    return ret;
}

Int32 SockMng::dispatch(FdInfo* info, MsgHdr* msg) {
    Bool bOk = TRUE;

    MsgCenter::chkCrc(msg);

    bOk = lock(info);
    if (bOk) {
        MsgCenter::notify(msg, &info->m_recv_que);

        unlock(info);

        m_dealer->addTask(&info->m_deal_task, BIT_EVENT_NORM);

        return 0;
    } else {
        MsgCenter::free(msg);
        return -1;
    }
}

Void SockMng::dealMsg(FdInfo* info) {
    Int32 ret = 0;
    bool bOk = true;
    list_node* pos = NULL;
    list_node* n = NULL;
    MsgHdr* msg = NULL;

    bOk = lock(info);
    if (bOk) {
        if (!list_empty(&info->m_recv_que)) {
            list_splice_back(&info->m_recv_que, &info->m_deal_que);
        }

        unlock(info);
    }

    if (!list_empty(&info->m_deal_que)) {
        list_for_each_safe(pos, n, &info->m_deal_que) {
            list_del(pos, &info->m_deal_que);

            msg = MsgCenter::node2msg(pos);

            if (ENUM_MSG_SYSTEM_EXIT < msg->m_cmd) {
                if (!info->m_deal_err) {
                    /* normal deal */
                    ret = procMsg(info, msg);
                    if (0 != ret) {
                        info->m_deal_err = TRUE;
                        closePoll(info);
                    }
                } else {
                    MsgCenter::free(msg); 
                }
            } else if (ENUM_MSG_SYSTEM_EXIT > msg->m_cmd) {
                /* system cmd [0, 100] */
                ret = procMsg(info, msg);
                if (0 != ret && !info->m_deal_err) {
                    info->m_deal_err = TRUE;
                    closePoll(info);
                }
            } else {
                MsgCenter::free(msg); 
                
                /* if cmd == exit, end deal task */
                closeDealer(info); 
                break;
            } 
        }
    }

    return;
}

Int32 SockMng::procMsg(FdInfo* info, MsgHdr* msg) {
    Int32 ret = 0;
    
    ret = m_obj->procMsg(info, msg);
    return ret;
}

Void SockMng::endFd(FdInfo* info) {
    m_obj->eof(info);
}

Int32 SockMng::notify(FdInfo* info, Uint16 cmd, Uint64 data) {
    Int32 ret = 0;
    MsgHdr* hdr = NULL;
    MsgNotify* msg = NULL;

    hdr = MsgCenter::creat<MsgNotify>(cmd);
    msg = MsgCenter::cast<MsgNotify>(hdr);
    msg->m_data = data;

    MsgCenter::addCrc(hdr);

    ret = dispatch(info, hdr);
    return ret;
}

Bool SockMng::lock(FdInfo* info) {
    int idx = info->m_fd;

    return m_lock->lock(idx);
}

Bool SockMng::unlock(FdInfo* info) {
    int idx = info->m_fd;

    return m_lock->unlock(idx);
}

Void SockMng::doIoTick(Uint32 val) {
    m_poll->doTick(val);
}

Void SockMng::doProcTick(Uint32 val) {
    m_dealer->doTick(val);
}

void SockMng::addIoTimer(struct TimerEle* ele, 
    Int32 type, Uint32 interval) { 
    m_poll->addTimer(ele, type, interval);
}

void SockMng::addProcTimer(struct TimerEle* ele, 
    Int32 type, Uint32 interval) { 
    m_dealer->addTimer(ele, type, interval);
}

Int32 SockMng::sendHeartBeat(FdInfo* info, Uint64 data) {
    Int32 ret = 0;
    MsgHdr* hdr = NULL;
    MsgNotify* msg = NULL;

    hdr = MsgCenter::creat<MsgNotify>(ENUM_MSG_HEART_BEAT);
    msg = MsgCenter::cast<MsgNotify>(hdr);

    msg->m_data = data;
    
    MsgCenter::addCrc(hdr);

    LOG_DEBUG("send_heart_beat| fd=%d| fd_type=%d|", 
        info->m_fd, info->m_fd_type);

    ret = sendMsg(info, hdr);
    return ret;
}

