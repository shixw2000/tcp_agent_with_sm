#ifndef __SOCKCENTER_H__
#define __SOCKCENTER_H__
#include"globaltype.h"
#include"msgtype.h"
#include"datatype.h"
#include"listnode.h"
#include"interfobj.h"
#include"config.h"


class SockMng;
class SockUsrListener;
class SockSessListener;
class SockUsrConn;
class SockUsrAccpt;
class SockSessConn;
class SockSessAccpt;
class SockEvent;

class SockCenter : public I_FdObj, public I_TimerDealer {
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

    virtual int readFd(struct FdInfo* info);
    virtual int writeFd(struct FdInfo* info);
    virtual Int32 procMsg(FdInfo* info, MsgHdr* msg);
    virtual Void eof(struct FdInfo* info);

    Int32 creatTimerData();

    Int32 chkConn(FdInfo* info);

    struct SysBase* getBase() {
        return &m_base;
    }

    Int32 addAgentCli(AgentCli* agent, list_head* list);
    Int32 addAgentSrv(AgentSrv* agent, list_head* list);
    
    Void freeUsrAccptQue(list_head* list);
    Void freeUsrConnQue(list_head* list);
    Void freeSessAccptQue(order_list_head* list);
    Void freeSessConnQue(order_list_head* list);

    Void freeMsgQue(list_head* list);
    Void freeFd(FdInfo* info);

    Void closeSock(SockBase* sock);
    Void resetSock(SockBase* sock); 
    Void resetNode(NodeBase* base);

    EvpBase* creatEvp();
    Void freeEvp(EvpBase* evp);

    EventData* newEventData();
    Void freeEventData(EventData* ev);
    
    TimerData* newTimerData();
    Void freeTimerData(TimerData* timer);

    ListenerTcp* newListenerTcp();
    Void freeListenerTcp(ListenerTcp* listener);

    ListenerDirty* newListenerDirty();
    Void freeListenerDirty(ListenerDirty* listener);
  
    SessionAccpt* newSessAccpt();
    Void freeSessAccpt(SessionAccpt* sess);

    SessionConn* newSessConn();
    Void freeSessConn(SessionConn* sess);

    UserAccpt* newUsrAccpt();
    Void freeUsrAccpt(UserAccpt* usr);

    UserConn* newUsrConn();
    Void freeUsrConn(UserConn* usr);

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

protected:
    virtual Void doTimeout(struct TimerEle* ele);

private:
    list_head m_list_service;
    Uint32 m_last_session_id;
    Uint32 m_last_user_id;
    SockMng* m_mng;
    const Char* m_path;
    Parser* m_parser;
    
    SockEvent* m_event;
    SockUsrListener* m_usr_listener;
    SockSessListener* m_sess_listener;
    SockUsrConn* m_usr_conn;
    SockUsrAccpt* m_usr_accpt;
    SockSessConn* m_sess_conn;
    SockSessAccpt* m_sess_accpt;

    I_FdObj* m_obj[ENUM_NODE_END];
    
    struct SysBase m_base;
};

#endif

