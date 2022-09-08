#include"authsess.h"
#include"msgcenter.h"
#include"managecenter.h"
#include"usercenter.h"
#include"msgtype.h"
#include"sockutil.h"


SessAccptAuth::SessAccptAuth(ManageCenter* mng, UserCenter* usr_center) {
    m_mng = mng;
    m_usr_center = usr_center;
}

Int32 SessAccptAuth::process(SessionAccpt* sessAccpt, 
    MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    
    if (ENUM_MSG_CMD_TCP_PLAIN == msg->m_cmd) {
        ret = procPlainData(sessAccpt, msg, pDel);
    } else if (ENUM_MSG_SESS_ARRIVAL == msg->m_cmd) {
        ret = procSessArrival(sessAccpt, msg, pDel);
    } else if (ENUM_MSG_CMD_STOP_PEER == msg->m_cmd) {
        ret = procStopSess(sessAccpt, msg, pDel);
    } else if (ENUM_MSG_CMD_CLOSE_SOCK == msg->m_cmd) {
        ret = procCloseSess(sessAccpt, msg, pDel);
    } else {
        MsgCenter::printMsg("sess_accpt_proc_err", msg);
        
        ret = -1;
    } 

    return ret;
}

Int32 SessAccptAuth::procPlainData(SessionAccpt* sessAccpt, 
    MsgHdr* msg, Bool* pDel) {
    UserConn* usrConn = sessAccpt->m_parent;
    MsgHdr* output = NULL;
    MsgTcpPlain* plain = NULL;
    
    plain = MsgCenter::cast<MsgTcpPlain>(msg);
    plain->m_user_id = usrConn->m_user_id;
    plain->m_session_id = sessAccpt->m_session_id;
    
    output = m_usr_center->encPlainText(usrConn->m_evp_snd, msg);

    /* out from user conn */
    m_mng->sendMsg(usrConn->m_fdinfo, output);
    return 0;
}

Int32 SessAccptAuth::procSessArrival(SessionAccpt* sessAccpt,
    MsgHdr* msg, Bool* pDel) {
    MsgSessArrival* req = NULL;    

    req = MsgCenter::cast<MsgSessArrival>(msg);

    /* if sess arrived, then start to read data */
    sessAccpt->m_fdinfo->m_test_rd = TRUE;
    
    LOG_INFO("proc_sess_arrival| user_id=%u| sess_id=%u| self_id=%u|"
        " msg=ok|",
        req->m_user_id, req->m_session_id, 
        sessAccpt->m_session_id);
    
    return 0;
}

Int32 SessAccptAuth::procStopSess(SessionAccpt* sessAccpt, 
    MsgHdr* msg, Bool* pDel) {
    MsgStopPeer* stopMsg = MsgCenter::cast<MsgStopPeer>(msg);

    /* notify to close */
    if (!sessAccpt->m_fdinfo->m_peer_err) {
        sessAccpt->m_fdinfo->m_peer_err = TRUE;
    }

    LOG_INFO("procStopSess| user_id=%u| session_id=%u|"
        " self_id=%u| reason=%d| msg=ok|",
        stopMsg->m_user_id, stopMsg->m_session_id, 
        sessAccpt->m_session_id, stopMsg->m_reason);

    return 0;
}

Int32 SessAccptAuth::procCloseSess(SessionAccpt* sessAccp, 
    MsgHdr* msg, Bool* pDel) {
    UserConn* parent = sessAccp->m_parent;
    MsgCloseSock* closeMsg = MsgCenter::cast<MsgCloseSock>(msg);
    
    /* notify peer to close */
    if (!sessAccp->m_fdinfo->m_peer_err) {
        m_usr_center->stopAgent(parent->m_fdinfo, parent->m_user_id,
            sessAccp->m_session_id, closeMsg->m_reason);
    }

    /* notify parent to release resource */
    m_usr_center->notifyCloseChild(parent->m_fdinfo, parent->m_user_id,
        sessAccp->m_session_id, closeMsg->m_reason);

    LOG_INFO("procCloseSess| user_id=%u| session_id=%u| reason=%d| msg=ok|",
        parent->m_user_id, sessAccp->m_session_id, closeMsg->m_reason); 

    /* end to parent */
    return 1;
}

SessConnAuth::SessConnAuth(ManageCenter* mng, UserCenter* usr_center) {
    m_mng = mng;
    m_usr_center = usr_center;
}

Int32 SessConnAuth::process(SessionConn* sessConn, MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    
    if (ENUM_MSG_CMD_TCP_PLAIN == msg->m_cmd) {
        ret = procPlainData(sessConn, msg, pDel);
    } else if (ENUM_MSG_CMD_STOP_PEER == msg->m_cmd) {
        ret = procStopSess(sessConn, msg, pDel);
    } else if (ENUM_MSG_CMD_CLOSE_SOCK == msg->m_cmd) {
        ret = procCloseSess(sessConn, msg, pDel);
    } else {
        MsgCenter::printMsg("sess_conn_proc_err", msg);
        ret = -1;
    }

    return ret;
}

Int32 SessConnAuth::procPlainData(SessionConn* sessConn, 
    MsgHdr* msg, Bool* pDel) {
    UserAccpt* usrAccpt = sessConn->m_parent;
    MsgHdr* output = NULL;
    MsgTcpPlain* plain = NULL;
    
    plain = MsgCenter::cast<MsgTcpPlain>(msg);
    plain->m_user_id = usrAccpt->m_user_id;
    plain->m_session_id = sessConn->m_session_id;

    output = m_usr_center->encPlainText(usrAccpt->m_evp_snd, msg); 

    /* out from user conn */
    m_mng->sendMsg(usrAccpt->m_fdinfo, output);
    return 0;
}

Int32 SessConnAuth::procStopSess(SessionConn* sessConn, 
    MsgHdr* msg, Bool* pDel) {
    MsgStopPeer* stopMsg = MsgCenter::cast<MsgStopPeer>(msg);

    /* notify to close */
    if (!sessConn->m_fdinfo->m_peer_err) {
        sessConn->m_fdinfo->m_peer_err = TRUE;
    }

    LOG_INFO("procStopSess| user_id=%u| session_id=%u|"
        " self_id=%u| reason=%d| msg=ok|",
        stopMsg->m_user_id, stopMsg->m_session_id, 
        sessConn->m_session_id, stopMsg->m_reason); 

    return 0;
}

Int32 SessConnAuth::procCloseSess(SessionConn* sessConn, 
    MsgHdr* msg, Bool* pDel) {
    UserAccpt* parent = sessConn->m_parent;
    MsgCloseSock* closeMsg = MsgCenter::cast<MsgCloseSock>(msg);

    /* notify peer to close */
    if (!sessConn->m_fdinfo->m_peer_err) {
        m_usr_center->stopAgent(parent->m_fdinfo, parent->m_user_id,
            sessConn->m_session_id, closeMsg->m_reason);
    }

    /* notify parent to release resource */
    m_usr_center->notifyCloseChild(parent->m_fdinfo, parent->m_user_id,
        sessConn->m_session_id, closeMsg->m_reason);
    
    LOG_INFO("procCloseSess| user_id=%u| session_id=%u| reason=%d| msg=ok|",
        parent->m_user_id, sessConn->m_session_id, closeMsg->m_reason);

    /* end to parent */
    return 1;
}

