#include"authuser.h"
#include"managecenter.h"
#include"usercenter.h"
#include"msgtype.h"
#include"msgcenter.h"


UsrAccptAuth::UsrAccptAuth(ManageCenter* mng, UserCenter* usr_center) {
    m_mng = mng;
    m_usr_center = usr_center;
}

Void UsrAccptAuth::process(UserAccpt* usrAccpt, MsgHdr* msg) {
    Int32 ret = 0;
    
    if (ENUM_MSG_CMD_TCP_CIPHER == msg->m_cmd) {
        ret = procCipherData(usrAccpt, msg); 
    } else if (ENUM_MSG_CMD_START_PEER == msg->m_cmd) {
        ret = procStartSess(usrAccpt, msg);
    } else if (ENUM_MSG_CMD_STOP_PEER == msg->m_cmd) {
        ret = procStopSess(usrAccpt, msg);
    } else if (ENUM_MSG_USR_AUTH_REQ == msg->m_cmd) {
        ret = procAuthReq(usrAccpt, msg);
    } else if (ENUM_MSG_USR_CIPHER_KEY_ACK == msg->m_cmd) {
        ret = procCipherKeyAck(usrAccpt, msg);
    } else {
    } 
}

Int32 UsrAccptAuth::procCipherData(UserAccpt* usrAccpt, MsgHdr* msg) {
    Int32 ret = 0;
    MsgHdr* output = NULL;
    MsgTcpCipher* cipher = NULL;
    SessionConn* sess = NULL; 

    cipher = MsgCenter::cast<MsgTcpCipher>(msg);
    
    do {
        if (ENUM_USER_LOGIN != usrAccpt->m_usr_status) {
            ret = -1;
            break;
        } 

        sess = m_usr_center->findSessConn(cipher->m_session_id, usrAccpt);
        if (NULL == sess) {
            ret = -2;
            break;
        }
        
        output = m_usr_center->decCipherText(usrAccpt->m_evp, msg);
        
        /* if send to peer, then offset my header */
        MsgCenter::offsetTcpRaw(output);
        
        m_mng->sendMsg(sess->m_fdinfo, output); 

        return 0;
    } while (0);

    m_usr_center->stopAgent(usrAccpt->m_fdinfo, 
        cipher->m_user_id, 
        cipher->m_session_id, 
        ENUM_ERR_INVALID_SESS);

    LOG_ERROR("procCipherData| ret=%d| user_id=%u| sess_id=%u| msg=invalid|",
        ret, cipher->m_user_id, cipher->m_session_id);

    return ret;
}

Int32 UsrAccptAuth::procStartSess(UserAccpt* usrAccpt, MsgHdr* msg) {
    Int32 ret = 0;
    SessionConn* sess = NULL;
    MsgStartPeer* startMsg = NULL;

    startMsg = MsgCenter::cast<MsgStartPeer>(msg);
    
    do {
        if (ENUM_USER_LOGIN != usrAccpt->m_usr_status) {
            ret = -1;
            break;
        } 
        
        sess = m_usr_center->findSessConn(startMsg->m_session_id, usrAccpt);
        if (NULL != sess) { 
            ret = -2;
            break;
        }
        
        ret = m_mng->startSessConn(&usrAccpt->m_parent->m_tcp_pairs->m_peer,
            startMsg->m_session_id, usrAccpt);
        if (0 != ret) {
            ret = -3;
            break;
        }
        
        LOG_DEBUG("procStartSess| user_id=%u| session_id=%u| msg=ok|",
            startMsg->m_user_id, startMsg->m_session_id);
        
        return 0;
    } while (0);
    
    m_usr_center->stopAgent(usrAccpt->m_fdinfo, 
        startMsg->m_user_id, 
        startMsg->m_session_id, 
        ENUM_ERR_CONN_SERVER);
    
    LOG_ERROR("procStartSess| ret=%d| user_id=%u|"
        " session_id=%u| msg=error|",
        ret, startMsg->m_user_id, startMsg->m_session_id);
    
    return ret;
}

Int32 UsrAccptAuth::procStopSess(UserAccpt* usrAccpt, MsgHdr* msg) {
    Int32 ret = 0;
    SessionConn* sess = NULL; 
    MsgStopPeer* stopMsg = MsgCenter::cast<MsgStopPeer>(msg);

    do {
        sess = m_usr_center->findSessConn(stopMsg->m_session_id, usrAccpt);
        if (NULL == sess) {
            ret = -1;
            break;
        } 
        
        m_mng->closeFd(sess->m_fdinfo); 

        LOG_DEBUG("procStopSess| user_id=%u| session_id=%u|"
            " reason=%d| msg=closing now|",
            stopMsg->m_user_id, stopMsg->m_session_id, 
            stopMsg->m_reason);

        return 0;
    } while (0);

    LOG_ERROR("procStopSess| ret=%d| user_id=%u| session_id=%u|"
        " reason=%d| msg=invalid|",
        ret, stopMsg->m_user_id, 
        stopMsg->m_session_id,
        stopMsg->m_reason);
    
    return ret;
}

Int32 UsrAccptAuth::procAuthReq(UserAccpt* usrAccpt, MsgHdr* msg) {
    Int32 ret = 0;
    MsgHdr* output = NULL;
    MsgUserExchKey* rsp = NULL;
    MsgUserAuthReq* req = NULL;

    req = MsgCenter::cast<MsgUserAuthReq>(msg);

    do { 
        if (ENUM_USER_INIT != usrAccpt->m_usr_status) {
            ret = -1;
            break;
        }

        /* check info */
        
        output = MsgCenter::creat<MsgUserExchKey>(ENUM_MSG_USR_EXCH_KEY);
        rsp = MsgCenter::cast<MsgUserExchKey>(msg); 

        /* filll response info */

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

Int32 UsrAccptAuth::procCipherKeyAck(UserAccpt* usrAccpt, MsgHdr* msg) {
    Int32 ret = 0;
    MsgUserCKeyAck* req = NULL;

    req = MsgCenter::cast<MsgUserCKeyAck>(msg);

    do {
        if (ENUM_USER_EXCH_KEY != usrAccpt->m_usr_status) {
            ret = -1;
            break;
        }

        /* set cipher key */
        m_usr_center->setCipherKey(usrAccpt->m_evp, DEF_SM4_KEY, DEF_SM4_KEY_LEN);

        usrAccpt->m_usr_status = ENUM_USER_LOGIN;

        LOG_DEBUG("procCipherKeyAck| user_id=%u| src_seid=%s|"
            " dst_seid=%s| msg=login ok|",
            req->m_user_id, req->m_src_seid, req->m_dst_seid);
        
        return 0;
    } while (0);

    LOG_ERROR("procCipherKeyAck| ret=%d| user_id=%u| src_seid=%s|"
        " dst_seid=%s| msg=login error|",
        ret, req->m_user_id, 
        req->m_src_seid, req->m_dst_seid);
    
    return ret;
}


UsrConnAuth::UsrConnAuth(ManageCenter* mng, UserCenter* usr_center) {
    m_mng = mng;
    m_usr_center = usr_center;
}

Void UsrConnAuth::process(UserConn* usrConn, MsgHdr* msg) {
    if (ENUM_MSG_CMD_TCP_CIPHER == msg->m_cmd) {
        procCipherData(usrConn, msg); 
    } else if (ENUM_MSG_CMD_STOP_PEER == msg->m_cmd) {
        procStopSess(usrConn, msg);
    } else if (ENUM_MSG_USR_EXCH_KEY == msg->m_cmd) {
        procExchKey(usrConn, msg);
    }
}

Int32 UsrConnAuth::procCipherData(UserConn* usrConn, MsgHdr* msg) {
    Int32 ret = 0;
    MsgHdr* output = NULL;
    MsgTcpCipher* cipher = NULL;
    SessionAccpt* sess = NULL; 

    cipher = MsgCenter::cast<MsgTcpCipher>(msg);

    do {
        if (ENUM_USER_LOGIN != usrConn->m_usr_status) {
            ret = -1;
            break;
        }
        
        sess = m_usr_center->findSessAccpt(cipher->m_session_id, usrConn);
        if (NULL == sess) {
            ret = -2;
            break;
        }
    
        output = m_usr_center->decCipherText(usrConn->m_evp, msg);

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

Int32 UsrConnAuth::procStopSess(UserConn* usrConn, MsgHdr* msg) {
    Int32 ret = 0;
    SessionAccpt* sess = NULL; 
    MsgStopPeer* stopMsg = MsgCenter::cast<MsgStopPeer>(msg);

    do {
        sess = m_usr_center->findSessAccpt(stopMsg->m_session_id, usrConn);
        if (NULL == sess) {
            ret = -2;
            break;
        } 
        
        m_mng->closeFd(sess->m_fdinfo);

        LOG_DEBUG("procStopSess| user_id=%u| session_id=%u|"
            " reason=%d| msg=closing now|",
            stopMsg->m_user_id, stopMsg->m_session_id, 
            stopMsg->m_reason);
        
        return 0;
    } while (0);

    m_usr_center->stopAgent(usrConn->m_fdinfo, 
        stopMsg->m_user_id, 
        stopMsg->m_session_id, 
        ENUM_ERR_INVALID_SESS);

    LOG_ERROR("procStopSess| ret=%d| user_id=%u| session_id=%u|"
        " reason=%d| msg=invalid|",
        ret, stopMsg->m_user_id, 
        stopMsg->m_session_id,
        stopMsg->m_reason);

    return ret;
}

Int32 UsrConnAuth::procExchKey(UserConn* usrConn, MsgHdr* msg) {
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

        ret = m_mng->sendMsg(usrConn->m_fdinfo, output);
        if (0 != ret) {
            break;
        }
        
        /* set cipher key */
        m_usr_center->setCipherKey(usrConn->m_evp, DEF_SM4_KEY, DEF_SM4_KEY_LEN);

        usrConn->m_usr_status = ENUM_USER_LOGIN;

        LOG_DEBUG("procExchKey| user_id=%u| src_seid=%s|"
            " dst_seid=%s| msg=exchange cipher key ok|",
            req->m_user_id, req->m_src_seid, req->m_dst_seid);
        
        return 0;
    } while (0);

    LOG_ERROR("procExchKey| ret=%d| user_id=%u| src_seid=%s|"
        " dst_seid=%s| msg=exchange cipher key error|",
        ret, req->m_user_id, req->m_src_seid, req->m_dst_seid);

    return ret;
}

