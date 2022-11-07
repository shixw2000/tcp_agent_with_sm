#ifndef __SOCKSESS_H__
#define __SOCKSESS_H__
#include"globaltype.h"
#include"interfobj.h"
#include"datatype.h"


struct MsgHdr;
class SockMng;
class SockCenter;
class SockSessAccpt;

class SockSessListener : public I_FdObj {
public:
    SockSessListener(SockMng* mng, SockCenter* center, 
        SockSessAccpt* accpt)
        : m_mng(mng), m_center(center), m_accpt(accpt) {}
    
    virtual int readFd(struct FdInfo* info);
    virtual int writeFd(struct FdInfo* info);
    virtual Int32 procMsg(FdInfo* info, MsgHdr* msg);
    virtual Void eof(struct FdInfo* info);

    ListenerTcp* setup(UserConn* usr, const TcpPairs* pairs);

private:
    Int32 procEnd(ListenerTcp* listener, MsgHdr* msg);
    
    Int32 procAddSess(ListenerTcp* listener, MsgHdr* msg);

private:
    SockMng* m_mng;
    SockCenter* m_center;
    SockSessAccpt* m_accpt;
};

class SockSessConn : public I_FdObj {
public:
    SockSessConn(SockMng* mng, SockCenter* center)
        : m_mng(mng), m_center(center) {}
    
    virtual int readFd(struct FdInfo* info);
    virtual int writeFd(struct FdInfo* info);
    virtual Int32 procMsg(FdInfo* info, MsgHdr* msg);
    virtual Void eof(struct FdInfo* info);

    SessionConn* setup(UserAccpt* usr, Uint32 sessId,
        const Char* ip, int port);

private:
    Int32 sendArrival(UserAccpt* usr, SessionConn* sess);
    Int32 process(SessionConn* sess, MsgHdr* msg);
    Int32 procPlainData(SessionConn* sess, MsgHdr* msg);
    Int32 procStopSess(SessionConn* sess, MsgHdr* msg);
    Int32 procCipherData(SessionConn* sess, MsgHdr* msg);
    Int32 procEnd(SessionConn* sess, MsgHdr* msg);

private:
    SockMng* m_mng;
    SockCenter* m_center;
};

class SockSessAccpt : public I_FdObj {
public:
    SockSessAccpt(SockMng* mng, SockCenter* center)
        : m_mng(mng), m_center(center) {}
    
    virtual int readFd(struct FdInfo* info);
    virtual int writeFd(struct FdInfo* info);
    virtual Int32 procMsg(FdInfo* info, MsgHdr* msg);
    virtual Void eof(struct FdInfo* info);

    SessionAccpt* setup(ListenerTcp* listener, int newfd);

private:
    Int32 process(SessionAccpt* sess, MsgHdr* msg);
    Int32 procPlainData(SessionAccpt* sess, MsgHdr* msg);
    Int32 procArrival(SessionAccpt* sess, MsgHdr* msg);
    Int32 procStopSess(SessionAccpt* sess, MsgHdr* msg);
    Int32 procCipherData(SessionAccpt* sess, MsgHdr* msg);
    Int32 procEnd(SessionAccpt* sess, MsgHdr* msg);

private:
    SockMng* m_mng;
    SockCenter* m_center;
};

#endif

