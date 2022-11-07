#ifndef __GLOBALTYPE_H__
#define __GLOBALTYPE_H__
#include<stdio.h>
#include<errno.h>
#include<string.h>
#include"sysoper.h"


#define PRINT_LOG(format,args...) do {\
    fprintf(stdout, "<%s> ", clockMs()); \
    fprintf(stdout, format, ##args); \
    fprintf(stdout, "|\n"); \
} while (0)

#define PRINT_ERR(format,args...) do {\
    fprintf(stderr, "<%s> ", clockMs()); \
    fprintf(stderr, format, ##args); \
    fprintf(stderr, "|\n"); \
} while (0)

#ifndef __LOG_LEVEL__
#define __LOG_LEVEL__ 0
#endif

#if (__LOG_LEVEL__ >= 2)
#define LOG_DEBUG(format,args...) PRINT_LOG(format, ##args)
#define RAW_LOG(format,args...) fprintf(stdout, format, ##args);
#else 
#define LOG_DEBUG(format,args...) 
#define RAW_LOG(format,args...)
#endif

#if (__LOG_LEVEL__ >= 1)
#define LOG_INFO(format,args...) PRINT_LOG(format, ##args)
#else
#define LOG_INFO(format,args...)
#endif 

#if (__LOG_LEVEL__ >= 0)
#define LOG_ERROR(format,args...) PRINT_LOG(format, ##args)
#else
#define LOG_ERROR(format,args...)
#endif

extern Void initLib();
extern const Char* clockMs();

#define ERR_MSG() strerror(errno) 

static const Int32 DEF_IP_SIZE = 64;
static const Int32 DEF_ADDR_SIZE = 128;
static const Int32 DEF_SESSION_ID_SIZE = 32; 
static const Int32 MAX_SYM_KEY_SIZE = 64;
static const Int32 DEF_ASYM_KEY_SIZE = 128;
static const Uint16 DEF_MSG_VER = 0x0814;
static const Int32 DEF_TCP_RETAIN_SIZE = 48;
static const Int32 DEF_FD_MAX_CAPACITY = 200000;

static const Int32 DEF_SM2_SIGN_SIZE = 64;
static const Int32 DEF_SM2_PUB_KEY_SIZE = 64;
static const Int32 DEF_SM2_PRI_KEY_SIZE = 32;
static const Int32 DEF_SM3_DIGEST_SIZE = 32;
static const Int32 DEF_SM4_BLOCK_SIZE = 16;
static const Int32 MAX_SM_DATA_SIZE = 512;
static const Int32 MAX_PIN_PASSWD_SIZE = 64;

static const Int32 DEF_CIPHER_MAX_SIZE = 64;

static const Int32 DEF_POLL_WAIT_MILLISEC = 1000;

static const Char DEF_BUILD_VER[] = __BUILD_VER__;

/* login parameters */
static const Int32 MAX_SEID_SIZE = 64;
static const Int32 DEF_SEID_SIZE = 16;
static const Int32 DEF_RAND_CODE_SIZE = 32;
static const Int32 DEF_SIGNATURE_DATA_SIZE = 32;
static const Int32 MAX_SECRET_DATA_SIZE = 256;
static const Int32 MAX_SIGNATURE_DATA_SIZE = 128;

static const Int32 DEF_ENC_RAND_SIZE = 32;
static const Int32 DEF_SESS_KEY_SIZE = 32;

static const Uint32 DEF_CHK_EKEY_TIMER_INTERVAL =3;

static const Char DEF_SM4_KEY[] = "123456";
static const Int32 DEF_SM4_KEY_LEN = 6;

extern Void printHex(const Char* prompt, const Void* data, int len);
extern Int32 hex2Bin(const Void* hex, Int32 len, Void* out);

extern const Byte* g_test_seid;

#endif

