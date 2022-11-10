#include"sockpot.h"
#include"sockutil.h"
#include"sockmng.h"
#include"sockcenter.h"
#include"datatype.h"
#include"msgtype.h"
#include"msgcenter.h"
#include"ticktimer.h"
#include"cthread.h"


static Uint32 g_tick_time = 0;
static Uint32 g_tick_cnt = 0;
static Uint32 g_mono_time = 0;
static Uint64 g_epoch_time = 0;

Uint32 tick() {
    return g_tick_time;
}

Uint32 tick_cnt() {
    return g_tick_cnt;
}

Uint32 now() {
    return g_mono_time;
}

int EventHandler::readFd(struct FdInfo* info) {
    Uint32 val = 0;

    readEvent(info->m_fd, &val); 
    return 0;
}

int EventHandler::writeFd(struct FdInfo*) {
    return -1;
}

Int32 EventHandler::procMsg(FdInfo* info, MsgHdr* msg) {
    Int32 ret = 0;
    EventData* ev = (EventData*)info->m_data;

    if (ENUM_MSG_SYSTEM_IO_END == msg->m_cmd) { 
        ret = procEnd(ev, msg);
    } else {
        MsgCenter::free(msg);
    } 
    
    return ret;
}

Int32 EventHandler::procEnd(EventData* ev, MsgHdr* msg) {
    ev->m_fdinfo->m_closing = TRUE;
    
    m_mng->notify(ev->m_fdinfo, ENUM_MSG_SYSTEM_EXIT);

    MsgCenter::free(msg);
    return 0;
}

Void EventHandler::eof(struct FdInfo* info) {    
    LOG_INFO("event_exit| fd=%d| msg=exit now|", info->m_fd);
    
    /* if  fail, then exit */
    m_mng->stop();
}

EventData* EventHandler::setup() {
    int fd = -1;
    FdInfo* info = NULL;
    EventData* ev = NULL;
    
    fd = creatEventFd(); 
    if (0 <= fd) { 
        info = m_mng->creatBase(fd, TRUE, FALSE); 
        ev = FdObjFactory::newEventData();
        ev->m_base.m_node_type = getType();

        ev->m_fdinfo = info;
        m_mng->addEvent(info, &ev->m_base);

        LOG_INFO("++++add_event_data| fd=%d| msg=ok|", fd);

        return ev;
    } else {
        LOG_ERROR("****add_event_data| msg=error|");
        
        return NULL;
    }
}


int TimerHandler::readFd(struct FdInfo* info) {
    Int32 ret = 0;
    Uint32 val = 0;

    g_mono_time = getMonoTime();
    
    ret = readEvent(info->m_fd, &val); 
    if (0 == ret) { 
        m_mng->notify(info, ENUM_MSG_NOTIFY_TICK_TIMER, val);
    }
    
    return 0;
}

int TimerHandler::writeFd(struct FdInfo*) {
    return -1;
}

Int32 TimerHandler::procMsg(FdInfo* info, MsgHdr* msg) {
    Int32 ret = 0;
    TimerData* timer = (TimerData*)info->m_data;

    if (ENUM_MSG_NOTIFY_TICK_TIMER == msg->m_cmd) {
        ret = procTickTimer(timer, msg);
    } else if (ENUM_MSG_SYSTEM_IO_END == msg->m_cmd) { 
        ret = procEnd(timer, msg);
    } else {
        MsgCenter::free(msg);
    } 
    
    return ret;
}

Int32 TimerHandler::procTickTimer(TimerData* timer, MsgHdr* msg) {
    Uint32 cnt = 0;
    MsgNotify* req = NULL;

    req = MsgCenter::cast<MsgNotify>(msg);
    cnt = (Uint32)req->m_data; 
    
    timer->m_timer->tick(cnt);

    g_tick_time = timer->m_timer->now();
    g_tick_cnt = timer->m_timer->monoTick();
    
    if (1 < cnt) {
        LOG_INFO("tick_timer| req_tick=%u|", cnt);
    } else {
        LOG_DEBUG("tick_timer");
    }

    MsgCenter::free(msg);
    return 0;
}

Int32 TimerHandler::procEnd(TimerData* timer, MsgHdr* msg) {
    timer->m_fdinfo->m_closing = TRUE;
    
    m_mng->notify(timer->m_fdinfo, ENUM_MSG_SYSTEM_EXIT);

    MsgCenter::free(msg);
    return 0;
}

Void TimerHandler::eof(struct FdInfo* info) {    
    LOG_INFO("event_exit| fd=%d| msg=exit now|", info->m_fd);
    
    /* if  fail, then exit */
    m_mng->stop();
}

TimerData* TimerHandler::setup() {
    int fd = -1;
    FdInfo* info = NULL;
    TimerData* timer = NULL; 
    
    fd = creatTimerFd(1000); 
    if (0 <= fd) { 
        info = m_mng->creatBase(fd, TRUE, FALSE); 
        timer = FdObjFactory::newTimerData();
        timer->m_base.m_node_type = getType();

        timer->m_timer->setDealer(m_center);

        timer->m_fdinfo = info;
        m_mng->addEvent(info, &timer->m_base);

        LOG_INFO("++++add_timer| fd=%d| msg=ok|", fd);

        g_tick_time = 0;
        g_mono_time = getMonoTime();
        g_epoch_time = getSysTime(); 
        
        LOG_INFO("prog_timer| sys_time=%llu.%llu|"
            " mono_time=%u| tick_time=%u| msg=ok|",
            to_sec(g_epoch_time), to_msec(g_epoch_time),
            g_mono_time, g_tick_time);

        return timer;
    } else {
        LOG_ERROR("****add_timer| msg=error|");
        
        return NULL;
    }
}

