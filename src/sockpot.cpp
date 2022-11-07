#include"sockpot.h"
#include"sockutil.h"
#include"sockmng.h"
#include"sockcenter.h"
#include"datatype.h"
#include"msgtype.h"
#include"msgcenter.h"


int SockEvent::readFd(struct FdInfo* info) {
    Uint32 val = 0;

    readEvent(info->m_fd, &val); 
    return 0;
}

int SockEvent::writeFd(struct FdInfo*) {
    return -1;
}

Int32 SockEvent::procMsg(FdInfo* info, MsgHdr* msg) {
    Int32 ret = 0;
    EventData* ev = (EventData*)info->m_data;

    if (ENUM_MSG_SYSTEM_IO_END == msg->m_cmd) { 
        ret = procEnd(ev, msg);
    } else {
        MsgCenter::free(msg);
    } 
    
    return ret;
}

Int32 SockEvent::procEnd(EventData* ev, MsgHdr* msg) {
    ev->m_fdinfo->m_closing = TRUE;
    
    m_mng->notify(ev->m_fdinfo, ENUM_MSG_SYSTEM_EXIT);

    MsgCenter::free(msg);
    return 0;
}

Void SockEvent::eof(struct FdInfo* info) {    
    LOG_INFO("event_exit| fd=%d| msg=exit now|", info->m_fd);
    
    /* if  fail, then exit */
    m_mng->stop();
}

