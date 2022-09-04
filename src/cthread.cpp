#include<unistd.h>
#include<pthread.h>
#include<signal.h>
#include<sys/types.h>
#include"globaltype.h"
#include"cthread.h"
#include"sockutil.h"


CThread::CThread() {
    m_thr = 0;
    m_isRun = FALSE;
    memset(m_name, 0, sizeof(m_name));
}

CThread::~CThread() {
}

int CThread::start(const char name[]) {
    int ret = 0;
    pthread_t thr;
    pthread_attr_t attr;

    if (NULL != name) {
        strncpy(m_name, name, 30);
    }
    
    if (0 < m_thr) {
        LOG_ERROR("pthread_start| m_thr=0x%llx|"
            " msg=thread is already running|", m_thr);
        return -1;
    }

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    m_isRun = TRUE;
    ret = pthread_create(&thr, &attr, &CThread::activate, this);
    if (0 == ret) {
        m_thr = thr;
        
        LOG_DEBUG("pthread_create| thrid=0x%llx| name=%s|", m_thr, m_name);
    } else {
        LOG_ERROR("pthread_create| ret=%d| error=%s|", ret, ERR_MSG());

        m_isRun = FALSE;
        ret =  -1;
    }
    
    pthread_attr_destroy(&attr); 
    return ret;
}

void CThread::stop() {
    m_isRun = FALSE;
}

void CThread::join() {
    if (0 < m_thr) { 
        pthread_join(m_thr, NULL);
        m_thr = 0;
    }
}

void* CThread::activate(void* arg) {
    Int32 ret = 0;
    CThread* pthr = NULL;

    pthr = (CThread*)arg; 
    
    ret = pthr->run(); 

    LOG_INFO("exit| ret=%d| thrid=0x%llx| name=%s|", 
        ret, pthr->m_thr, pthr->m_name); 
    
    return NULL;
}


CService::CService() {
    m_worker = NULL;
}

Void CService::set(I_Worker* worker) {
    m_worker = worker;
}

Int32 CService::run() { 
    Int32 ret = 0;
    
    while (isRun() && 0 == ret) { 
        ret = m_worker->work();
    }
    
    return ret;
}


EventService::EventService() {
    m_atom = 0;
    m_event_fd = creatEventFd();
}

EventService::~EventService() {
}

Int32 EventService::run() { 
    Int32 ret = 0;
    
    while (isRun() && 0 == ret) { 
        ret = doService();
    }
    
    return ret;
}

Void EventService::stop() {    
    CThread::stop(); 

    alarm();
}

Void EventService::signal() {
    Uint32 val = 0;

    val = ATOMIC_FETCH_INC(&m_atom);
    if (0 == val) { 
        this->alarm(); 
    }
}

Int32 EventService::doService() {
    Int32 ret = 0;
    Bool bOk = TRUE;
    Uint32 old = 0; 
    
    old = ACCESS_ONCE(m_atom); 
    
    ret = consume(); 
    if (0 == ret) {
        bOk = CAS(&m_atom, old, 0);
        if (bOk) {
            this->pause(DEF_POLL_WAIT_MILLISEC);
        }

        return 0;
    } else {
        return ret;
    }
}

void EventService::alarm() {
    writeEvent(m_event_fd); 
}
 
void EventService::pause(Int32 ms) {
    waitEvent(m_event_fd, ms);
}


