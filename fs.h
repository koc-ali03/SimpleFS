#ifndef FS_H
#define FS_H

#include <time.h>

#define DISK_FILE "disk.sim"
#define DISK_SIZE 1048576 // 1 MB
#define METADATA_SIZE 4096 // 4 KB
#define BLOCK_SIZE 512
#define MAX_FILES 64
#define FILENAME_LEN 32

typedef struct {
    char name[FILENAME_LEN];
    int size;
    int start_block;
    time_t created_at;
    int valid; // Boolean, aktif ise 1 değilse 0
} FileEntry;

int fs_init();
int fs_create(const char *filename);
int fs_delete(const char *filename);
int fs_write(const char *filename, const char *data, int size);
int fs_read(const char *filename, int offset, int size, char *buffer);
int fs_append(const char *filename, const char *data, int size);
int fs_truncate(const char *filename, int new_size);
int fs_mv(const char *old_path, const char *new_path);
int fs_exists(const char *filename);
int fs_size(const char *filename);
int fs_write(const char *filename, const char *data, int size);
int fs_read(const char *filename, int offset, int size, char *buffer);
int fs_append(const char *filename, const char *data, int size);
int fs_delete(const char *filename);
int fs_truncate(const char *filename, int new_size);
int fs_rename(const char *old_name, const char *new_name);
int fs_copy(const char *src, const char *dest);
int fs_rename(const char *old_name, const char *new_name);
int fs_size(const char *filename);
int fs_cat(const char *filename);
int fs_diff(const char *file1, const char *file2);
int fs_format();
void fs_ls();

#endif