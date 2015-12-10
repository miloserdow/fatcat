#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "structs.h"

// https://github.com/Qiuyongjie/ucs2-utf8/blob/master/ucs2-utf8-release.c
void ucs2utf (char* dst, uint16_t* src) {
    int i = 0, j = 0, next = 0;
    for (i = 0; i < 10; i++) {
        printf("%02X ", src[i]);
    }
    i = 0;
    printf("\n");
    while (src[i] != 0) {
        if (src[i] < 0x80) { 
            next = 1;
            dst[j] = 0;
            dst[j] = src[i];
        } else if (src[i] < 0x800) {
            next = 2;
            dst[j] = 0;
            dst[j + 1] = 0;
            dst[j + 1] = (char)((src[i] & 0x3F) | 0x80);
            dst[j] = (char)(((src[i] & 0x3F) & 0x1F) | 0xC0);
        } else {
            next = 3;
            dst[j] = 0;
            dst[j + 1] = 0;
            dst[j + 2] = 0;
            dst[j] |= ((((char)(src[i] >> 12)) & 0xF) | 0xE0);
            dst[j + 1] |= (((char)(src[i] >> 6) & 0x3F) | 0x80);
            dst[j + 2] |= (((char)(src[i] >> 0) & 0x3F) | 0x80);
        }
        j += next;
        i++;
    }
    printf("-> %s\n", dst);
    dst[j] = 0;
}

inline int get_sector(fat_desc_t* fat, int sector) {
    int res = fat->disk_start;
    res += fat->sector_size * sector;
    return res;
}

inline int get_cluster_size(fat_desc_t* fat, int start_cluster) {
    int cluster = start_cluster, size = 1;
    while (cluster != 0) {
        cluster = get_next_cluster(fat, cluster);
        size++;
    }
    return size;
}

inline int get_next_cluster(fat_desc_t* fat, int cluster)  {
    int fat_start = get_sector(fat, fat->reserved_sectors);
    uint8_t ptr[3];
    fseek(fat->in, fat_start + 3 * (cluster / 2), SEEK_SET);
    if (fread(&ptr, sizeof(uint8_t), 3, fat->in) != 3) {
        fprintf(stderr, "Error: failed to read cluster (0x%x)\n", cluster);
        exit(1);
    }
    int res;
    if (cluster & 1)
        res = (ptr[2] << 4) + ((ptr[1] & 0xf0) >> 4); 
    else
        res = ((ptr[1] & 0x0f) << 8) + ptr[0];
    if (res < 0x02 || res >= 0xff0) 
        res = 0;
    return res;
}
