#include"authadmin.h"
#include"msgcenter.h"
#include"managecenter.h"
#include"usercenter.h"
#include"msgtype.h"


AdminAccptAuth::AdminAccptAuth(ManageCenter* mng, UserCenter* usr_center) {
    m_mng = mng;
    m_usr_center = usr_center;
}

Void AdminAccptAuth::process(AdminAccpt* adminAccpt, MsgHdr* msg) {
    if (ENUM_MSG_CMD_ADM_INIT_EKEY == msg->m_cmd) {
        procEkeyInit(adminAccpt, msg);
    } else {
    } 
}

Int32 AdminAccptAuth::procEkeyInit(AdminAccpt* adminAccpt, MsgHdr* msg) {
    
    return 0;
}

AdminConnAuth::AdminConnAuth(ManageCenter* mng, UserCenter* usr_center) {
    m_mng = mng;
    m_usr_center = usr_center;
}

Void AdminConnAuth::process(AdminConn* adminConn, MsgHdr* msg) {
    if (ENUM_MSG_CMD_AUTH_PKEY_ACK == msg->m_cmd) {
        procAuthPkey(adminConn, msg);
    } else {
    } 
}

Int32 AdminConnAuth::procAuthPkey(AdminConn* adminConn, MsgHdr* msg) {
    
    return 0;
}
