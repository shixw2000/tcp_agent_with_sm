#include"globaltype.h"
#include"taskservice.h"
#include"cthread.h"


TestTasker::TestTasker(int max_cnt,  int prnt_mask, int type) 
    : TaskPool(type), m_max_cnt(max_cnt), m_prnt_mask(prnt_mask) {
    m_next = NULL;
    
    INIT_TASK(&m_task);
    
    m_do_cnt = 0;
    m_proc_cnt = 0;
    m_wait_cnt = 0;
    m_check_cnt = 0;
}
 
void TestTasker::check() {
    ++m_check_cnt;
    
    if (0 == (m_check_cnt & m_prnt_mask)) {
        LOG_INFO("<<<do_check| tid=%d| cnt=%d|", getTid(), m_check_cnt);
    }

    TaskPool::check();
}

void TestTasker::wait() {
    ++m_wait_cnt;
        
    if (0 == (m_wait_cnt & m_prnt_mask)) {
        LOG_INFO(">>>do_wait| tid=%d| cnt=%d|", getTid(), m_wait_cnt);
    }

    TaskPool::wait(); 
}

void TestTasker::preTasks() {
    ++m_do_cnt;
    
    if (1 != m_do_cnt && m_max_cnt != m_do_cnt) {
        
    } else {
        LOG_INFO("===consume| tid=%d| cnt=%d|", getTid(), m_do_cnt);
    }
}

unsigned int TestTasker::procTask(struct Task* task) {
    ++m_proc_cnt;
    
    if (1 < m_proc_cnt && m_max_cnt > m_proc_cnt) {
        m_next->addTask(&m_task, BIT_EVENT_NORM);
        
    } else if (1 == m_proc_cnt) {
        LOG_INFO("^^proc_task| tid=%d| cnt=%d|", getTid(), m_proc_cnt);

        m_next->addTask(&m_task, BIT_EVENT_NORM);
    } else {
        LOG_INFO("^^proc_task| tid=%d| cnt=%d|", getTid(), m_proc_cnt);

        m_next->endTask(&m_task);
    }

    return BIT_EVENT_NORM;
}

void TestTasker::procTaskEnd(struct Task* task) {
    LOG_INFO("***proc_task_end| tid=%d|", getTid());
}


