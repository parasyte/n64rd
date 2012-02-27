/*
    n64gshax - 2012 Parasyte (http://www.kodewerx.org/)

    Based on:
        GSUpload 0.3 (linux)
        An open source N64 Game Shark (tm) uploader
        based on disassembly of "Generic Uploader" and official utilities

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


/* Application information */
#define NAME "n64rd"
#define VERSION "v0.1"

/* Application options (from command line arguments) */
struct _options {
    uint16_t port;
    bool detect;
    bool read;
    char *read_file;
    bool write;
    char *write_file;
    bool show;
    uint32_t address;
    uint32_t length;
};


void usage(void);
void parse_error(char *string, int location);
void cleanup(void);
void *alloc(size_t size);
int detect(void);
int read_data(char *filename, uint32_t address, uint32_t size, bool show);
int write_data(char *filename, uint32_t address);
void hex_dump(uint8_t *data, uint32_t address, uint32_t size);


int main(int argc, char **argv) {
    struct _options options;
    char *err = 0;
    int c;

    printf(NAME " " VERSION "\n");
    printf("By Parasyte (parasyte@kodewerx.org)\n");
    printf("Website: http://www.kodewerx.org/\n");
    printf("Build date: " __DATE__ ", " __TIME__ "\n");
    printf("\n");

    /* Default option */
    memset(&options, 0, sizeof(options));
    options.port = 0x378;
    options.address = 0x80000000;
    options.length = 0x00400000;

    while ((c = getopt(argc, argv, "hp:da:l:r::w:s")) != -1) {
        switch (c) {
            case 'h':
                usage();
                return 0;

            case 'p':
                options.port = strtol(optarg, &err, 0);
                if (err[0]) {
                    fprintf(stderr, "Invalid port number\n");
                    parse_error(optarg, (err - optarg));
                    return 1;
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

            case 's':
                options.show = true;
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
    if (gs_init(options.port)) {
        ERRORPRINT("%s\n", "gs_init() failed");
        return 1;
    }

    atexit(cleanup);

    if (options.detect) {
        detect();
    }
    if (options.read) {
        read_data(options.read_file, options.address, options.length, false);
    }
    if (options.write) {
        write_data(options.write_file, options.address);
    }
    if (options.show) {
        read_data(NULL, options.address, options.length, true);
    }

    return 0;
}

void usage(void) {
    printf("Usage: " NAME " [options]\n");
    printf("Options:\n");
    printf("  -h            Print usage and quit.\n");
    printf("  -p <port>     Specify port number (default 0x378).\n");
    printf("  -d            Detect GS firmware version.\n");
    printf("  -a <address>  Specify address (default 0x80000000).\n");
    printf("  -l <length>   Specify length (default 0x00400000).\n");
    printf("  -r[file]      Read memory;\n");
    printf("                Copy <length> bytes from memory <address> (to [file]).\n");
    printf("  -w <file>     Write memory;\n");
    printf("                Copy from <file> to memory <address>.\n");
    printf("  -s            Show <length> bytes from memory <address>.\n");
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
    void *p = malloc(size);
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

int read_data(char *filename, uint32_t address, uint32_t size, bool show) {
    FILE *fp;
    uint8_t *data = alloc(size);
    uint8_t check;

    /* Verify GS is in-game */
    GS_ENTER();
    GS_WHERE(&check);
    if (check != GS_WHERE_GAME) {
        fprintf(stderr, "Read is only available while in-game\n");
        return 1;
    }

    /* The actual read happens here */
    GS_ENTER();
    GS_READ(data, address, size);
    GS_EXIT();

    /* Write data to file */
    if (filename) {
        /* FIXME: Add error handling */
        fp = fopen(filename, "wb");
        fwrite(data, 1, size, fp);
        fclose(fp);
    }

    /* And this displays it all pretty */
    if (show) {
        hex_dump(data, address, size);
    }

    free(data);

    return 0;
}

int write_data(char *filename, uint32_t address) {
    FILE *fp;
    uint32_t size;
    uint8_t *data;
    uint8_t check;

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
    size = ftell(fp);
    data = alloc(size);
    fseek(fp, 0, SEEK_SET);
    fread(data, 1, size, fp);
    fclose(fp);

    /* The actual write happens here */
    GS_ENTER();
    GS_WRITE(data, address, size);
    GS_EXIT();

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

