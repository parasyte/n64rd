
#include <stdint.h>
#include <stdio.h>


/* GameShark commands */
enum _gs_commands {
    GS_CMD_READ     = 0x01,
    GS_CMD_WRITE    = 0x02,
    GS_CMD_EXIT     = 0x64,
    GS_CMD_WHERE    = 0x65,
    GS_CMD_VERSION  = 0x66
};
typedef enum _gs_commands GS_COMMAND;


enum _gs_where {
    GS_WHERE_MENU   = 0x01,
    GS_WHERE_GAME   = 0x02
};


/* Function declarations */
int gs_init(uint16_t port);
int gs_quit(void);
int gs_enter(void);
int gs_exit(void);
int gs_where(uint8_t *out);
int gs_cmd(GS_COMMAND cmd);
int gs_exch_n(uint8_t *in, uint8_t out);
int gs_exch(uint8_t *in, uint8_t out);
int gs_version(uint8_t *size, char *version, int buf_size);
int gs_read(uint8_t *in, uint32_t address, uint32_t size);
int gs_write(uint8_t *out, uint32_t address, uint32_t size);


/* Handy macros */
#define CHECK_TIMEOUT(_timeout) \
    if (!(_timeout)) { \
        fprintf(stderr, "%s() timeout\n", __FUNCTION__); \
        return 1; \
    }

#define GS_MACRO_0(_FUNC) \
    if (_FUNC()) { \
        fprintf(stderr, "%s() " #_FUNC "() failed\n", __FUNCTION__); \
        return 1; \
    }

#define GS_MACRO_1(_FUNC, _ARG) \
    if (_FUNC((_ARG))) { \
        fprintf(stderr, "%s() " #_FUNC "() failed\n", __FUNCTION__); \
        return 1; \
    }

#define GS_MACRO_2(_FUNC, _ARG1, _ARG2) \
    if (_FUNC((_ARG1), (_ARG2))) { \
        fprintf(stderr, "%s() " #_FUNC "() failed, " #_ARG2 "=0x%X\n", __FUNCTION__, (_ARG2)); \
        return 1; \
    }

#define GS_MACRO_3(_FUNC, _ARG1, _ARG2, _ARG3) \
    if (_FUNC((_ARG1), (_ARG2), (_ARG3))) { \
        fprintf(stderr, "%s() " #_FUNC "() failed\n", __FUNCTION__); \
        return 1; \
    }

#define GS_ENTER()              GS_MACRO_0(gs_enter)
#define GS_EXIT()               GS_MACRO_0(gs_exit)
#define GS_WHERE(_a)            GS_MACRO_1(gs_where, _a)
#define GS_CMD(_a)              GS_MACRO_1(gs_cmd, _a)
#define GS_EXCH_N(_a, _b)       GS_MACRO_2(gs_exch_n, _a, _b)
#define GS_EXCH(_a, _b)         GS_MACRO_2(gs_exch, _a, _b)
#define GS_VERSION(_a, _b, _c)  GS_MACRO_3(gs_version, _a, _b, _c)
#define GS_READ(_a, _b, _c)     GS_MACRO_3(gs_read, _a, _b, _c)
#define GS_WRITE(_a, _b, _c)    GS_MACRO_3(gs_write, _a, _b, _c)

#ifdef DEBUG
    #define DEBUGPRINT(_fmt, ...) \
        fprintf(stderr, "%s:%d, %s() DEBUG: " _fmt, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)
#else /* NOT DEBUG */
    #define DEBUGPRINT(_fmt, ...) \
        do {} while (0)
#endif /* DEBUG */

#define ERRORPRINT(_fmt, ...) \
    fprintf(stderr, "%s:%d, %s() ERROR: " _fmt, __FILE__, __LINE__, __FUNCTION__, __VA_ARGS__)

