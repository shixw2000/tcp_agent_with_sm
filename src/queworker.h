#ifndef __QUEWORKER_H__
#define __QUEWORKER_H__
#include"globaltype.h"
#include"listnode.h"


struct MsgHdr;

class QueWorker {
public:
    QueWorker();
    
    virtual ~QueWorker() {}
    virtual Int32 start();
    virtual Void stop();
    
    virtual Bool notify(MsgHdr* msg);
    
    Bool doWork();

    void consume();
    
protected:
    virtual Void dealMsg(MsgHdr* msg) = 0;
    
private:
    Uint64 m_in_cnt;
    Uint64 m_throw_cnt;
    Uint64 m_free_cnt;  // total of deleted msgs
    Uint64 m_deal_cnt;  // total of handled msgs
    Bool m_running;
    list_head m_queue;
};

#endif

