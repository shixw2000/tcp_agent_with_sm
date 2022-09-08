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
    
    Int32 process(UserAccpt* usrAccpt, MsgHdr* msg, Bool* pDel);

private:
    Int32 procCipherData(UserAccpt* usrAccpt, MsgHdr* msg, Bool* pDel); 
    Int32 procStartSess(UserAccpt* usrAccpt, MsgHdr* msg, Bool* pDel);
    Int32 procStopSess(UserAccpt* usrAccpt, MsgHdr* msg, Bool* pDel);
    Int32 procStopUsrAccpt(UserAccpt* usrAccpt, MsgHdr* msg, Bool* pDel);

    Int32 procAuthReq(UserAccpt* usrAccpt, MsgHdr* msg, Bool* pDel);
    Int32 procCipherKeyAck(UserAccpt* usrAccpt, MsgHdr* msg, Bool* pDel);

    Int32 procCloseChild(UserAccpt* usrAccpt, MsgHdr* msg, Bool* pDel);

    Int32 procCloseUsr(UserAccpt* usrAccpt, MsgHdr* msg, Bool* pDel);
    
private:
    ManageCenter* m_mng;
    UserCenter* m_usr_center;
};

class UsrConnAuth {
public:
    UsrConnAuth(ManageCenter* mng, UserCenter* usr_center);
    
    Int32 process(UserConn* usrConn, MsgHdr* msg, Bool* pDel); 

private:
    Int32 procCipherData(UserConn* usrConn, MsgHdr* msg, Bool* pDel);
    Int32 procStopSess(UserConn* usrConn, MsgHdr* msg, Bool* pDel);
    Int32 procExchKey(UserConn* usrConn, MsgHdr* msg, Bool* pDel);
    Int32 procAuthEnd(UserConn* usrConn, MsgHdr* msg, Bool* pDel);
    Int32 procStopUsrConn(UserConn* usrConn, MsgHdr* msg, Bool* pDel);

    Int32 procUsrArrival(UserConn* usrConn, MsgHdr* msg, Bool* pDel);
    Int32 procSessArrival(UserConn* usrConn, MsgHdr* msg, Bool* pDel);

    Int32 procCloseChild(UserConn* usrConn, MsgHdr* msg, Bool* pDel);

    Int32 procCloseUsr(UserConn* usrConn, MsgHdr* msg, Bool* pDel);
  
private:
    ManageCenter* m_mng;
    UserCenter* m_usr_center;
};

#endif

