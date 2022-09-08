#include"authuser.h"
#include"managecenter.h"
#include"usercenter.h"
#include"msgtype.h"
#include"msgcenter.h"


UsrAccptAuth::UsrAccptAuth(ManageCenter* mng, UserCenter* usr_center) {
    m_mng = mng;
    m_usr_center = usr_center;
}

Int32 UsrAccptAuth::process(UserAccpt* usrAccpt, MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    
    if (ENUM_MSG_CMD_TCP_CIPHER == msg->m_cmd) {
        ret = procCipherData(usrAccpt, msg, pDel); 
    } else if (ENUM_MSG_CMD_START_PEER == msg->m_cmd) {
        ret = procStartSess(usrAccpt, msg, pDel);
    } else if (ENUM_MSG_CMD_STOP_PEER == msg->m_cmd) {
        ret = procStopSess(usrAccpt, msg, pDel);
    } else if (ENUM_MSG_USR_AUTH_REQ == msg->m_cmd) {
        ret = procAuthReq(usrAccpt, msg, pDel);
    } else if (ENUM_MSG_USR_CIPHER_KEY_ACK == msg->m_cmd) {
        ret = procCipherKeyAck(usrAccpt, msg, pDel);
    } else if (ENUM_MSG_CMD_STOP_GATEWAY == msg->m_cmd) {
        ret = procStopUsrAccpt(usrAccpt, msg, pDel);
    } else if (ENUM_MSG_CMD_CLOSE_CHILD == msg->m_cmd) {
        ret = procCloseChild(usrAccpt, msg, pDel);
    } else if (ENUM_MSG_CMD_CLOSE_SOCK == msg->m_cmd) {
        ret = procCloseUsr(usrAccpt, msg, pDel);
    } else {
        MsgCenter::printMsg("usr_accpt_proc_err", msg);
        ret = -1;
    } 

    return ret;
}

Int32 UsrAccptAuth::procCipherData(UserAccpt* usrAccpt, 
    MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    MsgHdr* output = NULL;
    MsgTcpCipher* cipher = NULL;
    SessionConn* sess = NULL; 

    cipher = MsgCenter::cast<MsgTcpCipher>(msg);
    
    do {
        if (ENUM_USER_LOGIN != usrAccpt->m_usr_status) {
            ret = ENUM_ERR_NOT_LOGIN_GATEWAY;
            break;
        } 

        sess = m_usr_center->findSessConn(cipher->m_session_id, usrAccpt);
        if (NULL == sess) {
            ret = ENUM_ERR_INVALID_SESS;
            break;
        }
        
        output = m_usr_center->decCipherText(usrAccpt->m_evp_rcv, msg);
        
        /* if send to peer, then offset my header */
        MsgCenter::offsetTcpRaw(output);
        
        m_mng->sendMsg(sess->m_fdinfo, output); 

        return 0;
    } while (0);

    m_usr_center->stopAgent(usrAccpt->m_fdinfo, 
        cipher->m_user_id, 
        cipher->m_session_id, 
        ret);

    LOG_ERROR("procCipherData| ret=%d| user_id=%u| sess_id=%u| msg=invalid|",
        ret, cipher->m_user_id, cipher->m_session_id);

    return ret;
}

Int32 UsrAccptAuth::procStartSess(UserAccpt* usrAccpt, 
    MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    SessionConn* sess = NULL;
    MsgStartPeer* startMsg = NULL;

    startMsg = MsgCenter::cast<MsgStartPeer>(msg);
    
    do {
        if (ENUM_USER_LOGIN != usrAccpt->m_usr_status) {
            ret = ENUM_ERR_NOT_LOGIN_USER;
            break;
        } 
        
        sess = m_usr_center->findSessConn(startMsg->m_session_id, usrAccpt);
        if (NULL != sess) { 
            ret = ENUM_ERR_SESS_ALREADY_EXISTS;
            break;
        }
        
        ret = m_mng->startSessConn(&usrAccpt->m_parent->m_tcp_pairs->m_peer,
            startMsg->m_session_id, usrAccpt);
        if (0 != ret) {
            ret = ENUM_ERR_CONN_SESSEION;
            break;
        }
        
        LOG_DEBUG("procStartSess| user_id=%u| session_id=%u| msg=ok|",
            startMsg->m_user_id, startMsg->m_session_id);
        
        return 0;
    } while (0);
    
    m_usr_center->stopAgent(usrAccpt->m_fdinfo, 
        startMsg->m_user_id, 
        startMsg->m_session_id, 
        ret);
    
    LOG_ERROR("procStartSess| ret=%d| user_id=%u|"
        " session_id=%u| msg=error|",
        ret, startMsg->m_user_id, startMsg->m_session_id);
    
    return ret;
}

Int32 UsrAccptAuth::procStopSess(UserAccpt* usrAccpt, 
    MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    SessionConn* sess = NULL; 
    MsgStopPeer* stopMsg = MsgCenter::cast<MsgStopPeer>(msg); 

    do {
        sess = m_usr_center->findSessConn(stopMsg->m_session_id, usrAccpt);
        if (NULL == sess) {
            ret = ENUM_ERR_INVALID_SESS;
            break;
        } 

        ret = m_mng->dispatchMsg(sess->m_fdinfo, msg);
        if (0 == ret) {
            *pDel = FALSE;

            LOG_DEBUG("procStopSess| user_id=%u| session_id=%u|"
                " reason=%d| msg=forward to session ok|",
                stopMsg->m_user_id, stopMsg->m_session_id, 
                stopMsg->m_reason);
        } else {
            *pDel = TRUE;

            ret = ENUM_ERR_DISPATCH_MSG;
            break;
        }
        
        return 0;
    } while (0);

    LOG_ERROR("procStopSess| ret=%d| user_id=%u| session_id=%u|"
        " reason=%d| msg=process failed|",
        ret, stopMsg->m_user_id, 
        stopMsg->m_session_id,
        stopMsg->m_reason);

    return ret;
}

Int32 UsrAccptAuth::procAuthReq(UserAccpt* usrAccpt, 
    MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    MsgHdr* output = NULL;
    MsgUserExchKey* rsp = NULL;
    MsgUserAuthReq* req = NULL;

    req = MsgCenter::cast<MsgUserAuthReq>(msg);

    do { 
        if (ENUM_USER_WAIT_REQ != usrAccpt->m_usr_status) {
            ret = -1;
            break;
        }

        /* check info */
        
        output = MsgCenter::creat<MsgUserExchKey>(ENUM_MSG_USR_EXCH_KEY);
        rsp = MsgCenter::cast<MsgUserExchKey>(msg); 

        /* filll response info */

        MsgCenter::addCrc(output);
        ret = m_mng->sendMsg(usrAccpt->m_fdinfo, output);
        if (0 != ret) {
            break;
        }

        usrAccpt->m_usr_status = ENUM_USER_EXCH_KEY;

        LOG_DEBUG("procAuthReq| user_id=%u| src_seid=%s|"
            " dst_seid=%s| msg=auth ok|",
            req->m_user_id, req->m_src_seid, req->m_dst_seid);
        
        return 0;
    } while (0);

    LOG_ERROR("procAuthReq| ret=%d| user_id=%u| src_seid=%s|"
        " dst_seid=%s| msg=auth error|",
        ret, req->m_user_id, 
        req->m_src_seid, req->m_dst_seid);

    return ret;
}

Int32 UsrAccptAuth::procCipherKeyAck(UserAccpt* usrAccpt, 
    MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    MsgHdr* output = NULL;
    MsgUserCKeyAck* req = NULL;
    MsgUserAuthEnd* rsp = NULL;

    req = MsgCenter::cast<MsgUserCKeyAck>(msg);

    do {
        if (ENUM_USER_EXCH_KEY != usrAccpt->m_usr_status) {
            ret = -1;
            break;
        } 

        output = MsgCenter::creat<MsgUserAuthEnd>(ENUM_MSG_USR_AUTH_END);
        rsp = MsgCenter::cast<MsgUserAuthEnd>(msg); 

        MsgCenter::addCrc(output);
        
        ret = m_mng->sendMsg(usrAccpt->m_fdinfo, output);
        if (0 != ret) {
            break;
        }

        /* set cipher key */
        m_usr_center->setCipherKey(usrAccpt->m_evp_rcv, DEF_SM4_KEY, DEF_SM4_KEY_LEN);
        m_usr_center->setCipherKey(usrAccpt->m_evp_snd, DEF_SM4_KEY, DEF_SM4_KEY_LEN);

        usrAccpt->m_usr_status = ENUM_USER_LOGIN;

        LOG_DEBUG("procCipherKeyAck| user_id=%u| src_seid=%s|"
            " dst_seid=%s| msg=key ack ok|",
            req->m_user_id, req->m_src_seid, req->m_dst_seid);

        /* notify user arrival */
        m_usr_center->sendUsrArrival(usrAccpt->m_fdinfo, usrAccpt->m_user_id); 
        return 0;
    } while (0);

    LOG_ERROR("procCipherKeyAck| ret=%d| user_id=%u| src_seid=%s|"
        " dst_seid=%s| msg=key ack error|",
        ret, req->m_user_id, 
        req->m_src_seid, req->m_dst_seid);
    
    return ret;
}

Int32 UsrAccptAuth::procStopUsrAccpt(UserAccpt* usrAccpt, 
    MsgHdr* msg, Bool* pDel) {
    MsgStopGateway* req = NULL;    

    req = MsgCenter::cast<MsgStopGateway>(msg);

    /* notify to close */
    if (!usrAccpt->m_fdinfo->m_peer_err) {
        usrAccpt->m_fdinfo->m_peer_err = TRUE;
    }

    LOG_INFO("stop_usr_accpt| req_router_id=%u| self_usr_id=%u|"
        " msg=stop user acception|",
        req->m_route_id, usrAccpt->m_user_id);
    
    return 0;
}

Int32 UsrAccptAuth::procCloseChild(UserAccpt* usrAccpt, 
    MsgHdr* msg, Bool* pDel) {
    SessionConn* sess = NULL;
    MsgCloseChild* req = NULL;    

    req = MsgCenter::cast<MsgCloseChild>(msg);

    sess = m_usr_center->findSessConn(req->m_session_id, usrAccpt);
    if (NULL != sess) {
        order_list_del(&sess->m_base.m_node, &usrAccpt->m_session_que);
        
        m_usr_center->freeData(sess);
    } 

    LOG_INFO("proc_close_child| user_id=%u| session_id=%u| reason=%d| msg=ok|",
        req->m_user_id, req->m_session_id, req->m_reason);
    
    return 0;
}

Int32 UsrAccptAuth::procCloseUsr(UserAccpt* usrAccpt, 
    MsgHdr* msg, Bool* pDel) {
    ListenerDirty* parent = usrAccpt->m_parent;
    MsgCloseSock* closeMsg = MsgCenter::cast<MsgCloseSock>(msg);
    

    LOG_INFO("procCloseUsr| user_id=%u| reason=%d| msg=ok|",
        usrAccpt->m_user_id, closeMsg->m_reason); 

    /* end to parent */
    return 1;
}


UsrConnAuth::UsrConnAuth(ManageCenter* mng, UserCenter* usr_center) {
    m_mng = mng;
    m_usr_center = usr_center;
}

Int32 UsrConnAuth::process(UserConn* usrConn, MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    
    if (ENUM_MSG_CMD_TCP_CIPHER == msg->m_cmd) {
        ret = procCipherData(usrConn, msg, pDel); 
    } else if (ENUM_MSG_SESS_ARRIVAL == msg->m_cmd) {
        ret = procSessArrival(usrConn, msg, pDel); 
    } else if (ENUM_MSG_USR_ARRIVAL == msg->m_cmd) {
        ret = procUsrArrival(usrConn, msg, pDel); 
    } else if (ENUM_MSG_CMD_STOP_PEER == msg->m_cmd) {
        ret = procStopSess(usrConn, msg, pDel);
    } else if (ENUM_MSG_USR_EXCH_KEY == msg->m_cmd) {
        ret = procExchKey(usrConn, msg, pDel);
    } else if (ENUM_MSG_USR_AUTH_END == msg->m_cmd) {
        ret = procAuthEnd(usrConn, msg, pDel);
    } else if (ENUM_MSG_CMD_STOP_GATEWAY == msg->m_cmd) {
        ret = procStopUsrConn(usrConn, msg, pDel); 
    } else if (ENUM_MSG_CMD_CLOSE_CHILD == msg->m_cmd) {
        ret = procCloseChild(usrConn, msg, pDel);
    } else if (ENUM_MSG_CMD_CLOSE_SOCK == msg->m_cmd) {
        ret = procCloseUsr(usrConn, msg, pDel);
    } else {
        MsgCenter::printMsg("usr_conn_proc_err", msg);
        ret = -1;
    }

    return ret;
}

Int32 UsrConnAuth::procCipherData(UserConn* usrConn, 
    MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    MsgHdr* output = NULL;
    MsgTcpCipher* cipher = NULL;
    SessionAccpt* sess = NULL; 

    cipher = MsgCenter::cast<MsgTcpCipher>(msg);

    do {
        if (ENUM_USER_LOGIN != usrConn->m_usr_status) {
            ret = ENUM_ERR_NOT_LOGIN_GATEWAY;
            break;
        }
        
        sess = m_usr_center->findSessAccpt(cipher->m_session_id, usrConn);
        if (NULL == sess) {
            ret = ENUM_ERR_INVALID_SESS;
            break;
        }
    
        output = m_usr_center->decCipherText(usrConn->m_evp_rcv, msg);

        /* if send to peer, then offset my header */
        MsgCenter::offsetTcpRaw(output); 
        
        m_mng->sendMsg(sess->m_fdinfo, output);
        
        return 0; 
    } while (0);

    m_usr_center->stopAgent(usrConn->m_fdinfo, 
        cipher->m_user_id, 
        cipher->m_session_id, 
        ENUM_ERR_INVALID_SESS);

    LOG_ERROR("procCipherData| ret=%d| user_id=%u|"
        " sess_id=%u| msg=invalid|",
        ret, cipher->m_user_id, cipher->m_session_id);

    return ret;
}

Int32 UsrConnAuth::procStopSess(UserConn* usrConn, 
    MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    SessionAccpt* sess = NULL; 
    MsgStopPeer* stopMsg = MsgCenter::cast<MsgStopPeer>(msg);

    do {
        sess = m_usr_center->findSessAccpt(stopMsg->m_session_id, usrConn);
        if (NULL == sess) {
            ret = ENUM_ERR_INVALID_SESS;
            break;
        } 

        ret = m_mng->dispatchMsg(sess->m_fdinfo, msg);
        if (0 == ret) {
            *pDel = FALSE;

            LOG_DEBUG("procStopSess| user_id=%u| session_id=%u|"
                " reason=%d| msg=forward to session ok|",
                stopMsg->m_user_id, stopMsg->m_session_id, 
                stopMsg->m_reason);
        } else {
            *pDel = TRUE;

            ret = ENUM_ERR_DISPATCH_MSG;
            break;
        }
        
        return 0;
    } while (0);

    LOG_ERROR("procStopSess| ret=%d| user_id=%u| session_id=%u|"
        " reason=%d| msg=process failed|",
        ret, stopMsg->m_user_id, 
        stopMsg->m_session_id,
        stopMsg->m_reason);

    return ret;
}

Int32 UsrConnAuth::procExchKey(UserConn* usrConn, 
    MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    MsgHdr* output = NULL;
    MsgUserExchKey* req = NULL;
    MsgUserCKeyAck* rsp = NULL;

    req = MsgCenter::cast<MsgUserExchKey>(msg);

    do {
        if (ENUM_USER_AUTH_REQ != usrConn->m_usr_status) {
            ret = -1;
            break;
        }

        /* check info */
        
        output = MsgCenter::creat<MsgUserCKeyAck>(ENUM_MSG_USR_CIPHER_KEY_ACK);
        rsp = MsgCenter::cast<MsgUserCKeyAck>(msg); 

        /* filll response info */
        
        MsgCenter::addCrc(output); 
        ret = m_mng->sendMsg(usrConn->m_fdinfo, output);
        if (0 != ret) {
            break;
        }

        usrConn->m_usr_status = ENUM_USER_EXCH_KEY_ACK;

        LOG_DEBUG("procExchKey| user_id=%u| src_seid=%s|"
            " dst_seid=%s| msg=key exchange ok|",
            req->m_user_id, req->m_src_seid, req->m_dst_seid);
        
        return 0;
    } while (0);

    LOG_ERROR("procExchKey| ret=%d| user_id=%u| src_seid=%s|"
        " dst_seid=%s| msg=key exchange error|",
        ret, req->m_user_id, req->m_src_seid, req->m_dst_seid);

    return ret;
}

Int32 UsrConnAuth::procAuthEnd(UserConn* usrConn, 
    MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    MsgHdr* output = NULL;
    MsgUserAuthEnd* req = NULL;

    req = MsgCenter::cast<MsgUserAuthEnd>(msg);

    do {
        if (ENUM_USER_EXCH_KEY_ACK != usrConn->m_usr_status) {
            ret = -1;
            break;
        }
        
        /* set cipher key */
        m_usr_center->setCipherKey(usrConn->m_evp_rcv, DEF_SM4_KEY, DEF_SM4_KEY_LEN);
        m_usr_center->setCipherKey(usrConn->m_evp_snd, DEF_SM4_KEY, DEF_SM4_KEY_LEN);

        usrConn->m_usr_status = ENUM_USER_LOGIN;

        LOG_DEBUG("procAuthEnd| user_id=%u| src_seid=%s|"
            " dst_seid=%s| msg=auth end ok|",
            req->m_user_id, req->m_src_seid, req->m_dst_seid);
        
        return 0;
    } while (0);

    LOG_ERROR("procAuthEnd| ret=%d| user_id=%u| src_seid=%s|"
        " dst_seid=%s| msg=auth end error|",
        ret, req->m_user_id, req->m_src_seid, req->m_dst_seid);

    return ret;
}

Int32 UsrConnAuth::procStopUsrConn(UserConn* usrConn,
    MsgHdr* msg, Bool* pDel) {
    MsgHdr* hdr = NULL;
    MsgStopGateway* req = NULL;    

    req = MsgCenter::cast<MsgStopGateway>(msg);

    /* notify to close */
    if (!usrConn->m_fdinfo->m_peer_err) {
        usrConn->m_fdinfo->m_peer_err = TRUE;
    }

    LOG_INFO("stop_usr_conn| req_router_id=%u| self_usr_id=%u|"
        " msg=stop user connection|",
        req->m_route_id, usrConn->m_user_id);
    
    return 0;
}

Int32 UsrConnAuth::procUsrArrival(UserConn* usrConn,
    MsgHdr* msg, Bool* pDel) {
    ListenerTcp* parent = usrConn->m_parent;
    MsgUserArrival* req = NULL;    
    
    req = MsgCenter::cast<MsgUserArrival>(msg);

    /* start to accept tcp session */
    parent->m_tunnel_ok = TRUE;

    LOG_INFO("proc_usr_arrival| user_id=%u| self_usr_id=%u|"
        " msg=user arrival ok|",
        req->m_user_id, usrConn->m_user_id);
    
    return 0;
}

Int32 UsrConnAuth::procSessArrival(UserConn* usrConn,
    MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    SessionAccpt* sess = NULL; 
    MsgHdr* hdr = NULL;
    MsgSessArrival* req = NULL;    

    req = MsgCenter::cast<MsgSessArrival>(msg);

    do {
        if (ENUM_USER_LOGIN != usrConn->m_usr_status) {
            ret = ENUM_ERR_NOT_LOGIN_GATEWAY;
            break;
        }
        
        sess = m_usr_center->findSessAccpt(req->m_session_id, usrConn);
        if (NULL == sess) {
            ret = ENUM_ERR_INVALID_SESS;
            break;
        }
        
        /* dispatch this msg to sess */
        ret = m_mng->dispatchMsg(sess->m_fdinfo, msg);
        if (0 == ret) {
            *pDel = FALSE;

            LOG_DEBUG("proc_sess_arrival| user_id=%u| sess_id=%u|"
                " msg=forward sess arrival ok|",
                req->m_user_id, req->m_session_id);
                    
        } else {
            ret = ENUM_ERR_SEND_MSG;
            break;
        }

        return 0;
    } while (0);
    

    LOG_ERROR("proc_sess_arrival| user_id=%u| sess_id=%u| ret=%d|"
        " msg=forward sess arrival error|",
        req->m_user_id, req->m_session_id, ret);
    
    return ret;
}

Int32 UsrConnAuth::procCloseChild(UserConn* usrConn, 
    MsgHdr* msg, Bool* pDel) {
    SessionAccpt* sess = NULL;
    MsgHdr* hdr = NULL;
    MsgCloseChild* req = NULL;    

    req = MsgCenter::cast<MsgCloseChild>(msg); 
   
    sess = m_usr_center->findSessAccpt(req->m_session_id, usrConn);
    if (NULL != sess) {
        order_list_del(&sess->m_base.m_node, &usrConn->m_session_que);
        
        m_usr_center->freeData(sess);
    } 

    LOG_INFO("proc_close_child| user_id=%u| session_id=%u| reason=%d| msg=ok|",
        req->m_user_id, req->m_session_id, req->m_reason);
    
    return 0;
}

Int32 UsrConnAuth::procCloseUsr(UserConn* usrConn, 
    MsgHdr* msg, Bool* pDel) {
    ListenerTcp* parent = usrConn->m_parent;
    MsgCloseSock* closeMsg = MsgCenter::cast<MsgCloseSock>(msg);
    

    LOG_INFO("procCloseUsr| user_id=%u| reason=%d| msg=ok|",
        usrConn->m_user_id, closeMsg->m_reason); 

    /* end to parent */
    return 1;
}

