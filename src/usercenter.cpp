#include"usercenter.h"
#include"listnode.h"
#include"msgtype.h"
#include"msgcenter.h"
#include"managecenter.h"
#include"smutil.h"
#include"ticktimer.h"


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

UserCenter::UserCenter(ManageCenter* mng) {
    m_mng = mng;
}

UserCenter::~UserCenter() {
}

EkeyBase* UserCenter::creatEkeyBase() {
    EkeyBase* base = NULL;

    I_NEW(EkeyBase, base);
    resetEkeyBase(base);

    return base;
}

Void UserCenter::resetEkeyBase(EkeyBase* base) {
    memset(base, 0, sizeof(EkeyBase));
}

Void UserCenter::closeEkeyBase(EkeyBase* base) {
    resetEkeyBase(base);
}

Void UserCenter::freeMsgQue(list_head* list) {
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

Void UserCenter::closeSock(SockBase* sock) { 
    if (NULL != sock->m_curr_snd) {
        MsgCenter::free(sock->m_curr_snd);

        sock->m_curr_snd = NULL;
    }

    if (NULL != sock->m_curr_rcv) {
        MsgCenter::free(sock->m_curr_rcv);

        sock->m_curr_rcv = NULL;
    }
}

Void UserCenter::resetSock(SockBase* sock) {
    memset(sock, 0, sizeof(SockBase));
}

Void UserCenter::resetNode(NodeBase* base) {
    memset(base, 0, sizeof(NodeBase));
    INIT_LIST_NODE(&base->m_node);
}

EvpBase* UserCenter::creatEvp() {
    EvpBase* evp = NULL;
    
    I_NEW(EvpBase, evp);
    evp->m_digest_type = ENUM_DIGEST_SM3;
    evp->m_cipher_type = ENUM_CIPHER_SM4;
    evp->m_pkey_type = ENUM_PKEY_SM2;
    evp->m_padding = TRUE; 
    
    return evp;
}

Void UserCenter::freeEvp(EvpBase* evp) {
    I_FREE(evp);
}

template<>
TimerData* UserCenter::creatData<TimerData>() {
    TimerData* timer = NULL;

    I_NEW(TimerData, timer);
    memset(timer, 0, sizeof(TimerData));

    I_NEW(TickTimer, timer->m_timer);
    
    resetNode(&timer->m_base);
    timer->m_base.m_node_type = ENUM_NODE_TIMER;

    return timer;
}

template<>
Void UserCenter::freeData<TimerData>(TimerData* timer) {
    if (NULL != timer) {  
        if (NULL != timer->m_timer) {
            I_FREE(timer->m_timer);
        }
        
        I_FREE(timer);
    }
}

template<>
EventData* UserCenter::creatData<EventData>() {
    EventData* event = NULL;

    I_NEW(EventData, event);
    memset(event, 0, sizeof(EventData));

    resetNode(&event->m_base);
    event->m_base.m_node_type = ENUM_NODE_EVENT_CMD;

    INIT_LIST_HEAD(&event->m_cmd_que);

    return event;
}

template<>
Void UserCenter::freeData<EventData>(EventData* event) {
    if (NULL != event) {
        freeMsgQue(&event->m_cmd_que);
        
        I_FREE(event);
    }
}

template<>
SessionAccpt* UserCenter::creatData<SessionAccpt>() {
    SessionAccpt* sess = NULL;

    I_NEW(SessionAccpt, sess);
    memset(sess, 0, sizeof(SessionAccpt));

    resetNode(&sess->m_base);
    resetSock(&sess->m_sock);
    sess->m_base.m_node_type = ENUM_NODE_SESS_ACCPT;

    return sess;
}

template<>
Void UserCenter::freeData<SessionAccpt>(SessionAccpt* sess) {
    closeSock(&sess->m_sock);
    
    I_FREE(sess);
}

template<>
SessionConn* UserCenter::creatData<SessionConn>() {
    SessionConn* sess = NULL;

    I_NEW(SessionConn, sess);
    memset(sess, 0, sizeof(SessionConn));

    resetNode(&sess->m_base);
    resetSock(&sess->m_sock);
    sess->m_base.m_node_type = ENUM_NODE_SESS_CONN;

    return sess;
}

template<>
Void UserCenter::freeData<SessionConn>(SessionConn* sess) {
    closeSock(&sess->m_sock);
    
    I_FREE(sess);
}

template<>
UserAccpt* UserCenter::creatData<UserAccpt>() {
    UserAccpt* user = NULL;

    I_NEW(UserAccpt, user);
    memset(user, 0, sizeof(UserAccpt));

    resetNode(&user->m_base);
    resetSock(&user->m_sock);
    user->m_base.m_node_type = ENUM_NODE_USER_ACCPT;

    INIT_ORDER_LIST_HEAD(&user->m_session_que, &cmpSessConnID);

    user->m_evp_rcv = creatEvp();
    user->m_evp_snd = creatEvp();

    user->m_usr_status = ENUM_USER_INIT;

    return user;
}

template<>
Void UserCenter::freeData<UserAccpt>(UserAccpt* usr) {
    closeSock(&usr->m_sock);
    
    if (NULL != usr->m_evp_rcv) {
        freeEvp(usr->m_evp_rcv);
        
        usr->m_evp_rcv = NULL;
    }

    if (NULL != usr->m_evp_snd) {
        freeEvp(usr->m_evp_snd);
        
        usr->m_evp_snd = NULL;
    }
    
    freeSessConnQue(&usr->m_session_que, TRUE);

    I_FREE(usr);
}

template<>
UserConn* UserCenter::creatData<UserConn>() {
    UserConn* user = NULL;

    I_NEW(UserConn, user);
    memset(user, 0, sizeof(UserConn));

    resetNode(&user->m_base);
    resetSock(&user->m_sock);
    user->m_base.m_node_type = ENUM_NODE_USER_CONN;
    
    INIT_ORDER_LIST_HEAD(&user->m_session_que, &cmpSessAccptID);

    user->m_evp_rcv = creatEvp();
    user->m_evp_snd = creatEvp();

    user->m_usr_status = ENUM_USER_INIT;

    return user;
}

template<>
Void UserCenter::freeData<UserConn>(UserConn* usr) {    
    closeSock(&usr->m_sock);

    if (NULL != usr->m_evp_rcv) {
        freeEvp(usr->m_evp_rcv);
        
        usr->m_evp_rcv = NULL;
    }

    if (NULL != usr->m_evp_snd) {
        freeEvp(usr->m_evp_snd);
        
        usr->m_evp_snd = NULL;
    }
  
    freeSessAccptQue(&usr->m_session_que, TRUE);
    
    I_FREE(usr);
}

template<>
AdminAccpt* UserCenter::creatData<AdminAccpt>() {
    AdminAccpt* admin = NULL;

    I_NEW(AdminAccpt, admin);
    memset(admin, 0, sizeof(AdminAccpt));

    resetNode(&admin->m_base);
    resetSock(&admin->m_sock);
    admin->m_base.m_node_type = ENUM_NODE_ADMIN_ACCPT;
    
    return admin;
}

template<>
Void UserCenter::freeData<AdminAccpt>(AdminAccpt* admin) {
    closeSock(&admin->m_sock);
    
    I_FREE(admin);
}

template<>
AdminConn* UserCenter::creatData<AdminConn>() {
    AdminConn* admin = NULL;

    I_NEW(AdminConn, admin);
    memset(admin, 0, sizeof(AdminConn));

    resetNode(&admin->m_base);
    resetSock(&admin->m_sock);
    admin->m_base.m_node_type = ENUM_NODE_ADMIN_CONN;

    return admin;
}

template<>
Void UserCenter::freeData<AdminConn>(AdminConn* admin) {
    closeSock(&admin->m_sock);
    
    I_FREE(admin);
}

template<>
ListenerTcp* UserCenter::creatData<ListenerTcp>() {
    UserConn* user = NULL;
    ListenerTcp* listener = NULL; 
    
    I_NEW(ListenerTcp, listener);
    memset(listener, 0, sizeof(ListenerTcp));

    resetNode(&listener->m_base);
    listener->m_base.m_node_type = ENUM_NODE_TCP_LISTENER;

    user = creatData<UserConn>(); 
    user->m_parent = listener;
    listener->m_user = user;

    return listener;
}

template<>
Void UserCenter::freeData<ListenerTcp>(ListenerTcp* listener) {
    if (NULL != listener) {
        if (NULL != listener->m_user) {
            freeData(listener->m_user);

            listener->m_user = NULL;
        }
        
        I_FREE(listener);
    }
}

template<>
ListenerDirty* UserCenter::creatData<ListenerDirty>() {
    ListenerDirty* listener = NULL;

    I_NEW(ListenerDirty, listener);
    memset(listener, 0, sizeof(ListenerDirty));

    resetNode(&listener->m_base);
    listener->m_base.m_node_type = ENUM_NODE_DIRTY_LISTENER;

    INIT_LIST_HEAD(&listener->m_usr_que);

    return listener;
}

template<>
Void UserCenter::freeData<ListenerDirty>(ListenerDirty* listener) {
    if (NULL != listener) {
        freeUsrAccptQue(&listener->m_usr_que, TRUE);
        
        I_FREE(listener);
    }
}

template<>
ListenerAdmin* UserCenter::creatData<ListenerAdmin>() {
    ListenerAdmin* listener = NULL;

    I_NEW(ListenerAdmin, listener);
    memset(listener, 0, sizeof(ListenerAdmin));

    resetNode(&listener->m_base);
    listener->m_base.m_node_type = ENUM_NODE_ADMIN_LISTENER;

    INIT_LIST_HEAD(&listener->m_admin_que);

    return listener;
}

template<>
Void UserCenter::freeData<ListenerAdmin>(ListenerAdmin* listener) {
    if (NULL != listener) {
        freeAdminAccptQue(&listener->m_admin_que, TRUE);
        
        I_FREE(listener);
    }
}

template<>
RouterPair* UserCenter::creatData<RouterPair>() {
    RouterPair* routerPair = NULL;

    I_NEW(RouterPair, routerPair);
    memset(routerPair, 0, sizeof(RouterPair));

    resetNode(&routerPair->m_base);
    routerPair->m_base.m_node_type = ENUM_NODE_ROUTER_PAIR;

    resetSock(&routerPair->m_router_in.m_sock);
    routerPair->m_router_in.m_evp_rcv = creatEvp();
    routerPair->m_router_in.m_evp_snd = creatEvp();
    routerPair->m_router_in.m_usr_status = ENUM_USER_INIT;
    
    resetSock(&routerPair->m_router_out.m_sock);
    routerPair->m_router_out.m_evp_rcv = creatEvp();
    routerPair->m_router_out.m_evp_snd = creatEvp();
    routerPair->m_router_out.m_usr_status = ENUM_USER_INIT;

    return routerPair;
}

template<>
Void UserCenter::freeData<Router>(Router* router) {
    closeSock(&router->m_sock);

    if (NULL != router->m_evp_rcv) {
        freeEvp(router->m_evp_rcv);
        
        router->m_evp_rcv = NULL;
    }

    if (NULL != router->m_evp_snd) {
        freeEvp(router->m_evp_snd);
        
        router->m_evp_snd = NULL;
    }
}

template<>
Void UserCenter::freeData<RouterPair>(RouterPair* routerPair) {
    I_FREE(routerPair);
}

template<>
ListenerRouter* UserCenter::creatData<ListenerRouter>() {
    ListenerRouter* listener = NULL;

    I_NEW(ListenerRouter, listener);
    memset(listener, 0, sizeof(ListenerRouter));

    resetNode(&listener->m_base);
    listener->m_base.m_node_type = ENUM_NODE_ROUTER_LISTENER;

    INIT_LIST_HEAD(&listener->m_route_pair_que);

    return listener;
}

template<>
Void UserCenter::freeData<ListenerRouter>(ListenerRouter* listener) {
    freeRoutePairQue(&listener->m_route_pair_que, TRUE);
        
    I_FREE(listener);
}

Void UserCenter::freeUsrAccptQue(list_head* list, Bool isClose) {
    list_node* pos = NULL;
    list_node* n = NULL;
    UserAccpt* usr = NULL;
    
    if (!list_empty(list)) {
        list_for_each_safe(pos, n, list) { 
            usr = (UserAccpt*)pos; 

            if (isClose) {
                /* if close, delete usr from que by the user itself */
            } else {
                list_del(pos, list);
                freeData(usr);
            }
        }
    }
}

Void UserCenter::freeUsrConnQue(list_head* list, Bool isClose) {
    list_node* pos = NULL;
    list_node* n = NULL;
    UserConn* usr = NULL;
    
    if (!list_empty(list)) {
        list_for_each_safe(pos, n, list) {
            usr = (UserConn*)pos;
            
            if (isClose) {
            } else {
                list_del(pos, list);
                freeData(usr);
            }
        }
    }
}

Void UserCenter::freeSessAccptQue(order_list_head* list, Bool isClose) {
    list_node* pos = NULL;
    list_node* n = NULL;
    SessionAccpt* sess = NULL;
    
    if (!order_list_empty(list)) {
        list_for_each_safe(pos, n, list) { 
            sess = (SessionAccpt*)pos;
            
            if (isClose) {
            } else {
                order_list_del(pos, list);
                freeData(sess);
            }
        }
    }
}

Void UserCenter::freeSessConnQue(order_list_head* list, Bool isClose) {
    list_node* pos = NULL;
    list_node* n = NULL;
    SessionConn* sess = NULL;
    
    if (!order_list_empty(list)) {
        list_for_each_safe(pos, n, list) {
            sess = (SessionConn*)pos;

            if (isClose) {
            } else {
                order_list_del(pos, list);
                freeData(sess);
            }
        }
    }
}

Void UserCenter::freeAdminAccptQue(list_head* list, Bool isClose) {
    list_node* pos = NULL;
    list_node* n = NULL;
    AdminAccpt* admin = NULL;
    
    if (!list_empty(list)) {
        list_for_each_safe(pos, n, list) { 
            admin = (AdminAccpt*)pos;
            
            if (isClose) {
            } else {
                list_del(pos, list);
                freeData(admin);
            }
        }
    }
}

Void UserCenter::freeRoutePairQue(list_head* list, Bool isClose) {
    list_node* pos = NULL;
    list_node* n = NULL;
    RouterPair* routerPair = NULL;
    
    if (!list_empty(list)) {
        list_for_each_safe(pos, n, list) { 
            routerPair = (RouterPair*)pos;
            
            if (isClose) {
            } else {
                list_del(pos, list);
                freeData(routerPair);
            }
        }
    }
}

SessionConn* UserCenter::findSessConn(Uint32 sessID, UserAccpt* user) {
    list_node* node = NULL;
    SessionConn* dst = NULL;
    SessionConn tmp;

    tmp.m_session_id = sessID;
    node = order_list_find(&tmp.m_base.m_node, &user->m_session_que);
    if (NULL != node) {
        dst = (SessionConn*)node;
        return dst;
    } else {
        return NULL;
    }
}

SessionAccpt* UserCenter::findSessAccpt(Uint32 sessID, UserConn* user) {
    list_node* node = NULL;
    SessionAccpt* dst = NULL;
    SessionAccpt tmp;

    tmp.m_session_id = sessID;
    node = order_list_find(&tmp.m_base.m_node, &user->m_session_que);
    if (NULL != node) {
        dst = (SessionAccpt*)node;

        return dst;
    } else {
        return NULL;
    }
}

Int32 UserCenter::startAgent(FdInfo* info, Uint32 usrId, Uint32 sessId) {
    Int32 ret = 0;
    MsgHdr* hdr = NULL;
    MsgStartPeer* startMsg = NULL;    

    hdr = MsgCenter::creat<MsgStartPeer>(ENUM_MSG_CMD_START_PEER);
    startMsg = MsgCenter::cast<MsgStartPeer>(hdr);
    startMsg->m_user_id = usrId;
    startMsg->m_session_id = sessId;

    MsgCenter::addCrc(hdr);

    ret = m_mng->sendMsg(info, hdr);
    return ret;
}

Int32 UserCenter::stopAgent(FdInfo* info, Uint32 usrId, 
    Uint32 sessId, Int32 reason) {
    Int32 ret = 0;
    MsgHdr* hdr = NULL;
    MsgStopPeer* msg = NULL;    

    hdr = MsgCenter::creat<MsgStopPeer>(ENUM_MSG_CMD_STOP_PEER);
    msg = MsgCenter::cast<MsgStopPeer>(hdr);
    msg->m_user_id = usrId;
    msg->m_session_id = sessId;
    msg->m_reason = reason;

    MsgCenter::addCrc(hdr);

    ret = m_mng->sendMsg(info, hdr);
    return ret;
}

Int32 UserCenter::notifyCloseSock(FdInfo* info, Int32 reason) {
    Int32 ret = 0;
    MsgHdr* hdr = NULL;
    MsgCloseSock* msg = NULL;    

    hdr = MsgCenter::creat<MsgCloseSock>(ENUM_MSG_CMD_CLOSE_SOCK);
    msg = MsgCenter::cast<MsgCloseSock>(hdr);
    msg->m_reason = reason;

    MsgCenter::addCrc(hdr);

    ret = m_mng->dispatchMsg(info, hdr);
    return ret;
}

Int32 UserCenter::notifyCloseChild(FdInfo* info, Uint32 usrId, 
    Uint32 sessId,Int32 reason) {
    Int32 ret = 0;
    MsgHdr* hdr = NULL;
    MsgCloseChild* msg = NULL;    

    hdr = MsgCenter::creat<MsgCloseChild>(ENUM_MSG_CMD_CLOSE_CHILD);
    msg = MsgCenter::cast<MsgCloseChild>(hdr);
    msg->m_user_id = usrId;
    msg->m_session_id = sessId;
    msg->m_reason = reason;

    MsgCenter::addCrc(hdr);

    ret = m_mng->dispatchMsg(info, hdr);
    return ret;
}

Int32 UserCenter::sendUsrArrival(FdInfo* info, Uint32 usrId) {
    Int32 ret = 0;
    MsgHdr* hdr = NULL;
    MsgUserArrival* msg = NULL;    

    hdr = MsgCenter::creat<MsgUserArrival>(ENUM_MSG_USR_ARRIVAL);
    msg = MsgCenter::cast<MsgUserArrival>(hdr);
    msg->m_user_id = usrId;

    MsgCenter::addCrc(hdr);

    ret = m_mng->sendMsg(info, hdr);
    return ret;
}

Int32 UserCenter::sendSessArrival(FdInfo* info, 
    Uint32 usrId, Uint32 sessId) {
    Int32 ret = 0;
    MsgHdr* hdr = NULL;
    MsgSessArrival* msg = NULL;    

    hdr = MsgCenter::creat<MsgSessArrival>(ENUM_MSG_SESS_ARRIVAL);
    msg = MsgCenter::cast<MsgSessArrival>(hdr);
    msg->m_user_id = usrId;
    msg->m_session_id = sessId;

    MsgCenter::addCrc(hdr);

    ret = m_mng->sendMsg(info, hdr);
    return ret;
}

Int32 UserCenter::routeMsg(FdInfo* info, MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0; 
    
    MsgCenter::reopen(msg);
    ret = m_mng->sendMsg(info, msg);
    if (0 == ret) {
        *pDel = FALSE;
        
        return 0;
    } else {
        return ENUM_ERR_SEND_MSG;
    }
}

Int32 UserCenter::stopGateway(FdInfo* info, Uint32 route_id, Int32 reason) {
    Int32 ret = 0;
    MsgHdr* hdr = NULL;
    MsgStopGateway* msg = NULL;    

    hdr = MsgCenter::creat<MsgStopGateway>(ENUM_MSG_CMD_STOP_GATEWAY);
    msg = MsgCenter::cast<MsgStopGateway>(hdr);
    msg->m_route_id = route_id;
    msg->m_reason = reason;

    MsgCenter::addCrc(hdr);

    ret = m_mng->sendMsg(info, hdr);
    return ret;
}

Int32 UserCenter::startAuth(UserConn* usrConn) {
    Int32 ret = 0;
    MsgHdr* hdr = NULL;
    MsgUserAuthReq* req = NULL;    

    hdr = MsgCenter::creat<MsgUserAuthReq>(ENUM_MSG_USR_AUTH_REQ);
    req = MsgCenter::cast<MsgUserAuthReq>(hdr);
    req->m_user_id = usrConn->m_user_id;

    MsgCenter::addCrc(hdr);

    ret = m_mng->sendMsg(usrConn->m_fdinfo, hdr);
    if (0 == ret) {
        usrConn->m_usr_status = ENUM_USER_AUTH_REQ;

        LOG_DEBUG("start_auth| user_id=%u| msg=start now|", usrConn->m_user_id);
    } else {
        LOG_ERROR("start_auth| user_id=%u| msg=invalid", usrConn->m_user_id);
    }
    
    return ret;
}

Int32 UserCenter::startRouterAuth(RouterPair* routerPair) {
    Int32 ret = 0;
    MsgHdr* hdr = NULL;
    MsgUserAuthReq* req = NULL;    

    hdr = MsgCenter::creat<MsgUserAuthReq>(ENUM_MSG_USR_AUTH_REQ);
    req = MsgCenter::cast<MsgUserAuthReq>(hdr);
    req->m_user_id = routerPair->m_router_id;

    MsgCenter::addCrc(hdr);

    ret = m_mng->sendMsg(routerPair->m_router_out.m_fdinfo, hdr);
    if (0 == ret) {
        routerPair->m_router_out.m_usr_status = ENUM_USER_AUTH_REQ;

        LOG_DEBUG("start_router_auth| router_id=%u| msg=start now|", 
            routerPair->m_router_id);
    } else {
        LOG_ERROR("start_router_auth| router_id=%u| msg=invalid", 
            routerPair->m_router_id);
    }
    
    return ret;
}

Uint32 UserCenter::hashMac(EvpBase* evp, const Void* data, Int32 len) {
    Uint32 mac = 0;
    Int32 outlen = 0;
    Byte buf[DEF_SM3_DIGEST_SIZE];

    /* calc hash code with sm3 */
    outlen = digest(evp, data, len, buf);
    mac = MsgCenter::calcCrc(buf, outlen);
    return mac;
}

MsgHdr* UserCenter::decCipherText(EvpBase* evp, MsgHdr* input) {
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
    } 

    /* try to get a prelen */
    pre_len = decrypt(evp, cipher->m_data, cipher->m_cipher_len, NULL);

    output = MsgCenter::creat<MsgTcpPlain>(ENUM_MSG_CMD_TCP_PLAIN, pre_len);
    output->m_seq = input->m_seq;
    
    plain = MsgCenter::cast<MsgTcpPlain>(output);
    
    /* decrypt data with sm4 */
    dec_len = decrypt(evp, cipher->m_data, cipher->m_cipher_len, plain->m_data);
    
    MsgCenter::shrink<MsgTcpPlain>(dec_len, output);
    
    plain->m_user_id = cipher->m_user_id;
    plain->m_session_id = cipher->m_session_id;
    plain->m_data_len = dec_len;

    MsgCenter::addCrc(output);

    if (dec_len == cipher->m_plain_len) {
        LOG_DEBUG("cipher_to_plain| user_id=%u| sess_id=%u|"
            " cipher_len=%d| plain_len=%d| pre_len=%d|"
            " dec_len=%d| msg=ok|",
            cipher->m_user_id, cipher->m_session_id,
            cipher->m_cipher_len, cipher->m_plain_len,
            pre_len, dec_len);
    } else {
        LOG_ERROR("cipher_to_plain| user_id=%u| sess_id=%u|"
            " cipher_len=%d| plain_len=%d| pre_len=%d|"
            " dec_len=%d| msg=dec invalid|",
            cipher->m_user_id, cipher->m_session_id,
            cipher->m_cipher_len, cipher->m_plain_len,
            pre_len, dec_len);
    }

    return output;
}

MsgHdr* UserCenter::encPlainText(EvpBase* evp, MsgHdr* input) {
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

Void UserCenter::setCipherKey(EvpBase* evp, 
    const Void* usr_key, Int32 len) {
    evp->m_cipher.setUsrKey(usr_key, len);
}

Int32 UserCenter::encrypt(EvpBase* evp, const Void* data, 
    Int32 len, Void* out) {
    Int32 outlen = 0;
    
    outlen = evp->m_cipher.sm4_cbc_encrypt(data, len, out);
    return outlen;
}

Int32 UserCenter::decrypt(EvpBase* evp, const Void* data, 
    Int32 len, Void* out) {
    Int32 outlen = 0;
    
    outlen = evp->m_cipher.sm4_cbc_decrypt(data, len, out);
    return outlen;
}

Int32 UserCenter::digest(EvpBase* evp, const Void* data, 
    Int32 len, Void* out) {
    Int32 outlen = 0;

    outlen = evp->m_digest.digest(data, len, out);

    return outlen;
}

