#include"managecenter.h"
#include"sockutil.h"
#include"pollpool.h"
#include"usercenter.h"
#include"msgcenter.h"
#include"ticktimer.h"
#include"sockdealer.h"
#include"sockoper.h"
#include"cthread.h"
#include"authuser.h"
#include"authsess.h"
#include"authadmin.h"
#include"authrouter.h"


template<>
Int32 ManageCenter::readFd<ENUM_RD_EVENT_CMD>(FdInfo* info) {
    Int32 ret = 0;
    EventData* data = (EventData*)info->m_io_data;

    ret = m_poll_pool->readEventCmd(data); 
    return ret;
}

template<>
Int32 ManageCenter::readFd<ENUM_RD_TIMER>(FdInfo* info) {
    Int32 ret = 0;
    Uint32 val = 0;
    TimerData* data = (TimerData*)info->m_io_data;
   
    ret = readEvent(info->m_fd, &val);
    if (0 == ret) {
        data->m_timer->tick(val);
    }
    
    return 0;
}

template<>
Int32 ManageCenter::readFd<ENUM_RD_TCP_LISTENER>(FdInfo* info) {
    Int32 ret = 0;
    Int32 newfd = -1; 
    ListenerTcp* listener = (ListenerTcp*)info->m_io_data;
    
    newfd = acceptCli(info->m_fd);
    while (0 <= newfd) { 
        ret = addSess(listener, newfd); 
        if (0 != ret) {
            closeHd(newfd);
        }
        
        newfd = acceptCli(info->m_fd);
    }

    return 0;
}

template<>
Int32 ManageCenter::readFd<ENUM_RD_DIRTY_LISTENER>(FdInfo* info) {
    Int32 ret = 0;
    Int32 newfd = -1; 
    ListenerDirty* listener = (ListenerDirty*)info->m_io_data;
    
    newfd = acceptCli(info->m_fd);
    while (0 <= newfd) { 
        ret = addUser(listener, newfd); 
        if (0 != ret) {
            closeHd(newfd);
        }
        
        newfd = acceptCli(info->m_fd);
    }

    return 0;
}

template<>
Int32 ManageCenter::readFd<ENUM_RD_ROUTER_LISTENER>(FdInfo* info) {
    Int32 ret = 0;
    Int32 newfd = -1; 
    ListenerRouter* listener = (ListenerRouter*)info->m_io_data;
    
    newfd = acceptCli(info->m_fd);
    while (0 <= newfd) { 
        ret = addRouter(listener, newfd);
        if (0 != ret) {
            closeHd(newfd);
        }
        
        newfd = acceptCli(info->m_fd);
    }

    return 0;
}

template<>
Int32 ManageCenter::readFd<ENUM_RD_ADMIN_LISTENER>(FdInfo* info) {
    Int32 ret = 0;
    Int32 newfd = -1; 
    ListenerAdmin* listener = (ListenerAdmin*)info->m_io_data;
    
    newfd = acceptCli(info->m_fd);
    while (0 <= newfd) { 
        ret = addAdmin(listener, newfd); 
        if (0 != ret) {
            closeHd(newfd);
        }
        
        newfd = acceptCli(info->m_fd);
    }

    return ret;
}

template<>
Int32 ManageCenter::readFd<ENUM_RD_TCP_RAW>(FdInfo* info) {
    Int32 ret = 0;
    SockBase* sock = (SockBase*)info->m_io_data;

    ret = m_poll_pool->readTcpRaw(info, sock);
    return ret;
}

template<>
Int32 ManageCenter::readFd<ENUM_RD_DIRTY_MSG>(FdInfo* info) {
    Int32 ret = 0;
    SockBase* sock = (SockBase*)info->m_io_data;

    ret = m_poll_pool->readData(info, sock); 
    return ret;
}

template<>
Int32 ManageCenter::writeFd<ENUM_WR_SOCK_MSG>(FdInfo* info) {
    Int32 ret = 0;
    SockBase* sock = (SockBase*)info->m_io_data;

    ret = m_poll_pool->writeData(info, sock); 
    return ret;
}

template<>
Int32 ManageCenter::writeFd<ENUM_WR_TCP_CONNECTING>(FdInfo* info) {
    Int32 ret = 0;
    SessionConn* sess = (SessionConn*)info->m_deal_data;
    UserAccpt* usrAccpt = sess->m_parent;

    ret = chkConnStatus(info->m_fd);
    if (0 == ret) {
        info->m_test_rd = TRUE;
        info->m_wr_type = ENUM_WR_SOCK_MSG;

        LOG_DEBUG("chk_tcp_connection| fd=%d| msg=ok|", info->m_fd);

        ret = m_usr_center->sendSessArrival(usrAccpt->m_fdinfo,
            usrAccpt->m_user_id, sess->m_session_id); 

        return 0;
    } else {
        LOG_ERROR("chk_tcp_connection| fd=%d| msg=invalid|", info->m_fd);
        
        return -1;
    }
}

template<>
Int32 ManageCenter::writeFd<ENUM_WR_DIRTY_CONNECTING>(FdInfo* info) {
    Int32 ret = 0;
    UserConn* usrConn = (UserConn*)info->m_deal_data;

    ret = chkConnStatus(info->m_fd);
    if (0 == ret) {
        info->m_test_rd = TRUE;
        info->m_wr_type = ENUM_WR_SOCK_MSG;

        ret = m_usr_center->startAuth(usrConn);

        return ret;
    } else {
        LOG_ERROR("chk_dirty_connection| fd=%d| msg=invalid|", info->m_fd);
        return -1;
    }
}

template<>
Int32 ManageCenter::writeFd<ENUM_WR_ROUTER_CONNECTING>(FdInfo* info) {
    Int32 ret = 0;
    RouterPair* routerPair = (RouterPair*)info->m_deal_data;

    ret = chkConnStatus(info->m_fd);
    if (0 == ret) {
        /* update out router to ready */
        info->m_test_rd = TRUE;
        info->m_wr_type = ENUM_WR_SOCK_MSG;

        ret = m_usr_center->startRouterAuth(routerPair);

        return ret;
    } else {
        LOG_ERROR("chk_router_connection| fd=%d| msg=invalid|", info->m_fd);
        return -1;
    }
}

template<>
Int32 ManageCenter::dealFd<ENUM_DEAL_SESSION_IN>(FdInfo* info,
    MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    SessionAccpt* sessAccpt = (SessionAccpt*)info->m_deal_data;

    ret = m_deal_sess_accpt->process(sessAccpt, msg, pDel);
    return ret;
}

template<>
Int32 ManageCenter::dealFd<ENUM_DEAL_SESSION_OUT>(FdInfo* info, 
    MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    SessionConn* sessConn = (SessionConn*)info->m_deal_data;
    
    ret = m_deal_sess_conn->process(sessConn, msg, pDel);
    return ret;
}

template<>
Int32 ManageCenter::dealFd<ENUM_DEAL_USER_IN>(FdInfo* info, 
    MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    UserAccpt* usrAccpt = (UserAccpt*)info->m_deal_data;

    ret = m_deal_usr_accpt->process(usrAccpt, msg, pDel);
    return ret;
}

template<>
Int32 ManageCenter::dealFd<ENUM_DEAL_USER_OUT>(FdInfo* info, 
    MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    UserConn* usrConn = (UserConn*)info->m_deal_data;

    ret = m_deal_usr_conn->process(usrConn, msg, pDel);
    return ret;
}

template<>
Int32 ManageCenter::dealFd<ENUM_DEAL_ADMIN_IN>(FdInfo* info, 
    MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    AdminAccpt* adminAccpt = (AdminAccpt*)info->m_deal_data;

    ret = m_deal_admin_accpt->process(adminAccpt, msg, pDel);
    return ret;
}

template<>
Int32 ManageCenter::dealFd<ENUM_DEAL_ADMIN_OUT>(FdInfo* info, 
    MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    AdminConn* adminConn = (AdminConn*)info->m_deal_data;

    ret = m_deal_admin_conn->process(adminConn, msg, pDel);
    return ret;
}

template<>
Int32 ManageCenter::dealFd<ENUM_DEAL_ROUTER_IN>(FdInfo* info, 
    MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    RouterPair* routerPair = (RouterPair*)info->m_deal_data;

    ret = m_deal_router->processIn(routerPair, msg, pDel);
    return ret;
}

template<>
Int32 ManageCenter::dealFd<ENUM_DEAL_ROUTER_OUT>(FdInfo* info, 
    MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    RouterPair* routerPair = (RouterPair*)info->m_deal_data;

    ret = m_deal_router->processOut(routerPair, msg, pDel);
    return ret;
}

const ManageCenter::PReader ManageCenter::m_rd_funcs[ENUM_RD_END] = {
    &ManageCenter::readFd<ENUM_RD_EVENT_CMD>,
    &ManageCenter::readFd<ENUM_RD_TIMER>,
    &ManageCenter::readFd<ENUM_RD_TCP_LISTENER>,
    &ManageCenter::readFd<ENUM_RD_DIRTY_LISTENER>,
    &ManageCenter::readFd<ENUM_RD_ROUTER_LISTENER>,
    &ManageCenter::readFd<ENUM_RD_ADMIN_LISTENER>,
    &ManageCenter::readFd<ENUM_RD_TCP_RAW>,
    &ManageCenter::readFd<ENUM_RD_DIRTY_MSG>
};

const ManageCenter::PWriter ManageCenter::m_wr_funcs[ENUM_WR_END] = {
    &ManageCenter::writeFd<ENUM_WR_TCP_CONNECTING>,
    &ManageCenter::writeFd<ENUM_WR_DIRTY_CONNECTING>,
    &ManageCenter::writeFd<ENUM_WR_ROUTER_CONNECTING>,
    &ManageCenter::writeFd<ENUM_WR_SOCK_MSG>
};

const ManageCenter::PDealer ManageCenter::m_deal_funcs[ENUM_DEAL_END] = {
    &ManageCenter::dealFd<ENUM_DEAL_SESSION_IN>,
    &ManageCenter::dealFd<ENUM_DEAL_SESSION_OUT>,
    
    &ManageCenter::dealFd<ENUM_DEAL_USER_IN>,
    &ManageCenter::dealFd<ENUM_DEAL_USER_OUT>,

    &ManageCenter::dealFd<ENUM_DEAL_ROUTER_IN>,
    &ManageCenter::dealFd<ENUM_DEAL_ROUTER_OUT>,

    &ManageCenter::dealFd<ENUM_DEAL_ADMIN_IN>,
    &ManageCenter::dealFd<ENUM_DEAL_ADMIN_OUT>,
};

ManageCenter::ManageCenter() {
    m_dealer = NULL;
    
    m_poll_pool = NULL;
    m_usr_center = NULL;

    m_deal_usr_accpt = NULL;
    m_deal_usr_conn = NULL;

    m_deal_sess_accpt = NULL;
    m_deal_sess_conn = NULL;

    m_deal_admin_accpt = NULL;
    m_deal_admin_conn = NULL;

    m_deal_router = NULL;

    INIT_LIST_HEAD(&m_listener_list);
}

ManageCenter::~ManageCenter() {
} 

Int32 ManageCenter::init() {
    Int32 ret = 0;

    do { 
        I_NEW_1(UserCenter, m_usr_center, this);

        I_NEW(PollPool, m_poll_pool);
        m_poll_pool->set(this, m_usr_center);
        ret = m_poll_pool->init();
        if (0 != ret) {
            break;
        } 

        I_NEW_2(UsrAccptAuth, m_deal_usr_accpt, this, m_usr_center);
        I_NEW_2(UsrConnAuth, m_deal_usr_conn, this, m_usr_center);

        I_NEW_2(SessAccptAuth, m_deal_sess_accpt, this, m_usr_center);
        I_NEW_2(SessConnAuth, m_deal_sess_conn, this, m_usr_center);

        I_NEW_2(AdminAccptAuth, m_deal_admin_accpt, this, m_usr_center);
        I_NEW_2(AdminConnAuth, m_deal_admin_conn, this, m_usr_center); 

        I_NEW_2(RouterDealer, m_deal_router, this, m_usr_center); 

        return 0;
    } while (0);

    return ret;
}

Void ManageCenter::finish() { 
    if (!list_empty(&m_listener_list)) {
    }

    if (NULL != m_deal_router) {        
        I_FREE(m_deal_router);
    }

    if (NULL != m_deal_usr_accpt) {        
        I_FREE(m_deal_usr_accpt);
    }

    if (NULL != m_deal_usr_conn) {        
        I_FREE(m_deal_usr_conn);
    }

    if (NULL != m_deal_sess_accpt) {        
        I_FREE(m_deal_sess_accpt);
    }

    if (NULL != m_deal_sess_conn) {        
        I_FREE(m_deal_sess_conn);
    }

    if (NULL != m_deal_admin_accpt) {        
        I_FREE(m_deal_admin_accpt);
    }

    if (NULL != m_deal_admin_conn) {        
        I_FREE(m_deal_admin_conn);
    } 

    if (NULL != m_poll_pool) {
        m_poll_pool->finish();
        I_FREE(m_poll_pool);
    }

    if (NULL != m_usr_center) {        
        I_FREE(m_usr_center);
    }
}

Void ManageCenter::set(SockDealer* dealer) {
    m_dealer = dealer;
}

Int32 ManageCenter::start() {
    Int32 ret = 0;
    
    ret = m_poll_pool->start("msg_poll"); 
    return ret;
}

Void ManageCenter::stop() { 
    m_poll_pool->stop();
    m_poll_pool->signal();
}

Void ManageCenter::wait() {
    m_poll_pool->join();
}

Int32 ManageCenter::readInfo(FdInfo* info) {
    Int32 ret = 0;

    ret = (this->*(m_rd_funcs[info->m_rd_type]))(info);
    return ret;
}

Int32 ManageCenter::writeInfo(FdInfo* info) {
    Int32 ret = 0;

    ret = (this->*(m_wr_funcs[info->m_wr_type]))(info);
    return ret;
}

Int32 ManageCenter::procMsg(FdInfo* info, MsgHdr* msg) {
    Int32 ret = 0;
    Bool bDel = TRUE;

    ret = (this->*(m_deal_funcs[info->m_deal_type]))(info, msg, &bDel); 

    if (bDel) {
        MsgCenter::free(msg);    
    }
    
    return ret;
}

Void ManageCenter::closeFd(FdInfo* info) {
    m_poll_pool->finishFd(info);
}

Int32 ManageCenter::sendMsg(FdInfo* info, MsgHdr* msg) {
    Int32 ret = 0;

    ret = m_poll_pool->sendMsg(info, msg);
    return ret;
}

Int32 ManageCenter::sendCmd(MsgHdr* msg) {
    Int32 ret = 0;

    ret = m_poll_pool->sendCmd(msg);
    return ret;
}

Int32 ManageCenter::dispatchMsg(FdInfo* info, MsgHdr* msg) {
    Int32 ret = 0;

    ret = m_dealer->dispatchMsg(info, msg);;
    return ret;
}

Int32 ManageCenter::addUser(ListenerDirty* listener, Int32 new_fd) {
    UserAccpt* user = NULL;
    FdInfo* info = NULL;

    user = m_usr_center->creatData<UserAccpt>();
    info = m_poll_pool->creatSock(new_fd, ENUM_RD_DIRTY_MSG, 
        ENUM_WR_SOCK_MSG, ENUM_DEAL_USER_IN, &user->m_sock, user);

    user->m_parent = listener;
    user->m_fdinfo = info;
    user->m_user_id = ++m_last_usr_id;
    user->m_usr_status = ENUM_USER_WAIT_REQ;
    list_add_back(&user->m_base.m_node, &listener->m_usr_que);

    m_poll_pool->addEvent(info);
            
    return 0;
}

Int32 ManageCenter::addRouter(ListenerRouter* listener, Int32 new_fd) {
    RouterPair* router = NULL;
    FdInfo* info = NULL;

    /* first to check if a valid login, then start a router connection */
    router = m_usr_center->creatData<RouterPair>(); 
    router->m_parent = listener;
    router->m_router_id = ++m_last_router_id; 
  
    info = m_poll_pool->creatSock(new_fd, ENUM_RD_DIRTY_MSG,
        ENUM_WR_SOCK_MSG, ENUM_DEAL_ROUTER_IN, 
        &router->m_router_in.m_sock, router); 
       
    router->m_router_in.m_fdinfo = info; 
    router->m_router_in.m_usr_status = ENUM_USER_WAIT_REQ;
    
    list_add_back(&router->m_base.m_node, &listener->m_route_pair_que);
    
    m_poll_pool->addEvent(info); 
            
    return 0;
}

Int32 ManageCenter::addSess(ListenerTcp* listener, Int32 new_fd) { 
    SessionAccpt* sess = NULL;
    FdInfo* info = NULL;
    UserConn* usrConn = listener->m_user;

    if (!listener->m_tunnel_ok) {
        return 1;
    }

    sess = m_usr_center->creatData<SessionAccpt>();
    info = m_poll_pool->creatSock(new_fd, ENUM_RD_TCP_RAW, 
        ENUM_WR_SOCK_MSG, ENUM_DEAL_SESSION_IN, 
        &sess->m_sock, sess);

    /* disable read until sess arrival */
    info->m_test_rd = FALSE;
    
    sess->m_parent = usrConn;
    sess->m_fdinfo = info;
    sess->m_session_id = ++usrConn->m_last_session_id;

    order_list_add(&sess->m_base.m_node, &usrConn->m_session_que);

    m_usr_center->startAgent(usrConn->m_fdinfo, 
        usrConn->m_user_id, sess->m_session_id);

    m_poll_pool->addEvent(info); 
    
    return 0;
}

Int32 ManageCenter::addAdmin(ListenerAdmin* listener, Int32 new_fd) {
    AdminAccpt* admin = NULL;
    FdInfo* info = NULL;

    admin = m_usr_center->creatData<AdminAccpt>();
    info = m_poll_pool->creatSock(new_fd, ENUM_RD_DIRTY_MSG, 
        ENUM_WR_SOCK_MSG, ENUM_DEAL_ADMIN_IN, 
        &admin->m_sock, admin);

    admin->m_parent = listener;
    admin->m_fdinfo = info;
    admin->m_admin_id = ++m_last_usr_id;
    list_add_back(&admin->m_base.m_node, &listener->m_admin_que);

    m_poll_pool->addEvent(info);
            
    return 0;
}

Int32 ManageCenter::startSessConn(const TcpParam* param, Uint32 sessID,
    UserAccpt* parent) {
    Int32 ret = 0;
    Int32 fd = -1;
    SessionConn* sess = NULL; 
    FdInfo* info = NULL; 

    ret = connFast(param, &fd);
    if (0 <= ret) {
        sess = m_usr_center->creatData<SessionConn>();
        info = m_poll_pool->creatSock(fd, ENUM_RD_TCP_RAW, 
            ENUM_WR_TCP_CONNECTING, ENUM_DEAL_SESSION_OUT, 
            &sess->m_sock, sess); 

        /* if connecting, then disable read */
        info->m_test_rd = FALSE;

        sess->m_fdinfo = info;
        sess->m_parent = parent; 
        sess->m_session_id = sessID;
        order_list_add(&sess->m_base.m_node, &parent->m_session_que);

        m_poll_pool->addEvent(info);

        LOG_DEBUG("++++tcp_conn| ret=%d| ip=%s| port=%d|"
            " fd=%d| user_id=%u| session_id=%u|"
            " msg=start tcp session|",
            ret, param->m_ip, param->m_port,
            fd, parent->m_user_id, sessID);
        
        return 0;
    } else {  
        LOG_ERROR("****tcp_conn| ip=%s| port=%d|"
            " user_id=%u| session_id=%u|"
            " msg=start tcp session error|", 
            param->m_ip, param->m_port,
            parent->m_user_id, sessID);
        
        return -1;
    } 
}

Int32 ManageCenter::startUsrConn(const TcpParam* param, UserConn* user) {
    Int32 ret = 0;
    Int32 fd = -1;
    FdInfo* info = NULL; 

    ret = connFast(param, &fd);
    if (0 <= ret) {
        info = m_poll_pool->creatSock(fd, ENUM_RD_DIRTY_MSG, 
            ENUM_WR_DIRTY_CONNECTING, ENUM_DEAL_USER_OUT, 
            &user->m_sock, user); 

        /* if connecting, then disable read */
        info->m_test_rd = FALSE;
        
        user->m_fdinfo = info;
        user->m_user_id = ++m_last_usr_id;

        m_poll_pool->addEvent(info);

        LOG_DEBUG("++++usr_conn| ret=%d| ip=%s| port=%d| fd=%d| user_id=%u|"
            " msg=start user connection|",
            ret, param->m_ip, param->m_port,
            fd, user->m_user_id);
        
        return 0;
    }  else {
        LOG_ERROR("****usr_conn| ip=%s| port=%d|"
            " msg=start user connection error|", 
            param->m_ip, param->m_port);
        
        return -1;
    } 
}

Int32 ManageCenter::startRouterConn(RouterPair* routerPair) {
    Int32 ret = 0;
    Int32 fd = -1;
    FdInfo* info = NULL;
    const TcpParam* param = &routerPair->m_parent->m_tcp_pairs->m_peer;

    ret = connFast(param, &fd);
    if (0 <= ret) {
        info = m_poll_pool->creatSock(fd, ENUM_RD_DIRTY_MSG, 
            ENUM_WR_ROUTER_CONNECTING, ENUM_DEAL_ROUTER_OUT, 
            &routerPair->m_router_out.m_sock, routerPair); 

        /* if connecting, then disable read */
        info->m_test_rd = FALSE;
        
        routerPair->m_router_out.m_fdinfo = info;

        m_poll_pool->addEvent(info);

        LOG_DEBUG("++++router_conn| ret=%d| ip=%s| port=%d| fd=%d| user_id=%u|"
            " msg=start router connection|",
            ret, param->m_ip, param->m_port,
            fd, routerPair->m_router_id);
        
        return 0;
    }  else {
        LOG_ERROR("****router_conn| ip=%s| port=%d|"
            " msg=start router connection error|", 
            param->m_ip, param->m_port);
        
        return -1;
    } 
}

Int32 ManageCenter::addListener(const TcpPairs* pairs) {
    Int32 ret = 0;

    if (ENUM_NODE_TCP_LISTENER == pairs->m_base.m_node_type) {
        ret = addTcpListener(pairs);
    } else if (ENUM_NODE_DIRTY_LISTENER == pairs->m_base.m_node_type) {
        ret = addDirtyListener(pairs);
    } else if (ENUM_NODE_ROUTER_LISTENER == pairs->m_base.m_node_type) {
        ret = addRouterListener(pairs);
    } else if (ENUM_NODE_ADMIN_LISTENER == pairs->m_base.m_node_type) {
        ret = addAdminListener(pairs);
    } else {
        ret = -1;
    }

    return ret;
}

Int32 ManageCenter::addDirtyListener(const TcpPairs* pairs) {
    Int32 ret = 0;
    Int32 fd = -1;
    FdInfo* info = NULL;
    ListenerDirty* listener = NULL;

    listener = m_usr_center->creatData<ListenerDirty>();
    listener->m_tcp_pairs = pairs;
    list_add_back(&listener->m_base.m_node, &m_listener_list);

    do { 
        fd = creatTcpSrv(&listener->m_tcp_pairs->m_local);
        if (0 > fd) {
            ret = -1;
            break;
        }

        info = m_poll_pool->creatReader(fd, ENUM_RD_DIRTY_LISTENER, listener);
        listener->m_fdinfo = info; 
        
        m_poll_pool->addEvent(info);

        return 0;
    } while (0);
    
    return ret;
} 

Int32 ManageCenter::addTcpListener(const TcpPairs* pairs) {
    Int32 ret = 0;
    Int32 fd = -1;
    FdInfo* info = NULL;
    ListenerTcp* listener = NULL;

    listener = m_usr_center->creatData<ListenerTcp>();
    listener->m_tcp_pairs = pairs;
    list_add_back(&listener->m_base.m_node, &m_listener_list);

    do { 
        fd = creatTcpSrv(&listener->m_tcp_pairs->m_local);
        if (0 > fd) {
            ret = -1;
            break;
        }

        ret = startUsrConn(&listener->m_tcp_pairs->m_peer, listener->m_user);
        if (0 != ret) {
            break;
        }

        /* the user end listener should wait until the tunnel is ok */
        info = m_poll_pool->creatReader(fd, ENUM_RD_TCP_LISTENER, listener);
        listener->m_fdinfo = info; 
        
        m_poll_pool->addEvent(info);

        return 0;
    } while (0);
    
    return ret;
}

Int32 ManageCenter::addRouterListener(const TcpPairs* pairs) {
    Int32 ret = 0;
    Int32 fd = -1;
    FdInfo* info = NULL;
    ListenerRouter* listener = NULL;

    listener = m_usr_center->creatData<ListenerRouter>();
    listener->m_tcp_pairs = pairs;
    list_add_back(&listener->m_base.m_node, &m_listener_list);

    do { 
        fd = creatTcpSrv(&listener->m_tcp_pairs->m_local);
        if (0 > fd) {
            ret = -1;
            break;
        }

        info = m_poll_pool->creatReader(fd, ENUM_RD_ROUTER_LISTENER, listener);
        listener->m_fdinfo = info; 
        
        m_poll_pool->addEvent(info);

        return 0;
    } while (0);
    
    return ret;
} 

Int32 ManageCenter::addAdminListener(const TcpPairs* pairs) {
    Int32 ret = 0;
    Int32 fd = -1;
    FdInfo* info = NULL;
    ListenerAdmin* listener = NULL;

    listener = m_usr_center->creatData<ListenerAdmin>();
    listener->m_tcp_pairs = pairs;
    list_add_back(&listener->m_base.m_node, &m_listener_list);
    
    do {
        fd = creatTcpSrv(&listener->m_tcp_pairs->m_local);
        if (0 > fd) {
            ret = -1;
            break;
        }

        info = m_poll_pool->creatReader(fd, ENUM_RD_ADMIN_LISTENER, listener);
        listener->m_fdinfo = info; 
        
        m_poll_pool->addEvent(info);

        return 0;
    } while (0);
    
    return ret;
}


