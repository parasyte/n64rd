
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

#if defined(_WIN32)
    /* UNIMPLEMENTED */
#else /* !define(_WIN32) */
    #include <sys/io.h>
#endif /* defined(_WIN32) */

#include "gspro.h"


/* Private variables */
uint16_t _gs_port = 0x378;

#define LPT_DATA _gs_port
#define LPT_STAT (_gs_port + 1)


/* Private functions */
uint8_t _gs_inb(uint16_t port);
void _gs_outb(uint8_t data, uint16_t port);


uint8_t _gs_inb(uint16_t port) {
    /* Linux, and other UNIX-like environments */
    #if defined(_WIN32)
        ERRORPRINT("%s\n", "UNIMPLEMENTED");
    #else /* !defined(_WIN32) */
        return inb(port);
    #endif /* defined(_WIN32) */
}

void _gs_outb(uint8_t data, uint16_t port) {
    /* Linux, and other UNIX-like environments */
    #if defined(_WIN32)
        ERRORPRINT("%s\n", "UNIMPLEMENTED");
    #else /* !defined(_WIN32) */
        outb(data, port);
    #endif /* defined(_WIN32) */
}


/* Public functions */

/* Initialize the library */
int gs_init(uint16_t port) {
    _gs_port = port;

    #if defined(_WIN32)
        ERRORPRINT("%s\n", "UNIMPLEMENTED");
    #else /* !defined(_WIN32) */
        if (ioperm(LPT_DATA, 2, 1)) {
            ERRORPRINT("%s\n", "Couldn't get LPT, are you root?");
            return 1;
        }
    #endif /* defined(_WIN32) */

    return 0;
}

/* Finish library calls */
int gs_quit(void) {
    _gs_outb(0, LPT_DATA);

    #if defined(_WIN32)
        ERRORPRINT("%s\n", "UNIMPLEMENTED");
    #else /* !defined(_WIN32) */
        ioperm(LPT_DATA, 2, 0);
    #endif /* defined(_WIN32) */

    return 0;
}

/* Read CPU memory */
int gs_read(uint8_t *in, uint32_t address, uint32_t size) {
    int i;
    uint8_t sum = 0;
    uint8_t calc_sum = 0;

    GS_CMD(GS_CMD_READ);

    /* Send address */
    DEBUGPRINT("Address: 0x%08X\n", address);
    GS_EXCH(NULL, address >> 24);
    GS_EXCH(NULL, address >> 16);
    GS_EXCH(NULL, address >> 8);
    GS_EXCH(NULL, address >> 0);

    /* Send data size */
    DEBUGPRINT("Size: 0x%08X\n", size);
    GS_EXCH(NULL, size >> 24);
    GS_EXCH(NULL, size >> 16);
    GS_EXCH(NULL, size >> 8);
    GS_EXCH(NULL, size >> 0);

    /* Read data */
    for (i = 0; i < size; i++) {
        GS_EXCH(&in[i], 0);
        sum += in[i];
    }

    /* Pump out 8 extra bytes, to help the hardware finish its checksum calculation */
    for (i = 0; i < 8; i++) {
        GS_EXCH(NULL, 0);
    }

    /* Verify */
    GS_EXCH(&calc_sum, 0);
    if (calc_sum != sum) {
        fprintf(stderr, "Checksum failure during read:\n");
        fprintf(stderr, "  Received: 0x%02X\n", sum);
        fprintf(stderr, "  Expected: 0x%02X\n", calc_sum);
        return 1;
    }

    return 0;
}

/* Write CPU memory */
int gs_write(uint8_t *out, uint32_t address, uint32_t size) {
    int i;
    uint8_t sum = 0;
    uint8_t calc_sum = 0;

    GS_CMD(GS_CMD_WRITE);

    /* Send address */
    DEBUGPRINT("Address: 0x%08X\n", address);
    GS_EXCH(NULL, address >> 24);
    GS_EXCH(NULL, address >> 16);
    GS_EXCH(NULL, address >> 8);
    GS_EXCH(NULL, address >> 0);

    /* Send data size */
    DEBUGPRINT("Size: 0x%08X\n", size);
    GS_EXCH(NULL, size >> 24);
    GS_EXCH(NULL, size >> 16);
    GS_EXCH(NULL, size >> 8);
    GS_EXCH(NULL, size >> 0);

    /* Write data */
    for (i = 0; i < size; i++) {
        GS_EXCH(NULL, out[i]);
        sum += out[i];
    }

    /* Pump out 8 extra bytes, to help the hardware finish its checksum calculation */
    for (i = 0; i < 8; i++) {
        GS_EXCH(NULL, 0);
    }

    /* Verify */
    GS_EXCH(&calc_sum, 0);
    if (calc_sum != sum) {
        fprintf(stderr, "Checksum failure during write:\n");
        fprintf(stderr, "  Received: 0x%02X\n", calc_sum);
        fprintf(stderr, "  Expected: 0x%02X\n", sum);
        return 1;
    }

    return 0;
}

/* Enter PC-control */
int gs_enter(void) {
    int timeout = 1000;
    uint8_t result = 0;
    uint8_t in = 0;

    while (--timeout) {
        /* Repeatedly send 0x3 until we receive 0x6 then 0x7 */
        GS_EXCH_N(&in, 3);

        result = ((result << 4) | (in & 0x0F));
        if (result == 0x67) break;
    }
    CHECK_TIMEOUT(timeout);

    return 0;
}

/* Exit PC-control */
int gs_exit(void) {
    GS_CMD(GS_CMD_EXIT);

    return 0;
}

/* Discover GS run mode (and exit PC-control) */
int gs_where(uint8_t *out) {
    GS_CMD(GS_CMD_WHERE);

    /* Returns GS_WHERE_MENU when in the menu, GS_WHERE_GAME when in game */
    GS_EXCH(out, 0);

    return 0;
}

/* Get GS firmware version (and exit PC-control) */
int gs_version(uint8_t *size, char *version, int buf_size) {
    uint8_t buf = 0;

    if (buf_size <= 0) {
        DEBUGPRINT("Invalid buf_size=%d", buf_size);
        return 1;
    }

    GS_CMD(GS_CMD_VERSION);

    while (buf != 0x2E) {
        GS_EXCH(&buf, 0);
        if (buf == 0x67) {
            printf("Cannot detect firmware version while in-game\n");
            GS_EXIT();
            return 1;
        }
    }
    GS_EXCH(size, 0);

    int i;
    for (i = 0; i < *size; i++) {
        GS_EXCH(&buf, 0);
        if (i < buf_size) {
            version[i] = buf;
        }
    }
    version[buf_size - 1] = 0;

    return 0;
}


/* Send one nybble, and receive another */
int gs_exch_n(uint8_t *in, uint8_t out) {
    int timeout = 100000;
    uint8_t data = 0;

    if (in) {
        *in = 0;
    }

    /* Wait until hardware is ready to receive a nybble */
    if (_gs_inb(LPT_STAT) & 0x08) {
        _gs_outb(0, LPT_DATA);
        while ((--timeout) && (_gs_inb(LPT_STAT) & 0x08));
        CHECK_TIMEOUT(timeout);
    }

    /* Send */
    DEBUGPRINT("send 0x%X\n", (out & 0x0F));
    _gs_outb((out & 0x0F) | 0x10, LPT_DATA);

    /* Wait until hardware is ready to send a nybble */
    timeout = 100000;
    while ((--timeout) && (!(_gs_inb(LPT_STAT) & 0x08)));
    CHECK_TIMEOUT(timeout);

    /* Receive */
    data = (_gs_inb(LPT_STAT) >> 4) ^ 0x08;
    DEBUGPRINT("recv 0x%X\n", data);
    if (in) {
        *in = data;
    }

    /* Reset for next time around... */
    _gs_outb(0, LPT_DATA);

    return 0;
}

/* Send one byte, and receive another */
int gs_exch(uint8_t *in, uint8_t out) {
    uint8_t upper = 0;
    uint8_t data = 0;

    if (in) {
        *in = 0;
    }

    DEBUGPRINT("send 0x%02X\n", out);

    GS_EXCH_N(&upper, (out >> 4));
    GS_EXCH_N(&data, out);
    data |= (upper << 4);

    DEBUGPRINT("recv 0x%02X\n", data);

    if (in) {
        *in = data;
    }

    return 0;
}

/* Send command to GS */
int gs_cmd(GS_COMMAND cmd) {
    uint8_t check = 0;
    int timeout = 1000;

    DEBUGPRINT("Sending command: 0x%02X\n", cmd);

    /* Command Handshake */
    while (--timeout) {
        GS_EXCH(&check, 'G'); /* Gavin */
        if (check != 'g') continue;

        GS_EXCH(&check, 'T'); /* Thornton */
        if (check == 't') break;
    }
    CHECK_TIMEOUT(timeout);

    return gs_exch(NULL, cmd);
}

