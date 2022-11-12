#ifndef __SOCKPOLL_H__
#define __SOCKPOLL_H__
#include"globaltype.h"
#include"taskpool.h"
#include"listnode.h"
#include"interfobj.h"
#include"datatype.h"


class SockMng;
class TickTimer;

class SockPoll : public TaskThread, public I_TimerDealer {
public:
    explicit SockPoll(int capacity);
    ~SockPoll();

    virtual int init();
    virtual void finish();

    Void set(SockMng* mng);

    FdInfo* creatFd(Int32 fd, Bool testRd, Bool testWr);
    
    virtual void procTaskEnd(struct Task* task);
    virtual unsigned int procTask(struct Task* task);

    virtual Void doTimeout(struct TimerEle* ele);
    Void doTick(Uint32 cnt);
    void addTimer(struct TimerEle* ele, Int32 type, Uint32 interval);
    
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

    Void flashTimeout();
    Void addFlash(FdInfo* info);
    Void updateRdFlash(FdInfo* info);
    Void updateWrFlash(FdInfo* info);
    Void delFlash(FdInfo* info);

    Int32 creatTimer();
    Int32 creatEvent();


private: 
    const Int32 m_capacity;
    
    struct pollfd* m_fds;
    struct FdInfo* m_infos;
    TickTimer* m_timer;
    SockMng* m_mng;
    FdInfo* m_event_fd;
    FdInfo* m_timer_fd;
    
    list_head m_rd_root;
    list_head m_run_root; 
    list_head m_flash_timeout_que;
    
    struct TimerEle m_chk_flash_ele; 
};

#endif

