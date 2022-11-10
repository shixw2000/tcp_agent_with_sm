#include"sockcenter.h"
#include"sockmng.h"
#include"smutil.h"
#include"sockutil.h"
#include"msgcenter.h"
#include"ticktimer.h"
#include"sockusr.h"
#include"socksess.h"
#include"sockpot.h"
#include"cthread.h"


static Int32 cmpSessAccptID(list_node* n1, list_node* n2) {
    SessionAccpt* p1 = NULL;
    SessionAccpt* p2 = NULL;

    p1 = (SessionAccpt*)n1;
    p2 = (SessionAccpt*)n2;
    
    if (p1->m_session_id < p2->m_session_id) {
        return -1;
    } else if (p1->m_session_id > p2->m_session_id) {
        return 1;
    } else {
        return 0;
    }
}

static Int32 cmpSessConnID(list_node* n1, list_node* n2) {
    SessionConn* p1 = NULL;
    SessionConn* p2 = NULL;

    p1 = (SessionConn*)n1;
    p2 = (SessionConn*)n2;
    
    if (p1->m_session_id < p2->m_session_id) {
        return -1;
    } else if (p1->m_session_id > p2->m_session_id) {
        return 1;
    } else {
        return 0;
    }
}


Int32 FdObjFactory::init() {
    Int32 ret = 0;
    EventHandler* event = NULL;
    TimerHandler* timer = NULL;
    SockUsrListener* usr_listener = NULL;
    SockSessListener* sess_listener = NULL;
    SockUsrConn* usr_conn = NULL;
    SockUsrAccpt* usr_accpt = NULL;
    SockSessConn* sess_conn = NULL;
    SockSessAccpt* sess_accpt = NULL;
    SockSessConnPseudo* sess_conn_pseudo = NULL;
    SockSessAccptPseudo* sess_accpt_pseudo = NULL;
    SockUsrListenerPseudo* usr_listener_pseudo = NULL;
    SockSessListenerPseudo* sess_listener_pseudo = NULL;
    SockUsrAccptPseudo* usr_accpt_pseudo = NULL;
    
    memset(m_obj, 0, sizeof(m_obj));

    I_NEW_2(EventHandler, event, m_mng, m_center);
    m_obj[ENUM_NODE_EVENT] = event;

    I_NEW_2(TimerHandler, timer, m_mng, m_center);
    m_obj[ENUM_NODE_TIMER] = timer;
    
    I_NEW_2(SockSessConn, sess_conn, m_mng, m_center);
    m_obj[ENUM_NODE_SESS_CONN] = sess_conn;

    I_NEW_2(SockSessConnPseudo, sess_conn_pseudo, m_mng, m_center);
    m_obj[ENUM_NODE_SESS_CONN_PSEUDO] = sess_conn_pseudo;
    
    I_NEW_2(SockSessAccpt, sess_accpt, m_mng, m_center);
    m_obj[ENUM_NODE_SESS_ACCPT] = sess_accpt;
    
    I_NEW_2(SockSessAccptPseudo, sess_accpt_pseudo, m_mng, m_center);
    m_obj[ENUM_NODE_SESS_ACCPT_PSEUDO] = sess_accpt_pseudo;
    
    I_NEW_3(SockSessListener, sess_listener, m_mng, m_center, sess_accpt);
    m_obj[ENUM_NODE_SESS_LISTENER] = sess_listener;

    I_NEW_3(SockSessListenerPseudo, sess_listener_pseudo, 
        m_mng, m_center, sess_accpt_pseudo);
    m_obj[ENUM_NODE_SESS_LISTENER_PSEUDO] = sess_listener_pseudo;

    I_NEW_2(SockUsrConn, usr_conn, m_mng, m_center);
    m_obj[ENUM_NODE_USR_CONN] = usr_conn;
    
    I_NEW_3(SockUsrAccpt, usr_accpt, m_mng, m_center, sess_conn);
    m_obj[ENUM_NODE_USR_ACCPT] = usr_accpt;

    I_NEW_3(SockUsrAccptPseudo, usr_accpt_pseudo, m_mng,
        m_center, sess_conn_pseudo);
    m_obj[ENUM_NODE_USR_ACCPT_PSEUDO] = usr_accpt_pseudo;
    
    I_NEW_3(SockUsrListener, usr_listener, m_mng, m_center, usr_accpt);
    m_obj[ENUM_NODE_USR_LISTENER] = usr_listener; 

    I_NEW_3(SockUsrListenerPseudo, usr_listener_pseudo, 
        m_mng, m_center, usr_accpt_pseudo);
    m_obj[ENUM_NODE_USR_LISTENER_PSEUDO] = usr_listener_pseudo;

    return ret;
}

Void FdObjFactory::finish() {
    for (int i=0; i<ENUM_NODE_OBJ_MAX; ++i) {
        I_FREE(m_obj[i]);
    }
}

Int32 FdObjFactory::addAgentSrv(AgentSrv* agent, list_head* list) {
    Int32 ret = 0; 
    list_node* pos = NULL;
    Address* addr = NULL;
    SockUsrListener* sockListener = NULL;
    ListenerDirty* listener = NULL;

    sockListener = getObj<SockUsrListener>(agent->m_base.m_node_type);
    
    list_for_each(pos, &agent->m_binds) {
        addr = (Address*)pos;

        listener = sockListener->setup(&addr->m_param);
        if (NULL != listener) {
            list_add_back(&listener->m_base.m_node, list);
        } else {
            ret = -1;
            break;
        }
    } 
    
    return ret;
} 

Int32 FdObjFactory::addAgentCli(AgentCli* agent, list_head* list) {
    Int32 ret = 0; 
    list_node* pos = NULL;
    AddrPairs* pairs = NULL;
    SockUsrConn* sockUsr = NULL;
    SockSessListener* sockListener = NULL;
    ListenerTcp* listener = NULL;
    UserConn* usr = NULL;

    sockListener = getObj<SockSessListener>(agent->m_base.m_node_type);
    sockUsr = getObj<SockUsrConn>(ENUM_NODE_USR_CONN);
    
    usr = sockUsr->setup(&agent->m_origin);
    if (NULL != usr) {
        list_add_back(&usr->m_base.m_node, list);
        
        list_for_each(pos, &agent->m_pairs) {
            pairs = (AddrPairs*)pos;

            listener = sockListener->setup(usr, &pairs->m_pairs);
            if (NULL != listener) {
                list_add_back(&listener->m_base.m_node, &usr->m_listener_que);
            } else {
                ret = -1;
                break;
            }
        }
    }
    
    return ret;
}

int FdObjFactory::readFd(struct FdInfo* info) {
    Int32 ret = 0;

    if (0 <= info->m_fd_type && ENUM_NODE_OBJ_MAX > info->m_fd_type) {
        if (NULL != m_obj[info->m_fd_type]) {
            ret = m_obj[info->m_fd_type]->readFd(info);
            
            LOG_DEBUG("read_fd| ret=%d| fd=%d| type=%d| data=%p|",
                ret, info->m_fd, info->m_fd_type, info->m_data);
        } else {
            LOG_ERROR("read_fd| fd=%d| type=%d| data=%p| msg=invalid|",
                info->m_fd, info->m_fd_type, info->m_data);
            ret = -1;
        }
    } else {
        LOG_ERROR("read_fd| fd=%d| type=%d| data=%p| msg=invalid type|",
            info->m_fd, info->m_fd_type, info->m_data);
        ret = -2;
    }
        
    return ret;
}

int FdObjFactory::writeFd(struct FdInfo* info) {
    Int32 ret = 0;

    if (0 <= info->m_fd_type && ENUM_NODE_OBJ_MAX > info->m_fd_type) {
        if (NULL != m_obj[info->m_fd_type]) {
            ret = m_obj[info->m_fd_type]->writeFd(info);
            
            LOG_DEBUG("write_fd| ret=%d| fd=%d| type=%d| data=%p|",
                ret, info->m_fd, info->m_fd_type, info->m_data);
        } else {
            LOG_ERROR("write_fd| fd=%d| type=%d| data=%p| msg=invalid|",
                info->m_fd, info->m_fd_type, info->m_data);
            ret = -1;
        }
    } else {
        LOG_ERROR("write_fd| fd=%d| type=%d| data=%p| msg=invalid type|",
            info->m_fd, info->m_fd_type, info->m_data);
        ret = -2;
    }
        
    return ret;
}

Int32 FdObjFactory::procMsg(FdInfo* info, MsgHdr* msg) {
    Int32 ret = 0;

    if (0 <= info->m_fd_type && ENUM_NODE_OBJ_MAX > info->m_fd_type) {
        if (NULL != m_obj[info->m_fd_type]) {
            ret = m_obj[info->m_fd_type]->procMsg(info, msg);
            
            LOG_DEBUG("proc_msg| ret=%d| fd=%d| type=%d| data=%p|",
                ret, info->m_fd, info->m_fd_type, info->m_data);
        } else {
            LOG_ERROR("proc_msg| fd=%d| type=%d| data=%p| msg=invalid|",
                info->m_fd, info->m_fd_type, info->m_data);
            ret = -1;
        }
    } else {
        LOG_ERROR("proc_msg| fd=%d| type=%d| data=%p| msg=invalid type|",
            info->m_fd, info->m_fd_type, info->m_data);
        ret = -2;
    }
        
    return ret;
}

void FdObjFactory::eof(struct FdInfo* info) {
    if (0 <= info->m_fd_type && ENUM_NODE_OBJ_MAX > info->m_fd_type) {
        if (NULL != m_obj[info->m_fd_type]) {
            m_obj[info->m_fd_type]->eof(info);
            
            LOG_INFO("end_fd| fd=%d| type=%d| data=%p|",
                info->m_fd, info->m_fd_type, info->m_data);
        } else {
            LOG_ERROR("end_fd| fd=%d| type=%d| data=%p| msg=invalid|",
                info->m_fd, info->m_fd_type, info->m_data);
        }
    } else {
        LOG_ERROR("end_fd| fd=%d| type=%d| data=%p| msg=invalid type|",
            info->m_fd, info->m_fd_type, info->m_data);
    }
        
    return;
}


Void FdObjFactory::freeListenerUsrQue(list_head* list) {
    list_node* pos = NULL;
    list_node* n = NULL;
    ListenerDirty* listener = NULL;
    
    if (!list_empty(list)) {
        list_for_each_safe(pos, n, list) {
            list_del(pos, list);
            
            listener = (ListenerDirty*)pos; 
            freeListenerDirty(listener);
        }
    }
}

Void FdObjFactory::freeListenerSessQue(list_head* list) {
    list_node* pos = NULL;
    list_node* n = NULL;
    ListenerTcp* listener = NULL;
    
    if (!list_empty(list)) {
        list_for_each_safe(pos, n, list) {
            list_del(pos, list);
            
            listener = (ListenerTcp*)pos; 
            freeListenerTcp(listener);
        }
    }
}

Void FdObjFactory::freeUsrAccptQue(list_head* list) {
    list_node* pos = NULL;
    list_node* n = NULL;
    UserAccpt* usr = NULL;
    
    if (!list_empty(list)) {
        list_for_each_safe(pos, n, list) {
            list_del(pos, list);
            
            usr = (UserAccpt*)pos; 
            freeUsrAccpt(usr);
        }
    }
}

Void FdObjFactory::freeUsrConnQue(list_head* list) {
    list_node* pos = NULL;
    list_node* n = NULL;
    UserConn* usr = NULL;
    
    if (!list_empty(list)) {
        list_for_each_safe(pos, n, list) {
            list_del(pos, list);
            
            usr = (UserConn*)pos;
            
            freeUsrConn(usr);
        }
    }
}

Void FdObjFactory::freeSessAccptQue(order_list_head* list) {
    list_node* pos = NULL;
    list_node* n = NULL;
    SessionAccpt* sess = NULL;
    
    if (!order_list_empty(list)) {
        list_for_each_safe(pos, n, list) { 
            order_list_del(pos, list);
            
            sess = (SessionAccpt*)pos;
            
            freeSessAccpt(sess);
        }
    }
}

Void FdObjFactory::freeSessConnQue(order_list_head* list) {
    list_node* pos = NULL;
    list_node* n = NULL;
    SessionConn* sess = NULL;
    
    if (!order_list_empty(list)) {
        list_for_each_safe(pos, n, list) {
            order_list_del(pos, list);
            
            sess = (SessionConn*)pos; 
            freeSessConn(sess);
        }
    }
}

Void FdObjFactory::freeMsgQue(list_head* list) {
    list_node* pos = NULL;
    list_node* n = NULL;
    MsgHdr* msg = NULL;
    
    if (!list_empty(list)) {
        list_for_each_safe(pos, n, list) {
            list_del(pos, list);
                    
            msg = MsgCenter::node2msg(pos);
            MsgCenter::free(msg);
        }
    }
}

Void FdObjFactory::freeFd(FdInfo* info) {
    Int32 fd = info->m_fd;

    if (0 <= fd) {
        freeMsgQue(&info->m_rd_que);
        freeMsgQue(&info->m_wr_que);
        freeMsgQue(&info->m_deal_que);
        freeMsgQue(&info->m_send_que);
        freeMsgQue(&info->m_recv_que);

        memset(info, 0, sizeof(FdInfo));
        info->m_fd = -1;

        closeHd(fd);
    }
} 

Void FdObjFactory::closeSock(SockBase* sock) { 
    if (NULL != sock->m_curr_snd) {
        MsgCenter::free(sock->m_curr_snd);

        sock->m_curr_snd = NULL;
    }

    if (NULL != sock->m_curr_rcv) {
        MsgCenter::free(sock->m_curr_rcv);

        sock->m_curr_rcv = NULL;
    }
}

Void FdObjFactory::resetSock(SockBase* sock) {
    memset(sock, 0, sizeof(SockBase));
}

Void FdObjFactory::resetNode(NodeBase* base) {
    memset(base, 0, sizeof(NodeBase));
    INIT_LIST_NODE(&base->m_node);
}

EvpBase* FdObjFactory::creatEvp() {
    EvpBase* evp = NULL;
    
    I_NEW(EvpBase, evp);
    evp->m_digest_type = ENUM_DIGEST_SM3;
    evp->m_cipher_type = ENUM_CIPHER_SM4;
    evp->m_pkey_type = ENUM_PKEY_SM2;
    evp->m_padding = TRUE; 
    
    return evp;
}

Void FdObjFactory::freeEvp(EvpBase* evp) {
    I_FREE(evp);
}

EventData* FdObjFactory::newEventData() {
    EventData* ev = NULL;

    I_NEW(EventData, ev);
    memset(ev, 0, sizeof(EventData));
    
    resetNode(&ev->m_base);

    return ev;
}

Void FdObjFactory::freeEventData(EventData* ev) {
    if (NULL != ev) {  
        I_FREE(ev);
    }
}

TimerData* FdObjFactory::newTimerData() {
    TimerData* timer = NULL;

    I_NEW(TimerData, timer);
    memset(timer, 0, sizeof(TimerData));

    I_NEW(TickTimer, timer->m_timer);
    
    resetNode(&timer->m_base);

    return timer;
}

Void FdObjFactory::freeTimerData(TimerData* timer) {
    if (NULL != timer) {  
        if (NULL != timer->m_timer) {
            timer->m_timer->stop();
            
            I_FREE(timer->m_timer);
        }
        
        I_FREE(timer);
    }
}

ListenerTcp* FdObjFactory::newListenerTcp() {
    ListenerTcp* listener = NULL; 
    
    I_NEW(ListenerTcp, listener);
    memset(listener, 0, sizeof(ListenerTcp));

    resetNode(&listener->m_base);

    return listener;
}

Void FdObjFactory::freeListenerTcp(ListenerTcp* listener) {
    if (NULL != listener) {
        if (NULL != listener->m_fdinfo) {
            freeFd(listener->m_fdinfo);

            listener->m_fdinfo = NULL;
        }
        
        I_FREE(listener);
    }
}

ListenerDirty* FdObjFactory::newListenerDirty() {
    ListenerDirty* listener = NULL;

    I_NEW(ListenerDirty, listener);
    memset(listener, 0, sizeof(ListenerDirty));

    resetNode(&listener->m_base);

    INIT_LIST_HEAD(&listener->m_usr_que);

    return listener;
}

Void FdObjFactory::freeListenerDirty(ListenerDirty* listener) {
    if (NULL != listener) {
        freeUsrAccptQue(&listener->m_usr_que);

        if (NULL != listener->m_fdinfo) {
            freeFd(listener->m_fdinfo);

            listener->m_fdinfo = NULL;
        }
        
        I_FREE(listener);
    }
}

SessionAccpt* FdObjFactory::newSessAccpt() {
    SessionAccpt* sess = NULL;

    I_NEW(SessionAccpt, sess);
    memset(sess, 0, sizeof(SessionAccpt));

    resetNode(&sess->m_base);
    
    resetSock(&sess->m_sock);

    return sess;
}

Void FdObjFactory::freeSessAccpt(SessionAccpt* sess) {
    closeSock(&sess->m_sock);

    if (NULL != sess->m_fdinfo) {
        freeFd(sess->m_fdinfo);

        sess->m_fdinfo = NULL;
    }
    
    I_FREE(sess);
}

SessionConn* FdObjFactory::newSessConn() {
    SessionConn* sess = NULL;

    I_NEW(SessionConn, sess);
    memset(sess, 0, sizeof(SessionConn));

    resetNode(&sess->m_base);
    resetSock(&sess->m_sock);

    return sess;
}

Void FdObjFactory::freeSessConn(SessionConn* sess) {
    closeSock(&sess->m_sock);

    if (NULL != sess->m_fdinfo) {
        freeFd(sess->m_fdinfo);

        sess->m_fdinfo = NULL;
    }
    
    I_FREE(sess);
}

UserAccpt* FdObjFactory::newUsrAccpt() {
    UserAccpt* user = NULL;

    I_NEW(UserAccpt, user);
    memset(user, 0, sizeof(UserAccpt));

    resetNode(&user->m_base);
    resetSock(&user->m_sock);

    INIT_ORDER_LIST_HEAD(&user->m_session_que, &cmpSessConnID);

    user->m_evp_rcv = creatEvp();
    user->m_evp_snd = creatEvp();

    user->m_usr_status = ENUM_USER_INIT;

    return user;
}

Void FdObjFactory::freeUsrAccpt(UserAccpt* usr) {
    closeSock(&usr->m_sock);
    
    if (NULL != usr->m_evp_rcv) {
        freeEvp(usr->m_evp_rcv);
        
        usr->m_evp_rcv = NULL;
    }

    if (NULL != usr->m_evp_snd) {
        freeEvp(usr->m_evp_snd);
        
        usr->m_evp_snd = NULL;
    }
    
    freeSessConnQue(&usr->m_session_que);

    if (NULL != usr->m_fdinfo) {
        freeFd(usr->m_fdinfo);

        usr->m_fdinfo = NULL;
    }

    I_FREE(usr);
}

UserConn* FdObjFactory::newUsrConn() {
    UserConn* user = NULL;

    I_NEW(UserConn, user);
    memset(user, 0, sizeof(UserConn));

    resetNode(&user->m_base);
    resetSock(&user->m_sock);
    
    INIT_LIST_HEAD(&user->m_listener_que);
    INIT_ORDER_LIST_HEAD(&user->m_session_que, &cmpSessAccptID);

    user->m_evp_rcv = creatEvp();
    user->m_evp_snd = creatEvp();

    user->m_usr_status = ENUM_USER_INIT;

    return user;
}

Void FdObjFactory::freeUsrConn(UserConn* usr) {    
    closeSock(&usr->m_sock);

    if (NULL != usr->m_evp_rcv) {
        freeEvp(usr->m_evp_rcv);
        
        usr->m_evp_rcv = NULL;
    }

    if (NULL != usr->m_evp_snd) {
        freeEvp(usr->m_evp_snd);
        
        usr->m_evp_snd = NULL;
    }
  
    freeListenerSessQue(&usr->m_listener_que);
    freeSessAccptQue(&usr->m_session_que);

    if (NULL != usr->m_fdinfo) {
        freeFd(usr->m_fdinfo);

        usr->m_fdinfo = NULL;
    }
    
    I_FREE(usr);
}


SockCenter::SockCenter() {
    INIT_LIST_HEAD(&m_list_server);
    INIT_LIST_HEAD(&m_list_client);

    INIT_TIMER_ELE(&m_minute_ele);
    INIT_TIMER_ELE(&m_hour_ele);
    
    m_last_session_id = 0;
    m_last_user_id = 0;

    m_parser = NULL;
    m_mng = NULL;
    m_fctry = NULL;
    m_event_data = NULL;
    m_timer_data = NULL;
}

Int32 SockCenter::init() {
    Int32 ret = 0;
    EventHandler* event = NULL;
    TimerHandler* timer = NULL;

    do {        
        I_NEW(Parser, m_parser);
        ret = m_parser->init(m_path);
        if (0 != ret) {
            break;
        } 

        getRand(m_base.m_seid, DEF_SEID_SIZE);
        getRand(m_base.m_rand, DEF_RAND_CODE_SIZE);
        
        I_NEW_1(SockMng, m_mng, DEF_FD_MAX_CAPACITY);
        ret = m_mng->init();
        if (0 != ret) {
            break;
        }
        
        I_NEW_2(FdObjFactory, m_fctry, m_mng, this); 
        ret = m_fctry->init();
        if (0 != ret) {
            break;
        }

        event = m_fctry->getObj<EventHandler>(ENUM_NODE_EVENT);
        m_event_data = event->setup();

        m_mng->set(m_event_data->m_fdinfo->m_fd, m_fctry);

        timer = m_fctry->getObj<TimerHandler>(ENUM_NODE_TIMER);
        m_timer_data = timer->setup();

        /* add a minutely timer */
        m_minute_ele.m_type = ENUM_TIMER_TYPE_MINUTELY;
        m_minute_ele.m_interval = DEF_MINUTE_TICK_CNT;
        m_timer_data->m_timer->addTimer(&m_minute_ele);

        /* add a hourly timer */
        m_hour_ele.m_type = ENUM_TIMER_TYPE_HOURLY;
        m_hour_ele.m_interval = ENUM_TIMER_TYPE_HOURLY;
        m_timer_data->m_timer->addTimer(&m_hour_ele);
    } while (0);

    return ret;
}

Void SockCenter::finish() {
    FdObjFactory::freeListenerUsrQue(&m_list_server);
    FdObjFactory::freeUsrConnQue(&m_list_client);
    
    if (NULL != m_mng) {
        m_mng->finish();
        I_FREE(m_mng);
    }

    if (NULL != m_parser) {
        m_parser->finish();
        I_FREE(m_parser);
    }

    if (NULL != m_fctry) {
        m_fctry->finish();
        I_FREE(m_fctry);
    }
}

Int32 SockCenter::notifySessEvent(FdInfo* info, Uint16 cmd,
    Uint32 usrId, Uint32 sessId) {
    Int32 ret = 0;
    MsgHdr* hdr = NULL;
    MsgSessHead* msg = NULL;

    hdr = MsgCenter::creat<MsgSessHead>(cmd);
    msg = MsgCenter::cast<MsgSessHead>(hdr);
    
    msg->m_user_id = usrId;
    msg->m_session_id = sessId;

    MsgCenter::addCrc(hdr);

    ret = m_mng->dispatch(info, hdr);
    return ret;
}

Int32 SockCenter::sendSessEvent(FdInfo* info, Uint16 cmd,
    Uint32 usrId, Uint32 sessId) {
    Int32 ret = 0;
    MsgHdr* hdr = NULL;
    MsgSessHead* msg = NULL;

    hdr = MsgCenter::creat<MsgSessHead>(cmd);
    msg = MsgCenter::cast<MsgSessHead>(hdr);
    
    msg->m_user_id = usrId;
    msg->m_session_id = sessId;

    MsgCenter::addCrc(hdr);

    ret = m_mng->sendMsg(info, hdr);
    return ret;
}

Int32 SockCenter::sendPeerSess(UserConn* usr, SessionAccpt* sess) {
    Int32 ret = 0;
    MsgHdr* hdr = NULL;
    MsgStartSess* msg = NULL;  

    hdr = MsgCenter::creat<MsgStartSess>(ENUM_MSG_CMD_START_SESS);
    msg = MsgCenter::cast<MsgStartSess>(hdr);
    
    msg->m_user_id = usr->m_user_id;
    msg->m_session_id = sess->m_session_id;
    msg->m_port = sess->m_pairs->m_peer.m_port;
    strncpy(msg->m_ip, sess->m_pairs->m_peer.m_ip, DEF_IP_SIZE);

    MsgCenter::addCrc(hdr);

    ret = m_mng->sendMsg(usr->m_fdinfo, hdr);
    return ret;
}

Int32 SockCenter::notifyUsrEvent(FdInfo* info, Uint16 cmd, Uint32 usrId) {
    Int32 ret = 0;
    MsgHdr* hdr = NULL;
    MsgUserHead* msg = NULL;

    hdr = MsgCenter::creat<MsgUserHead>(cmd);
    msg = MsgCenter::cast<MsgUserHead>(hdr);
    
    msg->m_user_id = usrId;

    MsgCenter::addCrc(hdr);

    ret = m_mng->dispatch(info, hdr);
    return ret;
}

Int32 SockCenter::sendUsrEvent(FdInfo* info, Uint16 cmd, Uint32 usrId) {
    Int32 ret = 0;
    MsgHdr* hdr = NULL;
    MsgUserHead* msg = NULL;

    hdr = MsgCenter::creat<MsgUserHead>(cmd);
    msg = MsgCenter::cast<MsgUserHead>(hdr);
    
    msg->m_user_id = usrId;

    MsgCenter::addCrc(hdr);

    ret = m_mng->sendMsg(info, hdr);
    return ret;
}

Int32 SockCenter::notifyChildUsrExit(FdInfo* info, 
    Uint32 usrId, Uint64 data) {
    Int32 ret = 0;
    MsgHdr* hdr = NULL;
    MsgChildUserExit* msg = NULL;

    hdr = MsgCenter::creat<MsgChildUserExit>(ENUM_MSG_SYSTEM_CHILD_EXIT);
    msg = MsgCenter::cast<MsgChildUserExit>(hdr);
    
    msg->m_user_id = usrId;
    msg->m_data = data;

    MsgCenter::addCrc(hdr);

    ret = m_mng->dispatch(info, hdr);
    return ret;
}

Uint32 SockCenter::hashMac(EvpBase* evp, const Void* data, Int32 len) {
    Uint32 mac = 0;
    Int32 outlen = 0;
    Byte buf[DEF_SM3_DIGEST_SIZE];

    /* calc hash code with sm3 */
    outlen = digest(evp, data, len, buf);
    mac = MsgCenter::calcCrc(buf, outlen);
    return mac;
}

MsgHdr* SockCenter::decCipherText(EvpBase* evp, MsgHdr* input) {
    MsgHdr* output = NULL;
    MsgTcpPlain* plain = NULL;
    MsgTcpCipher* cipher = NULL;
    Int32 pre_len = 0;
    Int32 dec_len = 0;
    Uint32 mac = 0; 

    cipher = MsgCenter::cast<MsgTcpCipher>(input);
    mac = hashMac(evp, cipher->m_data, cipher->m_cipher_len);
    if (mac != cipher->m_mac) {
        LOG_ERROR("cipher_to_plain| user_id=%u| sess_id=%u|"
            " cipher_len=%d| plain_len=%d| c_mac=0x%x|"
            " mac=0x%x| msg=check mac invalid|",
            cipher->m_user_id, cipher->m_session_id,
            cipher->m_cipher_len, cipher->m_plain_len,
            cipher->m_mac, mac);

        return NULL;
    } 

    /* try to get a prelen */
    pre_len = decrypt(evp, cipher->m_data, cipher->m_cipher_len, NULL);

    output = MsgCenter::creat<MsgTcpPlain>(ENUM_MSG_CMD_TCP_PLAIN, pre_len);
    output->m_seq = input->m_seq;
    
    plain = MsgCenter::cast<MsgTcpPlain>(output);
    
    /* decrypt data with sm4 */
    dec_len = decrypt(evp, cipher->m_data, cipher->m_cipher_len, plain->m_data);
    if (0 > dec_len || dec_len != cipher->m_plain_len) {
        LOG_ERROR("cipher_to_plain| user_id=%u| sess_id=%u|"
            " cipher_len=%d| plain_len=%d| c_mac=0x%x|"
            " pre_len=%d| dec_len=%d| msg=decrypt error|",
            cipher->m_user_id, cipher->m_session_id,
            cipher->m_cipher_len, cipher->m_plain_len,
            cipher->m_mac, pre_len, dec_len);

        MsgCenter::free(output);
        return NULL;
    }
    
    MsgCenter::shrink<MsgTcpPlain>(dec_len, output);
    
    plain->m_user_id = cipher->m_user_id;
    plain->m_session_id = cipher->m_session_id;
    plain->m_data_len = dec_len; 

    MsgCenter::addCrc(output);
    
    LOG_DEBUG("cipher_to_plain| user_id=%u| sess_id=%u|"
        " cipher_len=%d| plain_len=%d| pre_len=%d|"
        " dec_len=%d| msg=ok|",
        cipher->m_user_id, cipher->m_session_id,
        cipher->m_cipher_len, cipher->m_plain_len,
        pre_len, dec_len); 

    return output;
}

MsgHdr* SockCenter::encPlainText(EvpBase* evp, MsgHdr* input) {
    MsgHdr* output = NULL;
    MsgTcpPlain* plain = NULL;
    MsgTcpCipher* cipher = NULL;
    Int32 pre_len = 0;
    Int32 enc_len = 0;

    plain = MsgCenter::cast<MsgTcpPlain>(input);

    /* try to get a prelen */
    pre_len = encrypt(evp, plain->m_data, plain->m_data_len, NULL); 

    output = MsgCenter::creat<MsgTcpCipher>(ENUM_MSG_CMD_TCP_CIPHER, pre_len);
    output->m_seq = input->m_seq;
    
    cipher = MsgCenter::cast<MsgTcpCipher>(output);

    /* encrypt data with sm4 */
    enc_len = encrypt(evp, plain->m_data, plain->m_data_len, cipher->m_data);

    MsgCenter::shrink<MsgTcpCipher>(enc_len, output);

    cipher->m_user_id = plain->m_user_id;
    cipher->m_session_id = plain->m_session_id;
    
    cipher->m_plain_len = plain->m_data_len;
    cipher->m_cipher_len = enc_len;
    
    /* do a hash digest with sm3 */
    cipher->m_mac = hashMac(evp, cipher->m_data, cipher->m_cipher_len);

    MsgCenter::addCrc(output);
    
    LOG_DEBUG("plain_to_cipher| user_id=%u| sess_id=%u|"
        " plain_len=%d| cipher_len=%d| pre_len=%d| mac=0x%x| msg=ok|",
        plain->m_user_id, plain->m_session_id,
        cipher->m_plain_len, cipher->m_cipher_len, 
        pre_len, cipher->m_mac);

    return output;
}

Void SockCenter::setCipherKey(EvpBase* evp, 
    const Void* usr_key, Int32 len) {
    evp->m_cipher.setUsrKey(usr_key, len);
}

Int32 SockCenter::encrypt(EvpBase* evp, const Void* data, 
    Int32 len, Void* out) {
    Int32 outlen = 0;
    
    outlen = evp->m_cipher.sm4_cbc_encrypt(data, len, out);
    return outlen;
}

Int32 SockCenter::decrypt(EvpBase* evp, const Void* data, 
    Int32 len, Void* out) {
    Int32 outlen = 0;
    
    outlen = evp->m_cipher.sm4_cbc_decrypt(data, len, out);
    return outlen;
}

Int32 SockCenter::digest(EvpBase* evp, const Void* data, 
    Int32 len, Void* out) {
    Int32 outlen = 0;

    outlen = evp->m_digest.digest(data, len, out);

    return outlen;
}

Void SockCenter::doTimeout(struct TimerEle* ele) {
    if (ENUM_TIMER_TYPE_MINUTELY== ele->m_type) {
        LOG_INFO("minutely_task| msg=ok|");

        updateTimer(ele);
    } else if (ENUM_TIMER_TYPE_HOURLY== ele->m_type) {
        updateTimer(ele);
    } else {
    }
}

Int32 SockCenter::chkConn(FdInfo* info) {
    Int32 ret = 0;

    ret = chkConnStatus(info->m_fd);
    if (0 == ret) { 
        LOG_DEBUG("chk_connection| fd=%d| msg=ok|", info->m_fd); 

        return 0;
    } else {
        LOG_ERROR("chk_connection| fd=%d| msg=invalid|", info->m_fd);
        
        return -1;
    }
}

Int32 SockCenter::addAgentSrv(AgentSrv* agent, list_head* list) {
    Int32 ret = 0; 

    ret = m_fctry->addAgentSrv(agent, list);
    
    return ret;
} 

Int32 SockCenter::addAgentCli(AgentCli* agent, list_head* list) { 
    Int32 ret = 0; 

    ret = m_fctry->addAgentCli(agent, list);
    
    return ret;
}

Int32 SockCenter::startServer() {
    Int32 ret = 0;
    list_node* pos = NULL;
    NodeBase* base = NULL;
    AgentCli* cli = NULL;
    AgentSrv* srv = NULL;
    struct Config* config = m_parser->getConf();

    if (0 != strncmp(config->m_passwd, "123456", MAX_PIN_PASSWD_SIZE)) {
        return -1;
    }

    list_for_each(pos, &config->m_agent_list) {
        base = (NodeBase*)pos;

        if (ENUM_NODE_SESS_LISTENER == base->m_node_type
            || ENUM_NODE_SESS_LISTENER_PSEUDO == base->m_node_type) {
            cli = (AgentCli*)base;
            
            ret = addAgentCli(cli, &m_list_client);
        } else if (ENUM_NODE_USR_LISTENER == base->m_node_type
            || ENUM_NODE_USR_LISTENER_PSEUDO == base->m_node_type) {
            srv = (AgentSrv*)base;
            
            ret = addAgentSrv(srv, &m_list_server);
        } else {
            ret = -1;
        }

        if (0 != ret) {
            break;
        }
    }

    if (0 == ret) {
        ret = m_mng->start("manager");
        if (0 != ret) {
            return ret;
        }
    }

    return ret;
}

Void SockCenter::stopServer() { 
    m_mng->stop(); 
}

Void SockCenter::wait() {
    m_mng->join();
}

