#ifndef FS_H
#define FS_H

#include <stdbool.h>
#include <time.h>

#define DISK_FILE "disk.sim"
#define LOG_FILE "disk.log"
#define DISK_SIZE 1048576  // 1 MB
#define METADATA_SIZE 4096 // 4 KB
#define BLOCK_SIZE 512
#define MAX_FILES 64
#define FILENAME_LEN 32

typedef struct {
    char name[FILENAME_LEN];
    int size;
    int start_block;
    time_t created_at;
    bool valid;
} FileEntry;

bool log_init();
bool fs_init();
int fs_create(const char* filename);
int fs_delete(const char* filename);
int fs_write(const char* filename, const char* data, int size);
int fs_read(const char* filename, int offset, int size, char* buffer);
void fs_ls(bool is_called_from_menu);
int fs_format();
int fs_rename(const char* old_name, const char* new_name);
bool fs_exists(const char* filename);
int fs_size(const char* filename);
int fs_append(const char* filename, const char* data, int size);
int fs_truncate(const char* filename, int new_size);
int fs_copy(const char* src, const char* dest);
int fs_mv(const char* old_path, const char* new_path);
int fs_defragment();
int fs_check_integrity();
int fs_backup(const char* filename);
int fs_restore(const char* filename);
int fs_cat(const char* filename);
int fs_diff(const char* file1, const char* file2);
int fs_log();
void log_operation(const char* operation, const char* details);
static int find_free_block(int required_size);

#endif
