#include<poll.h>
#include"pollpool.h"
#include"managecenter.h"
#include"datatype.h"
#include"sockutil.h"
#include"msgcenter.h"
#include"lock.h"
#include"sockoper.h"
#include"usercenter.h"


PollPool::PollPool() : m_capacity(DEF_FD_MAX_CAPACITY){ 
    m_mng = NULL;
    m_usr_center = NULL;
    
    m_fds = NULL;
    m_lock = NULL;
    m_infos = NULL;
    m_event_data = NULL;
}

PollPool::~PollPool() {
}

Void PollPool::resetFd(FdInfo* info) {
    memset(info, 0, sizeof(FdInfo));

    INIT_LIST_NODE(&info->m_io_node);
    INIT_LIST_NODE(&info->m_deal_node);

    INIT_LIST_HEAD(&info->m_rd_que);
    INIT_LIST_HEAD(&info->m_wr_que); 
    INIT_LIST_HEAD(&info->m_deal_que);
    INIT_LIST_HEAD(&info->m_send_que);
    INIT_LIST_HEAD(&info->m_recv_que); 
    
    info->m_fd = -1;
    info->m_fd_status = ENUM_SOCK_INIT;
}

Void PollPool::finishFd(FdInfo* info) {
    
}

Int32 PollPool::init() {
    Int32 ret = 0;
    Int32 fd = -1;
    FdInfo* info = NULL;

    INIT_LIST_HEAD(&m_lock_list); 
    INIT_LIST_HEAD(&m_run_list);
    INIT_LIST_HEAD(&m_idle_list); 

    I_NEW(SpinLock, m_lock);
    ret = m_lock->init();
    if (0 != ret) {
        return -1;
    }

    ARR_NEW(struct pollfd, m_capacity, m_fds);
    memset(m_fds, 0, sizeof(struct pollfd) * m_capacity);

    ARR_NEW(FdInfo, m_capacity, m_infos);
    for (int i=0; i<m_capacity; ++i) {
        resetFd(&m_infos[i]);
    } 

    I_NEW_1(SockOper, m_sock_oper, m_mng); 

    fd = creatEventFd(); 
    if (0 > fd) { 
        return -1;
    }

    m_event_data = m_usr_center->creatData<EventData>();
    
    info = creatReader(fd, ENUM_RD_EVENT_CMD, m_event_data);
    m_event_data->m_fdinfo = info; 

    /* no need to lock in initializing step */
    list_add_back(&info->m_io_node, &m_run_list); 

    return ret;
}

Void PollPool::finish() {
    if (NULL != m_sock_oper) {
        I_FREE(m_sock_oper);
    }
    
    if (NULL != m_fds) { 
        ARR_FREE(m_fds);
    }

    if (NULL != m_infos) {
        for (int i=0; i<m_capacity; ++i) {
            finishFd(&m_infos[i]);
        } 
        
        ARR_FREE(m_infos);
    }

    if (NULL != m_lock) {
        m_lock->finish();
        I_FREE(m_lock);
    }
}

Void PollPool::set(ManageCenter* mng, UserCenter* usr_center) { 
    m_mng = mng;
    m_usr_center = usr_center;
}

FdInfo* PollPool::creatReader(Int32 fd, Int32 fd_type, Void* io) {
    FdInfo* info = NULL;

    if (0 <= fd && fd < m_capacity && 0 > m_infos[fd].m_fd) {
        info = &m_infos[fd];

        resetFd(info);
        
        info->m_fd = fd;
        
        info->m_rd_type = fd_type;
        info->m_wr_type = ENUM_WR_END;
        info->m_deal_type = ENUM_DEAL_END;
        
        info->m_io_data = io;
        
        info->m_test_rd = TRUE;
        info->m_test_wr = FALSE;
        return info;
    } else {
        return NULL;
    }
}

FdInfo* PollPool::creatSock(Int32 fd, Int32 fd_status,
    Int32 rd_type, Int32 wr_type, Int32 deal_type, 
    Void* io, Void* data) {
    FdInfo* info = NULL;

    if (0 <= fd && fd < m_capacity && 0 > m_infos[fd].m_fd) {
        info = &m_infos[fd];

        resetFd(info);
        
        info->m_fd = fd;

        info->m_rd_type = rd_type; 
        info->m_wr_type = wr_type;
        info->m_deal_type = deal_type;
        info->m_fd_status = fd_status;
        
        info->m_io_data = io;
        info->m_deal_data = data;
        
        info->m_test_rd = TRUE;
        info->m_test_wr = TRUE;
        return info;
    } else {
        return NULL;
    }
}

Void PollPool::addEvent(FdInfo* info) {
    Bool bOk = TRUE;

    bOk = lock();
    if (bOk) {
        list_add_back(&info->m_io_node, &m_lock_list); 
        unlock();

        signal();
    } else {
        finishFd(info);
    }
}

Void PollPool::delEvent(FdInfo* info) {
    LOG_ERROR("del_fd| fd=%d|", info->m_fd);
    
    list_del(&info->m_io_node, &m_run_list);
    list_add_back(&info->m_io_node, &m_idle_list); 
}

Int32 PollPool::fillEvent() { 
    list_node* pos = NULL;
    list_node* n = NULL;
    struct pollfd* pfd = NULL;
    FdInfo* info = NULL;
    Int32 ret = 0;
    Int32 cnt = 0;
    Bool bOk = FALSE;

    bOk = lock();
    if (bOk) {
        if (!list_empty(&m_lock_list)) {
            list_splice_back(&m_lock_list, &m_run_list);
        }
        unlock();
    }
    
    list_for_each_safe(pos, n, &m_run_list) {
        info = list_entry(pos, FdInfo, m_io_node);
        pfd = &m_fds[cnt]; 
        
        pfd->fd = info->m_fd;
        pfd->events = 0;
        pfd->revents = 0; 

        if (info->m_more_wr && info->m_can_wr) {
            ret = m_mng->writeInfo(info);
            if (0 != ret) {
                LOG_INFO("write_info| ret=%d| fd=%d|"
                    " msg=write error|",
                    ret, pfd->fd);
            
                info->m_wr_err = TRUE;
            }
        } 

        if (info->m_test_wr && !info->m_can_wr && !info->m_wr_err) { 
            pfd->events |= POLLOUT; 
        }

        if (info->m_test_rd) {
            pfd->events |= POLLIN;
        }

        ++cnt; 
    } 

    return cnt;
}

Int32 PollPool::waitEvent(Int32 cnt) { 
    static const Int32 ERR_FLAG = POLLERR | POLLHUP | POLLNVAL; 
    struct pollfd* pfd = NULL;
    FdInfo* info = NULL;
    Int32 ret = 0;
    
    ret = poll(m_fds, cnt, DEF_POLL_WAIT_MILLISEC);
    if (0 < ret) {
        ret = 0;
        
        for (int i=0; i<cnt; ++i) {
            pfd = &m_fds[i]; 
            info = &m_infos[pfd->fd]; 

            if (POLLOUT & pfd->revents) { 
                info->m_can_wr = TRUE; 

                /* write status change, then do once */
                ret = m_mng->writeInfo(info);
                if (0 != ret) {
                    LOG_INFO("write_info| ret=%d| fd=%d|"
                        " msg=write error|",
                        ret, pfd->fd);
                        
                    info->m_wr_err = TRUE;
                }
            }
            
            if (POLLIN & pfd->revents) {
                ret = m_mng->readInfo(info);
                if (0 != ret) {
                    LOG_INFO("read_info| ret=%d| fd=%d|"
                        " msg=read error|",
                        ret, pfd->fd);
                    
                    info->m_rd_err = TRUE;
                }
            } 
            
            if (ERR_FLAG & pfd->revents) { 
                info->m_rd_err = TRUE;
                info->m_wr_err = TRUE;
                
                LOG_ERROR("chk_event_poll| fd=%d| revent=0x%x|"
                    " error=poll error|",
                    pfd->fd, pfd->revents);
            }

            if (info->m_rd_err || info->m_wr_err) {
                delEvent(info);
                
                ret = m_mng->failInfo(info);
                if (0 != ret) {
                    break;
                }
            }
        }
    } else if (0 == ret || EINTR == errno) {
        ret = 0;
    } else {
        ret = -1;
    }

    return ret;
}

Int32 PollPool::run() { 
    Int32 ret = 0;
    
    while (isRun() && 0 == ret) {
        ret = pollEvent();
    }
    
    return ret;
}

Int32 PollPool::pollEvent() {
    Int32 ret = 0;
    Int32 cnt = 0; 
    
    cnt = fillEvent(); 
    ret = waitEvent(cnt);
    
    return ret;
} 

Void PollPool::signal() {
    Uint32 val = 0;

    val = ATOMIC_FETCH_INC(&m_event_data->m_event_atom);
    if (!val) {
        writeEvent(m_event_data->m_fdinfo->m_fd);
    }
}

Int32 PollPool::readEventCmd(EventData* data) {
    Bool bOk = TRUE;
    Uint32 val = 0;
    Uint32 old_atom = 0;
    list_head list;

    INIT_LIST_HEAD(&list);
    
    old_atom = ACCESS_ONCE(data->m_event_atom); 

    bOk = lock();
    if (bOk) {
        if (!list_empty(&data->m_cmd_que)) {
            list_move(&data->m_cmd_que, &list);
        } 

        unlock();
    }

    readEvent(data->m_fdinfo->m_fd, &val);
    
    bOk = CAS(&data->m_event_atom, old_atom, 0);
    if (!bOk) {
        writeEvent(data->m_fdinfo->m_fd);
    }

    if (!list_empty(&list)) {
        procEventCmd(&list); 
    }
    
    return 0;
}

Int32 PollPool::sendCmd(MsgHdr* msg) {
    Bool bOk = TRUE;
    Bool need = FALSE;    
 
    bOk = lock();
    if (bOk) {
        if (list_empty(&m_event_data->m_cmd_que)) {
            need = TRUE;
        }
        
        MsgCenter::notify(msg, &m_event_data->m_cmd_que);
        
        unlock();
        
        if (need) {
            signal();
        }
        
        return 0;
    } else {
        MsgCenter::free(msg);
        return -1;
    }
}

Int32 PollPool::procEventCmd(list_head* list) {
    list_node* pos = NULL;
    list_node* n = NULL;
    MsgHdr* msg = NULL;
  
    list_for_each_safe(pos, n, list) {
        list_del(pos, list);

        msg = MsgCenter::node2msg(pos);
        
        execCmd(msg);
        MsgCenter::free(msg);
    }    

    return 0;
}

Void PollPool::execCmd(MsgHdr* msg) {
}

Bool PollPool::lock() {
    return m_lock->lock();
}

Bool PollPool::unlock() {
    return m_lock->unlock();
}

Int32 PollPool::sendMsg(FdInfo* info, MsgHdr* msg) {
    Bool bOk = TRUE;
    Bool need = FALSE;

    MsgCenter::addCrc(msg);
 
    bOk = lock();
    if (bOk) {
        MsgCenter::notify(msg, &info->m_send_que);

        if (!info->m_more_wr) {
            info->m_more_wr = TRUE;

            need = TRUE;
        }
        
        unlock();
        
        if (need) {
            signal();
        }
        
        return 0;
    } else {
        MsgCenter::free(msg);
        return -1;
    }
}

/* return: 0: ok, -1: error */
Int32 PollPool::writeData(FdInfo* info, SockBase* sock) {
    Int32 ret = 0;
    Bool bOk = TRUE; 
    Bool finished = FALSE;

    while (!finished) { 
        ret = m_sock_oper->writeSock(info, sock);
        if (1 == ret) {
            /* if send all of wr_que, then try to get more */
            bOk = lock();
            if (bOk) {
                if (!list_empty(&info->m_send_que)) {
                    list_splice_back(&info->m_send_que, &info->m_wr_que);
                } else {
                    if (info->m_more_wr) {
                        info->m_more_wr = FALSE;
                    }

                    /* all finished */
                    finished = TRUE;
                }

                unlock();

                continue;
            } else { 
                break;
            }
        } else if (0 == ret) {
            info->m_can_wr = FALSE;
            break;
        } else {
            return -1;
        }
    }

    return 0;
}

Int32 PollPool::readData(FdInfo* info, SockBase* sock) {
    Int32 ret = 0;

    ret = m_sock_oper->readSock(info, sock); 
    return ret;
}

Int32 PollPool::readTcpRaw(FdInfo* info, SockBase* sock) {
    Int32 ret = 0;

    ret = m_sock_oper->readTcpRaw(info, sock);
    return ret;
}

