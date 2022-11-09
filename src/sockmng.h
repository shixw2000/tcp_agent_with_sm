#ifndef __SOCKMNG_H__
#define __SOCKMNG_H__
#include"globaltype.h"
#include"taskpool.h"
#include"listnode.h"
#include"interfobj.h"


struct FdInfo;
struct NodeBase;
class Lock;
class SockPoll;
class SockDealer;

class SockMng : public TaskPool, public I_Dispatcher {
public:
    explicit SockMng(int capacity);
    ~SockMng();
    
    virtual Int32 init();
    virtual Void finish(); 

    Void set(int fd, I_FdObj* obj);

    virtual int start(const char* name);
    virtual void join();
    virtual void stop(); 

    virtual Int32 dispatch(FdInfo* info, MsgHdr* msg); 
    virtual Int32 sendMsg(FdInfo* info, MsgHdr* msg);
    virtual void procTaskEnd(struct Task* task);
    virtual unsigned int procTask(struct Task* task);

    FdInfo* creatBase(Int32 fd, Bool testRd, Bool testWr);
        
    Int32 readMsg(struct FdInfo*);
    Int32 writeMsg(struct FdInfo*);
    Void dealMsg(struct FdInfo*);
    Void endFd(FdInfo* info); 

    Int32 notify(FdInfo* info, Uint16 cmd, Uint64 data = 0);

    Void addEvent(FdInfo* info, NodeBase* base);

    Bool lock(FdInfo* info);
    Bool unlock(FdInfo* info);

    void closeMng(struct FdInfo* info);
    

private:
    Int32 procMsg(FdInfo* info, MsgHdr* msg); 
    
    void closePoll(struct FdInfo* info);
    void closeDealer(struct FdInfo*);
    

private:
    static const Int32 DEF_LOCK_ORDER = 3;
    const Int32 m_capacity;

    I_FdObj* m_obj;
    Lock* m_lock;
    SockPoll* m_poll;
    SockDealer* m_dealer;
};

#endif

