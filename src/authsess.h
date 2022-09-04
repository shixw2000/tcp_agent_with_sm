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
    
    Void process(SessionAccpt* sessAccpt, MsgHdr* msg);

private:
    Int32 procPlainData(SessionAccpt* sessAccpt, MsgHdr* msg); 
    
private:
    ManageCenter* m_mng;
    UserCenter* m_usr_center;
};

class SessConnAuth {
public:
    SessConnAuth(ManageCenter* mng, UserCenter* usr_center);
    
    Void process(SessionConn* sessConn, MsgHdr* msg);

private:
    Int32 procPlainData(SessionConn* sessConn, MsgHdr* msg);

private:
    ManageCenter* m_mng;
    UserCenter* m_usr_center;
};

#endif

