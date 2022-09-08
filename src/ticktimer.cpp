#include<string.h>
#include"ticktimer.h"


struct TimerEle {
    hlist_node m_node;
    Uint32 m_expires;
    TimerCb m_pf;
    Void* m_p1;
    Void* m_p2;
};

TickTimer::TickTimer() {
    m_size = 0;
    m_tick = 0;
    m_time = 0;

    memset(m_tv1, 0, sizeof(m_tv1));
    memset(m_tv2, 0, sizeof(m_tv2));
}

TickTimer::~TickTimer() {
}

Void TickTimer::stop() {
    TimerEle* ele = NULL;
    hlist_node* pos = NULL;
    hlist_node* n = NULL; 
    
    for (int i=0; i<TV_1_SIZE; ++i) {
        hlist_for_each_entry_safe(ele, pos, n, &m_tv1[i], m_node) {
            hlist_del(pos);

            I_FREE(ele);
        }
    }

    for (int i=0; i<TV_2_SIZE; ++i) {
        hlist_for_each_entry_safe(ele, pos, n, &m_tv2[i], m_node) {
            hlist_del(pos);

            I_FREE(ele);
        }
    }
}

Void* TickTimer::creatTimer(TimerCb pf, Void* p1, Void* p2) { 
    TimerEle* ele = NULL;

    I_NEW(TimerEle, ele);
    
    if (NULL != ele) {
        memset(ele, 0, sizeof(*ele));

        INIT_HLIST_NODE(&ele->m_node);
        ele->m_pf = pf;
        ele->m_p1 = p1;
        ele->m_p2 = p2;
    }

    return ele;
}

Void TickTimer::delTimer(Void* timer) {
    TimerEle* ele = reinterpret_cast<TimerEle*>(timer);
    
    stopTimer(timer);

    I_FREE(ele);
}

Void TickTimer::resetTimer(Void* timer, Uint32 timeout) {
    stopTimer(timer);
    startTimer(timer, timeout);
}

Void TickTimer::stopTimer(Void* timer) {
    TimerEle* ele = reinterpret_cast<TimerEle*>(timer);
    
    if (!hlist_unhashed(&ele->m_node)) {
        hlist_del(&ele->m_node);

        --m_size;
    }
}

Void TickTimer::startTimer(Void* timer, Uint32 tick) {
    TimerEle* ele = reinterpret_cast<TimerEle*>(timer);
    Int32 index = 0;
    Uint32 timeout = tick + m_tick;
    
    if (TV_1_SIZE > tick) { 
        ele->m_expires = 0;
        index = timeout & TV_1_MASK;
        hlist_add(&ele->m_node, &m_tv1[index]);
    } else { 
        ele->m_expires = timeout & TV_1_MASK;
        index = (timeout >> TV_1_BITS) & TV_2_MASK; 
        hlist_add(&ele->m_node, &m_tv2[index]);
    }

    ++m_size;
}

void TickTimer::tick(Uint32 cnt) {
    Int32 i1 = 0;
    Int32 i2 = 0;
    Int32 next = 0;
    hlist_head list;
    TimerEle* ele = NULL;
    hlist_node* pos = NULL;
    hlist_node* n = NULL; 

    ++m_tick;
    m_time += cnt;

    INIT_HLIST_HEAD(&list);
    
    i1 = m_tick & TV_1_MASK;
    
    if (0 == i1) {
        i2 = (m_tick >> TV_1_BITS) & TV_2_MASK;

        if (!hlist_empty(&m_tv2[i2])) { 
            hlist_for_each_entry_safe(ele, pos, n, &m_tv2[i2], m_node) {
                hlist_del(pos);
                
                next = (m_tick + ele->m_expires) & TV_1_MASK;
                hlist_add(pos, &m_tv1[next]);
            }
        }
    } 

    if (!hlist_empty(&m_tv1[i1])) {
        hlist_move(&m_tv1[i1], &list); 
    
        hlist_for_each_entry_safe(ele, pos, n, &list, m_node) {
            hlist_del(pos); 
            --m_size;
            
            doTimer(ele); 
        }
    }
}

Void TickTimer::doTimer(Void* data) {
    TimerEle *ele = (TimerEle*)data;
    
    ele->m_pf(ele->m_p1, ele->m_p2);
}


