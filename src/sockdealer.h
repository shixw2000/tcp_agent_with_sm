#ifndef __SOCKDEALER_H__
#define __SOCKDEALER_H__
#include"globaltype.h"
#include"taskpool.h"


struct FdInfo;
class SockMng;

class SockDealer : public TaskPool { 
public:
    explicit SockDealer(SockMng* mng);
    ~SockDealer();

    Int32 init();
    Void finish(); 

    virtual void procTaskEnd(struct Task* task);
    virtual unsigned int procTask(struct Task* task); 

private:     
    SockMng* m_mng;
};

#endif 

