#ifndef __AUTHSESS_H__
#define __AUTHSESS_H__
#include"globaltype.h"
#include"datatype.h"


struct MsgHdr;
class ManageCenter;
class UserCenter;

class SessAccptAuth {
public:
    SessAccptAuth(ManageCenter* mng, UserCenter* usr_center);
    
    Int32 process(SessionAccpt* sessAccpt, MsgHdr* msg, Bool* pDel);

private:
    Int32 procPlainData(SessionAccpt* sessAccpt, MsgHdr* msg, Bool* pDel); 
    Int32 procSessArrival(SessionAccpt* sessAccpt, MsgHdr* msg, Bool* pDel);
    Int32 procStopSess(SessionAccpt* sessAccpt, MsgHdr* msg, Bool* pDel);
    Int32 procCloseSess(SessionAccpt* sessAccp, MsgHdr* msg, Bool* pDel);
    
private:
    ManageCenter* m_mng;
    UserCenter* m_usr_center;
};

class SessConnAuth {
public:
    SessConnAuth(ManageCenter* mng, UserCenter* usr_center);
    
    Int32 process(SessionConn* sessConn, MsgHdr* msg, Bool* pDel);

private:
    Int32 procPlainData(SessionConn* sessConn, MsgHdr* msg, Bool* pDel);
    Int32 procStopSess(SessionConn* sessConn, MsgHdr* msg, Bool* pDel);
    Int32 procCloseSess(SessionConn* sessConn, MsgHdr* msg, Bool* pDel);

private:
    ManageCenter* m_mng;
    UserCenter* m_usr_center;
};

#endif

