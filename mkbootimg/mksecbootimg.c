/*
** Copyright 2012, Havlena Petr <havlenapetr@gmail.com>
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>

#define CHECK_FILE(file)    \
    if(!file) {             \
        errno = EINVAL;     \
        return EINVAL;      \
    }

static void *load_file(const char *fn, unsigned *_sz)
{
    char *data;
    int sz;
    int fd;

    data = 0;
    fd = open(fn, O_RDONLY);
    if(fd < 0) return 0;

    sz = lseek(fd, 0, SEEK_END);
    if(sz < 0) goto oops;

    if(lseek(fd, 0, SEEK_SET) != 0) goto oops;

    data = (char*) malloc(sz);
    if(data == 0) goto oops;

    if(read(fd, data, sz) != sz) goto oops;
    close(fd);

    if(_sz) *_sz = sz;
    return data;

oops:
    close(fd);
    if(data != 0) free(data);
    return 0;
}

int usage(void)
{
    fprintf(stderr,"usage: mkbootimg\n"
            "       --kernel <filename>\n"
            "       --ramdisk <filename>\n"
            "       [ --cmdline <kernel-commandline> ]\n"
            "       [ --board <boardname> ]\n"
            "       [ --base <address> ]\n"
            "       [ --pagesize <pagesize> ]\n"
            "       -o|--output <filename>\n"
            );
    return 1;
}

static int align_offset(FILE* file)
{
    CHECK_FILE(file);

    int offset = ftell(file);
    if(fseek(file, (offset + 511) & ~511, SEEK_SET) != 0) return -1;
    return ftell(file);
}

static int append_image(FILE* file, void* data, int data_size, int* offset, int* length)
{
    CHECK_FILE(file);

    *offset = align_offset(file);
    if(fwrite(data, 1, data_size, file) != data_size) return -1;

    *length = align_offset(file) - *offset;
    assert (*offset % 512 == 0);
    assert (*length % 512 == 0);

    *offset /= 512;
    *length /= 512;

    return 0;
}

int main(int argc, char **argv)
{
    char *kernel_fn = 0;
    void *kernel_data = 0;
    int kernel_size = 0;
    char *ramdisk_fn = 0;
    void *ramdisk_data = 0;
    int ramdisk_size = 0;
    char *bootimg = 0;
    char *cmdline = "";
    int table_loc;
    char table[128], padding[512];
    FILE* file;

    argc--;
    argv++;

    while(argc > 0){
        char *arg = argv[0];
        char *val = argv[1];
        if(argc < 2) {
            return usage();
        }
        argc -= 2;
        argv += 2;
        if(!strcmp(arg, "--output") || !strcmp(arg, "-o")) {
            bootimg = val;
        } else if(!strcmp(arg, "--kernel")) {
            kernel_fn = val;
        } else if(!strcmp(arg, "--ramdisk")) {
            ramdisk_fn = val;
        } else if(!strcmp(arg, "--cmdline")) {
            cmdline = val;
        } else if(!strcmp(arg, "--base")) {
        } else if(!strcmp(arg, "--board")) {
        } else if(!strcmp(arg,"--pagesize")) {
        } else {
            return usage();
        }
    }

    if(bootimg == 0) {
        fprintf(stderr,"error: no output filename specified\n");
        return usage();
    }

    if(kernel_fn == 0) {
        fprintf(stderr,"error: no kernel image specified\n");
        return usage();
    }

    if(ramdisk_fn == 0) {
        fprintf(stderr,"error: no ramdisk image specified\n");
        return usage();
    }

    kernel_data = load_file(kernel_fn, &kernel_size);
    if(kernel_data == 0) {
        fprintf(stderr,"error: could not load kernel '%s'\n", kernel_fn);
        return 1;
    }

    if(!strcmp(ramdisk_fn,"NONE")) {
        ramdisk_data = 0;
    } else {
        ramdisk_data = load_file(ramdisk_fn, &ramdisk_size);
        if(ramdisk_data == 0) {
            fprintf(stderr,"error: could not load ramdisk '%s'\n", ramdisk_fn);
            return 1;
        }
    }

    file = fdopen(open(bootimg, O_CREAT | O_TRUNC | O_WRONLY, 0644), "w");
    if(!file) {
        fprintf(stderr,"error: could not create '%s'\n", bootimg);
        return 1;
    }

    if(fwrite(kernel_data, 1, kernel_size, file) != kernel_size) goto fail;
    table_loc = align_offset(file);
    if(table_loc < 0) goto fail;

    memset(&padding, 0x00, sizeof(padding));
    if(fwrite(&padding, 1, sizeof(padding), file) != sizeof(padding)) goto fail;

    int offset, length;
    if(append_image(file, ramdisk_data, ramdisk_size, &offset, &length) < 0) goto fail;

    memset(&table, 0, sizeof(table));
    sprintf(&table, "\n\nBOOT_IMAGE_OFFSETS\n" \
            "boot_offset=%d;boot_len=%d;\n\n", offset, length);

    if(fseek(file, table_loc, SEEK_SET) != 0) goto fail;
    if(fwrite(&table, 1, strlen(table), file) != strlen(table)) goto fail;

    fclose(file);

    return 0;

fail:
    unlink(bootimg);
    fclose(file);
    fprintf(stderr,"error: failed writing '%s': %s\n", bootimg,
            strerror(errno));
    return 1;
}
