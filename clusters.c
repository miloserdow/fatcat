#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>

#include "structs.h"

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
    fread(&ptr, sizeof(uint8_t), 3, fat->in);
    int res;
    if (cluster & 1)
        res = (ptr[2] << 4) + ((ptr[1] & 0xf0) >> 4); 
    else
        res = ((ptr[1] & 0x0f) << 8) + ptr[0];
    if (res < 0x02 || res >= 0xff0) 
        res = 0;
    return res;
}
