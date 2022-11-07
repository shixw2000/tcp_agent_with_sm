#include"socksess.h"
#include"sockutil.h"
#include"sockmng.h"
#include"sockcenter.h"
#include"datatype.h"
#include"msgtype.h"
#include"msgcenter.h"
#include"sockoper.h"
#include"sockusr.h"


int SockSessListener::readFd(struct FdInfo* info) {
    ListenerTcp* listener = (ListenerTcp*)info->m_data;

    /* disable continue read */
    info->m_test_rd = FALSE;
    
    m_mng->notify(listener->m_fdinfo, ENUM_MSG_NOTIFY_NEW_SESS);
        
    return 0;
}

int SockSessListener::writeFd(struct FdInfo*) {
    return -1;
}

Int32 SockSessListener::procMsg(FdInfo* info, MsgHdr* msg) {
    Int32 ret = 0;
    ListenerTcp* listener = (ListenerTcp*)info->m_data;

    if (ENUM_MSG_NOTIFY_NEW_SESS == msg->m_cmd) {
        ret = procAddSess(listener, msg);
    } else if (ENUM_MSG_SYSTEM_IO_END == msg->m_cmd) { 
        ret = procEnd(listener, msg);
    } else {
        MsgCenter::free(msg);
    } 
    
    return ret;
}

Void SockSessListener::eof(struct FdInfo* info) {
    ListenerTcp* listener = (ListenerTcp*)info->m_data;
    
    LOG_INFO("sess_listener_exit| fd=%d| local_addr=%s:%d|"
        " peer_addr=%s:%d| msg=exit now|",
        info->m_fd, 
        listener->m_pairs->m_local.m_ip, listener->m_pairs->m_local.m_port,
        listener->m_pairs->m_peer.m_ip, listener->m_pairs->m_peer.m_port);
    
    /* if  fail, then exit */
    m_mng->stop();
}

Int32 SockSessListener::procAddSess(ListenerTcp* listener, MsgHdr* msg) { 
    Int32 newfd = -1;
    SessionAccpt* sess = NULL;
    UserConn* usr = listener->m_user;
    FdInfo* info = listener->m_fdinfo;

    newfd = acceptCli(info->m_fd);
    while (0 <= newfd) { 
        sess = m_accpt->setup(listener, newfd);
        if (NULL != sess) {
            order_list_add(&sess->m_base.m_node, &usr->m_session_que); 

            m_center->sendPeerSess(usr, sess);
        }
        
        newfd = acceptCli(info->m_fd);
    }

    /* enable continue read */
    if (!info->m_test_rd) {
        info->m_test_rd = TRUE;
    }
    
    MsgCenter::free(msg);
    return 0;
}

ListenerTcp* SockSessListener::setup(UserConn* usr, const TcpPairs* pairs) {
    Int32 fd = -1;
    FdInfo* info = NULL;
    ListenerTcp* listener = NULL;

    fd = creatTcpSrv(&pairs->m_local);
    if (0 > fd) {
        return NULL;
    } 

    info = m_mng->creatBase(fd, TRUE, FALSE);
    
    listener = m_center->newListenerTcp();
    listener->m_user = usr;
    listener->m_pairs = pairs;
    
    listener->m_fdinfo = info; 

    LOG_INFO("++++add_sess_listener| fd=%d| user_id=%u|"
        " local_addr=%s:%d| peer_addr=%s:%d| msg=ok|",
        fd, usr->m_user_id, 
        pairs->m_local.m_ip, pairs->m_local.m_port,
        pairs->m_peer.m_ip, pairs->m_peer.m_port);
    
    m_mng->addEvent(info, &listener->m_base);

    return listener;
}

Int32 SockSessListener::procEnd(ListenerTcp* listener, MsgHdr* msg) {
    listener->m_fdinfo->m_closing = TRUE;
    
    m_mng->notify(listener->m_fdinfo, ENUM_MSG_SYSTEM_EXIT);

    MsgCenter::free(msg);
    return 0;
}


int SockSessAccpt::readFd(struct FdInfo* info) {
    Int32 ret = 0;
    SessionAccpt* sess = (SessionAccpt*)info->m_data;

    ret = SockOper::readTcpRaw(sess->m_fdinfo, &sess->m_sock, m_mng);    
    return ret;
    
}

int SockSessAccpt::writeFd(struct FdInfo* info) {
    Int32 ret = 0;
    SessionAccpt* sess = (SessionAccpt*)info->m_data;

    ret = SockOper::writeTcpRaw(sess->m_fdinfo, &sess->m_sock); 
    return ret;
}

Int32 SockSessAccpt::procMsg(FdInfo* info, MsgHdr* msg) {
    Int32 ret = 0;
    SessionAccpt* sess = (SessionAccpt*)info->m_data;

    ret = process(sess, msg); 
    return ret;
}

Void SockSessAccpt::eof(struct FdInfo* info) {
    SessionAccpt* sess = (SessionAccpt*)info->m_data;
    UserConn* usr = sess->m_parent;

    if (sess->m_peer_open) {
        m_center->sendSessEvent(usr->m_fdinfo, ENUM_MSG_SYSTEM_STOP_SESS,
            usr->m_user_id, sess->m_session_id);
        
        sess->m_peer_open = FALSE;
    }

    m_center->notifySessEvent(usr->m_fdinfo, ENUM_MSG_SYSTEM_CHILD_EXIT,
        usr->m_user_id, sess->m_session_id);
    
    return;
}


Int32 SockSessAccpt::process(SessionAccpt* sess, MsgHdr* msg) {
    Int32 ret = 0;
    
    if (ENUM_MSG_CMD_TCP_PLAIN == msg->m_cmd) {
        ret = procPlainData(sess, msg);
    } else if (ENUM_MSG_CMD_TCP_CIPHER == msg->m_cmd) {
        ret = procCipherData(sess, msg);
    } else if (ENUM_MSG_SYSTEM_STOP_SESS == msg->m_cmd) {
        ret = procStopSess(sess, msg);
    } else if (ENUM_MSG_SESS_ARRIVAL == msg->m_cmd) {
        ret = procArrival(sess, msg);
    } else if (ENUM_MSG_SYSTEM_IO_END == msg->m_cmd) { 
        ret = procEnd(sess, msg); 
    } else {
        MsgCenter::printMsg("sess_conn_proc_err", msg);

        MsgCenter::free(msg);
    }

    return ret;
}

Int32 SockSessAccpt::procPlainData(SessionAccpt* sess, MsgHdr* msg) {
    MsgHdr* output = NULL;
    UserConn* usr = sess->m_parent;
    MsgTcpPlain* plain = NULL;
    
    plain = MsgCenter::cast<MsgTcpPlain>(msg);
    plain->m_user_id = usr->m_user_id;
    plain->m_session_id = sess->m_session_id;

    output = m_center->encPlainText(usr->m_evp_snd, msg); 

    /* out from user conn */
    m_mng->sendMsg(usr->m_fdinfo, output);

    MsgCenter::free(msg);
    return 0;
}

Int32 SockSessAccpt::procCipherData(SessionAccpt* sess, MsgHdr* msg) {
    Int32 ret = 0;
    MsgHdr* output = NULL;
    UserConn* usr = sess->m_parent;
    
    do {
        output = m_center->decCipherText(usr->m_evp_rcv, msg);
        if (NULL == output) {
            ret = ENUM_ERR_DECRYPT_TXT;
            break;
        }
        
        m_mng->sendMsg(sess->m_fdinfo, output); 

        MsgCenter::free(msg);
        return 0;
    } while (0);

    MsgCenter::free(msg);
    return ret;
}

Int32 SockSessAccpt::procStopSess(SessionAccpt* sess, MsgHdr* msg) {
    FdInfo* info = sess->m_fdinfo;
    MsgSessHead* req = NULL;    

    req = MsgCenter::cast<MsgSessHead>(msg);
    
    LOG_INFO("proc_sess_stop| user_id=%u|"
        " sess_id=%u| self_id=%u| msg=peer stop|",
        req->m_user_id, req->m_session_id, sess->m_session_id);

    sess->m_peer_open = FALSE;
    
    MsgCenter::free(msg);
    return ENUM_ERR_PEER_NOT_READY;
}

Int32 SockSessAccpt::procArrival(SessionAccpt* sess, MsgHdr* msg) {
    MsgSessHead* req = NULL;    

    req = MsgCenter::cast<MsgSessHead>(msg);

    /* if sess arrived, then start to read data */
    sess->m_fdinfo->m_test_rd = TRUE;
    sess->m_peer_open = TRUE;
    
    LOG_INFO("proc_sess_arrival| user_id=%u| sess_id=%u| self_id=%u|"
        " msg=ok|",
        req->m_user_id, req->m_session_id, 
        sess->m_session_id);
    
    MsgCenter::free(msg);
    return 0;
}

SessionAccpt* SockSessAccpt::setup(ListenerTcp* listener, int newfd) { 
    FdInfo* info = NULL;
    SessionAccpt* sess = NULL;
    UserConn* usr = listener->m_user;

    if (usr->m_peer_open) { 
    
        /* disable read but write for poll until sess arrival */
        info = m_mng->creatBase(newfd, FALSE, TRUE);
        if (NULL != info) {
            sess = m_center->newSessAccpt();
            sess->m_parent = listener->m_user; 
            sess->m_pairs = listener->m_pairs; 
            sess->m_session_id = m_center->nextSessId();
            
            sess->m_fdinfo = info;

            m_mng->addEvent(info, &sess->m_base);

            LOG_INFO("++++add_sess| fd=%d| user_id=%u| sess_id=%u| msg=ok|",
                newfd, usr->m_user_id, sess->m_session_id);
            
            return sess;
        } else {
            closeHd(newfd);
            return NULL;
        } 
    } else {
        closeHd(newfd);
        return NULL;
    }
}

Int32 SockSessAccpt::procEnd(SessionAccpt* sess, MsgHdr* msg) {
    sess->m_fdinfo->m_closing = TRUE;
    
    m_mng->notify(sess->m_fdinfo, ENUM_MSG_SYSTEM_EXIT);

    MsgCenter::free(msg);
    return 0;
}


int SockSessConn::readFd(struct FdInfo* info) {
    Int32 ret = 0;
    SessionConn* sess = (SessionConn*)info->m_data;

    ret = SockOper::readTcpRaw(sess->m_fdinfo, &sess->m_sock, m_mng); 
    return ret;
}

int SockSessConn::writeFd(struct FdInfo* info) {
    Int32 ret = 0;
    SessionConn* sess = (SessionConn*)info->m_data;
    UserAccpt* usr = sess->m_parent;

    if (sess->m_connected) {
        ret = SockOper::writeTcpRaw(sess->m_fdinfo, &sess->m_sock); 
    } else {
        ret = m_center->chkConn(sess->m_fdinfo);    
        if (0 == ret) {
            /* begin to read msg */
            info->m_test_rd = TRUE;
            sess->m_connected = TRUE;
            
            /* notify peer session the arrival */
            sendArrival(usr, sess); 
            
            ret = SockOper::writeTcpRaw(sess->m_fdinfo, &sess->m_sock); 
        } else {
            ret = -1;
        }
    }
    
    return ret;
}

int SockSessConn::procMsg(FdInfo* info, MsgHdr* msg) {
    Int32 ret = 0;
    SessionConn* sess = (SessionConn*)info->m_data;

    ret = process(sess, msg); 
    return ret;
}

Void SockSessConn::eof(struct FdInfo* info) {
    SessionConn* sess = (SessionConn*)info->m_data;
    UserAccpt* usr = sess->m_parent;

    if (sess->m_peer_open) {
        m_center->sendSessEvent(usr->m_fdinfo, ENUM_MSG_SYSTEM_STOP_SESS,
            usr->m_user_id, sess->m_session_id);
        
        sess->m_peer_open = FALSE;
    }

    m_center->notifySessEvent(usr->m_fdinfo, ENUM_MSG_SYSTEM_CHILD_EXIT,
        usr->m_user_id, sess->m_session_id);
    
    return;
}

Int32 SockSessConn::sendArrival(UserAccpt* usr, SessionConn* sess) {
    Int32 ret = 0;

    ret = m_center->sendSessEvent(usr->m_fdinfo, ENUM_MSG_SESS_ARRIVAL,
        usr->m_user_id, sess->m_session_id);
    
    return ret;
}

Int32 SockSessConn::process(SessionConn* sess, MsgHdr* msg) {
    Int32 ret = 0;
    
    if (ENUM_MSG_CMD_TCP_PLAIN == msg->m_cmd) {
        ret = procPlainData(sess, msg);
    } else if (ENUM_MSG_CMD_TCP_CIPHER == msg->m_cmd) {
        ret = procCipherData(sess, msg);
    } else if (ENUM_MSG_SYSTEM_STOP_SESS == msg->m_cmd) {
        ret = procStopSess(sess, msg);
    } else if (ENUM_MSG_SYSTEM_IO_END == msg->m_cmd) { 
        ret = procEnd(sess, msg); 
    } else {
        MsgCenter::printMsg("sess_conn_proc_err", msg);

        MsgCenter::free(msg);
    }

    return ret;
}

Int32 SockSessConn::procPlainData(SessionConn* sess, MsgHdr* msg) {
    MsgHdr* output = NULL;
    UserAccpt* usrAccpt = sess->m_parent;
    MsgTcpPlain* plain = NULL;
    
    plain = MsgCenter::cast<MsgTcpPlain>(msg);
    plain->m_user_id = usrAccpt->m_user_id;
    plain->m_session_id = sess->m_session_id;

    output = m_center->encPlainText(usrAccpt->m_evp_snd, msg); 

    /* out from user conn */
    m_mng->sendMsg(usrAccpt->m_fdinfo, output);

    MsgCenter::free(msg);
    return 0;
}

Int32 SockSessConn::procStopSess(SessionConn* sess, MsgHdr* msg) {
    FdInfo* info = sess->m_fdinfo;
    MsgSessHead* req = NULL;    

    req = MsgCenter::cast<MsgSessHead>(msg);
    
    LOG_INFO("proc_sess_stop| user_id=%u|"
        " sess_id=%u| self_id=%u| msg=peer stop|",
        req->m_user_id, req->m_session_id, sess->m_session_id);

    sess->m_peer_open = FALSE;
    
    MsgCenter::free(msg);
    return ENUM_ERR_PEER_NOT_READY;
}

Int32 SockSessConn::procCipherData(SessionConn* sess, MsgHdr* msg) {
    Int32 ret = 0;
    MsgHdr* output = NULL;
    UserAccpt* usr = sess->m_parent;
    
    do {
        output = m_center->decCipherText(usr->m_evp_rcv, msg);
        if (NULL == output) {
            ret = ENUM_ERR_DECRYPT_TXT;
            break;
        }
        
        m_mng->sendMsg(sess->m_fdinfo, output); 

        MsgCenter::free(msg);
        return 0;
    } while (0);

    MsgCenter::free(msg);
    return ret;
}

SessionConn* SockSessConn::setup(UserAccpt* usr, Uint32 sessId,
    const Char* ip, int port) {
    Int32 ret = 0;
    Int32 fd = -1;
    SessionConn* sess = NULL; 
    FdInfo* info = NULL; 
    TcpParam param;

    ret = buildParam(ip, port, &param);
    if (0 != ret) {
        LOG_ERROR("****add_sess| ip=%s| port=%d|"
            " user_id=%u| session_id=%u|"
            " msg=parse session param error|", 
            ip, port, usr->m_user_id, sessId);
        
        return NULL;
    }

    ret = connFast(&param, &fd);
    if (0 <= ret) {
        /* if connecting, then disable read */
        info = m_mng->creatBase(fd, FALSE, TRUE); 

        sess = m_center->newSessConn();
        sess->m_parent = usr; 
        sess->m_session_id = sessId;
        sess->m_peer_open = TRUE;
        memcpy(&sess->m_param, &param, sizeof(param));

        sess->m_fdinfo = info;
        m_mng->addEvent(info, &sess->m_base);

        LOG_INFO("++++add_sess| fd=%d| ret=%d| ip=%s| port=%d|"
            " user_id=%u| session_id=%u|"
            " msg=start connect|",
            fd, ret, ip, port, usr->m_user_id, sessId);

        return sess;
    } else {
        LOG_ERROR("****add_sess| ip=%s| port=%d|"
            " user_id=%u| session_id=%u|"
            " msg=connect error|", 
            ip, port, usr->m_user_id, sessId);
        
        return NULL;
    }
}

Int32 SockSessConn::procEnd(SessionConn* sess, MsgHdr* msg) {
    sess->m_fdinfo->m_closing = TRUE;
    
    m_mng->notify(sess->m_fdinfo, ENUM_MSG_SYSTEM_EXIT);

    MsgCenter::free(msg);
    return 0;
}

