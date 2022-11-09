#ifndef __SOCKPOT_H__
#define __SOCKPOT_H__
#include"globaltype.h"
#include"interfobj.h"
#include"datatype.h"


struct MsgHdr;
class SockMng;
class SockCenter;

class EventHandler : public I_FdObj {
public:
    EventHandler(SockMng* mng, SockCenter* center)
        : m_mng(mng), m_center(center) {}

    virtual Int32 getType() const {
        return ENUM_NODE_EVENT;
    }
    
    virtual int readFd(struct FdInfo* info);
    virtual int writeFd(struct FdInfo* info);
    virtual Int32 procMsg(FdInfo* info, MsgHdr* msg);
    virtual Void eof(struct FdInfo* info);

    EventData* setup();

private:
    Int32 procEnd(EventData* ev, MsgHdr* msg);

private:
    SockMng* m_mng;
    SockCenter* m_center;
};

class TimerHandler : public I_FdObj {
public:
    TimerHandler(SockMng* mng, SockCenter* center)
        : m_mng(mng), m_center(center) {}

    virtual Int32 getType() const {
        return ENUM_NODE_TIMER;
    }
    
    virtual int readFd(struct FdInfo* info);
    virtual int writeFd(struct FdInfo* info);
    virtual Int32 procMsg(FdInfo* info, MsgHdr* msg);
    virtual Void eof(struct FdInfo* info);

    TimerData* setup();

private:
    Int32 procTickTimer(TimerData* timer, MsgHdr* msg);
    Int32 procEnd(TimerData* timer, MsgHdr* msg);

private:
    SockMng* m_mng;
    SockCenter* m_center;
};

#endif

