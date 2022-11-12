#ifndef __SOCKCENTER_H__
#define __SOCKCENTER_H__
#include"globaltype.h"
#include"msgtype.h"
#include"datatype.h"
#include"listnode.h"
#include"interfobj.h"
#include"config.h"


class SockMng;
class SockCenter; 

class FdObjFactory : public I_FdObj {
public:
    FdObjFactory(SockMng* mng, SockCenter* center)
        : m_mng(mng), m_center(center) {};

    virtual Int32 getType() const {
        return ENUM_NODE_OBJ_MAX;
    }

    Int32 init();
    Void finish(); 

    template<typename T>
    T* getObj(Int32 type) {
        T* obj = NULL;

        if (ENUM_NODE_OBJ_MIN <= type && type < ENUM_NODE_OBJ_MAX
            && type == m_obj[type]->getType()) {
            obj = dynamic_cast<T*>(m_obj[type]);
        } 

        return obj;
    }

    Int32 addAgentCli(AgentCli* agent, list_head* list);
    Int32 addAgentSrv(AgentSrv* agent, list_head* list);

    virtual int readFd(struct FdInfo* info);
    virtual int writeFd(struct FdInfo* info);
    virtual Int32 procMsg(FdInfo* info, MsgHdr* msg);
    virtual Void eof(struct FdInfo* info);

public:
    static Void freeListenerUsrQue(list_head* list);
    static Void freeListenerSessQue(list_head* list);
    static Void freeUsrAccptQue(list_head* list);
    static Void freeUsrConnQue(list_head* list);
    static Void freeSessAccptQue(order_list_head* list);
    static Void freeSessConnQue(order_list_head* list);

    static Void freeMsgQue(list_head* list);
    static Void freeFd(FdInfo* info);

    static Void closeSock(SockBase* sock);
    static Void resetSock(SockBase* sock); 
    static Void resetNode(NodeBase* base);

    static EvpBase* creatEvp();
    static Void freeEvp(EvpBase* evp);

    static ListenerTcp* newListenerTcp();
    static Void freeListenerTcp(ListenerTcp* listener);

    static ListenerDirty* newListenerDirty();
    static Void freeListenerDirty(ListenerDirty* listener);
  
    static SessionAccpt* newSessAccpt();
    static Void freeSessAccpt(SessionAccpt* sess);

    static SessionConn* newSessConn();
    static Void freeSessConn(SessionConn* sess);

    static UserAccpt* newUsrAccpt();
    static Void freeUsrAccpt(UserAccpt* usr);

    static UserConn* newUsrConn();
    static Void freeUsrConn(UserConn* usr);

private:
    SockMng* m_mng;
    SockCenter* m_center;
    I_FdObj* m_obj[ENUM_NODE_OBJ_MAX];
};

class SockCenter {
public:
    SockCenter();

    Void set(const Char* path) {
        m_path = path;
    }

    Int32 init();
    Void finish(); 

    Int32 startServer();
    Void stopServer();
    Void wait();

    Int32 chkConn(FdInfo* info);

    struct SysBase* getBase() {
        return &m_base;
    }

    Int32 addAgentCli(AgentCli* agent, list_head* list);
    Int32 addAgentSrv(AgentSrv* agent, list_head* list);
  
    MsgHdr* decCipherText(EvpBase* evp, MsgHdr* input);
    MsgHdr* encPlainText(EvpBase* evp, MsgHdr* input);

    Void setCipherKey(EvpBase* evp, const Void* usr_key, Int32 len);
    Int32 encrypt(EvpBase* evp, const Void* data, Int32 len, Void* out);
    Int32 decrypt(EvpBase* evp, const Void* data, Int32 len, Void* out);

    Int32 digest(EvpBase* evp, const Void* data, Int32 len, Void* out);

    Uint32 hashMac(EvpBase* evp, const Void* data, Int32 len);
        
    Int32 sendPeerSess(UserConn* usr, SessionAccpt* sess);

    Int32 notifySessEvent(FdInfo* info, Uint16 cmd,
        Uint32 usrId, Uint32 sessId);
    
    Int32 sendSessEvent(FdInfo* info, Uint16 cmd, 
        Uint32 usrId, Uint32 sessId);

    Int32 notifyUsrEvent(FdInfo* info, Uint16 cmd, Uint32 usrId);
    Int32 sendUsrEvent(FdInfo* info, Uint16 cmd, Uint32 usrId);

    Int32 notifyChildUsrExit(FdInfo* info, Uint32 usrId, Uint64 data);

    unsigned int nextSessId() {
        return ++m_last_session_id;
    }

    unsigned int nextUsrId() {
        return ++m_last_user_id;
    }
  
private:
    list_head m_list_server;
    list_head m_list_client;
    Uint32 m_last_session_id;
    Uint32 m_last_user_id;
    const Char* m_path;
    SockMng* m_mng; 
    Parser* m_parser;
    FdObjFactory* m_fctry; 
    
    struct SysBase m_base;
};

#endif

