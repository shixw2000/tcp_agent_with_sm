#include"sockdealer.h"
#include"datatype.h"
#include"sockmng.h"
#include"ticktimer.h"


SockDealer::SockDealer() {
    m_mng = NULL;
    m_timer = NULL;
    INIT_TIMER_ELE(&m_minute_ele);
    INIT_TIMER_ELE(&m_hour_ele);
}

SockDealer::~SockDealer() {
}

Int32 SockDealer::init() { 
    Int32 ret = 0;

    ret = TaskPool::init();
    if (0 != ret) {
        return ret;
    }

    I_NEW(TickTimer, m_timer);
    m_timer->setDealer(this);
    
    return ret;
}

Void SockDealer::finish() { 
    if (NULL != m_timer) {
        I_FREE(m_timer);
    }
    
    TaskPool::finish();
}

Void SockDealer::set(SockMng* mng) {
    m_mng = mng;
}

Void SockDealer::closeTask(FdInfo* info) {
    info->m_deal_err = TRUE;
    m_mng->closeMng(info);
}

unsigned int SockDealer::procTask(struct Task* task) {
    FdInfo* info = NULL;

    info = list_entry(task, FdInfo, m_deal_task);
    
    m_mng->dealMsg(info);
    return BIT_EVENT_NORM; 
}

void SockDealer::procTaskEnd(struct Task* task) {
    FdInfo* info = NULL;

    info = list_entry(task, FdInfo, m_deal_task);

    /* del timer for heartbeat */
    stopHeartbeat(info);
    
    m_mng->endFd(info);
    return;
}

int SockDealer::setup() {
    Int32 ret = 0;

    ret = TaskPool::setup();
    if (0 != ret) {
        return ret;
    }
    
    /* add a minutely timer */
    addTimer(&m_minute_ele, ENUM_TIMER_TYPE_MINUTELY,
        DEF_MINUTE_TICK_CNT);

    /* add a hourly timer */
    addTimer(&m_hour_ele, ENUM_TIMER_TYPE_HOURLY,
        ENUM_TIMER_TYPE_HOURLY);

    return 0;
}

void SockDealer::teardown() {
    if (NULL != m_timer) {
        m_timer->stop();
    }

    TaskPool::teardown();
}

Void SockDealer::doTimeout(struct TimerEle* ele) {
    if (ENUM_TIMER_HEAR_BEAT == ele->m_type) {
        FdInfo* info = list_entry(ele, FdInfo, m_heartbeat_timer);

        m_mng->sendHeartBeat(info);
        updateTimer(ele);
    } else if (ENUM_TIMER_TYPE_MINUTELY == ele->m_type) {
        LOG_DEBUG("minutely_task| msg=ok|");

        updateTimer(ele);
    } else if (ENUM_TIMER_CHK_LOGIN == ele->m_type) {
        FdInfo* info = list_entry(ele, FdInfo, m_heartbeat_timer);

        LOG_INFO("chk_login_timeout| fd=%d| type=%d|"
            " interval=%u| msg=timeout and close|",
            info->m_fd, info->m_fd_type, DEF_CHK_LOGIN_INTERVAL);

        /* donot restart timer */
        closeTask(info); 
    } else if (ENUM_TIMER_TYPE_HOURLY== ele->m_type) {
        updateTimer(ele);
    } else {
    }
}

Void SockDealer::doTick(Uint32 cnt) {
    m_timer->tick(cnt); 
}

void SockDealer::addTimer(struct TimerEle* ele, 
    Int32 type, Uint32 interval) {
    ele->m_type = type;
    ele->m_interval = interval;
    
    m_timer->addTimer(ele);
}

Void SockDealer::stopHeartbeat(FdInfo* info) {
    if (ENUM_NODE_SOCK_FLASH_MIN < info->m_fd_type 
        && ENUM_NODE_SOCK_FLASH_MAX > info->m_fd_type) {

        /* del timer for heartbeat */
        delTimer(&info->m_heartbeat_timer);
    }
}

