#include<string.h>
#include<stdlib.h>
#include<stdio.h>
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
    MsgStartPeer* msg = NULL;    

    hdr = MsgCenter::creat<MsgStartPeer>(ENUM_MSG_CMD_START_PEER);
    msg = MsgCenter::cast<MsgStartPeer>(hdr);

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

int test_main(int argc, char* argv[]) {
    int ret = 0;
    int opt = 0;

    opt = atoi(argv[1]);
    if (1 == opt) {
        test_hex();
    } else if (2 == opt) {
        testSm4();
    } else if (3 == opt) {
        test_crc();
    } else if (4 == opt) {
        testSm3();
    } else {
        ret = creatAgent(argc, argv);
    }

    return ret;
}

