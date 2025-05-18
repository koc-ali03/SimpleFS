#include "fs.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static FileEntry file_table[MAX_FILES];

static int disk_fd = -1; // Başta -1 çünkü henüz atama yapılmadı

// disk.sim içinden metadatayı al, hafızaya yükle
int load_metadata() {
    lseek(disk_fd, 0, SEEK_SET);
    return read(disk_fd, file_table, sizeof(file_table));
}

// Hafızada tutulan metadatayı disk.sim içine kaydet
int save_metadata() {
    lseek(disk_fd, 0, SEEK_SET);
    return write(disk_fd, file_table, sizeof(file_table));
}

// Diski başlat (yoksa oluştur, varsa yükle)
int fs_init() {
    disk_fd = open(DISK_FILE, O_RDWR);
    if (disk_fd < 0) {
        disk_fd = open(DISK_FILE, O_RDWR | O_CREAT, 0666);
        if (disk_fd < 0) {
            write(STDOUT_FILENO, "disk file open/create failed\n", 29);
            return -1;
        }
        ftruncate(disk_fd, DISK_SIZE);
        memset(file_table, 0, sizeof(file_table));
        save_metadata();
    } else {
        load_metadata();
    }
    return 0;
}

// Yeni dosya oluştur
int fs_create(const char* filename) {
    if (fs_exists(filename)) {
        write(STDOUT_FILENO, "File already exists.\n", 21);
        return -1;
    }

    for (int i = 0; i < MAX_FILES; ++i) {
        if (!file_table[i].valid) {
            strncpy(file_table[i].name, filename, FILENAME_LEN);
            file_table[i].size = 0;
            file_table[i].start_block = (i * BLOCK_SIZE) + METADATA_SIZE; // /////////////////////////////// FRAGMENTASYON SONRASI DEĞİŞECEK
            file_table[i].created_at = time(NULL);
            file_table[i].valid = 1;
            return save_metadata();
        }
    }

    write(STDOUT_FILENO, "No space left in file table.\n", 29);
    return -1;
}

// Dosyayı sil
int fs_delete(const char* filename) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].valid && strcmp(file_table[i].name, filename) == 0) {
            file_table[i].valid = 0;
            memset(file_table[i].name, 0, FILENAME_LEN);
            file_table[i].size = 0;
            file_table[i].start_block = 0;
            file_table[i].created_at = 0;
            return save_metadata();
        }
    }
    write(STDOUT_FILENO, "File not found: ", 16);
    write(STDOUT_FILENO, filename, strlen(filename));
    write(STDOUT_FILENO, "\n", 1);
    return -1;
}

// Dosya içine yaz
int fs_write(const char* filename, const char* data, int size) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].valid && strcmp(file_table[i].name, filename) == 0) {
            lseek(disk_fd, file_table[i].start_block, SEEK_SET);
            write(disk_fd, data, size);
            file_table[i].size = size;
            return save_metadata();
        }
    }
    write(STDOUT_FILENO, "File not found: ", 16);
    write(STDOUT_FILENO, filename, strlen(filename));
    write(STDOUT_FILENO, "\n", 1);
    return -1;
}

// Dosyayı oku
int fs_read(const char* filename, int offset, int size, char* buffer) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].valid && strcmp(file_table[i].name, filename) == 0) {
            if (offset + size > file_table[i].size) {
                write(STDOUT_FILENO, "Read exceeds file size.\n", 24);
                return -1;
            }
            lseek(disk_fd, file_table[i].start_block + offset, SEEK_SET);
            return read(disk_fd, buffer, size);
        }
    }
    write(STDOUT_FILENO, "File not found: ", 16);
    write(STDOUT_FILENO, filename, strlen(filename));
    write(STDOUT_FILENO, "\n", 1);
    return -1;
}

// Tüm dosyaları göster
void fs_ls() {
    write(STDOUT_FILENO, "Files on disk:\n", 15);
    for (int i = 0; i < MAX_FILES; ++i) {
        if (file_table[i].valid) {
            char size_buf[32];
            int len = snprintf(size_buf, sizeof(size_buf), " (%d bytes)\n", file_table[i].size);

            write(STDOUT_FILENO, " - ", 3);
            write(STDOUT_FILENO, file_table[i].name, strlen(file_table[i].name));
            write(STDOUT_FILENO, size_buf, len);
        }
    }
}

// Diski formatla
int fs_format() {
    memset(file_table, 0, sizeof(file_table));
    if (ftruncate(disk_fd, DISK_SIZE) != 0) {
        write(STDOUT_FILENO, "ftruncate failed\n", 17);
        return -1;
    }
    return save_metadata();
}

// Dosyayı yeniden adlandır (fs_mv bu özelliği zaten içeriyor)
int fs_rename(const char* old_name, const char* new_name) { return fs_mv(old_name, new_name); }

// Dosya varlığını kontrol et
int fs_exists(const char* filename) {
    for (int i = 0; i < MAX_FILES; ++i) {
        if (file_table[i].valid && strcmp(file_table[i].name, filename) == 0)
            return 1;
    }
    return 0;
}

// Dosyanın boyutunu bul
int fs_size(const char* filename) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].valid && strcmp(file_table[i].name, filename) == 0) {
            return file_table[i].size;
        }
    }
    write(STDOUT_FILENO, "File not found: ", 16);
    write(STDOUT_FILENO, filename, strlen(filename));
    write(STDOUT_FILENO, "\n", 1);
    return -1;
}

// Dosyaya ekleme yap
int fs_append(const char* filename, const char* data, int size) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].valid && strcmp(file_table[i].name, filename) == 0) {
            int offset = file_table[i].start_block + file_table[i].size;
            lseek(disk_fd, offset, SEEK_SET);
            write(disk_fd, data, size);
            file_table[i].size += size;
            return save_metadata();
        }
    }
    write(STDOUT_FILENO, "File not found: ", 16);
    write(STDOUT_FILENO, filename, strlen(filename));
    write(STDOUT_FILENO, "\n", 1);
    return -1;
}

// Dosyayı kırp (boyutu küçültmek için)
int fs_truncate(const char* filename, int new_size) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].valid && strcmp(file_table[i].name, filename) == 0) {
            if (new_size > file_table[i].size) {
                write(STDOUT_FILENO, "New size is larger than current file size.\n", 43);
                return -1;
            }
            file_table[i].size = new_size;
            return save_metadata();
        }
    }
    write(STDOUT_FILENO, "File not found: ", 16);
    write(STDOUT_FILENO, filename, strlen(filename));
    write(STDOUT_FILENO, "\n", 1);
    return -1;
}

// Dosyayı kopyala
int fs_copy(const char* src, const char* dest) {
    if (!fs_exists(src)) {
        write(STDOUT_FILENO, "Source file not found.\n", 23);
        return -1;
    }

    if (fs_exists(dest)) {
        write(STDOUT_FILENO, "Destination file already exists.\n", 33);
        return -1;
    }

    char* buffer;
    int size;

    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].valid && strcmp(file_table[i].name, src) == 0) {
            size = file_table[i].size;
            buffer = malloc(size);
            lseek(disk_fd, file_table[i].start_block, SEEK_SET);
            read(disk_fd, buffer, size);

            fs_create(dest);
            for (int j = 0; j < MAX_FILES; j++) {
                if (file_table[j].valid && strcmp(file_table[j].name, dest) == 0) {
                    lseek(disk_fd, file_table[j].start_block, SEEK_SET);
                    write(disk_fd, buffer, size);
                    file_table[j].size = size;
                    save_metadata();
                    free(buffer);
                    return 0;
                }
            }
        }
    }

    return -1;
}

// Dosyayı taşı (fs_rename özelliğini zaten içeriyor)
int fs_mv(const char* old_path, const char* new_path) {
    if (fs_exists(new_path)) {
        write(STDOUT_FILENO, "Path already contains a file with the same name.\n", 28);
        return -1;
    }

    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].valid && strcmp(file_table[i].name, old_path) == 0) {
            strncpy(file_table[i].name, new_path, FILENAME_LEN);
            return save_metadata();
        }
    }

    write(STDOUT_FILENO, "File not found: ", 16);
    write(STDOUT_FILENO, old_path, strlen(old_path));
    write(STDOUT_FILENO, "\n", 1);
    return -1;
}

// Dosyayının içeriğini ekrana yazdır
int fs_cat(const char* filename) {
    char buffer[BLOCK_SIZE];
    int size = fs_size(filename);

    if (size <= 0) return -1;

    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].valid && strcmp(file_table[i].name, filename) == 0) {
            lseek(disk_fd, file_table[i].start_block, SEEK_SET);
            read(disk_fd, buffer, size);
            write(STDOUT_FILENO, buffer, size);
            write(STDOUT_FILENO, "\n", 1);
            return 0;
        }
    }

    return -1;
}

// İki dosyanın verilerini karşılaştır
int fs_diff(const char* file1, const char* file2) {
    int size1 = fs_size(file1);
    int size2 = fs_size(file2);

    if (size1 != size2) {
        write(STDOUT_FILENO, "Files differ in size.\n", 22);
        return 1;
    }

    char* buf1 = malloc(size1);
    char* buf2 = malloc(size2);

    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].valid && strcmp(file_table[i].name, file1) == 0) {
            lseek(disk_fd, file_table[i].start_block, SEEK_SET);
            read(disk_fd, buf1, size1);
        }
        if (file_table[i].valid && strcmp(file_table[i].name, file2) == 0) {
            lseek(disk_fd, file_table[i].start_block, SEEK_SET);
            read(disk_fd, buf2, size2);
        }
    }

    int result = memcmp(buf1, buf2, size1);
    if (result == 0) {
        write(STDOUT_FILENO, "Files are identical.\n", 21);
    } else {
        write(STDOUT_FILENO, "Files differ.\n", 14);
    }

    free(buf1);
    free(buf2);
    return result;
}
