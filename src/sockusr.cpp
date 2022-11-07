#include"sockusr.h"
#include"sockutil.h"
#include"sockmng.h"
#include"sockcenter.h"
#include"datatype.h"
#include"msgtype.h"
#include"msgcenter.h"
#include"sockoper.h"
#include"smutil.h"
#include"socksess.h"


int SockUsrListener::readFd(struct FdInfo* info) {
    ListenerDirty* listener = (ListenerDirty*)info->m_data;

    /* disable continue read */
    info->m_test_rd = FALSE;
    
    m_mng->notify(listener->m_fdinfo, ENUM_MSG_NOTIFY_NEW_USER);
        
    return 0;
}

int SockUsrListener::writeFd(struct FdInfo*) {
    return -1;
}

Int32 SockUsrListener::procMsg(FdInfo* info, MsgHdr* msg) {
    Int32 ret = 0;
    ListenerDirty* listener = (ListenerDirty*)info->m_data;

    if (ENUM_MSG_NOTIFY_NEW_USER == msg->m_cmd) {
        ret = procAddUsr(listener, msg);
    } else if (ENUM_MSG_SYSTEM_CHILD_EXIT == msg->m_cmd) {
        ret = procChildExit(listener, msg);
    } else if (ENUM_MSG_SYSTEM_IO_END == msg->m_cmd) { 
        ret = procEnd(listener, msg);
    } else {
        MsgCenter::free(msg);
    } 
    
    return ret;
}

Int32 SockUsrListener::procChildExit(ListenerDirty* listener, MsgHdr* msg) {
    MsgChildUserExit* req = NULL;   
    UserAccpt* usr = NULL;
   
    req = MsgCenter::cast<MsgChildUserExit>(msg); 
    usr = (UserAccpt*)req->m_data;

    if (req->m_user_id == usr->m_user_id) {
        LOG_INFO("proc_child_user_exit| user_id=%u| fd=%d|"
            " addr=%s:%d| msg=end|",
            req->m_user_id, usr->m_fdinfo->m_fd,
            usr->m_param->m_ip, usr->m_param->m_port);

        list_del(&usr->m_base.m_node, &listener->m_usr_que);

        m_center->freeUsrAccpt(usr);
    } else {
        LOG_ERROR("proc_child_user_exit| req_user_id=%u|"
            " user_id=%u| data=%p| msg=invalid data|",
            req->m_user_id, usr->m_user_id, usr);
    }
    
    MsgCenter::free(msg);
    return 0;
}


Void SockUsrListener::eof(struct FdInfo* info) {
    ListenerDirty* listener = (ListenerDirty*)info->m_data;

    LOG_INFO("usr_listener_exit| fd=%d| addr=%s:%d|"
        " msg=exit now|",
        info->m_fd, listener->m_param->m_ip, listener->m_param->m_port);
    
    /* if  fail, then exit */
    m_mng->stop();
}

Int32 SockUsrListener::procAddUsr(ListenerDirty* listener, MsgHdr* msg) { 
    Int32 newfd = -1;
    UserAccpt* usr = NULL;
    FdInfo* info = listener->m_fdinfo;

    newfd = acceptCli(info->m_fd);
    while (0 <= newfd) {
        usr = m_accpt->setup(listener, newfd);
        if (NULL != usr) {
            list_add_back(&usr->m_base.m_node, &listener->m_usr_que);
        }
        
        newfd = acceptCli(info->m_fd);
    }

    if (!info->m_test_rd) {
        info->m_test_rd = TRUE;
    }
    
    MsgCenter::free(msg);
    return 0;
}

ListenerDirty* SockUsrListener::setup(const TcpParam* param) {
    Int32 fd = -1;
    FdInfo* info = NULL;
    ListenerDirty* listener = NULL;

    fd = creatTcpSrv(param);
    if (0 > fd) {
        return NULL;
    } 

    info = m_mng->creatBase(fd, TRUE, FALSE);
    listener = m_center->newListenerDirty();
    listener->m_param = param;
    listener->m_fdinfo = info; 

    LOG_INFO("++++add_usr_listener| fd=%d| local_addr=%s:%d| msg=ok|",
        fd, param->m_ip, param->m_port);
    
    m_mng->addEvent(info, &listener->m_base); 
    return listener; 
}

Int32 SockUsrListener::procEnd(ListenerDirty* listener, MsgHdr* msg) {
    listener->m_fdinfo->m_closing = TRUE;
    
    m_mng->notify(listener->m_fdinfo, ENUM_MSG_SYSTEM_EXIT);

    MsgCenter::free(msg);
    return 0;
}


int SockUsrAccpt::readFd(struct FdInfo* info) {
    Int32 ret = 0;
    UserAccpt* usr = (UserAccpt*)info->m_data;

    ret = SockOper::readSock(usr->m_fdinfo, &usr->m_sock, m_mng);    
    return ret;
    
}

int SockUsrAccpt::writeFd(struct FdInfo* info) {
    Int32 ret = 0;
    UserAccpt* usr = (UserAccpt*)info->m_data;

    ret = SockOper::writeSock(usr->m_fdinfo, &usr->m_sock); 
    return ret;
}

Int32 SockUsrAccpt::procMsg(FdInfo* info, MsgHdr* msg) {
    Int32 ret = 0;
    UserAccpt* usr = (UserAccpt*)info->m_data;

    ret = process(usr, msg); 
    return ret;
}

Int32 SockUsrAccpt::process(UserAccpt* usr, MsgHdr* msg) {
    Int32 ret = 0;
    
    if (ENUM_MSG_CMD_TCP_CIPHER == msg->m_cmd
        || ENUM_MSG_SYSTEM_STOP_SESS == msg->m_cmd) {
        ret = transfer2Sess(usr, msg);
    } else if (ENUM_MSG_CMD_START_SESS == msg->m_cmd) {
        ret = procStartSess(usr, msg);
    } else if (ENUM_MSG_SYSTEM_CHILD_EXIT == msg->m_cmd) {
        ret = procChildExit(usr, msg);
    } else if (ENUM_MSG_USR_AUTH_REQ == msg->m_cmd) {
        ret = procAuthReq(usr, msg);
    } else if (ENUM_MSG_USR_CIPHER_KEY_ACK == msg->m_cmd) {
        ret = procCipherKeyAck(usr, msg);
    } else if (ENUM_MSG_SYSTEM_IO_END == msg->m_cmd) {
        ret = procEnd(usr, msg);
    } else {
        MsgCenter::printMsg("user_accept_proc_err", msg);

        MsgCenter::free(msg);
    }

    return ret;
}

Int32 SockUsrAccpt::transfer2Sess(UserAccpt* usr, MsgHdr* msg) {
    Int32 ret = 0;
    SessionConn* sess = NULL;
    MsgSessHead* req = NULL;

    req = MsgCenter::cast<MsgSessHead>(msg);

    do {
        if (ENUM_USER_LOGIN != usr->m_usr_status) {
            ret = ENUM_ERR_NOT_LOGIN_USER;
            
            LOG_ERROR("transfer_Sess| ret=%d| cmd=%u|"
                " user_id=%u| sess_id=%u| msg=invalid status|",
                ret, msg->m_cmd, req->m_user_id, req->m_session_id);
            
            break;
        }

        sess = findSess(usr, req->m_session_id);
        if (NULL == sess) { 
            ret = ENUM_ERR_INVALID_SESS;

            LOG_INFO("transfer_Sess| ret=%d| cmd=%u|"
                " user_id=%u| sess_id=%u| msg=invalid sess|",
                ret, msg->m_cmd, req->m_user_id, req->m_session_id);
            
            break;
        } 
        
        LOG_DEBUG("transfer_Sess| cmd=%u| user_id=%u| sess_id=%u| msg=ok|",
            msg->m_cmd, req->m_user_id, req->m_session_id);

        /* this msg must not be freed here if successfully !!! */
        m_mng->dispatch(sess->m_fdinfo, msg);
        return 0;
    } while (0);
    
    /* these msg donot matter the user status */
    MsgCenter::free(msg);
    return 0;
}

Int32 SockUsrAccpt::parseAuthReq(UserAccpt* usr, MsgUserAuthReq* req) {
    Int32 ret = 0;
    Byte digest[DEF_SM3_DIGEST_SIZE] = {0};
    ExtraData* extra_data = &usr->m_extra_data;
    SysBase* base = m_center->getBase();

    do { 
        if (DEF_RAND_CODE_SIZE + DEF_SEID_SIZE != req->m_secret_len) {
            LOG_ERROR("parse_auth_req| declen=%d| msg=invalid data|",
                req->m_secret_len);

            ret = -1;
            break;
        }
        
        Sm3Util::digest(req->m_secret, req->m_secret_len, digest);

        if (0 != memcmp(req->m_signature, digest, req->m_sig_len)) {
            LOG_ERROR("parse_auth_req| msg=invalid signature|");
            ret = -1;
            break;
        } 

        usr->m_user_id = req->m_user_id;
        memcpy(extra_data->m_peer_seid, &req->m_secret, DEF_SEID_SIZE);
        memcpy(extra_data->m_rand1, &req->m_secret[DEF_SEID_SIZE],
            DEF_RAND_CODE_SIZE); 

        memcpy(extra_data->m_rand2, base->m_rand, DEF_RAND_CODE_SIZE);

        return 0;
    } while (0);

    return ret;
}

MsgHdr* SockUsrAccpt::creatExchKeyReq(UserAccpt* usr) {
    Int32 inlen = 0;
    MsgHdr* hdr = NULL;
    MsgUserExchKey* req = NULL; 
    Byte secret[MAX_SECRET_DATA_SIZE] = {0};
    ExtraData* extra_data = &usr->m_extra_data;
    SysBase* base = m_center->getBase();

    hdr = MsgCenter::creat<MsgUserExchKey>(ENUM_MSG_USR_EXCH_KEY);
    req = MsgCenter::cast<MsgUserExchKey>(hdr);

    do {
        req->m_user_id = usr->m_user_id;
        memcpy(req->m_src_seid, base->m_seid, DEF_SEID_SIZE);
        memcpy(req->m_dst_seid, extra_data->m_peer_seid, DEF_SEID_SIZE);

        inlen = 0;
        memcpy(&secret[inlen], base->m_seid, DEF_SEID_SIZE);

        inlen += DEF_SEID_SIZE;
        memcpy(&secret[inlen], extra_data->m_rand2, DEF_RAND_CODE_SIZE);

        inlen += DEF_RAND_CODE_SIZE;

        req->m_secret_len = inlen; 
        memcpy(req->m_secret, secret, req->m_secret_len);

        Sm3Util::digest(req->m_secret, req->m_secret_len, req->m_digest);

        req->m_sig_len = DEF_SM3_DIGEST_SIZE;
        memcpy(req->m_signature, req->m_digest, req->m_sig_len);
        
        MsgCenter::addCrc(hdr);

        return hdr;
    } while (0);

    MsgCenter::free(hdr);
    return NULL;
}

Int32 SockUsrAccpt::procAuthReq(UserAccpt* usr, MsgHdr* msg) {
    Int32 ret = 0;
    MsgHdr* output = NULL;
    MsgUserAuthReq* req = NULL;

    req = MsgCenter::cast<MsgUserAuthReq>(msg);

    do { 
        if (ENUM_USER_WAIT_REQ != usr->m_usr_status) {
            ret = -1;
            break;
        }

        /* check info */
        ret = parseAuthReq(usr, req);
        if (0 != ret) {
            break;
        }

        output = creatExchKeyReq(usr);
        if (NULL == output) {
            LOG_ERROR("procAuthReq| user_id=%u|"
                " msg=create exchange key msg error|",
                req->m_user_id);

            ret = -1;
            break;
        }
    
        ret = m_mng->sendMsg(usr->m_fdinfo, output);
        if (0 != ret) {
            break;
        }

        usr->m_usr_status = ENUM_USER_EXCH_KEY;

        LOG_DEBUG("procAuthReq| user_id=%u| msg=auth ok|",
            req->m_user_id);
        
        MsgCenter::free(msg);
        return 0;
    } while (0);

    LOG_ERROR("procAuthReq| ret=%d| user_id=%u| msg=auth error|",
        ret, req->m_user_id);

    MsgCenter::free(msg);
    return ret;
}

Int32 SockUsrAccpt::parseCKeyAck(UserAccpt* usr, MsgUserCKeyAck* req) {
    Int32 ret = 0;
    Int32 inlen = 0;
    Byte digest[DEF_SM3_DIGEST_SIZE] = {0};
    Byte secret[MAX_SECRET_DATA_SIZE] = {0};
    ExtraData* extra_data = &usr->m_extra_data;
    SysBase* base = m_center->getBase();

    do {
        if (0 != memcmp(req->m_dst_seid, base->m_seid, DEF_SEID_SIZE)) {
            LOG_ERROR("parse_ckey_ack| msg=invalid dst seid|");
            ret = -1;
            break;
        }

        inlen = 0;
        memcpy(&secret[inlen], extra_data->m_rand1, DEF_RAND_CODE_SIZE);

        inlen += DEF_RAND_CODE_SIZE; 
        memcpy(&secret[inlen], extra_data->m_rand2, DEF_RAND_CODE_SIZE);

        inlen += DEF_RAND_CODE_SIZE; 

        Sm3Util::digest(secret, inlen, digest); 

        if (0 != memcmp(req->m_secret, digest, req->m_secret_len)) {
            LOG_ERROR("parse_ckey_ack| msg=invalid signature|");
            ret = -1;
            break;
        } 

        extra_data->m_sess_key_size = req->m_secret_len;
        memcpy(extra_data->m_sess_key, req->m_secret, req->m_secret_len);
        return 0;
    } while (0);

    return ret;
}

MsgHdr* SockUsrAccpt::creatAuthEnd(UserAccpt* usr) {
    MsgHdr* hdr = NULL;
    MsgUserAuthEnd* req = NULL; 
    ExtraData* extra_data = &usr->m_extra_data;
    
    hdr = MsgCenter::creat<MsgUserAuthEnd>(ENUM_MSG_USR_AUTH_END);
    req = MsgCenter::cast<MsgUserAuthEnd>(hdr); 

    req->m_user_id = usr->m_user_id;
    memcpy(req->m_dst_seid, extra_data->m_peer_seid, DEF_SEID_SIZE);

    MsgCenter::addCrc(hdr);
    return hdr;
}

Int32 SockUsrAccpt::procCipherKeyAck(UserAccpt* usr, MsgHdr* msg) {
    Int32 ret = 0;
    MsgHdr* output = NULL;
    MsgUserCKeyAck* req = NULL;
    ExtraData* extra_data = &usr->m_extra_data;

    req = MsgCenter::cast<MsgUserCKeyAck>(msg);

    do {
        if (ENUM_USER_EXCH_KEY != usr->m_usr_status) {
            ret = -1;
            break;
        } 

        ret = parseCKeyAck(usr, req);
        if (0 != ret) {
            ret = ENUM_ERR_AUTH_GATEWAY;
            break;
        }

        output = creatAuthEnd(usr);
        if (NULL == output) {
            LOG_ERROR("procCipherKeyAck| user_id=%u|"
                " msg=create auth end msg error|",
                req->m_user_id);

            ret = ENUM_ERR_AUTH_GATEWAY;
            break;
        }
        
        ret = m_mng->sendMsg(usr->m_fdinfo, output);
        if (0 != ret) {
            break;
        }

        /* set cipher key */
        m_center->setCipherKey(usr->m_evp_rcv, extra_data->m_sess_key,
            extra_data->m_sess_key_size);
        m_center->setCipherKey(usr->m_evp_snd, extra_data->m_sess_key,
            extra_data->m_sess_key_size);

        usr->m_usr_status = ENUM_USER_LOGIN;

        LOG_DEBUG("procCipherKeyAck| user_id=%u| msg=key ack ok|",
            req->m_user_id);

        printHex("enc_sess_key", extra_data->m_sess_key,
            extra_data->m_sess_key_size);

        /* notify user arrival for peer */
        m_center->sendUsrEvent(usr->m_fdinfo, ENUM_MSG_USR_ARRIVAL,
            usr->m_user_id);

        MsgCenter::free(msg);
        return 0;
    } while (0);

    LOG_ERROR("procCipherKeyAck| ret=%d| user_id=%u| msg=key ack error|",
        ret, req->m_user_id);
    
    MsgCenter::free(msg);
    return ret;
}

Int32 SockUsrAccpt::procStartSess(UserAccpt* usr, MsgHdr* msg) {
    Int32 ret = 0;
    SessionConn* sess = NULL;
    MsgStartSess* req = NULL;  

    req = MsgCenter::cast<MsgStartSess>(msg);
    
    do {
        if (ENUM_USER_LOGIN != usr->m_usr_status) {
            ret = ENUM_ERR_NOT_LOGIN_USER;
            break;
        } 
        
        sess = findSess(usr, req->m_session_id);
        if (NULL != sess) { 
            ret = ENUM_ERR_SESS_ALREADY_EXISTS;
            break;
        }
        
        ret = startSessConn(usr, req);
        if (0 != ret) {
            ret = ENUM_ERR_CONN_SESSEION;
            break;
        }
        
        MsgCenter::free(msg);
        return 0;
    } while (0);

    LOG_ERROR("****procStartSess| ret=%d| ip=%s| port=%d|"
        " user_id=%u| session_id=%u|"
        " msg=start session error|", 
        ret, req->m_ip, req->m_port,
        req->m_user_id, req->m_session_id); 

    /* if session fails, no matter for the users, then return ok */
    m_center->sendSessEvent(usr->m_fdinfo, ENUM_MSG_SYSTEM_STOP_SESS,
        req->m_user_id, req->m_session_id);

    MsgCenter::free(msg);
    return 0;
}

Int32 SockUsrAccpt::startSessConn(UserAccpt* usr, MsgStartSess* req) {
    SessionConn* sess = NULL; 

    sess = m_conn->setup(usr, req->m_session_id, req->m_ip, req->m_port);
    if (NULL != sess) { 
        order_list_add(&sess->m_base.m_node, &usr->m_session_que);

        return 0;
    } else {
        return -1;
    }
}

SessionConn* SockUsrAccpt::findSess(UserAccpt* usr, Uint32 sessID) {
    list_node* node = NULL;
    SessionConn* dst = NULL;
    SessionConn tmp;

    tmp.m_session_id = sessID;
    node = order_list_find(&tmp.m_base.m_node, &usr->m_session_que);
    if (NULL != node) {
        dst = (SessionConn*)node;
        return dst;
    } else {
        return NULL;
    }
}

UserAccpt* SockUsrAccpt::setup(ListenerDirty* listener, int newfd) { 
    FdInfo* info = NULL;
    UserAccpt* usr = NULL;
    
    /* enable read and write */
    info = m_mng->creatBase(newfd, TRUE, TRUE);
    if (NULL != info) {
        usr = m_center->newUsrAccpt();
        usr->m_parent = listener;
        usr->m_param = listener->m_param;
        usr->m_fdinfo = info;

        /* ready for login protocol */
        usr->m_usr_status = ENUM_USER_WAIT_REQ; 

        LOG_INFO("++++add_usr_accpt| fd=%d| ip=%s| port=%d| msg=ok|",
            newfd, usr->m_param->m_ip, usr->m_param->m_port);

        m_mng->addEvent(info, &usr->m_base);
        return usr;
    } else {
        closeHd(newfd);
        return NULL;
    } 
}

Int32 SockUsrAccpt::procChildExit(UserAccpt* usr, MsgHdr* msg) {
    SessionConn* sess = NULL;
    MsgSessHead* req = NULL;    
   
    req = MsgCenter::cast<MsgSessHead>(msg);   
    
    sess = findSess(usr, req->m_session_id);
    if (NULL != sess) {        
        order_list_del(&sess->m_base.m_node, &usr->m_session_que);
        
        m_center->freeSessConn(sess);

        LOG_INFO("proc_child_exit| user_id=%u|"
            " session_id=%u| msg=ok|",
            req->m_user_id, req->m_session_id);
    } else {
        LOG_INFO("proc_child_exit| user_id=%u|"
            " session_id=%u| msg=not found|",
            req->m_user_id, req->m_session_id);        
    } 

    /* if is exiting and no child here, then stop */
    if (usr->m_fdinfo->m_closing && order_list_empty(&usr->m_session_que)) {
        m_mng->notify(usr->m_fdinfo, ENUM_MSG_SYSTEM_EXIT); 
    }
    
    MsgCenter::free(msg);
    return 0;
}

Int32 SockUsrAccpt::procEnd(UserAccpt* usr, MsgHdr* msg) {
    list_node* pos = NULL;
    SessionConn* sess = NULL;
    
    usr->m_fdinfo->m_closing = TRUE;
    
    if (!order_list_empty(&usr->m_session_que)) {
        list_for_each(pos, &usr->m_session_que) {
            
            sess = (SessionConn*)pos; 

            /* notify to close all of children */
            m_mng->closeMng(sess->m_fdinfo);
        }
    } else {
        m_mng->notify(usr->m_fdinfo, ENUM_MSG_SYSTEM_EXIT); 
    }

    MsgCenter::free(msg);

    return 0;
}

Void SockUsrAccpt::eof(struct FdInfo* info) {
    UserAccpt* usr = (UserAccpt*)info->m_data;
    ListenerDirty* listener = usr->m_parent;

    /* notify to parent */
    m_center->notifyChildUsrExit(listener->m_fdinfo, 
        usr->m_user_id, (Uint64)usr);
    
    return;
}


int SockUsrConn::readFd(struct FdInfo* info) {
    Int32 ret = 0;
    UserConn* usr = (UserConn*)info->m_data;

    ret = SockOper::readSock(usr->m_fdinfo, &usr->m_sock, m_mng);    
    return ret;
    
}

int SockUsrConn::writeFd(struct FdInfo* info) {
    Int32 ret = 0;
    UserConn* usr = (UserConn*)info->m_data;

    if (usr->m_connected) {
        ret = SockOper::writeSock(usr->m_fdinfo, &usr->m_sock);
    } else {
        ret = writeUsrConn(usr);
    }
    
    return ret;
}

Int32 SockUsrConn::writeUsrConn(UserConn* usr) {
    Int32 ret = 0;
    FdInfo* info = usr->m_fdinfo;

    ret = m_center->chkConn(info);    
    if (0 == ret) {
        /* begin to read msg */
        info->m_test_rd = TRUE;
        usr->m_connected = TRUE;

        m_mng->notify(usr->m_fdinfo, ENUM_MSG_USR_SETUP_AUTH); 
        return 0;
    } else {
        return ret; 
    } 
}

Int32 SockUsrConn::procMsg(FdInfo* info, MsgHdr* msg) {
    Int32 ret = 0;
    UserConn* usr = (UserConn*)info->m_data;

    ret = process(usr, msg); 
    return ret;
}

Int32 SockUsrConn::process(UserConn* usr, MsgHdr* msg) {
    Int32 ret = 0;

    if (ENUM_MSG_CMD_TCP_CIPHER == msg->m_cmd
        || ENUM_MSG_SYSTEM_STOP_SESS == msg->m_cmd
        || ENUM_MSG_SESS_ARRIVAL == msg->m_cmd) {
        ret = transfer2Sess(usr, msg);
    } else if (ENUM_MSG_SYSTEM_CHILD_EXIT == msg->m_cmd) {
        ret = procChildExit(usr, msg);
    } else if (ENUM_MSG_USR_EXCH_KEY == msg->m_cmd) {
        ret = procExchKey(usr, msg);
    } else if (ENUM_MSG_USR_AUTH_END == msg->m_cmd) {
        ret = procAuthEnd(usr, msg);
    } else if (ENUM_MSG_USR_ARRIVAL == msg->m_cmd) {
        ret = procUsrArrival(usr, msg);
    } else if (ENUM_MSG_USR_SETUP_AUTH == msg->m_cmd) {
        ret = procSetupAuth(usr, msg);
    } else if (ENUM_MSG_SYSTEM_IO_END == msg->m_cmd) { 
        ret = procEnd(usr, msg);
    } else {
        MsgCenter::printMsg("user_conn_proc_err", msg);

        MsgCenter::free(msg);
    }

    return ret;
}

Int32 SockUsrConn::transfer2Sess(UserConn* usr, MsgHdr* msg) {
    Int32 ret = 0;
    SessionAccpt* sess = NULL;
    MsgSessHead* req = NULL;

    req = MsgCenter::cast<MsgSessHead>(msg);

    do {
        if (ENUM_USER_LOGIN != usr->m_usr_status) {
            ret = ENUM_ERR_NOT_LOGIN_USER;

            LOG_ERROR("transfer_Sess| ret=%d| cmd=%u|"
                " user_id=%u| sess_id=%u| msg=invalid status|",
                ret, msg->m_cmd, req->m_user_id, req->m_session_id);
            
            break;
        }

        sess = findSess(usr, req->m_session_id);
        if (NULL == sess) { 
            ret = ENUM_ERR_INVALID_SESS;

            LOG_INFO("transfer_Sess| ret=%d| cmd=%u|"
                " user_id=%u| sess_id=%u| msg=invalid sess|",
                ret, msg->m_cmd, req->m_user_id, req->m_session_id);
            
            break;
        } 
        
        LOG_DEBUG("transfer_Sess| cmd=%u| user_id=%u| sess_id=%u| msg=ok|",
            msg->m_cmd, req->m_user_id, req->m_session_id);

        /* this msg must not be freed here if successfully !!! */
        m_mng->dispatch(sess->m_fdinfo, msg);
        return 0;
    } while (0);
    
    /* these msg donot matter the user status */
    MsgCenter::free(msg);
    return 0;
}

MsgHdr* SockUsrConn::creatAuthReq(UserConn* usr) {
    Int32 inlen = 0;
    MsgHdr* hdr = NULL;
    MsgUserAuthReq* req = NULL; 
    struct SysBase* base = m_center->getBase();
    Byte secret[MAX_SECRET_DATA_SIZE] = {0};
    Byte digest[DEF_SM3_DIGEST_SIZE] = {0};

    hdr = MsgCenter::creat<MsgUserAuthReq>(ENUM_MSG_USR_AUTH_REQ);
    req = MsgCenter::cast<MsgUserAuthReq>(hdr);

    do {
        req->m_user_id = usr->m_user_id;
        memcpy(req->m_src_seid, base->m_seid, DEF_SEID_SIZE);

        inlen = 0;
        memcpy(&secret[inlen], base->m_seid, DEF_SEID_SIZE);

        inlen += DEF_SEID_SIZE; 
        memcpy(&secret[inlen], base->m_rand, DEF_RAND_CODE_SIZE);

        inlen += DEF_RAND_CODE_SIZE; 
        
        req->m_secret_len = inlen; 
        memcpy(req->m_secret, secret, req->m_secret_len);
        
        Sm3Util::digest(req->m_secret, req->m_secret_len, digest);

        req->m_sig_len = DEF_SM3_DIGEST_SIZE;
        memcpy(req->m_signature, digest, req->m_sig_len);
        
        MsgCenter::addCrc(hdr);

        return hdr;
    } while (0);

    MsgCenter::free(hdr);
    return NULL;
}

Int32 SockUsrConn::parseExchKeyReq(UserConn* usr, MsgUserExchKey* req) {
    Int32 ret = 0;
    Byte digest[DEF_SM3_DIGEST_SIZE] = {0};
    ExtraData* extra_data = &usr->m_extra_data;
    struct SysBase* base = m_center->getBase();

    do {
        if (usr->m_user_id != req->m_user_id) {
            LOG_ERROR("parse_exch_key_req| msg=invalid user id|");
            ret = -1;
            break;
        }
        
        if (0 != memcmp(req->m_dst_seid, base->m_seid, DEF_SEID_SIZE)) {
            LOG_ERROR("parse_exch_key_req| msg=invalid dst seid|");
            ret = -1;
            break;
        }

        Sm3Util::digest(req->m_secret, req->m_secret_len, digest);

        if (0 != memcmp(req->m_signature, digest, req->m_sig_len)) {
            LOG_ERROR("parse_exch_key_req| msg=invalid signature|");
            ret = -1;
            break;
        } 
        
        memcpy(extra_data->m_peer_seid, &req->m_secret, DEF_SEID_SIZE);
        memcpy(extra_data->m_rand2, &req->m_secret[DEF_SEID_SIZE],
            DEF_RAND_CODE_SIZE);
        
        memcpy(extra_data->m_rand1, base->m_rand, DEF_RAND_CODE_SIZE);
        return 0;
    } while (0);

    return ret;
}

MsgHdr* SockUsrConn::creatCKeyAck(UserConn* usr) {
    Int32 inlen = 0;
    MsgHdr* hdr = NULL;
    MsgUserCKeyAck* req = NULL; 
    ExtraData* extra_data = &usr->m_extra_data;
    SysBase* base = m_center->getBase();
    Byte secret[MAX_SECRET_DATA_SIZE] = {0};
    Byte digest[DEF_SM3_DIGEST_SIZE] = {0};

    hdr = MsgCenter::creat<MsgUserCKeyAck>(ENUM_MSG_USR_CIPHER_KEY_ACK);
    req = MsgCenter::cast<MsgUserCKeyAck>(hdr);
  
    req->m_user_id = usr->m_user_id;
    memcpy(req->m_src_seid, base->m_seid, DEF_SEID_SIZE);
    memcpy(req->m_dst_seid, extra_data->m_peer_seid, DEF_SEID_SIZE);

    inlen = 0;
    memcpy(&secret[inlen], extra_data->m_rand1, DEF_RAND_CODE_SIZE);

    inlen += DEF_RAND_CODE_SIZE; 
    memcpy(&secret[inlen], extra_data->m_rand2, DEF_RAND_CODE_SIZE);

    inlen += DEF_RAND_CODE_SIZE; 

    Sm3Util::digest(secret, inlen, digest);
    
    req->m_secret_len = DEF_SM3_DIGEST_SIZE;
    memcpy(req->m_secret, digest, req->m_secret_len); 

    extra_data->m_sess_key_size = req->m_secret_len;
    memcpy(extra_data->m_sess_key, req->m_secret, req->m_secret_len);

    MsgCenter::addCrc(hdr); 

    return hdr;
}

Int32 SockUsrConn::procExchKey(UserConn* usr, MsgHdr* msg) {
    Int32 ret = 0;
    MsgHdr* output = NULL;
    MsgUserExchKey* req = NULL;

    req = MsgCenter::cast<MsgUserExchKey>(msg);

    do {
        if (ENUM_USER_AUTH_REQ != usr->m_usr_status) {
            ret = -1;
            break;
        }

        /* check info */
        ret = parseExchKeyReq(usr, req);
        if (0 != ret) {
            break;
        }

        output = creatCKeyAck(usr);
        if (NULL == output) {
            LOG_ERROR("proc_exch_key| user_id=%u|"
                " msg=create exchange key ack error|",
                req->m_user_id);

            ret = -1;
            break;
        }
        
        ret = m_mng->sendMsg(usr->m_fdinfo, output);
        if (0 != ret) {
            break;
        }

        usr->m_usr_status = ENUM_USER_EXCH_KEY_ACK;

        LOG_DEBUG("proc_exch_key| user_id=%u| msg=key exchange ok|",
            req->m_user_id);
        
        MsgCenter::free(msg);
        return 0;
    } while (0);

    LOG_ERROR("proc_exch_key| ret=%d| user_id=%u|"
        " msg=key exchange error|",
        ret, req->m_user_id);

    MsgCenter::free(msg);
    return ret;
}

Int32 SockUsrConn::procAuthEnd(UserConn* usr, MsgHdr* msg) {
    Int32 ret = 0;
    MsgUserAuthEnd* req = NULL;
    ExtraData* extra_data = &usr->m_extra_data;

    req = MsgCenter::cast<MsgUserAuthEnd>(msg);

    do {
        if (ENUM_USER_EXCH_KEY_ACK != usr->m_usr_status) {
            ret = -1;
            break;
        }
        
        /* set cipher key */ 
        m_center->setCipherKey(usr->m_evp_rcv, extra_data->m_sess_key,
            extra_data->m_sess_key_size);
        m_center->setCipherKey(usr->m_evp_snd, extra_data->m_sess_key,
            extra_data->m_sess_key_size);

        usr->m_usr_status = ENUM_USER_LOGIN;

        LOG_DEBUG("procAuthEnd| user_id=%u| msg=auth end ok|",
            req->m_user_id);

        printHex("enc_sess_key", extra_data->m_sess_key,
            extra_data->m_sess_key_size);
        
        MsgCenter::free(msg);
        return 0;
    } while (0);

    LOG_ERROR("procAuthEnd| ret=%d| user_id=%u|"
        " msg=auth end error|",
        ret, req->m_user_id);

    MsgCenter::free(msg);
    return ret;
}

Int32 SockUsrConn::procUsrArrival(UserConn* usr, MsgHdr* msg) {
    Int32 ret = 0;
    MsgUserHead* req = NULL;    

    req = MsgCenter::cast<MsgUserHead>(msg);
    
    if (ENUM_USER_LOGIN == usr->m_usr_status) { 
        /* start to accept tcp session */
        usr->m_peer_open = TRUE;

        LOG_INFO("proc_usr_arrival| user_id=%u| self_usr_id=%u|"
            " msg=user arrival ok|",
            req->m_user_id, usr->m_user_id);
    } else {
        ret = -1;
    }
    
    MsgCenter::free(msg);
    return ret;
}

Int32 SockUsrConn::procSetupAuth(UserConn* usr, MsgHdr* msg) {
    Int32 ret = 0;
    MsgHdr* hdr = NULL;

    do {
        if (ENUM_USER_INIT != usr->m_usr_status) {
            ret = -1;
            break;
        }
        
        hdr = creatAuthReq(usr);
        if (NULL == hdr) {
            ret = -1;
            break;
        }
        
        m_mng->sendMsg(usr->m_fdinfo, hdr); 
        
        usr->m_usr_status = ENUM_USER_AUTH_REQ;

        LOG_DEBUG("start_auth| user_id=%u| msg=start now|", usr->m_user_id); 

        MsgCenter::free(msg);
        return 0;
    } while (0);

    LOG_ERROR("start_auth| user_id=%u| msg=invalid", usr->m_user_id);
    
    MsgCenter::free(msg);
    return ret;
}

SessionAccpt* SockUsrConn::findSess(UserConn* usr, Uint32 sessID) {
    list_node* node = NULL;
    SessionAccpt* dst = NULL;
    SessionAccpt tmp;

    tmp.m_session_id = sessID;
    node = order_list_find(&tmp.m_base.m_node, &usr->m_session_que);
    if (NULL != node) {
        dst = (SessionAccpt*)node;

        return dst;
    } else {
        return NULL;
    }
}

UserConn* SockUsrConn::setup(const TcpParam* param) {
    Int32 ret = 0;
    Int32 fd = -1;
    FdInfo* info = NULL; 
    UserConn* usr = NULL;

    ret = connFast(param, &fd);
    if (0 <= ret) {
        
        /* if connecting, then disable read */
        info = m_mng->creatBase(fd, FALSE, TRUE); 
        usr = m_center->newUsrConn(); 
        
        usr->m_param = param;
        usr->m_user_id = m_center->nextUsrId();
        usr->m_fdinfo = info;

        m_mng->addEvent(info, &usr->m_base);

        LOG_INFO("++++add_usr_conn| fd=%d| ret=%d| ip=%s| port=%d|"
            " msg=start connect|",
            fd, ret, param->m_ip, param->m_port);
        
        return usr;
    }  else {
        LOG_ERROR("****usr_conn| ip=%s| port=%d|"
            " msg=start user connection error|", 
            param->m_ip, param->m_port);
        
        return NULL;
    } 
}

Int32 SockUsrConn::procChildExit(UserConn* usr, MsgHdr* msg) {
    SessionAccpt* sess = NULL;
    MsgSessHead* req = NULL;    
   
    req = MsgCenter::cast<MsgSessHead>(msg);   
    
    sess = findSess(usr, req->m_session_id);
    if (NULL != sess) {        
        order_list_del(&sess->m_base.m_node, &usr->m_session_que);
        
        m_center->freeSessAccpt(sess);

        LOG_INFO("proc_child_exit| user_id=%u|"
            " session_id=%u| msg=ok|",
            req->m_user_id, req->m_session_id);
    } else {
        LOG_INFO("proc_child_exit| user_id=%u|"
            " session_id=%u| msg=not found|",
            req->m_user_id, req->m_session_id);
    } 

    /* if is exiting and no child here, then stop */
    if (usr->m_fdinfo->m_closing && order_list_empty(&usr->m_session_que)) {
        m_mng->notify(usr->m_fdinfo, ENUM_MSG_SYSTEM_EXIT); 
    }
    
    MsgCenter::free(msg);
    return 0;
}

Int32 SockUsrConn::procEnd(UserConn* usr, MsgHdr* msg) {
    list_node* pos = NULL;
    SessionAccpt* sess = NULL;
    
    usr->m_fdinfo->m_closing = TRUE;
    
    if (!order_list_empty(&usr->m_session_que)) {
        list_for_each(pos, &usr->m_session_que) {
            
            sess = (SessionAccpt*)pos; 

            /* notify to close all of children */
            m_mng->closeMng(sess->m_fdinfo);
        }
    } else {
        m_mng->notify(usr->m_fdinfo, ENUM_MSG_SYSTEM_EXIT); 
    }

    MsgCenter::free(msg);

    return 0;
}

Void SockUsrConn::eof(struct FdInfo* info) {
    UserConn* usr = (UserConn*)info->m_data;

    LOG_INFO("usr_conn_exit| fd=%d| user_id=%u|"
        " addr=%s:%d| msg=exit now|",
        info->m_fd, usr->m_user_id,
        usr->m_param->m_ip, usr->m_param->m_port);

    m_mng->stop();
    return;
}


