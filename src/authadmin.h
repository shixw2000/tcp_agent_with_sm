#ifndef __AUTHADMIN_H__
#define __AUTHADMIN_H__
#include"globaltype.h"
#include"datatype.h"


struct MsgHdr;
class ManageCenter;
class UserCenter;

class AdminAccptAuth {
public:
    AdminAccptAuth(ManageCenter* mng, UserCenter* usr_center);
    
    Int32 process(AdminAccpt* adminAccpt, MsgHdr* msg, Bool* pDel);

private:
    Int32 procEkeyInit(AdminAccpt* adminAccpt, MsgHdr* msg, Bool* pDel); 
    
private:
    ManageCenter* m_mng;
    UserCenter* m_usr_center;
};

class AdminConnAuth {
public:
    AdminConnAuth(ManageCenter* mng, UserCenter* usr_center);
    
    Int32 process(AdminConn* adminConn, MsgHdr* msg, Bool* pDel);

private:
    Int32 procAuthPkey(AdminConn* adminConn, MsgHdr* msg, Bool* pDel); 
    
private:
    ManageCenter* m_mng;
    UserCenter* m_usr_center;
};

#endif

