#ifndef __TASKSERVICE_H__
#define __TASKSERVICE_H__
#include"taskpool.h"


class TestTasker : public TaskPool { 
public: 
    TestTasker(int max_cnt, int m_prnt_mask, int type);

    void setNext(TaskPool* next) {
        m_next = next;
    } 

    void check();
    void wait();

    void preTasks();
    
    unsigned int procTask(struct Task* task);
    void procTaskEnd(struct Task* task);

private:
    const int m_max_cnt;
    const int m_prnt_mask;
    TaskPool* m_next;

    struct Task m_task;
    int m_do_cnt;
    int m_proc_cnt;
    int m_wait_cnt;
    int m_check_cnt;
};


#endif

