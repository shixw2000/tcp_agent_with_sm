#include"sockpot.h"
#include"sockutil.h"
#include"sockmng.h"
#include"sockcenter.h"
#include"datatype.h"
#include"msgtype.h"
#include"msgcenter.h"
#include"ticktimer.h"
#include"cthread.h"


static Uint32 g_mono_time = 0;
static Uint64 g_epoch_time = 0;
static Uint64 g_sys_time = 0;


Uint32 now() {
    return g_mono_time;
}

Uint64 curr() {
    return g_sys_time;
}

void initTime() {
    g_mono_time = getMonoTime();
    g_epoch_time = getSysTime();
    g_sys_time = to_sec(g_epoch_time);
    
    LOG_INFO("prog_start| version=%s| sys_time=%llu.%llu|"
        " mono_time=%u| msg=ok|",
        DEF_BUILD_VER,
        to_sec(g_epoch_time), to_msec(g_epoch_time),
        g_mono_time);

    /* standize the epoch time */
    g_epoch_time = g_sys_time - g_mono_time;
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

    if (ENUM_MSG_SYSTEM_IO_END == msg->m_cmd) { 
        ret = procEnd(info, msg);
    } else {
        MsgCenter::free(msg);
    } 
    
    return ret;
}

Int32 EventHandler::procEnd(FdInfo* info, MsgHdr* msg) {
    info->m_closing = TRUE;
    
    m_mng->notify(info, ENUM_MSG_SYSTEM_EXIT);

    MsgCenter::free(msg);
    return 0;
}

Void EventHandler::eof(struct FdInfo* info) {    
    LOG_INFO("event_exit| fd=%d| msg=exit now|", info->m_fd);
    
    /* if  fail, then exit */
    m_mng->stop();
}


int TimerHandler::readFd(struct FdInfo* info) {
    Int32 ret = 0;
    Uint32 val = 0; 
    
    ret = readEvent(info->m_fd, &val); 
    if (0 == ret) { 
        g_mono_time = getMonoTime();
        g_sys_time = g_epoch_time + g_mono_time;
    
        /* asynchronous for dealer thread timer */
        m_mng->notify(info, ENUM_MSG_NOTIFY_TICK_TIMER, val);

        /* synchronous for io thread timer */
        m_mng->doIoTick(val);
    }
    
    return 0;
}

int TimerHandler::writeFd(struct FdInfo*) {
    return -1;
}

Int32 TimerHandler::procMsg(FdInfo* info, MsgHdr* msg) {
    Int32 ret = 0;

    if (ENUM_MSG_NOTIFY_TICK_TIMER == msg->m_cmd) {
        ret = procTickTimer(info, msg);
    } else if (ENUM_MSG_SYSTEM_IO_END == msg->m_cmd) { 
        ret = procEnd(info, msg);
    } else {
        MsgCenter::free(msg);
    } 
    
    return ret;
}

Int32 TimerHandler::procTickTimer(struct FdInfo* info, MsgHdr* msg) {
    Uint32 cnt = 0;
    MsgNotify* req = NULL;

    req = MsgCenter::cast<MsgNotify>(msg);
    cnt = (Uint32)req->m_data; 
    
    m_mng->doProcTick(cnt);
    
    if (1 < cnt) {
        LOG_INFO("tick_timer| fd=%d| req_tick=%u|", info->m_fd, cnt);
    } else {
        LOG_DEBUG("tick_timer| fd=%d|", info->m_fd);
    }

    MsgCenter::free(msg);
    return 0;
}

Int32 TimerHandler::procEnd(struct FdInfo* info, MsgHdr* msg) {
    info->m_closing = TRUE;
    
    m_mng->notify(info, ENUM_MSG_SYSTEM_EXIT);

    MsgCenter::free(msg);
    return 0;
}

Void TimerHandler::eof(struct FdInfo* info) {    
    LOG_INFO("timer_exit| fd=%d| msg=exit now|", info->m_fd);
    
    /* if  fail, then exit */
    m_mng->stop();
}


