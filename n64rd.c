/*
    n64rd - 2012 Parasyte (http://www.kodewerx.org/)

    Based on:
        GSUpload 0.3 (linux)
        An open source N64 Game Shark (tm) uploader
        based on disassembly of "Generic Uploader" and official utilities
        By Adam Gashlin (hcs) http://gashlin.net/
        http://n64dev.cvs.sourceforge.net/n64dev/n64dev/util/gsupload/

    And:
        Action Replay/GameShark Pro v3.2 Communications Protocol
        by Russ K. (x87bliss[nospam]yahoo.com)
        Document version 1.0 - 6/19/07
        http://hitmen.c02.at/files/docs/psx/GS32Comms.htm
*/

#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "gspro.h"


/* Handy macros */
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
#define GS_READ(_a, _b, _c)     GS_MACRO_3(gs_read, _a, _b, _c)
#define GS_WRITE(_a, _b, _c)    GS_MACRO_3(gs_write, _a, _b, _c)
#define GS_WHERE(_a)            GS_MACRO_1(gs_where, _a)
#define GS_VERSION(_a, _b, _c)  GS_MACRO_3(gs_version, _a, _b, _c)


/* Application information */
#define NAME "n64rd"
#define VERSION "v0.1"

/* Application options (from command line arguments) */
struct _options {
    uint16_t    port;
    char *      port_dev;
    bool        detect;
    bool        read;
    char *      read_file;
    bool        write;
    char *      write_file;
    uint32_t    address;
    uint32_t    length;
};
typedef struct _options OPTIONS;


void usage(void);
void parse_error(char *string, int location);
void cleanup(void);
void *alloc(size_t size);
int detect(void);
int read_data(char *filename, uint32_t address, uint32_t size);
int write_data(char *filename, uint32_t address);
void hex_dump(uint8_t *data, uint32_t address, uint32_t size);


int main(int argc, char **argv) {
    OPTIONS options;
    GS_CONFIG config;
    char *err = 0;
    int c;

    printf(NAME " " VERSION "\n");
    printf("By Parasyte (parasyte@kodewerx.org)\n");
    printf("Website: http://www.kodewerx.org/\n");
    printf("Build date: " __DATE__ ", " __TIME__ "\n");
    printf("\n");

    /* Default option */
    memset(&options, 0, sizeof(options));
    options.address = 0x80000000;
    options.length = 0x00400000;

    while ((c = getopt(argc, argv, "hp:da:l:r::w:")) != -1) {
        switch (c) {
            case 'h':
                usage();
                return 0;

            case 'p':
                options.port = strtol(optarg, &err, 0);
                if (err[0]) {
                    options.port_dev = optarg;
                }
                break;

            case 'd':
                options.detect = true;
                break;

            case 'a':
                options.address = strtoll(optarg, &err, 0);
                if (err[0]) {
                    fprintf(stderr, "Invalid address\n");
                    parse_error(optarg, (err - optarg));
                    return 1;
                }
                break;

            case 'l':
                options.length = strtoll(optarg, &err, 0);
                if (err[0]) {
                    fprintf(stderr, "Invalid length\n");
                    parse_error(optarg, (err - optarg));
                    return 1;
                }
                break;

            case 'r':
                options.read = true;
                if (optarg) {
                    options.read_file = optarg;
                }
                break;

            case 'w':
                options.write = true;
                options.write_file = optarg;
                break;

            case '?':
                if ((optopt == 'p') ||
                    (optopt == 'a') ||
                    (optopt == 'l') ||
                    (optopt == 'w')) {
                    fprintf(stderr, "Option -%c requires an argument.\n", optopt);
                }
                else if (isprint(optopt)) {
                    fprintf(stderr, "Unknown option '-%c'.\n", optopt);
                }
                else {
                    fprintf(stderr, "Unknown option character '\\x%x'.\n", optopt);
                }
                return 1;

            default:
                abort();
        }
    }

    printf("Using port 0x%04X...\n", options.port);

    memset(&config, 0, sizeof(GS_CONFIG));
    config.port = options.port;
    config.port_dev = options.port_dev;

    if (gs_init(&config)) {
        ERRORPRINT("%s\n", "gs_init() failed");
        return 1;
    }

    atexit(cleanup);

    if (options.detect) {
        detect();
    }
    if (options.read) {
        read_data(options.read_file, options.address, options.length);
    }
    if (options.write) {
        write_data(options.write_file, options.address);
    }

    return 0;
}

void usage(void) {
    printf("Usage: " NAME " [options]\n");
    printf("Options:\n");
    printf("  -h            Print usage and quit.\n");
    printf("  -p <port>     Specify port number (default 0x378).\n");
    printf("                Linux systems with PPDev can use a path.\n");
    printf("                e.g. \"/dev/parport0\"\n");
    printf("  -d            Detect GS firmware version.\n");
    printf("  -a <address>  Specify address (default 0x80000000).\n");
    printf("  -l <length>   Specify length (default 0x00400000).\n");
    printf("  -r[file]      Read memory;\n");
    printf("                Copy <length> bytes from memory <address> (to [file]).\n");
    printf("  -w <file>     Write memory;\n");
    printf("                Copy from <file> to memory <address>.\n");
}

void parse_error(char *string, int location) {
    int i;

    fprintf(stderr, "Parse error at character %d:\n", location);
    fprintf(stderr, "%s\n", string);
    for (i = 0; i < location; i++) {
        fprintf(stderr, "-");
    }
    fprintf(stderr, "^\n");
}

void cleanup(void) {
    DEBUGPRINT("%s\n", "Good night! ZZzzz...");
    gs_quit();
}

void *alloc(size_t size) {
    void *p = calloc(size, 1);
    if (!p) {
        abort();
    }

    return p;
}

int detect(void) {
    uint8_t version_size = 0;
    char version[64] = { 0 };

    GS_ENTER();
    GS_VERSION(&version_size, version, sizeof(version));

    if (version_size >= sizeof(version)) {
        DEBUGPRINT("%s\n", "Version string overflow:");
        DEBUGPRINT("%d bytes required, %d bytes available\n", version_size + 1, sizeof(version));
    }
    printf("Detected: %s\n", version);

    return 0;
}

void callback(int range, uint32_t size) {
    printf(".");
    fflush(stdout);
}

int read_data(char *filename, uint32_t address, uint32_t size) {
    FILE *fp;
    uint8_t *data = alloc(size);
    uint8_t check;
    GS_RANGE range[2] = {
        {
            address,
            size
        },
        {
            0, 0
        }
    };

    /* Verify GS is in-game */
    GS_ENTER();
    GS_WHERE(&check);
    if (check != GS_WHERE_GAME) {
        fprintf(stderr, "Read is only available while in-game\n");
        return 1;
    }

    /* The actual read happens here */
    GS_ENTER();
    GS_READ(data, range, callback);
    GS_EXIT();

    printf("\n");

    if (filename) {
        /* Write data to file */
        /* FIXME: Add error handling */
        fp = fopen(filename, "wb");
        fwrite(data, 1, size, fp);
        fclose(fp);
    }
    else {
        /* Or display it all pretty */
        hex_dump(data, address, size);
    }

    free(data);

    return 0;
}

int write_data(char *filename, uint32_t address) {
    FILE *fp;
    uint8_t *data;
    uint8_t check;
    GS_RANGE range[2] = { { 0 } };

    /* Verify GS is in-game */
    GS_ENTER();
    GS_WHERE(&check);
    if (check != GS_WHERE_GAME) {
        fprintf(stderr, "Write is only available while in-game\n");
        return 1;
    }

    /* Read file data */
    /* FIXME: Add error handling */
    fp = fopen(filename, "rb");
    fseek(fp, 0, SEEK_END);
    range[0].size = ftell(fp);
    data = alloc(range[0].size);
    fseek(fp, 0, SEEK_SET);
    fread(data, 1, range[0].size, fp);
    fclose(fp);

    /* The actual write happens here */
    GS_ENTER();
    GS_WRITE(data, range, callback);
    GS_EXIT();

    printf("\n");

    free(data);

    return 0;
}

void hex_dump(uint8_t *data, uint32_t address, uint32_t size) {
    char ascii[16 + 1] = { 0 };
    int i;

    for (i = 0; i < size; i++) {
        /* Address */
        if (!(i % 16)) {
            printf("%08X  ", address + i);
        }

        /* Hex */
        printf("%02X ", data[i]);

        /* ASCII */
        sprintf(&ascii[i % 16], "%c", isprint(data[i]) ? data[i] : '.');
        if ((i % 16) == 15) {
            printf(" %s\n", ascii);
        }
    }

    /* Pad the output, if necessary */
    if (size & 15) {
        for (i = 0; i < (16 - (size & 15)); i++) {
            printf("   ");
        }
        printf(" %s\n", ascii);
    }

    printf("\n");
}
