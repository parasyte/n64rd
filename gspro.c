
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#if defined(_WIN32)
    /* Windows, including 64-bit */
    /* UNIMPLEMENTED */
#elif defined(linux)
    /* Linux */
    #include <sys/ioctl.h>
    #include <linux/parport.h>
    #include <linux/ppdev.h>
    #include <fcntl.h>

    /* Private variables */
    int _gs_port_fd = -1;
    int _gs_port_mode = 0;
#else /* !define(_WIN32) */
    /* Other UNIX-like environments */
    #include <sys/io.h>
#endif /* defined(_WIN32) */

#include "gspro.h"
#include "except.h"


/* Exceptions */
enum _exception_types {
    GS_Unimplemented = 1,
    GS_TimeoutException
};

#define TIMEOUT() \
    do { \
        Exception e = { \
            EXCEPTION_INFO, \
            GS_TimeoutException, \
            "Communications link timed out." \
        }; \
        _throw(e); \
    } while (0)

#define UNIMPLEMENTED() \
    do { \
        Exception e = { \
            EXCEPTION_INFO, \
            GS_Unimplemented, \
            "UNIMPLEMENTED." \
        }; \
        _throw(e); \
    } while (0)


/* Private variables */
static int _gs_ready = 0;
static GS_CONFIG _gs_config = { 0 };


/* Private defines */
#define _GS_DEFAULT_PORT 0x378
#define _GS_LPT_DATA _gs_config.port
#define _GS_LPT_STAT (_gs_config.port + 1)

#define _GS_DEFAULT_PORT_DEV "/dev/parport0"


/* Private function declarations */
uint8_t _gs_in(uint16_t port);
void _gs_out(uint8_t data, uint16_t port);
void _gs_cmd(GS_COMMAND cmd);
uint8_t _gs_exch_4(uint8_t out);
uint8_t _gs_exch_8(uint8_t out);
uint32_t _gs_exch_32(uint32_t out);
void _gs_mem(uint8_t *data, GS_RANGE *range, void (*callback)(int, uint32_t), bool write);


/* Private functions */

/* Receive one nybble */
uint8_t _gs_in(uint16_t port) {
    #if defined(_WIN32)
        /* Windows, including 64-bit */
        UNIMPLEMENTED();

        return 0;
    #elif defined(linux)
        /* Linux */
        uint8_t data;
        ioctl(_gs_port_fd, PPRSTATUS, &data);

        return data;
    #else /* !defined(_WIN32) */
        /* Linux, and other UNIX-like environments */
        return inb(port);
    #endif /* defined(_WIN32) */
}

/* Send one nybble */
void _gs_out(uint8_t data, uint16_t port) {
    #if defined(_WIN32)
        /* Windows, including 64-bit */
        UNIMPLEMENTED();
    #elif defined(linux)
        /* Linux */
        ioctl(_gs_port_fd, PPWDATA, &data);
    #else /* !defined(_WIN32) */
        /* Linux, and other UNIX-like environments */
        outb(data, port);
    #endif /* defined(_WIN32) */
}

/* Send one nybble, and receive another */
uint8_t _gs_exch_4(uint8_t out) {
    int timeout = 100000;
    uint8_t data = 0;

    /* Wait until hardware is ready to receive a nybble */
    if (_gs_config.in_callback(_GS_LPT_STAT) & 0x08) {
        _gs_config.out_callback(0, _GS_LPT_DATA);
        while ((--timeout) && (_gs_config.in_callback(_GS_LPT_STAT) & 0x08));
        if (!timeout) TIMEOUT();
    }

    /* Send */
    DEBUGPRINT("send 0x%X\n", (out & 0x0F));
    _gs_config.out_callback((out & 0x0F) | 0x10, _GS_LPT_DATA);

    /* Wait until hardware is ready to send a nybble */
    timeout = 100000;
    while ((--timeout) && (!(_gs_config.in_callback(_GS_LPT_STAT) & 0x08)));
    if (!timeout) TIMEOUT();

    /* Receive */
    data = (_gs_config.in_callback(_GS_LPT_STAT) >> 4) ^ 0x08;
    DEBUGPRINT("recv 0x%X\n", data);

    /* Reset for next time around... */
    _gs_config.out_callback(0, _GS_LPT_DATA);

    return data;
}

/* Send one byte, and receive another */
uint8_t _gs_exch_8(uint8_t out) {
    uint8_t data = 0;

    DEBUGPRINT("send 0x%02X\n", out);

    data  = _gs_exch_4(out >> 4) << 4;
    data |= _gs_exch_4(out >> 0) << 0;

    DEBUGPRINT("recv 0x%02X\n", data);

    return data;
}

/* Send one word, and receive another */
uint32_t _gs_exch_32(uint32_t out) {
    uint32_t result;

    result  = _gs_exch_8(out >> 24) << 24;
    result |= _gs_exch_8(out >> 16) << 16;
    result |= _gs_exch_8(out >> 8)  << 8;
    result |= _gs_exch_8(out >> 0)  << 0;

    return result;
}

/* Send command to GS */
void _gs_cmd(GS_COMMAND cmd) {
    int timeout = 1000;

    DEBUGPRINT("Sending command: 0x%02X\n", cmd);

    /* Command Handshake */
    while (--timeout) {
        if (_gs_exch_8('G') != 'g') /* Gavin */
            continue;
        if (_gs_exch_8('T') == 't') /* Thornton */
            break;
    }
    if (!timeout) TIMEOUT();

    _gs_exch_8(cmd);
}

/* Send or receive data using READ or WRITE command */
void _gs_mem(uint8_t *data, GS_RANGE *range, void (*callback)(int, uint32_t), bool write) {
    int i = 0;
    int count = 0;
    uint8_t sum = 0;
    uint8_t calc_sum = 0;
    static char error[80] = { 0 };

    _gs_cmd(write ? GS_CMD_WRITE : GS_CMD_READ);

    while (range[count].address && range[count].size) {
        /* Send address */
        DEBUGPRINT("Address: 0x%08X\n", range[count].address);
        _gs_exch_32(range[count].address);

        /* Send data size */
        DEBUGPRINT("Size: 0x%08X\n", range[count].size);
        _gs_exch_32(range[count].size);

        /* Read data */
        for (i = 0; i < range[count].size; i++) {
            /* Run callback periodically */
            if (callback && i && (!(i & 0x3FFF))) {
                callback(count, i);
            }

            if (write) {
                _gs_exch_8(data[i]);
            }
            else {
                data[i] = _gs_exch_8(0);
            }

            sum += data[i];
        }

        count++;
    }

    /* Final callback */
    if (callback && (i & 0x3FFF)) {
        callback(count, i);
    }

    /* Send a null address and size to exit the read loop */
    DEBUGPRINT("Address: 0x%08X\n", 0);
    _gs_exch_32(0);

    DEBUGPRINT("Size: 0x%08X\n", 0);
    _gs_exch_32(0);

    /* Verify */
    calc_sum = _gs_exch_8(0);
    if (calc_sum != sum) {
        sprintf(error, "Checksum failure during %s:\n"
            "  Received: 0x%02X\n"
            "  Expected: 0x%02X\n",
            (write ? "write" : "read"), calc_sum, sum);

        Exception e = {
            EXCEPTION_INFO,
            GS_TimeoutException,
            error
        };
        _throw(e);
    }
}


/* Public functions */

/* Initialize the library */
GS_STATUS gs_init(GS_CONFIG *config) {
    if (_gs_ready) {
        return GS_SUCCESS;
    }

    _gs_config.port = _GS_DEFAULT_PORT;
    _gs_config.port_dev = _GS_DEFAULT_PORT_DEV;
    _gs_config.in_callback = _gs_in;
    _gs_config.out_callback = _gs_out;

    if (config) {
        if (config->port)
            _gs_config.port = config->port;
        if (config->port_dev)
            _gs_config.port_dev = config->port_dev;
        if (config->in_callback)
            _gs_config.in_callback = config->in_callback;
        if (config->out_callback)
            _gs_config.out_callback = config->out_callback;
    }

    #if defined(_WIN32)
        /* Windows, including 64-bit */
        /* Do not use the UNIMPLEMENTED() macro here; no try/catch sugar */
        ERRORPRINT("%s\n", "UNIMPLEMENTED");

        return GS_ERROR;
    #elif defined (linux)
        /* Linux */
        assert(_gs_config.port_dev);

        _gs_port_fd = open(_gs_config.port_dev, O_RDWR);
        if (_gs_port_fd == -1) {
            ERRORPRINT("Unable to open '%s' for read/write\n", _gs_config.port_dev);

            return GS_ERROR;
        }

        if (ioctl(_gs_port_fd, PPCLAIM, NULL)) {
            ERRORPRINT("Could not claim '%s'\n", _gs_config.port_dev);
            close(_gs_port_fd);
            _gs_port_fd = -1;

            return GS_ERROR;
        }

        int _gs_port_mode = IEEE1284_MODE_NIBBLE;
        if (ioctl(_gs_port_fd, PPSETMODE, &_gs_port_mode)) {
            ERRORPRINT("Could not set nibble mode for '%s'\n", _gs_config.port_dev);
            ioctl(_gs_port_fd, PPRELEASE);
            close(_gs_port_fd);
            _gs_port_fd = -1;

            return GS_ERROR;
        }
    #else /* !defined(_WIN32) */
        /* Other UNIX-like environments */
        if (ioperm(_GS_LPT_DATA, 2, 1)) {
            ERRORPRINT("%s\n", "Couldn't get LPT, are you root?");

            return GS_ERROR;
        }
    #endif /* defined(_WIN32) */

    _gs_ready++;

    return GS_SUCCESS;
}

/* Finish library calls */
GS_STATUS gs_quit(void) {
    assert(_gs_ready);

    if (_gs_ready > 0) {
        _gs_ready--;

        if (_gs_ready > 0) {
            return GS_SUCCESS;
        }
    }

    _gs_config.out_callback(0, _GS_LPT_DATA);

    _gs_config.port = 0;
    _gs_config.port_dev = NULL;
    _gs_config.in_callback = NULL;
    _gs_config.out_callback = NULL;

    #if defined(_WIN32)
        /* Windows, including 64-bit */
        /* Do not use the UNIMPLEMENTED() macro here; no try/catch sugar */
        ERRORPRINT("%s\n", "UNIMPLEMENTED");

        return GS_ERROR;
    #elif defined (linux)
        /* Linux */
        ioctl(_gs_port_fd, PPRELEASE);
        close(_gs_port_fd);
        _gs_port_fd = -1;
    #else /* !defined(_WIN32) */
        /* Other UNIX-like environments */
        ioperm(_GS_LPT_DATA, 2, 0);
    #endif /* defined(_WIN32) */

    return GS_SUCCESS;
}

/* Enter PC-control */
GS_STATUS gs_enter(void) {
    int timeout = 1000;
    uint8_t result = 0;

    assert(_gs_ready);

    DEBUGPRINT("%s\n", "Entering...");

    _try (e) {
        while (--timeout) {
            /*
             * Repeatedly send 0x3 until we receive 'g'.
             *
             * The 0x03 puts GS into "awaiting command" state.
             * 'g' is the response to the first byte of the command handshake.
             *
             * This function synchronizes nybble-mode communication line,
             * and puts the GS into its "awaiting command" state.
             */
            result = (result << 4) | _gs_exch_4(3);
            if (result == 'g') break;
        }
        if (!timeout) TIMEOUT();
    }
    _catch {
        ERRORPRINT("%s:%d, %s(): %s\n", e->file, e->line, e->function, e->msg);

        return GS_ERROR;
    }

    return GS_SUCCESS;
}

/* Exit PC-control */
GS_STATUS gs_exit(void) {
    assert(_gs_ready);

    _try (e) {
        _gs_cmd(GS_CMD_UNPAUSE);
    }
    _catch {
        ERRORPRINT("%s:%d, %s(): %s\n", e->file, e->line, e->function, e->msg);

        return GS_ERROR;
    }

    return GS_SUCCESS;
}

/* Read CPU memory */
GS_STATUS gs_read(uint8_t *in, GS_RANGE *range, void (*callback)(int, uint32_t)) {
    assert(_gs_ready);

    _try (e) {
        _gs_mem(in, range, callback, false);
    }
    _catch {
        ERRORPRINT("%s:%d, %s(): %s\n", e->file, e->line, e->function, e->msg);

        return GS_ERROR;
    }

    return GS_SUCCESS;
}

/* Write CPU memory */
GS_STATUS gs_write(uint8_t *out, GS_RANGE *range, void (*callback)(int, uint32_t)) {
    assert(_gs_ready);

    _try (e) {
        _gs_mem(out, range, callback, true);
    }
    _catch {
        ERRORPRINT("%s:%d, %s(): %s\n", e->file, e->line, e->function, e->msg);

        return GS_ERROR;
    }

    return GS_SUCCESS;
}

/* Discover GS run mode (and exit PC-control) */
GS_STATUS gs_where(uint8_t *out) {
    assert(_gs_ready);

    _try (e) {
        _gs_cmd(GS_CMD_WHERE);

        /* Returns GS_WHERE_MENU when in the menu, GS_WHERE_GAME when in game */
        *out = _gs_exch_8(0);
    }
    _catch {
        ERRORPRINT("%s:%d, %s(): %s\n", e->file, e->line, e->function, e->msg);

        return GS_ERROR;
    }

    return GS_SUCCESS;
}

/* Get GS firmware version (and exit PC-control) */
GS_STATUS gs_version(uint8_t *size, char *version, int buf_size) {
    int i;
    uint8_t buf = 0;

    assert(_gs_ready);
    assert(buf_size > 0); /* We need a buffer with valid size */

    _try (e) {
        _gs_cmd(GS_CMD_VERSION);

        /* FIXME: Verify this... */
        while (buf != 0x2E) {
            buf = _gs_exch_8(0);
            if (buf == 'g') {
                /* Exit GS "awaiting command" state" */
                gs_exit();

                ERRORPRINT("%s\n", "Cannot detect firmware version while in-game");

                return GS_ERROR;
            }
        }

        /* Get size of version string */
        *size = _gs_exch_8(0);

        /* Get version string */
        for (i = 0; i < *size; i++) {
            buf = _gs_exch_8(0);
            if (i < buf_size) {
                version[i] = buf;
            }
        }

        /* Append null-terminator to version string */
        version[buf_size - 1] = '\0';
    }
    _catch {
        ERRORPRINT("%s:%d, %s(): %s\n", e->file, e->line, e->function, e->msg);

        return GS_ERROR;
    }

    return GS_SUCCESS;
}

/* Read CPU memory 32-bits at a time (and exit PC-control) */
GS_STATUS gs_read_rom(uint8_t *data, GS_RANGE *range, void (*callback)(uint32_t)) {
    int i = 0;
    uint8_t sum = 0;
    uint8_t calc_sum = 0;
    uint32_t word = 0;

    _gs_cmd(GS_CMD_READ_ROM);

    /* Send address */
    range->address &= ~3;
    DEBUGPRINT("Address: 0x%08X\n", range->address);
    _gs_exch_32(range->address);

    /* Send data size */
    range->size = (range->size + 3) & ~3;
    DEBUGPRINT("Size: 0x%08X\n", range->size);
    _gs_exch_32(range->size);

    /* Read data */
    for (i = 0; i < range->size; i += 4) {
        /* Run callback periodically */
        if (callback && i && (!(i & 0x3FFF))) {
            callback(i);
        }

        word = _gs_exch_32(0);
        sum += word;

        data[i + 0] = word >> 24;
        data[i + 1] = word >> 16;
        data[i + 2] = word >> 8;
        data[i + 3] = word >> 0;
    }

    /* Final callback */
    if (callback && (i & 0x3FFF)) {
        callback(i);
    }

    /* Verify */
    calc_sum = _gs_exch_8(0);
    if (calc_sum != sum) {
        ERRORPRINT("Checksum failure during ROM read:\n"
            "  Received: 0x%02X\n"
            "  Expected: 0x%02X\n",
            calc_sum, sum);

        return GS_ERROR;
    }

    return GS_SUCCESS;
}
