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

    virtual Int32 getType() const {
        return ENUM_NODE_SESS_LISTENER;
    }
    
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

    virtual Int32 getType() const {
        return ENUM_NODE_SESS_CONN;
    }
    
    virtual int readFd(struct FdInfo* info);
    virtual int writeFd(struct FdInfo* info);
    virtual Int32 procMsg(FdInfo* info, MsgHdr* msg);
    virtual Void eof(struct FdInfo* info);

    SessionConn* setup(UserAccpt* usr, Uint32 sessId,
        const Char* ip, int port);

protected:
    virtual Int32 process(SessionConn* sess, MsgHdr* msg);

private:
    Int32 sendArrival(UserAccpt* usr, SessionConn* sess);
    Int32 procPlainData(SessionConn* sess, MsgHdr* msg);
    Int32 procStopSess(SessionConn* sess, MsgHdr* msg);
    Int32 procCipherData(SessionConn* sess, MsgHdr* msg);
    Int32 procEnd(SessionConn* sess, MsgHdr* msg);

protected:
    SockMng* m_mng;
    SockCenter* m_center;
};

class SockSessAccpt : public I_FdObj {
public:
    SockSessAccpt(SockMng* mng, SockCenter* center)
        : m_mng(mng), m_center(center) {}

    virtual Int32 getType() const {
        return ENUM_NODE_SESS_ACCPT;
    }
    
    virtual int readFd(struct FdInfo* info);
    virtual int writeFd(struct FdInfo* info);
    virtual Int32 procMsg(FdInfo* info, MsgHdr* msg);
    virtual Void eof(struct FdInfo* info);

    SessionAccpt* setup(ListenerTcp* listener, int newfd);

protected:
    virtual Int32 process(SessionAccpt* sess, MsgHdr* msg);

private:
    Int32 procPlainData(SessionAccpt* sess, MsgHdr* msg);
    Int32 procArrival(SessionAccpt* sess, MsgHdr* msg);
    Int32 procStopSess(SessionAccpt* sess, MsgHdr* msg);
    Int32 procCipherData(SessionAccpt* sess, MsgHdr* msg);
    Int32 procEnd(SessionAccpt* sess, MsgHdr* msg);

protected:
    SockMng* m_mng;
    SockCenter* m_center;
};


/* this class is uncrypted version of sess conn */
class SockSessConnPseudo : public SockSessConn {
public:
    SockSessConnPseudo(SockMng* mng, SockCenter* center)
        : SockSessConn(mng, center) {}

    virtual Int32 getType() const {
        return ENUM_NODE_SESS_CONN_PSEUDO;
    }

protected:
    Int32 process(SessionConn* sess, MsgHdr* msg);

private:
    Int32 procPlainData(SessionConn* sess, MsgHdr* msg);
    Int32 procPseudoData(SessionConn* sess, MsgHdr* msg);
};


/* this class is unencrypted version of sess accpt */
class SockSessAccptPseudo : public SockSessAccpt {
public:
    SockSessAccptPseudo(SockMng* mng, SockCenter* center)
        : SockSessAccpt(mng, center) {}

    virtual Int32 getType() const {
        return ENUM_NODE_SESS_ACCPT_PSEUDO;
    }

protected:
    virtual Int32 process(SessionAccpt* sess, MsgHdr* msg);

private:
    Int32 procPlainData(SessionAccpt* sess, MsgHdr* msg);
    Int32 procPseudoData(SessionAccpt* sess, MsgHdr* msg);
};

class SockSessListenerPseudo : public SockSessListener {
public:
    SockSessListenerPseudo(SockMng* mng, SockCenter* center, 
        SockSessAccptPseudo* accpt) 
        : SockSessListener(mng, center, accpt) {}

    virtual Int32 getType() const {
        return ENUM_NODE_SESS_LISTENER_PSEUDO;
    }
};

#endif

