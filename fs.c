#include "fs.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

static FileEntry file_table[MAX_FILES];

// Başta -1 çünkü henüz atama yapılmadı
static int disk_fd = -1;
static int log_fd = -1;

// disk.sim içinden metadatayı al, hafızaya yükle
static int load_metadata() {
    lseek(disk_fd, 0, SEEK_SET);
    return read(disk_fd, file_table, sizeof(file_table));
}

// Hafızada tutulan metadatayı disk.sim içine kaydet
static int save_metadata() {
    lseek(disk_fd, 0, SEEK_SET);
    return write(disk_fd, file_table, sizeof(file_table));
}

// Log sistemini başlat
bool log_init() {
    log_fd = open(LOG_FILE, O_WRONLY | O_CREAT | O_APPEND, 0666);
    if (log_fd < 0) {
        write(STDOUT_FILENO, "Log dosyasi acilamadi veya olusturulamadi\n", 43);
        return false;
    }
    return true;
}

// Diski başlat (yoksa oluştur, varsa yükle)
bool fs_init() {
    disk_fd = open(DISK_FILE, O_RDWR);
    if (disk_fd < 0) {
        disk_fd = open(DISK_FILE, O_RDWR | O_CREAT, 0666);
        if (disk_fd < 0) {
            write(STDOUT_FILENO, "Disk dosyasi acilamadi veya olusturulamadi\n", 44);
            return false;
        }
        ftruncate(disk_fd, DISK_SIZE);
        memset(file_table, 0, sizeof(file_table));
        save_metadata();
    } else {
        load_metadata();
    }
    return true;
}

// Yeni dosya oluştur
int fs_create(const char* filename) {
    if (fs_exists(filename)) {
        write(STDOUT_FILENO, "Dosya zaten mevcut.\n", 21);
        return -1;
    }

    // Boş bir dosya girdisi bul
    int free_slot = -1;
    for (int i = 0; i < MAX_FILES; ++i) {
        if (!file_table[i].valid) {
            free_slot = i;
            break;
        }
    }

    if (free_slot == -1) {
        write(STDOUT_FILENO, "Dosya tablosunda bos yer kalmadi.\n", 35);
        return -1;
    }

    // Diskte uygun yer bul
    int start_block = find_free_block(0);
    if (start_block == -1) {
        // Disk dolu, defragmentasyon denenir
        if (fs_defragment() < 0) {
            write(STDOUT_FILENO, "Diskte bos alan bulunamadi ve defragmentasyon basarisiz.\n", 58);
            return -1;
        }

        // Defragmentasyon sonrası tekrar deneme
        start_block = find_free_block(0);
        if (start_block == -1) {
            write(STDOUT_FILENO, "Defragmentasyona ragmen diskte yeterli alan bulunamadi.\n", 57);
            return -1;
        }
    }

    // Dosya girdisini doldur
    strncpy(file_table[free_slot].name, filename, FILENAME_LEN);
    file_table[free_slot].size = 0;
    file_table[free_slot].start_block = start_block;
    file_table[free_slot].created_at = time(NULL);
    file_table[free_slot].valid = 1;

    return save_metadata();
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
    write(STDOUT_FILENO, "Dosya bulunamadi: ", 19);
    write(STDOUT_FILENO, filename, strlen(filename));
    write(STDOUT_FILENO, "\n", 1);
    return -1;
}

// Dosya içine yaz
int fs_write(const char* filename, const char* data, int size) {
    // Geçersiz veri kontrolü
    if (data == NULL || size <= 0) {
        write(STDOUT_FILENO, "Yazilacak veri bulunamadi.\n", 28);
        return -1;
    }

    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].valid && strcmp(file_table[i].name, filename) == 0) {
            lseek(disk_fd, file_table[i].start_block, SEEK_SET);
            write(disk_fd, data, size);
            file_table[i].size = size;
            return save_metadata();
        }
    }

    write(STDOUT_FILENO, "Dosya bulunamadi: ", 19);
    write(STDOUT_FILENO, filename, strlen(filename));
    write(STDOUT_FILENO, "\n", 1);
    return -1;
}

// Dosyayı oku
int fs_read(const char* filename, int offset, int size, char* buffer) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].valid && strcmp(file_table[i].name, filename) == 0) {
            if (offset + size > file_table[i].size) {
                write(STDOUT_FILENO, "Okuma dosya boyutunu asiyor.\n", 30);
                return -1;
            }
            lseek(disk_fd, file_table[i].start_block + offset, SEEK_SET);
            return read(disk_fd, buffer, size);
        }
    }
    write(STDOUT_FILENO, "Dosya bulunamadi: ", 19);
    write(STDOUT_FILENO, filename, strlen(filename));
    write(STDOUT_FILENO, "\n", 1);
    return -1;
}

// Tüm dosyaları göster
void fs_ls(bool is_called_from_menu) {
    bool files_exist = false;
    for (int i = 0; i < MAX_FILES; ++i) {
        if (file_table[i].valid) {
            files_exist = true;
            break;
        }
    }

    // Dosya yoksa ve menüden çağrıldıysa mesaj göster
    if (!files_exist) {
        if (is_called_from_menu) write(STDOUT_FILENO, "Diskte dosya bulunamadi.\n", 26);
        return;
    }

    // Dosyalar varsa liste göster
    write(STDOUT_FILENO, "Diskteki Dosyalar:\n", 20);
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
        write(STDOUT_FILENO, "ftruncate islemi basarisiz oldu.\n", 34);
        return -1;
    }
    return save_metadata();
}

// Dosyayı yeniden adlandır (fs_mv bu özelliği zaten içeriyor)
int fs_rename(const char* old_name, const char* new_name) { return fs_mv(old_name, new_name); }

// Dosya varlığını kontrol et
bool fs_exists(const char* filename) {
    for (int i = 0; i < MAX_FILES; ++i) {
        if (file_table[i].valid && strcmp(file_table[i].name, filename) == 0)
            return true;
    }
    return false;
}

// Dosyanın boyutunu bul
int fs_size(const char* filename) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].valid && strcmp(file_table[i].name, filename) == 0) {
            return file_table[i].size;
        }
    }
    write(STDOUT_FILENO, "Dosya bulunamadi: ", 19);
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
    write(STDOUT_FILENO, "Dosya bulunamadi: ", 19);
    write(STDOUT_FILENO, filename, strlen(filename));
    write(STDOUT_FILENO, "\n", 1);
    return -1;
}

// Dosyayı kırp (boyutu küçültmek için)
int fs_truncate(const char* filename, int new_size) {
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].valid && strcmp(file_table[i].name, filename) == 0) {
            if (new_size > file_table[i].size) {
                write(STDOUT_FILENO, "Yeni boyut mevcut dosya boyutundan buyuk.\n", 43);
                return -1;
            }
            file_table[i].size = new_size;
            return save_metadata();
        }
    }
    write(STDOUT_FILENO, "Dosya bulunamadi: ", 19);
    write(STDOUT_FILENO, filename, strlen(filename));
    write(STDOUT_FILENO, "\n", 1);
    return -1;
}

// Dosyayı kopyala
int fs_copy(const char* src, const char* dest) {
    if (!fs_exists(src)) {
        write(STDOUT_FILENO, "Kaynak dosya bulunamadi: ", 26);
        return -1;
    }

    if (fs_exists(dest)) {
        write(STDOUT_FILENO, "Hedef dosya zaten mevcut.\n", 27);
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
        write(STDOUT_FILENO, "Hedef konumda ayni isimde dosya zaten var.\n", 44);
        return -1;
    }

    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].valid && strcmp(file_table[i].name, old_path) == 0) {
            strncpy(file_table[i].name, new_path, FILENAME_LEN);
            return save_metadata();
        }
    }

    write(STDOUT_FILENO, "Dosya bulunamadi: ", 19);
    write(STDOUT_FILENO, old_path, strlen(old_path));
    write(STDOUT_FILENO, "\n", 1);
    return -1;
}

int fs_defragment() {
    int fragmented_count = 0;
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].valid) {
            fragmented_count++;
        }
    }

    if (fragmented_count == 0) {
        write(STDOUT_FILENO, "Diskte dosya bulunmamaktadir.\n", 31);
        return 0;
    }

    // Gecici dosya tablosu oluştur
    FileEntry temp_table[MAX_FILES];
    memset(temp_table, 0, sizeof(temp_table));

    char* buffer = malloc(BLOCK_SIZE);
    if (!buffer) {
        write(STDOUT_FILENO, "Bellek ayirma hatasi.\n", 23);
        return -1;
    }

    // Her dosya sırayla yeni bloklar halinde düzenlenir
    int next_block = METADATA_SIZE;
    int new_index = 0;

    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].valid) {
            lseek(disk_fd, file_table[i].start_block, SEEK_SET);
            char* content = malloc(file_table[i].size);
            if (!content) {
                free(buffer);
                write(STDOUT_FILENO, "Bellek ayirma hatasi.\n", 23);
                return -1;
            }

            read(disk_fd, content, file_table[i].size);

            // Yeni tabloyu güncelle
            temp_table[new_index] = file_table[i];
            temp_table[new_index].start_block = next_block;

            // Dosyayı yeni konumuna yaz
            lseek(disk_fd, next_block, SEEK_SET);
            write(disk_fd, content, file_table[i].size);

            // Bir sonraki bloğa ilerle
            next_block += ((file_table[i].size + BLOCK_SIZE - 1) / BLOCK_SIZE) * BLOCK_SIZE;
            new_index++;

            free(content);
        }
    }

    // Dosya tablosunu güncelle
    memcpy(file_table, temp_table, sizeof(file_table));

    free(buffer);

    int result = save_metadata();
    if (result >= 0) {
        write(STDOUT_FILENO, "Disk alanindaki bosluklar basariyla birlestirildi.\n", 52);
    }

    return result;
}

int fs_check_integrity() {
    int error_count = 0;

    // Dosya tablosunu kontrol et
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].valid) {
            // Başlangıç bloğunun sınırlar içinde olup olmadığı kontrol edilir
            if (file_table[i].start_block < METADATA_SIZE ||
                file_table[i].start_block >= DISK_SIZE) {
                write(STDOUT_FILENO, "Hata: Dosya baslangic blogu disk sinirlarinin disinda: ", 56);
                write(STDOUT_FILENO, file_table[i].name, strlen(file_table[i].name));
                write(STDOUT_FILENO, "\n", 1);
                error_count++;
            }

            // Dosya boyutunun sınırlar içinde olup olmadığı kontrol edilir
            if (file_table[i].size < 0 ||
                file_table[i].start_block + file_table[i].size > DISK_SIZE) {
                write(STDOUT_FILENO, "Hata: Dosya boyutu gecersiz: ", 30);
                write(STDOUT_FILENO, file_table[i].name, strlen(file_table[i].name));
                write(STDOUT_FILENO, "\n", 1);
                error_count++;
            }

            // Dosya isimlerinin geçerli olup olmadığı kontrol edilir
            if (strlen(file_table[i].name) == 0) {
                write(STDOUT_FILENO, "Hata: Gecersiz dosya adi (bos) bulundu.\n", 41);
                error_count++;
            }

            // Dosya bloklarının çakışıp çakışmadığı kontrol edilir
            for (int j = i + 1; j < MAX_FILES; j++) {
                if (file_table[j].valid) {
                    // Blok aralıklarının çakışması kontrolü
                    int start_i = file_table[i].start_block;
                    int end_i = start_i + file_table[i].size;
                    int start_j = file_table[j].start_block;
                    int end_j = start_j + file_table[j].size;

                    if ((start_i <= start_j && start_j < end_i) ||
                        (start_i < end_j && end_j <= end_i) ||
                        (start_j <= start_i && start_i < end_j) ||
                        (start_j < end_i && end_i <= end_j)) {
                        write(STDOUT_FILENO, "Hata: Dosya bloklari cakismasi: ", 33);
                        write(STDOUT_FILENO, file_table[i].name, strlen(file_table[i].name));
                        write(STDOUT_FILENO, " ve ", 4);
                        write(STDOUT_FILENO, file_table[j].name, strlen(file_table[j].name));
                        write(STDOUT_FILENO, "\n", 1);
                        error_count++;
                    }
                }
            }
        }
    }

    if (error_count == 0) {
        write(STDOUT_FILENO, "Dosya sistemi butunlugu kontrol edildi, hata bulunamadi.\n", 58);
        return 0;
    } else {
        char error_msg[64];
        int len = snprintf(error_msg, sizeof(error_msg), "Dosya sisteminde %d hata bulundu.\n", error_count);
        write(STDOUT_FILENO, error_msg, len);
        return error_count;
    }
}

int fs_backup(const char* backup_file) {
    if (!backup_file || strlen(backup_file) == 0) backup_file = "disk.sim.backup"; // Varsayılan yedek dosya adı

    lseek(disk_fd, 0, SEEK_SET);

    int backup_fd = open(backup_file, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (backup_fd < 0) {
        write(STDOUT_FILENO, "Yedek dosyasi olusturulamadi.\n", 31);
        return -1;
    }

    // Disk içeriğini yedek dosyasına kopyala
    char buffer[BLOCK_SIZE];
    int bytes_read;
    int total_bytes = 0;

    lseek(disk_fd, 0, SEEK_SET);

    while ((bytes_read = read(disk_fd, buffer, BLOCK_SIZE)) > 0) {
        if (write(backup_fd, buffer, bytes_read) != bytes_read) {
            write(STDOUT_FILENO, "Yedekleme sirasinda yazma hatasi olustu.\n", 42);
            close(backup_fd);
            return -1;
        }
        total_bytes += bytes_read;
    }

    close(backup_fd);

    if (bytes_read < 0) {
        write(STDOUT_FILENO, "Yedekleme sirasinda okuma hatasi olustu.\n", 42);
        return -1;
    }

    char msg[128];
    int len = snprintf(msg, sizeof(msg), "Disk basariyla \"%s\" dosyasina yedeklendi. (%d bytes)\n", backup_file, total_bytes);
    write(STDOUT_FILENO, msg, len);

    return 0;
}

int fs_restore(const char* backup_file) {
    if (!backup_file || strlen(backup_file) == 0) backup_file = "disk.sim.backup"; // Varsayılan yedek dosya adı

    // Yedek dosyasını aç
    int backup_fd = open(backup_file, O_RDONLY);
    if (backup_fd < 0) {
        write(STDOUT_FILENO, "Yedek dosyasi bulunamadi.\n", 27);
        return -1;
    }

    // Dosya boyutunu al
    struct stat st;
    if (fstat(backup_fd, &st) != 0 || st.st_size > DISK_SIZE) {
        write(STDOUT_FILENO, "Yedek dosyasi gecersiz boyutta.\n", 33);
        close(backup_fd);
        return -1;
    }

    // Disk dosyasını sıfırla
    ftruncate(disk_fd, 0);
    ftruncate(disk_fd, DISK_SIZE);

    // Yedekten geri yükle
    char buffer[BLOCK_SIZE];
    int bytes_read;
    int total_bytes = 0;

    lseek(disk_fd, 0, SEEK_SET);
    lseek(backup_fd, 0, SEEK_SET);

    while ((bytes_read = read(backup_fd, buffer, BLOCK_SIZE)) > 0) {
        if (write(disk_fd, buffer, bytes_read) != bytes_read) {
            write(STDOUT_FILENO, "Geri yukleme sirasinda yazma hatasi olustu.\n", 45);
            close(backup_fd);
            return -1;
        }
        total_bytes += bytes_read;
    }

    close(backup_fd);

    if (bytes_read < 0) {
        write(STDOUT_FILENO, "Geri yukleme sirasinda okuma hatasi olustu.\n", 45);
        return -1;
    }

    // Metadatayı hafızaya yükle
    load_metadata();

    char msg[128];
    int len = snprintf(msg, sizeof(msg), "Disk basariyla \"%s\" dosyasindan geri yuklendi. (%d bytes)\n", backup_file, total_bytes);
    write(STDOUT_FILENO, msg, len);

    return 0;
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

// İki dosyayı karşılaştır
int fs_diff(const char* file1, const char* file2) {
    if (!fs_exists(file1)) {
        write(STDOUT_FILENO, "Birinci dosya bulunamadi: ", 27);
        write(STDOUT_FILENO, file1, strlen(file1));
        write(STDOUT_FILENO, "\n", 1);
        return -1;
    }

    if (!fs_exists(file2)) {
        write(STDOUT_FILENO, "Ikinci dosya bulunamadi: ", 26);
        write(STDOUT_FILENO, file2, strlen(file2));
        write(STDOUT_FILENO, "\n", 1);
        return -1;
    }

    int size1 = fs_size(file1);
    int size2 = fs_size(file2);

    if (size1 < 0 || size2 < 0) {
        write(STDOUT_FILENO, "Dosya boyutu gecersiz.\n", 24);
        return -1;
    }

    if (size1 != size2) {
        if (size1 > size2) {
            char msg[128];
            int len = snprintf(msg, sizeof(msg), "Dosyalar boyut olarak farkli: \"%s\" (%d bytes) \"%s\"'den (%d bytes) daha buyuk.\n", file1, size1, file2, size2);
            write(STDOUT_FILENO, msg, len);
        } else {
            char msg[128];
            int len = snprintf(msg, sizeof(msg), "Dosyalar boyut olarak farkli: \"%s\" (%d bytes) \"%s\"'den (%d bytes) daha buyuk.\n", file2, size2, file1, size1);
            write(STDOUT_FILENO, msg, len);
        }
        return 1;
    }

    char* buf1 = malloc(size1);
    char* buf2 = malloc(size2);

    if (buf1 == NULL || buf2 == NULL) {
        write(STDOUT_FILENO, "Bellek ayirma hatasi.\n", 23);
        if (buf1) free(buf1);
        if (buf2) free(buf2);
        return -1;
    }

    bool file1_read = false;
    bool file2_read = false;

    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].valid && strcmp(file_table[i].name, file1) == 0) {
            lseek(disk_fd, file_table[i].start_block, SEEK_SET);
            read(disk_fd, buf1, size1);
            file1_read = true;
        }
        if (file_table[i].valid && strcmp(file_table[i].name, file2) == 0) {
            lseek(disk_fd, file_table[i].start_block, SEEK_SET);
            read(disk_fd, buf2, size2);
            file2_read = true;
        }
    }

    if (!file1_read || !file2_read) {
        write(STDOUT_FILENO, "Dosya okuma hatasi.\n", 21);
        free(buf1);
        free(buf2);
        return -1;
    }

    int result = memcmp(buf1, buf2, size1);
    if (result == 0) {
        write(STDOUT_FILENO, "Dosyalar ayni.\n", 16);
    } else {
        write(STDOUT_FILENO, "Dosyalar farkli.\n", 18);
    }

    free(buf1);
    free(buf2);
    return result;
}

// Log dosyasını göster
int fs_log() {
    // Yazma için açık olan log dosyasını kapat
    if (log_fd >= 0) {
        close(log_fd);
        log_fd = -1;
    }

    // Log dosyasını okuma için aç
    int read_log_fd = open(LOG_FILE, O_RDONLY);
    if (read_log_fd < 0) {
        write(STDOUT_FILENO, "Log dosyasi bulunamadi veya okunamadi.\n", 40);
        log_init(); // Yazma için yeniden aç
        return -1;
    }

    // Log içeriğini oku ve göster
    char buffer[BLOCK_SIZE];
    int bytes_read;

    write(STDOUT_FILENO, "\n==============DOSYA SISTEMI ISLEM GUNLUGU===============\n\n", 60);

    while ((bytes_read = read(read_log_fd, buffer, BLOCK_SIZE - 1)) > 0) {
        buffer[bytes_read] = '\0';
        write(STDOUT_FILENO, buffer, bytes_read);
    }

    write(STDOUT_FILENO, "\n========================================================\n", 59);

    close(read_log_fd);

    // Log dosyasını yeniden yazma için aç
    log_init();

    return 0;
}

// İşlemi logla
// Log formatı: [ZAMAN] IŞLEM: İŞLEM YAPILAN DOSYA
void log_operation(const char* operation, const char* details) {
    if (log_fd >= 0) {
        time_t now = time(NULL);
        struct tm* time_info = localtime(&now);
        char time_str[32];
        strftime(time_str, sizeof(time_str), "%d-%m-%Y %H:%M:%S", time_info);

        char log_entry[512];
        int len;

        // Detay yoksa sadece işlem loglanır
        if (details == NULL || details[0] == '\0')
            len = snprintf(log_entry, sizeof(log_entry), "[%s] %s\n", time_str, operation);
        else
            len = snprintf(log_entry, sizeof(log_entry), "[%s] %s: %s\n", time_str, operation, details);

        write(log_fd, log_entry, len);
    }
}

static int find_free_block(int required_size) {
    // Kullanılan blokları işaretle
    bool used_blocks[DISK_SIZE / BLOCK_SIZE];
    memset(used_blocks, 0, sizeof(used_blocks));

    // Metadata alanını kullanılıyor olarak işaretle
    int metadata_blocks = METADATA_SIZE / BLOCK_SIZE;
    for (int i = 0; i < metadata_blocks; i++) {
        used_blocks[i] = true;
    }

    // Geçerli dosyaların bloklarını işaretle
    for (int i = 0; i < MAX_FILES; i++) {
        if (file_table[i].valid) {
            int start_block = file_table[i].start_block / BLOCK_SIZE;
            int blocks_count = (file_table[i].size + BLOCK_SIZE - 1) / BLOCK_SIZE;

            for (int j = 0; j < blocks_count; j++) {
                if ((start_block + j) < (DISK_SIZE / BLOCK_SIZE)) {
                    used_blocks[start_block + j] = true;
                }
            }
        }
    }

    // Gerekli blok sayısını hesapla
    int required_blocks = (required_size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (required_blocks == 0) required_blocks = 1; // Minimum 1 blok

    // İlk yeterli boş alanı bul
    int consecutive_free = 0;
    int start_block = -1;

    for (int i = metadata_blocks; i < (DISK_SIZE / BLOCK_SIZE); i++) {
        if (!used_blocks[i]) {
            if (consecutive_free == 0) {
                start_block = i * BLOCK_SIZE;
            }
            consecutive_free++;

            if (consecutive_free >= required_blocks) {
                return start_block;
            }
        } else {
            consecutive_free = 0;
            start_block = -1;
        }
    }

    return -1;
}
