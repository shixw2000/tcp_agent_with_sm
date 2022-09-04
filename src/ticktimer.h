#ifndef __TICKTIMER_H__
#define __TICKTIMER_H__
#include"globaltype.h"
#include"listnode.h"


#define TV_1_BITS 8
#define TV_1_SIZE (1 << TV_1_BITS)
#define TV_1_MASK (TV_1_SIZE-1)
#define TV_2_BITS 8
#define TV_2_SIZE (1 << TV_2_BITS)
#define TV_2_MASK (TV_2_SIZE-1)


class TickTimer { 
public:
    TickTimer();
    virtual ~TickTimer(); 

    virtual Int32 start();
    virtual Void stop();
    
    inline Uint32 monoTick() const {
        return m_tick;
    }

    inline Uint32 now() const {
        return m_time;
    }

    Void* creatTimer(TimerCb func, Void* p1, Void* p2);

    Void delTimer(Void* timer);

    Void resetTimer(Void* timer, Uint32 timeout);
    
    Void stopTimer(Void* timer);

    void startTimer(Void* timer, Uint32 timeout);

    void tick(Uint32 cnt);

private:
    void doTimer(Void*);

private:
    Uint32 m_size;
    Uint32 m_tick;
    Uint32 m_time;
    hlist_head m_tv1[TV_1_SIZE];
    hlist_head m_tv2[TV_2_SIZE];
};

#endif

