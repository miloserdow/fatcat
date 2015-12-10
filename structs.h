#ifndef STRUCTS_H
#define STRUCTS_H

#include <stdint.h>
#include <stdlib.h>
#include <stdint.h>
#include <wchar.h>
#include <wctype.h>

// Entry
typedef struct {
    uint8_t fname[8]; 
    uint8_t ext[3]; 
    uint8_t atr; 
    uint8_t reserved[10];
    uint32_t secs : 5;
    uint32_t mins : 6;
    uint32_t hrs  : 5;
    uint32_t days   : 5;
    uint32_t months : 4;
    uint32_t years  : 7;
    uint16_t cluster;
    uint32_t size;
} __attribute((packed)) entry_t;

// Boot sector
typedef struct {
    uint8_t bootcode[3];
    uint8_t oem_data[8];
    uint16_t sector_size;
    uint8_t clust_sz;
    uint16_t reserved_sectors;
    uint8_t fat_cnt;
    uint16_t root_entries;
    uint16_t unused;
    uint8_t media_type;
    uint16_t fat_size;
    uint16_t sectors_per_track;
    uint16_t head_cnt;
    uint32_t hidden_sectors;
    uint32_t total_sect;
    uint8_t drive_number;
    uint8_t reserved;
    uint8_t eboot_sign;
    uint32_t volume_id;
    char volume_label[11];
    char fs_type[8];
    uint8_t boot_code[448];
    uint16_t boot_sign;
} __attribute((packed)) bootsector_t;

// FAT info
typedef struct {
    unsigned long disk_start;  // seek pos
    FILE* in;
    bootsector_t* bs;
    size_t sector_size;
    unsigned int clust_sz;
    unsigned int root_entries;
    unsigned int sectors_per_fat;
    unsigned int hidden_sectors;
    unsigned int reserved_sectors;
    unsigned int sectors_for_root;
    unsigned int rootdir_offset;
    unsigned int cluster_offset; 
    unsigned int fat_cnt;
    unsigned int clusters;
} fat_desc_t;

struct file_t;
typedef struct file_t file_t;
typedef struct dir_t dir_t;

struct dir_t  {
    dir_t* parent;
    entry_t* entry;
    dir_t** subdirs;
    file_t** files;
    unsigned int max_subdirs;
    unsigned int max_files;
    unsigned int sector;
    unsigned int cluster;
    unsigned int clust_sz;
    unsigned int subdir_cnt;
    unsigned int file_cnt;
    unsigned int byte_size;
    char* pth;
};

struct file_t {
    dir_t* dir;
    entry_t* entry;
    unsigned int cluster;
    unsigned int clust_sz;
    unsigned int byte_size;
    char* pth;
};

// ALL the functions involved...
dir_t* process_root(fat_desc_t* fat, int root_cluster);
void dir_process(fat_desc_t *fat, dir_t *parent);
dir_t* init_dir(fat_desc_t* fat, entry_t* entry, dir_t* parent);
void add_subdir(dir_t* parent, dir_t* subdir);
void add_file(dir_t* parent, file_t* file);
char* dir_pth(dir_t* dir);
void print_dir(dir_t* dir);
void free_dir(dir_t* dir);

size_t entry_namelen(entry_t* dir);
size_t entry_extlen(entry_t* dir);
char* file_path(file_t* dir); 
file_t* init_file(fat_desc_t* fat, entry_t* entry, dir_t* dir); 
void print_file(file_t* file);

inline int get_sector(fat_desc_t* fat, int sector);
inline int get_cluster_size(fat_desc_t* fat, int start_cluster);
inline int get_next_cluster(fat_desc_t* fat, int cluster);
#endif // STRUCTS_H
