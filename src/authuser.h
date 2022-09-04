#ifndef __AUTHUSER_H__
#define __AUTHUSER_H__
#include"globaltype.h"
#include"datatype.h"


struct MsgHdr;
class ManageCenter;
class UserCenter;

class UsrAccptAuth {
public:
    UsrAccptAuth(ManageCenter* mng, UserCenter* usr_center);
    
    Void process(UserAccpt* usrAccpt, MsgHdr* msg);

private:
    Int32 procCipherData(UserAccpt* usrAccpt, MsgHdr* msg); 
    Int32 procStartSess(UserAccpt* usrAccpt, MsgHdr* msg);
    Int32 procStopSess(UserAccpt* usrAccpt, MsgHdr* msg);

    Int32 procAuthReq(UserAccpt* usrAccpt, MsgHdr* msg);
    Int32 procCipherKeyAck(UserAccpt* usrAccpt, MsgHdr* msg);
    
private:
    ManageCenter* m_mng;
    UserCenter* m_usr_center;
};

class UsrConnAuth {
public:
    UsrConnAuth(ManageCenter* mng, UserCenter* usr_center);
    
    Void process(UserConn* usrConn, MsgHdr* msg); 

private:
    Int32 procCipherData(UserConn* usrConn, MsgHdr* msg);
    Int32 procStopSess(UserConn* usrConn, MsgHdr* msg);
    Int32 procExchKey(UserConn* usrConn, MsgHdr* msg);

private:
    ManageCenter* m_mng;
    UserCenter* m_usr_center;
};

#endif

