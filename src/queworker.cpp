#include"queworker.h"
#include"msgcenter.h"
#include"msgtype.h"


QueWorker::QueWorker() {
    m_running = FALSE;
    INIT_LIST_HEAD(&m_queue);

    m_in_cnt = 0;
    m_throw_cnt = 0;
    m_free_cnt = 0;
    m_deal_cnt = 0;
}

Int32 QueWorker::start() {
    m_running = TRUE;
    return 0;
}

Void QueWorker::stop() {
    m_running = FALSE;
    consume();
}

Bool QueWorker::notify(MsgHdr* msg) {    
    if (NULL != msg) {
        if (m_running) {
            ++m_in_cnt;
            MsgCenter::notify(msg, &m_queue);

            return TRUE;
        } else {
            ++m_throw_cnt;
            MsgCenter::free(msg);

            return FALSE;
        }
    } else {
        return FALSE;
    }
}

Bool QueWorker::doWork() {
    list_node* node = NULL;
    MsgHdr* msg = NULL;

    if (!list_empty(&m_queue)) {
        node = LIST_FIRST(&m_queue);

        list_del(node, &m_queue);
        
        msg = MsgCenter::node2msg(node);
    }

    if (NULL != msg) { 
        if (m_running) {
            ++m_deal_cnt;
            dealMsg(msg);
        } else {
            ++m_free_cnt;
            MsgCenter::free(msg);
        }

        return TRUE;
    } else {
        return FALSE;
    }
}

Void QueWorker::consume() {
    Bool done = TRUE;

    do {
        done = doWork();
    } while (done);
}

