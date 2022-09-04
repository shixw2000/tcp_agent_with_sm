#ifndef __GLOBALTYPE_H__
#define __GLOBALTYPE_H__
#include<stdio.h>
#include<errno.h>
#include<string.h>


typedef unsigned char Byte;
typedef char Char;
typedef short Int16;
typedef unsigned short Uint16;
typedef int Int32;
typedef unsigned int Uint32;
typedef long long Int64;
typedef unsigned long long Uint64;
typedef bool Bool;
typedef void Void;

typedef Void (*TimerCb)(Void* p1, Void* p2);

enum BOOL_VAL {
    FALSE = 0,
    TRUE = 1
};


#ifndef NULL
#define NULL 0
#endif

#define I_FREE(x) do { if (NULL != (x)) {delete (x); (x)=NULL;} } while (0)
#define I_NEW(type, x)  ((x) = new type)
#define I_NEW_1(type, x, val)  ((x) = new type(val))
#define I_NEW_2(type, x, v1, v2)  ((x) = new type(v1, v2))
#define ARR_FREE(x) do { if (NULL != (x)) {delete[] (x); (x)=NULL;} } while (0)
#define ARR_NEW(type,size, x) ((x) = new type[size])

#define offset_of(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)
#define container_of(ptr, type, member) ({			\
	const typeof(((type *)0)->member)* __mptr = (ptr);	\
	(type *)((char*)__mptr - offset_of(type, member)); })

extern Void initLib();
extern const Char* clockMs();

#define PRINT_LOG(format,args...) do {\
    fprintf(stdout, "<%s> ", clockMs()); \
    fprintf(stdout, format, ##args); \
    fprintf(stdout, "|\n"); \
} while (0)

#ifndef __LOG_LEVEL__
#define __LOG_LEVEL__ 1
#endif

#if (__LOG_LEVEL__ >= 2)
#define LOG_DEBUG(format,args...) PRINT_LOG(format, ##args)
#else 
#define LOG_DEBUG(format,args...) 
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


#define ERR_MSG() strerror(errno)

#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))
#define CAS(ptr,test,val) __sync_bool_compare_and_swap((ptr), (test), (val))
#define CMPXCHG(ptr,test,val) __sync_val_compare_and_swap((ptr), (test), (val)) 
#define ATOMIC_SET(ptr,val) __sync_lock_test_and_set((ptr), (val))
#define ATOMIC_CLEAR(ptr) __sync_lock_release(ptr)
#define ATOMIC_FETCH_INC(ptr) __sync_fetch_and_add((ptr), 1)
#define ATOMIC_INC_FETCH(ptr) __sync_add_and_fetch((ptr), 1)


static const Int32 DEF_IP_SIZE = 64;
static const Int32 DEF_ADDR_SIZE = 128;
static const Int32 DEF_SESSION_ID_SIZE = 32; 
static const Int32 DEF_SYM_KEY_SIZE = 128;
static const Int32 DEF_ASYM_KEY_SIZE = 128;
static const Int16 DEF_MSG_VER = 0x0814;
static const Int32 DEF_TCP_RETAIN_SIZE = 48;
static const Int32 DEF_FD_MAX_CAPACITY = 100000;

static const Int32 DEF_SM3_DIGEST_SIZE = 32;
static const Int32 DEF_SM4_BLOCK_SIZE = 16;

static const Int32 DEF_CIPHER_MAX_SIZE = 64;

static const Int32 DEF_POLL_WAIT_MILLISEC = 8000;

/* login parameters */
static const Int32 DEF_SEID_SIZE = 32;
static const Int32 DEF_RAND_CODE_SIZE = 32;
static const Int32 DEF_SECRET_DATA_SIZE = 32;
static const Int32 DEF_SIGNATURE_DATA_SIZE = 32;

static const Char DEF_SM4_KEY[] = "123456";
static const Int32 DEF_SM4_KEY_LEN = 6;

extern Void printHex(const Char* prompt, const Void* data, int len);

#endif

