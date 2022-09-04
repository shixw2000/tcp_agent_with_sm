#include"sockutil.h"
#include"sockdealer.h"
#include"managecenter.h"
#include"datatype.h"
#include"lock.h"
#include"msgcenter.h"
#include"msgtype.h"


SockDealer::SockDealer(ManageCenter* mng) {
    m_mng = mng;
    m_cond = NULL;
    m_stoped = FALSE;

    INIT_LIST_HEAD(&m_lock_list);
}

SockDealer::~SockDealer() {
}

Int32 SockDealer::init() { 
    Int32 ret = 0;
    
    do {
        I_NEW(MutexCond, m_cond);
        ret = m_cond->init();
        if (0 != ret) {
            break;
        }
        
        return 0;
    } while (0);
    
    return ret;
}

Void SockDealer::finish() { 
    if (NULL != m_cond) {
        m_cond->finish();
        I_FREE(m_cond);
    }
}

Int32 SockDealer::work() {
    Int32 ret = 0;
    Bool bOk = TRUE;
    FdInfo* info = NULL; 

    if (!m_stoped) {
        bOk = m_cond->lock();
        if (bOk) {
            if (!list_empty(&m_lock_list)) {
                info = list_first_entry(&m_lock_list, FdInfo, m_deal_node);
                list_del(&info->m_deal_node, &m_lock_list);
            } else {
                if (!m_stoped) {
                    m_cond->wait();
                }
            }
            
            m_cond->unlock();
        }

        if (NULL != info) {
            ret = dealFd(info);
        }
    } else {
        /* mark to end the loop running */
        ret = 1;
    }

    return ret;
}

Int32 SockDealer::dispatchMsg(FdInfo* info, MsgHdr* msg) {
    Bool bOk = TRUE;

    MsgCenter::chkCrc(msg);
 
    bOk = m_cond->lock();
    if (bOk) {        
        MsgCenter::notify(msg, &info->m_recv_que);

        if (!info->m_is_dealing) {
            info->m_is_dealing = TRUE;
            
            addRunQue(info);
        }
        
        m_cond->unlock();
        
        return 0;
    } else {
        MsgCenter::free(msg);
        return -1;
    }
}

Int32 SockDealer::dealFd(FdInfo* info) {
    Bool bOk = TRUE;
    Bool finished = FALSE;
    list_node* pos = NULL;
    MsgHdr* msg = NULL;

    while (!finished && !m_stoped) {
        if (!list_empty(&info->m_deal_que)) {
            pos = LIST_FIRST(&info->m_deal_que);
            list_del(pos, &info->m_deal_que);

            msg = MsgCenter::node2msg(pos);
            
            m_mng->procMsg(info, msg); 
        } else {
            bOk = m_cond->lock();
            if (bOk) {
                if (!list_empty(&info->m_recv_que)) {
                    list_splice_back(&info->m_recv_que, &info->m_deal_que);
                } else {
                    if (info->m_is_dealing) {
                        info->m_is_dealing = FALSE;
                    }

                    finished = TRUE;
                }

                m_cond->unlock();
            }
        }
    }
    
    return 0;
}

Void SockDealer::addRunQue(FdInfo* info) {
    list_add_back(&info->m_deal_node, &m_lock_list);
    m_cond->signal();
}

void SockDealer::stopWork() {
    Bool bOk = TRUE; 

    m_stoped = TRUE;
    
    bOk = m_cond->lock();
    if (bOk) { 
        m_cond->broadcast();
        m_cond->unlock();
    }
}

