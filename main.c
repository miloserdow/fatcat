#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "structs.h"

typedef struct {
    uint8_t first_byte;
    uint8_t start_chs[3];
    uint8_t partition_type;
    uint8_t end_chs[3];
    uint32_t start_sector;
    uint32_t length_sectors;
} __attribute((packed)) partition_table_t;

fat_desc_t* fat_init(FILE* in, unsigned int offset) {
    fat_desc_t* fat = (fat_desc_t*)malloc(sizeof(fat_desc_t));
    assert(fat);
    memset(fat, 0, sizeof(fat_desc_t));
    
    fat->disk_start = offset;
    fat->in = in;
    fseek(in, offset, SEEK_SET);
    bootsector_t* bs = (bootsector_t*) malloc(sizeof(bootsector_t));
    memset(bs, 0, sizeof(bootsector_t));
    fread(bs, sizeof(bootsector_t), 1, in);
    
    fat->bs = bs;
    
    fat->sectors_per_fat = bs->fat_size;
    fat->sector_size = bs->sector_size;
    fat->hidden_sectors = bs->hidden_sectors;
    fat->reserved_sectors = bs->reserved_sectors;
    fat->clust_sz = bs->clust_sz;
    fat->root_entries = bs->root_entries;
    fat->fat_cnt = bs->fat_cnt;
    
    fat->sectors_for_root = (fat->root_entries * 32) / fat->sector_size;
    fat->reserved_sectors = fat->reserved_sectors;
    fat->rootdir_offset = fat->reserved_sectors + fat->fat_cnt * fat->sectors_per_fat;
    fat->cluster_offset = fat->rootdir_offset + fat->sectors_for_root;
    
    return fat;
}

void print_filesystem_info(fat_desc_t* fat) {
    printf("Label: %.11s\n", fat->bs->volume_label);
    printf("Volume id: 0x%x\n", fat->bs->volume_id);
    printf("OEM data: %.8s\n", fat->bs->oem_data);
    printf("Sector size: %d\n", fat->sector_size);
    printf("Cluster size (sectors): %d\n", fat->clust_sz);
    printf("Root entries: %d\n", fat->root_entries);
    printf("Number of FATs: %d\n", fat->bs->fat_cnt);
    printf("Sectors per fat: %d\n", fat->sectors_per_fat);
    printf("Reserved sectors: %d\n", fat->reserved_sectors);
    printf("Hidden sectors: %d\n", fat->hidden_sectors);
    printf("Fat offset in sectors: %d\n", fat->reserved_sectors);
    printf("Root directory offset in sectors: %d\n", fat->rootdir_offset);
    printf("First cluster offset in sectors: %d\n", fat->cluster_offset);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: fatinfo <input file>\n");
        exit(1);
    }
    FILE* in = fopen(argv[1], "rb");
    int i;
    partition_table_t pt[4];
    fseek(in, 0x1BE, SEEK_SET); // go to partition table start
    fread(pt, sizeof(partition_table_t), 4, in); // read all four entries
    for (i = 0; i < 4; i++) {
        printf("Partition %d, type %02X", i, pt[i].partition_type);
        switch (pt[i].partition_type) {
        case 0x0:
            printf(" (empty)");
            break;
        case 0x01:
            printf(" (FAT12)");
            break;
        case 0x04:
        case 0x06:
            printf(" (FAT16)");
            break;
        case 0x11:
            printf (" (FAT32)");
            break;
        }
        printf("\n  Start sector %08X, %d sectors long\n===========\n", 
                pt[i].start_sector, pt[i].length_sectors);
        if (pt[i].partition_type != 0x01 && pt[i].partition_type != 0x06)
            continue; // not FAT
    
        fat_desc_t* fat = fat_init(in, 512 * pt[i].start_sector);
        print_filesystem_info(fat);
        puts("----------------------------\n");
        dir_t* root = process_root(fat, fat->rootdir_offset);
        if (pt[i].partition_type != 0x01) {
            puts("Skipping content (FAT16 is not supported)...\n");
            continue;
        }
        puts("<-- BEGIN CONTENT -->\n");
        for (i = 0; i < (int) root->subdir_cnt; i++) 
            print_dir(root->subdirs[i]);
        for (i = 0; i < (int) root->file_cnt; i++)  
            print_file(root->files[i]);
        puts("<-- END OF FAT FILESYSTEM -->\n");
    }
    return 0;
}
