#include"authsess.h"
#include"msgcenter.h"
#include"managecenter.h"
#include"usercenter.h"
#include"msgtype.h"


SessAccptAuth::SessAccptAuth(ManageCenter* mng, UserCenter* usr_center) {
    m_mng = mng;
    m_usr_center = usr_center;
}

Void SessAccptAuth::process(SessionAccpt* sessAccpt, MsgHdr* msg) {
    if (ENUM_MSG_CMD_TCP_PLAIN == msg->m_cmd) {
        procPlainData(sessAccpt, msg);
    } else {
    } 
}

Int32 SessAccptAuth::procPlainData(SessionAccpt* sessAccpt, MsgHdr* msg) {
    UserConn* usrConn = sessAccpt->m_parent;
    MsgHdr* output = NULL;
    MsgTcpPlain* plain = NULL;
    
    plain = MsgCenter::cast<MsgTcpPlain>(msg);
    plain->m_user_id = usrConn->m_user_id;
    plain->m_session_id = sessAccpt->m_session_id;
    
    output = m_usr_center->encPlainText(usrConn->m_evp, msg);

    /* out from user conn */
    m_mng->sendMsg(usrConn->m_fdinfo, output);
    return 0;
}


SessConnAuth::SessConnAuth(ManageCenter* mng, UserCenter* usr_center) {
    m_mng = mng;
    m_usr_center = usr_center;
}

Void SessConnAuth::process(SessionConn* sessConn, MsgHdr* msg) {
    if (ENUM_MSG_CMD_TCP_PLAIN == msg->m_cmd) {
        procPlainData(sessConn, msg);
    }
}

Int32 SessConnAuth::procPlainData(SessionConn* sessConn, MsgHdr* msg) {
    UserAccpt* usrAccpt = sessConn->m_parent;
    MsgHdr* output = NULL;
    MsgTcpPlain* plain = NULL;
    
    plain = MsgCenter::cast<MsgTcpPlain>(msg);
    plain->m_user_id = usrAccpt->m_user_id;
    plain->m_session_id = sessConn->m_session_id;

    output = m_usr_center->encPlainText(usrAccpt->m_evp, msg); 

    /* out from user conn */
    m_mng->sendMsg(usrAccpt->m_fdinfo, output);
    return 0;
}


