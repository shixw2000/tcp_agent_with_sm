#include<string.h>
#include"server.h"
#include"config.h"
#include"sockutil.h"
#include"managecenter.h"
#include"datatype.h"
#include"cthread.h"
#include"sockdealer.h"


Server::Server() {
    m_parser = NULL;
    m_mng = NULL;
    
    m_dealer = NULL;
    m_deal_svc = NULL; 
    
    m_path = NULL; 
}

Server::~Server() {
}

Void Server::set(const Char* path) {
    m_path = path;
}

Int32 Server::init() {
    Int32 ret = 0;

    do {
        initLib();

        I_NEW(Parser, m_parser);
        ret = m_parser->init(m_path);
        if (0 != ret) {
            break;
        } 

        I_NEW(ManageCenter, m_mng);
        ret = m_mng->init();
        if (0 != ret) {
            break;
        }

        I_NEW_1(SockDealer, m_dealer, m_mng);
        ret = m_dealer->init();
        if (0 != ret) {
            break;
        }

        m_mng->set(m_dealer);

        I_NEW(CService, m_deal_svc);
        m_deal_svc->set(m_dealer);

        return 0;
    } while (0);

    return ret;
}

Void Server::finish() {
    if (NULL != m_deal_svc) {
        I_FREE(m_deal_svc);
    }

    if (NULL != m_dealer) {
        m_dealer->finish();
        I_FREE(m_dealer);
    }
    
    if (NULL != m_mng) {
        m_mng->finish();
        I_FREE(m_mng);
    } 

    if (NULL != m_parser) {
        m_parser->finish();
        I_FREE(m_parser);
    }
}

Int32 Server::startServer() {
    Int32 ret = 0;
    list_node* pos = NULL;
    TcpPairs* pairs = NULL; 
    Config* config = NULL;

    config = m_parser->getConf();
    
    list_for_each(pos, &config->m_tcp_pair_list) {        
        pairs = (TcpPairs*)pos;

        ret = m_mng->addListener(pairs);
        if (0 != ret) {
            return ret;
        }
    }

    ret = m_deal_svc->start("msg_dealer");
    if (0 != ret) {
        return ret;
    }

    ret = m_mng->start();
    if (0 != ret) {
        return ret;
    }

    return 0;
}

Void Server::stopServer() {
    m_dealer->stopWork();
    
    m_mng->stop();
    m_deal_svc->stop();
}

Void Server::wait() {
    m_mng->wait();

    m_deal_svc->join();
}

