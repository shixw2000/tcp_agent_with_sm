#ifndef __SOCKPOLL_H__
#define __SOCKPOLL_H__
#include"globaltype.h"
#include"taskpool.h"
#include"listnode.h"


struct pollfd;
struct FdInfo;
class SockMng;

class SockPoll : public TaskThread {
public:
    SockPoll(int capacity, SockMng* mng);
    ~SockPoll();

    virtual int init();
    virtual void finish();

    FdInfo* creatFd(Int32 fd, Bool testRd, Bool testWr);
    
    virtual void procTaskEnd(struct Task* task);
    virtual unsigned int procTask(struct Task* task);
    
protected: 
    virtual int setup();
    virtual void teardown();

    virtual void check();
    virtual void wait();
    virtual void alarm();

    virtual void preTasks();

    Void resetFd(FdInfo* info);

private:
    Int32 fillEvent();
    Int32 waitEvent(Int32 cnt, Int32 timeout); 
    Int32 pollEvent(int timeout);

    void closeTask(FdInfo* info);

    Int32 writeMsg(FdInfo* info);
    Int32 readMsg(FdInfo* info);

private: 
    const Int32 m_capacity;
    
    struct pollfd* m_fds;
    struct FdInfo* m_infos;
    SockMng* m_mng;
    
    list_head m_rd_root;
    list_head m_run_root; 

    int m_event_fd;
};

#endif

