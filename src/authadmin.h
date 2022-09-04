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
    
    Void process(AdminAccpt* adminAccpt, MsgHdr* msg);

private:
    Int32 procEkeyInit(AdminAccpt* adminAccpt, MsgHdr* msg); 
    
private:
    ManageCenter* m_mng;
    UserCenter* m_usr_center;
};

class AdminConnAuth {
public:
    AdminConnAuth(ManageCenter* mng, UserCenter* usr_center);
    
    Void process(AdminConn* adminConn, MsgHdr* msg);

private:
    Int32 procAuthPkey(AdminConn* adminConn, MsgHdr* msg); 
    
private:
    ManageCenter* m_mng;
    UserCenter* m_usr_center;
};

#endif

