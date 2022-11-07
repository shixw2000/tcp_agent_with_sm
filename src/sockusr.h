#ifndef __SOCKUSR_H__
#define __SOCKUSR_H__
#include"globaltype.h"
#include"interfobj.h"
#include"datatype.h"


struct MsgHdr;
class SockMng;
class SockCenter;
class SockUsrAccpt;
class SockSessConn;

class SockUsrListener : public I_FdObj {
public:
    SockUsrListener(SockMng* mng, SockCenter* center,
        SockUsrAccpt* accpt)
        : m_mng(mng), m_center(center), m_accpt(accpt) {}
    
    virtual int readFd(struct FdInfo* info);
    virtual int writeFd(struct FdInfo* info);
    virtual Int32 procMsg(FdInfo* info, MsgHdr* msg);
    virtual Void eof(struct FdInfo* info);

    ListenerDirty* setup(const TcpParam* param);

private:
    Int32 procEnd(ListenerDirty* listener, MsgHdr* msg);
    Int32 procChildExit(ListenerDirty* listener, MsgHdr* msg);
    Int32 procAddUsr(ListenerDirty* listener, MsgHdr* msg);
    
    Int32 newUser(ListenerDirty* listener, int newfd);

private:
    SockMng* m_mng;
    SockCenter* m_center;
    SockUsrAccpt* m_accpt;
};

class SockUsrAccpt : public I_FdObj {
public:
    SockUsrAccpt(SockMng* mng, SockCenter* center, SockSessConn* conn)
        : m_mng(mng), m_center(center), m_conn(conn) {}
    
    virtual int readFd(struct FdInfo* info);
    virtual int writeFd(struct FdInfo* info);
    virtual Int32 procMsg(FdInfo* info, MsgHdr* msg);
    virtual Void eof(struct FdInfo* info);

    SessionConn* findSess(UserAccpt* usr, Uint32 sessID);
    UserAccpt* setup(ListenerDirty* listener, int newfd);

private:
    Int32 process(UserAccpt* usr, MsgHdr* msg);
    Int32 procAuthReq(UserAccpt* usr, MsgHdr* msg);
    Int32 procCipherKeyAck(UserAccpt* usr, MsgHdr* msg); 
    Int32 procStartSess(UserAccpt* usr, MsgHdr* msg);
    Int32 procChildExit(UserAccpt* usr, MsgHdr* msg);
    Int32 procEnd(UserAccpt* usr, MsgHdr* msg);

    Int32 parseAuthReq(UserAccpt* usr, MsgUserAuthReq* req);
    MsgHdr* creatExchKeyReq(UserAccpt* usr);
    Int32 parseCKeyAck(UserAccpt* usr, MsgUserCKeyAck* req);
    MsgHdr* creatAuthEnd(UserAccpt* usr);

    Int32 startSessConn(UserAccpt* usr, MsgStartSess* req);

    Int32 transfer2Sess(UserAccpt* usr, MsgHdr* msg);

private:
    SockMng* m_mng;
    SockCenter* m_center;
    SockSessConn* m_conn;
};

class SockUsrConn : public I_FdObj {
public:
    SockUsrConn(SockMng* mng, SockCenter* center)
        : m_mng(mng), m_center(center) {}
    
    virtual int readFd(struct FdInfo* info);
    virtual int writeFd(struct FdInfo* info);
    virtual Int32 procMsg(FdInfo* info, MsgHdr* msg);
    virtual Void eof(struct FdInfo* info);

    SessionAccpt* findSess(UserConn* usr, Uint32 sessID);

    UserConn* setup(const TcpParam* param);

private:
    Int32 process(UserConn* usr, MsgHdr* msg);
    Int32 procSetupAuth(UserConn* usr, MsgHdr* msg);
    Int32 procExchKey(UserConn* usr, MsgHdr* msg);
    Int32 procAuthEnd(UserConn* usr, MsgHdr* msg);
    Int32 procUsrArrival(UserConn* usr, MsgHdr* msg);
    Int32 procChildExit(UserConn* usr, MsgHdr* msg);
    Int32 procEnd(UserConn* usr, MsgHdr* msg);

    MsgHdr* creatAuthReq(UserConn* usr);
    Int32 parseExchKeyReq(UserConn* usr, MsgUserExchKey* req);
    MsgHdr* creatCKeyAck(UserConn* usr);

    Int32 writeUsrConn(UserConn* usr);

    Int32 transfer2Sess(UserConn* usr, MsgHdr* msg);

private:
    SockMng* m_mng;
    SockCenter* m_center;
};

#endif

