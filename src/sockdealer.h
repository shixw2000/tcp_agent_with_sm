#ifndef __SOCKDEALER_H__
#define __SOCKDEALER_H__
#include"globaltype.h"
#include"taskpool.h"
#include"datatype.h"
#include"interfobj.h"


struct FdInfo;
class SockMng;
class TickTimer;

class SockDealer : public TaskPool, public I_TimerDealer { 
public:
    SockDealer();
    ~SockDealer();

    Int32 init();
    Void finish(); 

    Void set(SockMng* mng);

    virtual void procTaskEnd(struct Task* task);
    virtual unsigned int procTask(struct Task* task); 

    virtual Void doTimeout(struct TimerEle* ele);
    Void doTick(Uint32 cnt);
    void addTimer(struct TimerEle* ele, Int32 type, Uint32 interval);

protected: 
    virtual int setup();
    virtual void teardown();

private:     
    SockMng* m_mng;
    TickTimer* m_timer;
    struct TimerEle m_minute_ele;
    struct TimerEle m_hour_ele;
};

#endif 

