#include"sockdealer.h"
#include"datatype.h"
#include"sockmng.h"


SockDealer::SockDealer(SockMng* mng): m_mng(mng) {
}

SockDealer::~SockDealer() {
}

Int32 SockDealer::init() { 
    Int32 ret = 0;

    ret = TaskPool::init();
    if (0 != ret) {
        return ret;
    }
    
    return ret;
}

Void SockDealer::finish() { 
    TaskPool::finish();
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

    m_mng->endFd(info);
    return;
}

