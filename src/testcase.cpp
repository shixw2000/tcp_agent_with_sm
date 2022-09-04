#include<string.h>
#include<stdlib.h>
#include"testcase.h"
#include"managecenter.h"
#include"msgcenter.h"
#include"smutil.h"
#include"msgtype.h"
#include"server.h"


Int32 creatAgent(int argc, char* argv[]) {
    Int32 ret = 0;
    Server* srv = NULL;

    I_NEW(Server, srv);

    if (3 == argc) {
        srv->set(argv[2]);
    }
    
    ret = srv->init();
    if (0 != ret) {
        return ret;
    }

    

    ret = srv->startServer();
    if (0 != ret) {
        return -1;
    }

    srv->wait();
    srv->finish();

    return 0;
}

Void testSm4() {
    Int32 pre_len = 0;
    Int32 enc_len = 0;
    Int32 dec_len = 0;
    EvpBase evp;
    Char input[128] = "helo\r\n";
    Char output1[128] = {0};
    Char output2[128] = {0};

    evp.m_cipher.setUsrKey(DEF_SM4_KEY, DEF_SM4_KEY_LEN);

    pre_len = strlen(input);
    LOG_INFO("plain_text| len=%d| msg=%s|", pre_len, input);
    printHex("====plain", input, pre_len); 
    
    enc_len = evp.m_cipher.sm4_cbc_encrypt(input, pre_len, output1); 

    enc_len = evp.m_cipher.sm4_cbc_encrypt(input, pre_len, output1); 

    dec_len = evp.m_cipher.sm4_cbc_decrypt(output1, enc_len, output2);
    
    LOG_INFO("test_sm4| enc=%d| dec=%d|",
        enc_len, dec_len);
}

void test_crc() {
    MsgHdr* hdr = NULL;
    MsgStartPeer* msg = NULL;    

    hdr = MsgCenter::creat<MsgStartPeer>(ENUM_MSG_CMD_START_PEER);
    msg = MsgCenter::cast<MsgStartPeer>(hdr);

    msg->m_user_id = 13432;
    msg->m_session_id = 112343432;

    MsgCenter::addCrc(hdr);

    MsgCenter::chkCrc(hdr);
    
}

int test_main(int argc, char* argv[]) {
    int ret = 0;
    int opt = 0;

    opt = atoi(argv[1]);
    if (2 == opt) {
        testSm4();
    } else if (3 == opt) {
        test_crc();
    } else {
        ret = creatAgent(argc, argv)
    }

    return ret;
}

