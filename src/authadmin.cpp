#include"authadmin.h"
#include"msgcenter.h"
#include"managecenter.h"
#include"usercenter.h"
#include"msgtype.h"


AdminAccptAuth::AdminAccptAuth(ManageCenter* mng, UserCenter* usr_center) {
    m_mng = mng;
    m_usr_center = usr_center;
}

Int32 AdminAccptAuth::process(AdminAccpt* adminAccpt, MsgHdr* msg, Bool* pDel) {
    Int32 ret = -1;
    
    if (ENUM_MSG_CMD_ADM_INIT_EKEY == msg->m_cmd) {
        ret = procEkeyInit(adminAccpt, msg, pDel);
    } else {
        ret = -1;
    } 

    return ret;
}

Int32 AdminAccptAuth::procEkeyInit(AdminAccpt* adminAccpt, 
    MsgHdr* msg, Bool* pDel) {
    
    return 0;
}

AdminConnAuth::AdminConnAuth(ManageCenter* mng, UserCenter* usr_center) {
    m_mng = mng;
    m_usr_center = usr_center;
}

Int32 AdminConnAuth::process(AdminConn* adminConn, MsgHdr* msg, Bool* pDel) {
    Int32 ret = 0;
    
    if (ENUM_MSG_CMD_AUTH_PKEY_ACK == msg->m_cmd) {
        ret = procAuthPkey(adminConn, msg, pDel);
    } else {
        ret = -1;
    } 

    return ret;
}

Int32 AdminConnAuth::procAuthPkey(AdminConn* adminConn, 
    MsgHdr* msg, Bool* pDel) {
    
    return 0;
}
