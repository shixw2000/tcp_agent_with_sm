#include<string.h>
#include<stdlib.h>
#include<stdio.h>
#include<signal.h>
#include"testcase.h"
#include"smutil.h"
#include"msgtype.h"
#include"msgcenter.h"
#include"taskservice.h"
#include"ticktimer.h"
#include"cthread.h"
#include"sockmng.h"
#include"sockcenter.h"


Void testSm3() {
    Int32 ret = 0;
    Int32 inlen = 0;
    EvpBase evp;
    Char input[256] = "helo\r\n";
    Byte output[32] = {0};

    LOG_INFO("Enter a msg:");
    scanf("%s", input);
    inlen = strlen(input);

    evp.m_digest.digest(input, inlen, output);
    printHex("sm3_test", output, 32);
}

Void testSm4() {
    Int32 pre_len = 0;
    Int32 enc_len = 0;
    Int32 dec_len = 0;
    Int32 type = 0;
    EvpBase evp;
    Char input[128] = "helo\r\n";
    Char output1[128] = {0};
    Char output2[128] = {0};

    evp.m_cipher.setUsrKey(DEF_SM4_KEY, DEF_SM4_KEY_LEN);

    LOG_INFO("Enter a string:");
    scanf("%s", input);
    pre_len = strlen(input);
    LOG_INFO("plain_text| len=%d| msg=%s|", pre_len, input);

    LOG_INFO("choose your type[0-cbc, 1-ecb]:");
    scanf("%d", &type);

    if (0 == type) {
        enc_len = evp.m_cipher.sm4_cbc_encrypt(input, pre_len, output1); 

        dec_len = evp.m_cipher.sm4_cbc_decrypt(output1, enc_len, output2);
    } else {
        enc_len = evp.m_cipher.sm4_ecb_encrypt(input, pre_len, output1); 

        dec_len = evp.m_cipher.sm4_ecb_decrypt(output1, enc_len, output2);
    }

    printHex("enc_hex", output1, enc_len);
    
    LOG_INFO("test_sm4| type=%d| enc=%d| dec=%d| dec_txt=%s|",
        type, enc_len, dec_len, output2);
}

void test_crc() {
    MsgHdr* hdr = NULL;
    MsgSessHead* msg = NULL;    

    hdr = MsgCenter::creat<MsgSessHead>(ENUM_MSG_SESS_ARRIVAL);
    msg = MsgCenter::cast<MsgSessHead>(hdr);

    msg->m_user_id = 13432;
    msg->m_session_id = 112343432;

    MsgCenter::addCrc(hdr);

    MsgCenter::chkCrc(hdr);
    
}

Void test_hex() {
    Int32 msglen = 0;
    Int32 inlen = 0;
    Int32 outlen = 0;
    Char msg[256] = {0};
    Char hex[256] = {0};
    Char output[256] = {0};

    LOG_INFO("Enter a msg:");
    scanf("%s", msg);

    msglen = strlen(msg);
    printHex("enc_hex", msg, msglen);

    LOG_INFO("Enter a hex:");
    scanf("%s", hex);
    
    inlen = strlen(hex);
    outlen = hex2Bin(hex, inlen, output);
    LOG_INFO("hex2bin| inlen=%d| outlen=%d| output=%s|",
        inlen, outlen, output);
    
}

void test_sig() {
    const int cnt = 3;
    int ret = 0;
    int max = 10000000;
    int prnt_mask = (1<<20)-1;
    struct Task tasks[cnt];
    TestTasker* tester[cnt] = {NULL};
    
    for (int i=0; i<cnt; ++i) {
        INIT_TASK(&tasks[i]);
        
        tester[i] = new TestTasker(max, prnt_mask, 0);

        ret = tester[i]->init();
        if (0 != ret) {
            return;
        } 
    }

    for (int i=0; i<cnt; ++i) {
        tester[i]->setNext(tester[(i+1)%cnt]); 
    } 
    
    for (int i=0; i<cnt; ++i) {
        ret = tester[i]->start("test");
        if (0 != ret) {
            return;
        }
    }

    sleepSec(1);
    
    for (int i=0; i<1; ++i) {
        tester[i]->addTask(&tasks[i], BIT_EVENT_NORM);
    }

    for (int i=0; i<cnt; ++i) {
        //tester[i]->stop();
        tester[i]->join();
        tester[i]->finish();
    }
}


class TestTimer : public I_TimerDealer {
public:
    virtual Void doTimeout(struct TimerEle* ele) {
        LOG_INFO("do_timeout| tick=%u| time=%u|",
            ele->m_base->monoTick(),
            ele->m_base->now());

        updateTimer(ele);
    }
};

void test_timer() {
    TickTimer* timer = NULL;
    TestTimer* dealer = NULL;
    long timeout = 2000;
    int max = 200000;
    struct TimerEle ele;

    timer = new TickTimer;
    dealer = new TestTimer;
    timer->setDealer(dealer);

    INIT_TIMER_ELE(&ele);

    ele.m_type = 0;
    ele.m_interval = timeout;
    timer->addTimer(&ele);

    for (int i=0; i<max; ++i) {
        timer->tick(1);
    }

    timer->delTimer(&ele);
    timer->stop();
    delete timer;
    delete dealer;
}

static void signon(int sig) {
}

static void mask_sig() {
    /* nothing */
    maskSig(SIGUSR1);
    armSig(SIGUSR1, &signon);
}

void testAgent(const char* path) {
    Int32 ret = 0;
    SockCenter* center = NULL;

    do {
        I_NEW(SockCenter, center);
        center->set(path);
        ret = center->init();
        if (0 != ret) {
            break;
        }

        ret = center->startServer();
        if (0 != ret) {
            break;
        }

        center->wait();
    } while (0);

    center->finish();
    return;
}


int test_main(int argc, char* argv[]) {
    int ret = 0;
    int opt = 0;

    mask_sig();

    opt = atoi(argv[1]);
    if (0 == opt) {
        if (4 == argc) {
        }
    } else if (1 == opt) {
        if (3 == argc) {
            testAgent(argv[2]);
        }
    } else if (2 == opt) {
        testSm4();
    } else if (3 == opt) {
        test_crc();
    } else if (4 == opt) {
        testSm3();
    } else if (5 == opt) {
    } else if (6 == opt) {
        test_hex();
    } else if (7 == opt) {
        test_sig();
    } else if (8 == opt) {
        test_timer();
    } else {
    }

    return ret;
}

