#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "structs.h"

int entry_is_dir(entry_t* dir);
size_t entry_namelen(entry_t* dir);
size_t entry_extlen(entry_t* dir);
char* file_path(file_t* dir); 
file_t* init_file(fat_desc_t* fat, entry_t* entry, dir_t* dir); 
void free_file(file_t** file_ptr);
void print_file(file_t* file);

char* file_path(file_t* file) {
    int namelen = entry_namelen(file->entry), 
        extlen = entry_extlen(file->entry),
        plen = strlen(file->dir->pth);
    
    char* pth = malloc(plen + namelen + extlen + 3);
    assert(pth != NULL);
    
    pth[plen] = '/';
    pth[plen + namelen + extlen + 2] = '\0';

    if (plen > 0) 
        strncpy(pth, file->dir->pth, plen);
    strncpy(pth + plen + 1, file->entry->fname, namelen);
    
    if (extlen > 0) {
        pth[plen + namelen + 1] = '.';
        strncpy(pth + plen + namelen + 2, file->entry->ext, extlen);
    }
    return pth;
}

void print_file(file_t* file)  {
    printf("file: %s\n", file->pth + 6); // skip root label
    printf("size: %u bytes, ", file->byte_size);
    printf("%u clusters\n", file->cluster);
    printf("modify time: %02d-%02d-%02d %02d:%02d:%02d\n\n", file->entry->days + 1,
        file->entry->months + 1, file->entry->years + 1980, file->entry->hrs,
        file->entry->mins, file->entry->secs);
}

size_t entry_namelen(entry_t* dir)  {
    size_t len = 0;
    while (len < 8 && dir->fname[len] != 0x20 && dir->fname[len] != 0x0)
        len++;
    return len;
}

size_t entry_extlen(entry_t* dir) {
    size_t len = 0;
    while (len < 3 && dir->ext[len] != 0x20 && dir->ext[len] != 0x0)
        len++;
    return len;
}

file_t* init_file(fat_desc_t* fat, entry_t* entry, dir_t* dir)  {
    file_t* file = malloc(sizeof(file_t));
    assert(file != NULL);
    memset(file, 0, sizeof(file_t));
    file->dir = dir;
    
    file->cluster = entry->cluster;
    file->clust_sz = get_cluster_size(fat, entry->cluster);
    file->byte_size = entry->size;
    
    file->entry = entry;
    file->pth = file_path(file);
    return file;
}
