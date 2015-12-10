#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <string.h>

#include "structs.h"

uint16_t last_name[200];
int offset = 0;

char* dir_pth(dir_t* dir)  {
    char *p = (dir->parent == NULL ? NULL : dir->parent->pth);
    int plen = (p == NULL ? 0 : strlen(p));
    int len = plen + 1 + entry_namelen(dir->entry) + 1;
    char *pth = (char*) malloc(len * sizeof(char));
    assert(pth != NULL);
    if (plen)
        strncpy(pth, p, plen);
    pth[plen] = '/';
    strncpy(pth + plen + 1, (const char *) dir->entry->fname, entry_namelen(dir->entry));
    pth[len - 1] = '\0';
    return pth;
}

void print_dir(dir_t* dir)  {
    printf("directory: %s\n", dir->pth + 6); // skip root label
    if (dir->ext_name != NULL) {
        int i = 0;
        while (dir->ext_name[i] != 0x20 && dir->ext_name[i] != 0x0)
            putwchar((wchar_t) dir->ext_name[i]);
        putchar('\n');
    }
    printf("size: %d bytes, ", dir->byte_size);
    printf("%d clusters\n", dir->clust_sz);
    printf("modify time: %02d-%02d-%02d %02d:%02d:%02d\n\n", dir->entry->days + 1,
        dir->entry->months + 1, dir->entry->years + 1980, dir->entry->hrs,
        dir->entry->mins, dir->entry->secs);
    int i;
    for (i = 0; i < (int) dir->file_cnt; i++)  
        print_file(dir->files[i]);
    for (i = 0; i < (int) dir->subdir_cnt; i++) 
        print_dir(dir->subdirs[i]);
}

dir_t* init_dir(fat_desc_t* fat, entry_t* entry, dir_t* parent) {
    dir_t* dir = (dir_t*) malloc(sizeof(dir_t));
    assert(dir != NULL);
    memset(dir, 0, sizeof(dir_t));
    dir->byte_size = 0;
    dir->parent = parent;
    dir->entry = entry;
    
    dir->max_files = 100;
    dir->max_subdirs = 100;
    dir->subdirs = malloc(dir->max_subdirs * sizeof(dir_t*));
    dir->files = malloc(dir->max_files * sizeof(file_t*));
    assert(dir->subdirs != NULL);
    assert(dir->files != NULL);
    dir->file_cnt = 0;
    dir->subdir_cnt = 0;
    
    // finally get the path
    dir->clust_sz = get_cluster_size(fat, entry->cluster);
    dir->cluster = entry->cluster;
    dir->pth = dir_pth(dir);
    return dir;
}

void free_dir(dir_t* dir) {
    int i;
    for (i = 0; i < (int) dir->subdir_cnt; i++)
        free_dir(dir->subdirs[i]);
    free(dir->subdirs);
    for (i = 0; i < (int) dir->file_cnt; i++) {
        free(dir->files[i]->pth);
        free(dir->files[i]->ext_name);
        free(dir->files[i]->entry);
        free(dir->files[i]);
    }
    free(dir->files);
    free(dir->entry);
    free(dir->pth);
    free(dir->ext_name);
    free(dir);
    dir = NULL;
}

int process_entry(fat_desc_t* fat, dir_t* dir, int pos) {
    entry_t* cur = (entry_t*) malloc(sizeof(entry_t));
    assert(cur != NULL);
    memset(cur, 0, sizeof(entry_t));
    fseek(fat->in, pos, SEEK_SET);
    if (fread(cur, sizeof(entry_t), 1, fat->in) != 1) {
        fprintf(stderr, "Error: failed to read root dir entry\n");
        exit(1);
    }
    
    if (cur->fname[0] == 0x0) { // empty entry
        free(cur); 
        return 0;
    } else if (cur->fname[0] == 0xe5) { // deleted
        free(cur);
        return 1;
    } else if (cur->atr == 0x0f) { // extended name
        memcpy(last_name + offset, cur + 1, 10);
        offset += 10;
        free(cur);
        return 1;
    } else if (cur->fname[0] == '.' && (cur->fname[1] == '.' || // . and ..
            !isalpha(cur->fname[1])) && !isalpha(cur->fname[1])) {
        free(cur);
        return 1;
    }
    
    if (cur->atr & 0x10) { // directory
        dir_t* subdir = init_dir(fat, cur, dir);
        if (offset) { // writting ext. name if necessary
            dir->ext_name = (char*) malloc(offset * 2 + 2);
            ucs2utf(dir->ext_name, last_name);
            //dir->ext_name[offset] = '\0';
            memset(last_name, 0, offset);
            offset = 0;
        }
        dir_process(fat, subdir);
        add_subdir(dir, subdir);
    } else {
        file_t* file = init_file(fat, cur, dir);
        if (offset) { // writting ext. name if necessary
            file->ext_name = (char*) malloc(offset * 2 + 2);
            int i;
            for (i = 0; i < 10; i++) {
              printf("%02X ", last_name[i]);
            }
            printf("\n");
            ucs2utf(file->ext_name, last_name);
            //dir->ext_name[offset] = '\0';
            memset(last_name, 0, offset);
            offset = 0;
        }
        add_file(dir, file);
    }
    return 1;
}
 
dir_t* process_root(fat_desc_t* fat, int root_cluster) {
    entry_t* root_entry = malloc(sizeof(entry_t));
    assert(root_entry != NULL);
    memset(root_entry, 0, sizeof(entry_t));
    memcpy(root_entry->fname, "ROOT", sizeof(char) * 4);
    root_entry->cluster = root_cluster;
    
    dir_t* dir = init_dir(fat, root_entry, NULL);
    assert(dir != NULL);
    
    int i;
    int root_dirs = (32 * fat->sector_size) / sizeof(entry_t);
    int root_ptr = get_sector(fat, fat->rootdir_offset);
    for (i = 0; i < root_dirs; i++)
        if (!process_entry(fat, dir, root_ptr + i * sizeof(entry_t)))
            break;
    return dir;
}

void dir_process(fat_desc_t* fat, dir_t* dir)  {
    int cluster = dir->cluster;
    int dirs_per_cluster = (fat->clust_sz * fat->sector_size) / sizeof(entry_t);
    while (cluster != 0) {
        int cluster_ptr = get_sector(fat, fat->cluster_offset + (cluster - 2) * fat->clust_sz);
        int i;
        for (i = 0; i < dirs_per_cluster; i++)
            if (!process_entry(fat, dir, cluster_ptr + i * sizeof(entry_t)))
                break;
        cluster = get_next_cluster(fat, cluster);
    }
}

void add_subdir(dir_t* dir, dir_t* subdir) {
    // if we ran out of space...
    if (dir->max_subdirs < dir->subdir_cnt) {
        dir->max_subdirs += 100;
        dir->subdirs = realloc(dir->subdirs, dir->max_subdirs * sizeof(dir_t*));
        assert(dir->subdirs != NULL);
    }
    dir->subdirs[dir->subdir_cnt] = subdir;
    dir->subdir_cnt += 1;
}

void add_file(dir_t* dir, file_t* file) {
    // if we ran out of space...
    if (dir->max_files < dir->file_cnt) {
        dir->max_files += 100;
        dir->files = realloc(dir->files, dir->max_files * sizeof(file_t*));
        assert(dir->files != NULL);
    }
    dir->files[dir->file_cnt] = file;
    dir->file_cnt += 1;
    dir->byte_size += file->byte_size;
}
