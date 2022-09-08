#include"authrouter.h"
#include"managecenter.h"
#include"usercenter.h"
#include"msgtype.h"
#include"msgcenter.h"


RouterDealer::RouterDealer(ManageCenter* mng, UserCenter* usr_center) {
    m_mng = mng;
    m_usr_center = usr_center;
}

Int32 RouterDealer::processIn(RouterPair* routerPair,
    MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    Router* local = &routerPair->m_router_in;
    Router* peer = &routerPair->m_router_out;
    
    if (ENUM_MSG_CMD_TCP_CIPHER == msg->m_cmd) {
        ret = procCipherData(routerPair, local, peer, msg, pDel); 
    } else if (ENUM_MSG_CMD_TCP_PLAIN == msg->m_cmd) {
        ret = procPlainData(routerPair, local, peer, msg, pDel); 
    } else if (ENUM_MSG_USR_AUTH_REQ == msg->m_cmd) {
        ret = procAuthReq(routerPair, local, peer, msg, pDel);
    } else if (ENUM_MSG_USR_CIPHER_KEY_ACK == msg->m_cmd) {
        ret = procCipherKeyAck(routerPair, local, peer, msg, pDel);
    } else if (ENUM_MSG_CMD_STOP_GATEWAY == msg->m_cmd) {
        ret = procStopGateWay(routerPair, local, peer, msg, pDel);
    } else if (ENUM_MSG_CMD_CLOSE_SOCK == msg->m_cmd) {
        ret = procCloseRouter(routerPair, local, peer, msg, pDel);
    } else {
        if (ENUM_USER_LOGIN == local->m_usr_status
            && ENUM_USER_LOGIN == peer->m_usr_status) {
            /* forward msg to next hop */
            ret = m_usr_center->routeMsg(peer->m_fdinfo, msg, pDel);
        } else {
            MsgCenter::printMsg("router_proc_err", msg);
            
            ret = ENUM_ERR_NOT_LOGIN_GATEWAY;
        }
    } 

    return ret;
}

Int32 RouterDealer::processOut(RouterPair* routerPair, 
    MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    Router* local = &routerPair->m_router_out;
    Router* peer = &routerPair->m_router_in;

    if (ENUM_MSG_CMD_TCP_CIPHER == msg->m_cmd) {
        ret = procCipherData(routerPair, local, peer, msg, pDel); 
    } else if (ENUM_MSG_CMD_TCP_PLAIN == msg->m_cmd) {
        ret = procPlainData(routerPair, local, peer, msg, pDel); 
    } else if (ENUM_MSG_USR_EXCH_KEY == msg->m_cmd) {
        ret = procExchKey(routerPair, local, peer, msg, pDel);
    } else if (ENUM_MSG_USR_AUTH_END == msg->m_cmd) {
        ret = procAuthEnd(routerPair, local, peer, msg, pDel); 
    } else if (ENUM_MSG_CMD_STOP_GATEWAY == msg->m_cmd) {
        ret = procStopGateWay(routerPair, local, peer, msg, pDel);
    } else if (ENUM_MSG_CMD_CLOSE_SOCK == msg->m_cmd) {
        ret = procCloseRouter(routerPair, local, peer, msg, pDel);
    } else {
        if (ENUM_USER_LOGIN == local->m_usr_status
            && ENUM_USER_LOGIN == peer->m_usr_status) {
            /* forward msg to next hop */
            ret = m_usr_center->routeMsg(peer->m_fdinfo, msg, pDel);
        } else {
            MsgCenter::printMsg("router_proc_err", msg);
            
            ret = ENUM_ERR_NOT_LOGIN_GATEWAY;
        }
    } 
    
    return ret;
}

Int32 RouterDealer::procCipherData(RouterPair* routerPair, 
    Router* local, Router* peer, MsgHdr* msg, Bool* pDel) {
    MsgHdr* output = NULL;
    MsgTcpCipher* cipher = NULL;

    cipher = MsgCenter::cast<MsgTcpCipher>(msg);
  
    if (ENUM_USER_LOGIN != local->m_usr_status 
        || ENUM_USER_LOGIN != peer->m_usr_status) {
        LOG_ERROR("procCipherData| router_id=%u| msg=invalid|",
            cipher->m_user_id);

        return ENUM_ERR_NOT_LOGIN_GATEWAY;
    } 

    /* decrypt data by local evp */
    output = m_usr_center->decCipherText(local->m_evp_rcv, msg);
    
    /* in router, plain data shouldnot to offset
            transfer to peer */ 
    m_mng->sendMsg(peer->m_fdinfo, output); 
    return 0;
}

Int32 RouterDealer::procPlainData(RouterPair* routerPair, 
    Router* local, Router* peer, MsgHdr* msg, Bool* pDel) {
    MsgHdr* output = NULL;
    MsgTcpPlain* plain = NULL;

    plain = MsgCenter::cast<MsgTcpPlain>(msg);
    
    if (ENUM_USER_LOGIN != local->m_usr_status 
        || ENUM_USER_LOGIN != peer->m_usr_status) {
        LOG_ERROR("procPlainData| router_id=%u| msg=invalid|", 
            plain->m_user_id);

        return ENUM_ERR_NOT_LOGIN_GATEWAY;
    }
    
    /* encrypt data by peer evp */
    output = m_usr_center->encPlainText(peer->m_evp_snd, msg);

    /* transfer to peer */
    m_mng->sendMsg(peer->m_fdinfo, output); 
    return 0;
}

Int32 RouterDealer::procAuthReq(RouterPair* routerPair, 
    Router* local, Router* peer, MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    MsgHdr* output = NULL;
    MsgUserExchKey* rsp = NULL;
    MsgUserAuthReq* req = NULL;

    req = MsgCenter::cast<MsgUserAuthReq>(msg);

   
    if (ENUM_USER_WAIT_REQ != local->m_usr_status) {
        LOG_ERROR("procAuthReq| ret=%d| req_router_id=%u| self_router_id=%u|"
            " src_seid=%s| dst_seid=%s| msg=auth error|",
            ret, req->m_user_id, routerPair->m_router_id,
            req->m_src_seid, req->m_dst_seid);

        return ENUM_ERR_AUTH_GATEWAY;
    }

    /* check info */
    
    output = MsgCenter::creat<MsgUserExchKey>(ENUM_MSG_USR_EXCH_KEY);
    rsp = MsgCenter::cast<MsgUserExchKey>(msg); 

    /* filll response info */

    MsgCenter::addCrc(output);
    ret = m_mng->sendMsg(local->m_fdinfo, output);
    if (0 != ret) {
        LOG_ERROR("procAuthReq| ret=%d| req_router_id=%u| self_router_id=%u|"
            " src_seid=%s| dst_seid=%s| msg=send response msg error|",
            ret, req->m_user_id, routerPair->m_router_id,
            req->m_src_seid, req->m_dst_seid);
        
        return ENUM_ERR_AUTH_GATEWAY;
    }

    local->m_usr_status = ENUM_USER_EXCH_KEY;

    LOG_DEBUG("procAuthReq| req_router_id=%u| self_router_id=%u|"
        " src_seid=%s| dst_seid=%s| msg=auth ok|",
        req->m_user_id, routerPair->m_router_id,
        req->m_src_seid, req->m_dst_seid);
    
    return 0;
}

Int32 RouterDealer::procCipherKeyAck(RouterPair* routerPair, 
    Router* local, Router* peer, MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    MsgUserCKeyAck* req = NULL;
    MsgHdr* output = NULL;
    MsgUserAuthEnd* rsp = NULL;

    req = MsgCenter::cast<MsgUserCKeyAck>(msg);
  
    if (ENUM_USER_EXCH_KEY != local->m_usr_status) {
        LOG_ERROR("procCipherKeyAck| ret=%d| req_router_id=%u| self_router_id=%u|"
            " src_seid=%s| dst_seid=%s| msg=key ack error|",
            ret, req->m_user_id, routerPair->m_router_id,
            req->m_src_seid, req->m_dst_seid);

        return ENUM_ERR_AUTH_GATEWAY;
    }

    output = MsgCenter::creat<MsgUserAuthEnd>(ENUM_MSG_USR_AUTH_END);
    rsp = MsgCenter::cast<MsgUserAuthEnd>(msg); 

    MsgCenter::addCrc(output);
    ret = m_mng->sendMsg(local->m_fdinfo, output);
    if (0 != ret) {
        LOG_ERROR("procCipherKeyAck| ret=%d| req_router_id=%u| self_router_id=%u|"
            " src_seid=%s| dst_seid=%s| msg=send response msg error|",
            ret, req->m_user_id, routerPair->m_router_id,
            req->m_src_seid, req->m_dst_seid);
        
        return ENUM_ERR_AUTH_GATEWAY;
    }

    /* set cipher key */
    m_usr_center->setCipherKey(local->m_evp_rcv, DEF_SM4_KEY, DEF_SM4_KEY_LEN);
    m_usr_center->setCipherKey(local->m_evp_snd, DEF_SM4_KEY, DEF_SM4_KEY_LEN);

    local->m_usr_status = ENUM_USER_LOGIN;

    LOG_DEBUG("procCipherKeyAck| req_router_id=%u| self_router_id=%u|"
        " src_seid=%s| dst_seid=%s| msg=key ack ok|",
        req->m_user_id, routerPair->m_router_id,
        req->m_src_seid, req->m_dst_seid);

    /* if login ok, then start router conection */
    ret = m_mng->startRouterConn(routerPair);
    
    return ret;
}

Int32 RouterDealer::procExchKey(RouterPair* routerPair, 
    Router* local, Router* peer, MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    MsgHdr* output = NULL;
    MsgUserExchKey* req = NULL;
    MsgUserCKeyAck* rsp = NULL;

    req = MsgCenter::cast<MsgUserExchKey>(msg);
 
    if (ENUM_USER_AUTH_REQ != local->m_usr_status) {
        LOG_ERROR("procExchKey| ret=%d| req_router_id=%u| self_router_id=%u|"
            " src_seid=%s| dst_seid=%s| msg=key exchange error|",
            ret, req->m_user_id, routerPair->m_router_id,
            req->m_src_seid, req->m_dst_seid);
        
        return ENUM_ERR_AUTH_GATEWAY;
    }

    /* check info */
    
    output = MsgCenter::creat<MsgUserCKeyAck>(ENUM_MSG_USR_CIPHER_KEY_ACK);
    rsp = MsgCenter::cast<MsgUserCKeyAck>(msg); 

    /* filll response info */

    MsgCenter::addCrc(output);
    ret = m_mng->sendMsg(local->m_fdinfo, output);
    if (0 != ret) {
        LOG_ERROR("procExchKey| ret=%d| req_router_id=%u| self_router_id=%u|"
            " src_seid=%s| dst_seid=%s| msg=send response msg error|",
            ret, req->m_user_id, routerPair->m_router_id,
            req->m_src_seid, req->m_dst_seid);
        
        return ENUM_ERR_AUTH_GATEWAY;
    }
    
    local->m_usr_status = ENUM_USER_EXCH_KEY_ACK;

    LOG_DEBUG("procExchKey| req_router_id=%u| self_router_id=%u|"
        " src_seid=%s| dst_seid=%s| msg=key exchange ok|",
        req->m_user_id, routerPair->m_router_id,
        req->m_src_seid, req->m_dst_seid);
    
    return 0;
}

Int32 RouterDealer::procAuthEnd(RouterPair* routerPair, 
    Router* local, Router* peer, MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    MsgUserAuthEnd* req = NULL;

    req = MsgCenter::cast<MsgUserAuthEnd>(msg);
   
    if (ENUM_USER_EXCH_KEY_ACK != local->m_usr_status) {
        LOG_ERROR("procAuthEnd| ret=%d| req_router_id=%u| self_router_id=%u|"
            " src_seid=%s| dst_seid=%s| msg=auth end error|",
            ret, req->m_user_id, routerPair->m_router_id,
            req->m_src_seid, req->m_dst_seid);

        return ENUM_ERR_AUTH_GATEWAY;
    }
    
    /* set cipher key */
    m_usr_center->setCipherKey(local->m_evp_rcv, DEF_SM4_KEY, DEF_SM4_KEY_LEN);
    m_usr_center->setCipherKey(local->m_evp_snd, DEF_SM4_KEY, DEF_SM4_KEY_LEN);

    local->m_usr_status = ENUM_USER_LOGIN; 

    LOG_DEBUG("procAuthEnd| req_router_id=%u| self_router_id=%u|"
        " src_seid=%s| dst_seid=%s| msg=auth end ok|",
        req->m_user_id, routerPair->m_router_id,
        req->m_src_seid, req->m_dst_seid); 
    
    return 0;
}

Int32 RouterDealer::procStopGateWay(RouterPair* routerPair, 
    Router* local, Router* peer, MsgHdr* msg, Bool* pDel) {
    MsgStopGateway* req = NULL;    

    req = MsgCenter::cast<MsgStopGateway>(msg);

    /* notify to close */
    if (!local->m_fdinfo->m_peer_err) {
        local->m_fdinfo->m_peer_err = TRUE;
    }

    LOG_INFO("stopGateWay| req_router_id=%u| self_router_id=%u|"
        " msg=stop gateway|",
        req->m_route_id, routerPair->m_router_id);
    
    return 0;
}

Int32 RouterDealer::procCloseRouter(RouterPair* routerPair, 
    Router* local, Router* peer, MsgHdr* msg, Bool* pDel) {
    MsgCloseSock* closeMsg = MsgCenter::cast<MsgCloseSock>(msg);
    

    LOG_INFO("procCloseRouter| router_id=%u| reason=%d| msg=ok|",
        routerPair->m_router_id, closeMsg->m_reason); 

    /* end to parent */
    return 1;
}

