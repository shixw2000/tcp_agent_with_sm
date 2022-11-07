#include<unistd.h>
#include<sys/syscall.h>
#include<sys/types.h>
#include<signal.h>
#include<pthread.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>
#include"globaltype.h"
#include"cthread.h"


struct CThread::_intern {
    pthread_t m_thr;
    char m_name[32];
};

CThread::CThread() {
    m_intern = NULL;
    m_service = NULL;
}

CThread::~CThread() {
    if (NULL != m_intern) {
        delete m_intern;
    }
}

unsigned long CThread::getThr() const {
    if (NULL != m_intern) {
        return m_intern->m_thr;
    } else {
        return 0;
    }
}

int CThread::start(const char name[], I_Service* service) {
    int ret = 0;
    pthread_attr_t attr;

    m_intern = new struct _intern;
    memset(m_intern, 0, sizeof(*m_intern));
    
    if (NULL != name) {
        strncpy(m_intern->m_name, name, sizeof(m_intern->m_name));
    }

    if (NULL != service) {
        m_service = service;
    }

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    ret = pthread_create(&m_intern->m_thr, &attr, &CThread::activate, this);
    if (0 == ret) {        
        LOG_DEBUG("pthread_create| thrid=0x%lx| name=%s|", 
            m_intern->m_thr, m_intern->m_name);
    } else {
        LOG_ERROR("pthread_create| ret=%d| error=%s|", ret, ERR_MSG());

        ret =  -1;
    }
    
    pthread_attr_destroy(&attr); 
    return ret;
}

void CThread::join() {
    if (NULL != m_intern && 0 != m_intern->m_thr) { 
        pthread_join(m_intern->m_thr, NULL);
        m_intern->m_thr = 0;
    }
}

void* CThread::activate(void* arg) {
    int ret = 0;
    CThread* pthr = (CThread*)arg;
    
    ret = pthr->run(); 

    LOG_INFO("exit| ret=%d| thrid=0x%lx| name=%s|", 
        ret, pthr->m_intern->m_thr, pthr->m_intern->m_name); 
    
    return (void*)(Uint64)ret;
}

int getTid() {
    int tid = 0;

    tid = syscall(SYS_gettid);
    return tid;
}

void sleepSec(int sec) {
    sleep(sec);
}

void maskSig(int sig) {
    sigset_t sets;

    sigemptyset(&sets);
    sigaddset(&sets, sig);

    pthread_sigmask(SIG_BLOCK, &sets, NULL);
}

void armSig(int sig, void (*fn)(int)) {
    struct sigaction act;

    memset(&act, 0, sizeof(act));
    sigemptyset(&act.sa_mask);

    act.sa_flags = SA_RESTART;
    act.sa_handler = fn;

    sigaction(sig, &act, NULL);
}

void getRand(Void* buf, Int32 len) {
    static Uint64 g_rand = 0;
    int fd = -1;
    int cnt = 0;

    if (0 < len) {
        fd = open("/dev/urandom", O_RDONLY|O_NONBLOCK);
        if (0 <= fd) {
            cnt = read(fd, buf, len);
            close(fd);
        }

        if (cnt != len) {
            --g_rand;
            
            memcpy(buf, &g_rand, len);
        }
    }
    
    return;
}

