#ifndef __CTHREAD_H__
#define __CTHREAD_H__
#include"globaltype.h"


class CThread {
public:
    CThread();
    virtual ~CThread();

    virtual int start(const char name[]);
    virtual void stop();
    
    void join(); 

    Bool isRun() const {
        return m_isRun;
    }

protected:
    virtual Int32 run() = 0;

private:
    static void* activate(void* arg);
    
private:
    Uint64 m_thr;
    Bool m_isRun;
    char m_name[32];
};


class CService;
class I_Worker {
public:
    virtual ~I_Worker() {}

    virtual Int32 work() = 0;
};

class CService : public CThread {
public:
    CService();

    Void set(I_Worker* worker); 

private:
    virtual int run();  

private:
    I_Worker* m_worker;
};


class EventService : public CThread {
public:
    EventService();
    virtual ~EventService();
    
    virtual void stop();
    
    void signal(); 
    
protected:     
    virtual void alarm(); 
    virtual void pause(Int32 ms);

    virtual Int32 consume() = 0;

private:
    virtual int run();
    int doService();

private:
    Uint32 m_atom;
    Int32 m_event_fd;
};


#endif

