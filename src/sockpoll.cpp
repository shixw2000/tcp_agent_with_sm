#include<poll.h>
#include"sockpoll.h"
#include"sockutil.h"
#include"sockmng.h"
#include"datatype.h"


SockPoll::SockPoll(Int32 capacity, SockMng* mng) : m_capacity(capacity) {
    m_mng = mng;
    m_infos = NULL;

    m_fds = NULL; 
    m_event_fd = -1;
}

SockPoll::~SockPoll() {
}

Int32 SockPoll::init() {
    Int32 ret = 0;

    INIT_LIST_HEAD(&m_rd_root);
    INIT_LIST_HEAD(&m_run_root);

    ret = TaskThread::init();
    if (0 != ret) {
        return -1;
    } 

    ARR_NEW(struct pollfd, m_capacity, m_fds);
    memset(m_fds, 0, sizeof(struct pollfd) * m_capacity); 

    ARR_NEW(struct FdInfo, m_capacity, m_infos);
    for (int i=0; i<m_capacity; ++i) {
        resetFd(&m_infos[i]);
    }

    return ret;
}

Void SockPoll::finish() { 
    if (0 <= m_event_fd) { 
        m_event_fd = -1;
    }

    if (NULL != m_infos) { 
        ARR_FREE(m_infos);
    }
    
    if (NULL != m_fds) { 
        ARR_FREE(m_fds);
    } 

    TaskThread::finish();
}

FdInfo* SockPoll::creatFd(Int32 fd, Bool testRd, Bool testWr) {
    FdInfo* info = NULL;

    if (0 <= fd && fd < m_capacity && 0 > m_infos[fd].m_fd) {
        info = &m_infos[fd];

        resetFd(info);
        
        info->m_fd = fd; 
        info->m_test_rd = testRd;
        info->m_test_wr = testWr;
        
        return info;
    } else {
        return NULL;
    }
}

Void SockPoll::resetFd(FdInfo* info) {
    memset(info, 0, sizeof(FdInfo));

    info->m_fd = -1;

    INIT_TASK(&info->m_wr_task);
    INIT_TASK(&info->m_deal_task);
    INIT_TASK(&info->m_mng_task);

    INIT_LIST_NODE(&info->m_run_node);
    INIT_LIST_NODE(&info->m_rd_node);

    INIT_LIST_HEAD(&info->m_rd_que);
    INIT_LIST_HEAD(&info->m_wr_que); 
    INIT_LIST_HEAD(&info->m_deal_que);
    INIT_LIST_HEAD(&info->m_send_que);
    INIT_LIST_HEAD(&info->m_recv_que); 
}

int SockPoll::setup() {
    if (0 <= m_event_fd) {
        return 0;
    } else {
        return -1;
    }
}

void SockPoll::teardown() {
}

Int32 SockPoll::fillEvent() { 
    list_node* pos = NULL;
    struct pollfd* pfd = NULL;
    FdInfo* info = NULL;
    Int32 cnt = 0;
  
    list_for_each(pos, &m_run_root) {
        info = list_entry(pos, FdInfo, m_run_node);

        pfd = &m_fds[cnt]; 
    
        pfd->fd = info->m_fd;
        pfd->events = 0;
        pfd->revents = 0;

        if (info->m_test_wr && !info->m_can_wr) { 
            pfd->events |= POLLOUT; 
        }

        if (info->m_test_rd) {
            pfd->events |= POLLIN;
        }

        if (0 != pfd->events) {
            ++cnt; 
        }
    }

    return cnt;
}

Int32 SockPoll::waitEvent(Int32 cnt, Int32 timeout) { 
    static const Int32 ERR_FLAG = POLLERR | POLLHUP | POLLNVAL; 
    struct pollfd* pfd = NULL;
    FdInfo* info = NULL;
    int ret = 0;
    
    ret = poll(m_fds, cnt, timeout);
    if (0 < ret) {        
        for (int i=0; i<cnt; ++i) {
            pfd = &m_fds[i]; 
            info = &m_infos[pfd->fd]; 
            
            if (POLLIN & pfd->revents) {
                list_add_back(&info->m_rd_node, &m_rd_root);
            } 

            if (POLLOUT & pfd->revents) { 
                info->m_can_wr = true;
                
                addTask(&info->m_wr_task, BIT_EVENT_WRITE);
            }

            if (ERR_FLAG & pfd->revents) { 
                LOG_INFO("chk_event_poll| fd=%d| revent=0x%x|"
                    " error=poll error|",
                    pfd->fd, pfd->revents); 
                                
                info->m_rd_err = TRUE;
                info->m_wr_err = TRUE;
                closeTask(info);
            }
        }

        ret = 0;
    } else if (0 == ret || EINTR == errno) {
        ret = 0;
    } else {
        ret = -1;
    }

    return ret;
}

Int32 SockPoll::pollEvent(int timeout) {
    Int32 ret = 0;
    Int32 cnt = 0; 
    
    cnt = fillEvent(); 
    ret = waitEvent(cnt, timeout);
    
    return ret;
} 

Void SockPoll::alarm() {
    writeEvent(m_event_fd);
}

Void SockPoll::wait() {
    pollEvent(DEF_POLL_WAIT_MILLISEC);
}

Void SockPoll::check() {
    pollEvent(0);
}

void SockPoll::preTasks() {
    int ret = 0;
    list_node* pos = NULL;
    list_node* n = NULL;
    FdInfo* info = NULL;
    
    if (!list_empty(&m_rd_root)) {
        list_for_each_safe(pos, n, &m_rd_root) {
            list_del(pos, &m_rd_root);
            
            info = list_entry(pos, FdInfo, m_rd_node);

            ret = readMsg(info);
            if (0 != ret) {
                info->m_rd_err = TRUE;
                closeTask(info);
            }
        }
    }
}

unsigned int SockPoll::procTask(struct Task* task) {
    int ret = 0;
    FdInfo* info = NULL;

    info = list_entry(task, FdInfo, m_wr_task);
    
    if (info->m_io_run) { 
        ret = writeMsg(info);
        if (0 == ret) {
            return BIT_EVENT_NORM;
        } else if (1 == ret) {
            /* here need to writable */
            info->m_can_wr = FALSE; 
            
            return BIT_EVENT_WRITE;
        } else {
            info->m_wr_err = TRUE;
            closeTask(info);

            /* here stop write */
            return BIT_EVENT_ERROR;
        }
    } else {
        /* add a new info */
        list_add_back(&info->m_run_node, &m_run_root); 

        info->m_io_run = TRUE; 
        return BIT_EVENT_NORM;
    } 
}

void SockPoll::closeTask(FdInfo* info) {     
    if (info->m_rd_err && info->m_test_rd) {
        info->m_test_rd = FALSE;
    }

    if (info->m_wr_err && info->m_test_wr) {
        info->m_test_wr = FALSE;
        info->m_can_wr = FALSE;
    }
    
    m_mng->closeMng(info); 
}

void SockPoll::procTaskEnd(struct Task* task) {
    FdInfo* info = NULL;

    info = list_entry(task, FdInfo, m_wr_task);

    LOG_INFO("del_fd| fd=%d|", info->m_fd); 
    
    list_del(&info->m_run_node, &m_run_root);

    if (info->m_can_wr) {
        /* send the remainder msgs */
        writeMsg(info);
    }
    
    if (ENUM_NODE_SESS_ACCPT <= info->m_fd_type
        && ENUM_NODE_USR_CONN >= info->m_fd_type) {
        /* sock data */
        shutdownHd(info->m_fd);
    }  

    m_mng->notify(info, ENUM_MSG_SYSTEM_IO_END); 
    return;
}

/* return: 0-ok, 1-blocking write, other: error */
Int32 SockPoll::writeMsg(FdInfo* info) {
    Int32 ret = 0;

    ret = m_mng->writeMsg(info);
    return ret;
}

/* return: 0-ok, other: error */
Int32 SockPoll::readMsg(FdInfo* info) {
    Int32 ret = 0;
    
    ret = m_mng->readMsg(info); 
    return ret;
}

