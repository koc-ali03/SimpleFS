#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h> // Terminal ayarları için
#include <unistd.h>
#include "fs.h"

void display_menu();
int get_user_choice(char input[], int input_size);
bool get_filename(const char* prompt, char* filename);
int wait_for_user_input();
void create_file(char* filename);
void delete_file(char* filename);
void write_to_file(char* filename, char* data);
void read_file(char* filename, char* input);
void read_file_partial(const char* filename, int file_size, char* input);
void list_files();
void format_disk();
void rename_file(char* filename, char* filename2);
void show_file_size(char* filename);
void append_to_file(char* filename, char* data);
void truncate_file(char* filename, char* input);
void copy_file(char* filename, char* filename2);
void move_file(char* filename, char* filename2);
void compare_files(char* filename, char* filename2);
void defragment_disk();
void backup_disk(char* filename);
void restore_disk(char* filename);
void clear_input_buffer();

int main() {
    int choice;
    char input[4];
    bool is_first_run = 1; // İlk çalışma olup olmadığını kontrol etmek için boolean
    char filename[FILENAME_LEN];
    char filename2[FILENAME_LEN];
    char data[BLOCK_SIZE]; // Veri yazmak için kullanılacak buffer

    do {
        if (!is_first_run) { // Eğer ilk çalışma değilse
            printf("\nDevam etmek icin lutfen bir tusa basin...");
            wait_for_user_input(); // Herhangi bir tuşa basılmasını bekle
            printf("\n");
        } else if (fs_init() == false || log_init() == false) {
            return -1;
        }

        display_menu();
        
        choice = get_user_choice(input,sizeof(input));
        if (choice == -1) return 0;
        if (choice == 0) {
            is_first_run = 0;
            continue;
        }
        system("clear");

        switch (choice) {
            case 1:
                create_file(filename);
                break;
            case 2:
                delete_file(filename);
                break;
            case 3:
                write_to_file(filename, data);
                break;
            case 4:
                read_file(filename, input);
                break;
            case 5:
                list_files();
                break;
            case 6:
                format_disk();
                break;
            case 7:
                rename_file(filename, filename2);
                break;
            case 8:
                show_file_size(filename);
                break;
            case 9:
                append_to_file(filename, data);
                break;
            case 10:
                truncate_file(filename, input);
                break;
            case 11:
                copy_file(filename, filename2);
                break;
            case 12:
                move_file(filename, filename2);
                break;
            case 13:
                compare_files(filename, filename2);
                break;
            case 14:
                defragment_disk();
                break;
            case 15:
                backup_disk(filename);
                break;
            case 16:
                restore_disk(filename);
                break;
            case 17:
                fs_log();
                break;
            case 18:
                printf("Cikis yapiliyor...\n");
                log_operation("CIKIS_YAPILDI", NULL);
                break;
            default:
                printf("Gecersiz secim. Lutfen (1-18) arasi bir secim yapin.\n");
                break;
        }
        is_first_run = 0;
    } while (choice != 18);
    return 0;
}

void display_menu() {
    system("clear");
    printf("             Dosya Sistemi Menusu\n");
    puts("==============================================");
    printf(" 1. Dosya olustur\n");
    printf(" 2. Dosya sil\n");
    printf(" 3. Dosyaya veri yaz\n");
    printf(" 4. Dosyadan veri oku\n");
    printf(" 5. Dosyalari listele\n");
    printf(" 6. Diske format at\n");
    printf(" 7. Dosya ismini degistir\n");
    printf(" 8. Dosya boyutunu goster\n");
    printf(" 9. Dosya sonuna veri ekle\n");
    printf("10. Dosyayi kirp\n");
    printf("11. Dosya kopyala\n");
    printf("12. Dosyayi tasi\n");
    printf("13. Dosyalari karsilastir\n");
    printf("14. Disk uzerindeki bos alanlari birlestir\n");
    printf("15. Diski yedekle\n");
    printf("16. Varolan disk yedegini geri yukle\n");
    printf("17. Loglari Goruntule\n");
    printf("18. Cikis\n");
    puts("==============================================");
    printf("Seciminizi girin(1-18): ");
}

int get_user_choice(char input[], int input_size) {
    // Kullanıcı girişini metin olarak al
    if (fgets(input, input_size, stdin) == NULL) {
        system("clear");
        return -1;  // Special code for exit
    }

    // fgets'in sonuna eklediği satır sonu karakterini kaldır
    size_t len = strlen(input);
    bool newline_found = 0;

    if (len > 0 && input[len - 1] == '\n') {
        input[len - 1] = '\0';
        newline_found = 1; // Newline bulundu ve kaldırıldı
    }

    // Eğer newline bulunamazsa, giriş bufferında hala karakter var demektir
    if (!newline_found) clear_input_buffer();

    // Giriş sadece sayılardan mı oluşuyor kontrol et
    bool is_valid = 1;
    for (int i = 0; input[i] != '\0'; i++) {
        if (input[i] < '0' || input[i] > '9') {
            is_valid = 0;
            break;
        }
    }

    if (!is_valid) {
        system("clear");
        printf("Gecersiz secim. Lütfen sadece bir sayi girin.\n");
        return 0;
    }

    // Geçerli bir sayı olduğu için şimdi int'e çevir
    return atoi(input);
}

// Kullanıcıdan dosya adını alır ve geçerli olup olmadığını kontrol eder
bool get_filename(const char* prompt, char* filename) {
    printf("%s", prompt);

    if (fgets(filename, FILENAME_LEN, stdin) == NULL) {
        printf("Dosya adi okunamadi!\n");
        return 0;
    }

    size_t len = strlen(filename);
    if (len > 0 && filename[len - 1] == '\n') filename[len - 1] = '\0';

    if (strlen(filename) == 0) {
        printf("Gecersiz dosya adi! Dosya adi bos olamaz.\n");
        return 0;
    }

    return 1;
}

// Enter'a basmadan tek karakter okumak için fonksiyon
int wait_for_user_input() {
    struct termios old_terminal;
    struct termios new_terminal;
    int ch;
    tcgetattr(STDIN_FILENO, &old_terminal); // Mevcut terminal ayarlarını al
    new_terminal = old_terminal;
    new_terminal.c_lflag &= ~(ICANON | ECHO);
    // ICANON: Canonical (standart, satır tabanlı) giriş modunu devre dışı bırak.
    // ECHO: Basılan karakterlerin terminale otomatik olarak yazdırılmasını engelle.
    tcsetattr(STDIN_FILENO, TCSANOW, &new_terminal); // Yeni ayarları uygula
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &old_terminal); // Eski terminal ayarlarını geri yükle
    return ch;
}

void create_file(char* filename) {
    printf("Dosya olusturma secildi.\n");
    fs_ls(false);

    if (!get_filename("Dosya adini girin: ", filename)) return;

    if (fs_create(filename) >= 0) {
        log_operation("DOSYA_OLUSTURULDU", filename);
        printf("\"%s\" dosyasi basariyla olusturuldu.\n", filename);
    } else {
        printf("\"%s\" dosyasi olusturulamadi!\n", filename);
    }
}

void delete_file(char* filename) {
    printf("Dosya silme secildi.\n");
    fs_ls(false);

    if (!get_filename("Silinecek dosya adini girin: ", filename)) return;
    if (fs_delete(filename) >= 0) {
        log_operation("DOSYA_SILINDI", filename);
        printf("\"%s\" dosyasi basariyla silindi.\n", filename);
    } else {
        printf("\"%s\" dosyasi silinemedi!\n", filename);
    }
}

void write_to_file(char* filename, char* data) {
    printf("Dosyaya veri yazma secildi.\n");
    fs_ls(false);

    if (!get_filename("Veri yazilacak dosya adini girin: ", filename)) return;

    printf("Yazilacak veriyi girin: ");
    if (fgets(data, BLOCK_SIZE, stdin) == NULL) {
        printf("Veri okunamadi!\n");
        return;
    }
    size_t data_len = strlen(data);
    if (data_len > 0 && data[data_len - 1] == '\n') data[data_len - 1] = '\0';

    if (fs_write(filename, data, (int)strlen(data)) >= 0) {
        log_operation("DOSYAYA_VERI_YAZILDI", filename);
        printf("\"%s\" dosyasina veri basariyla yazildi.\n", filename);
    } else {
        printf("\"%s\" dosyasina veri yazilamadi!\n", filename);
    }
}

void read_file(char* filename, char* input) {
    printf("Dosyadan veri okuma secildi.\n");
    fs_ls(false);

    if (!get_filename("Okunacak dosya adini girin: ", filename)) return;

    int file_size = fs_size(filename);
    if (file_size < 0) {
        printf("\"%s\" dosyasi bulunamadi!\n", filename);
        return;
    }

    printf("Okuma secenegi:\n");
    printf("1. Tum dosyayi oku\n");
    printf("2. Belirli bir bolumu oku\n");
    printf("Seciminiz (1-2): ");

    if (fgets(input, 4, stdin) == NULL) {
        printf("Secim okunamadi!\n");
        return;
    }

    int read_choice = atoi(input);

    if (read_choice == 1) {
        // Tüm dosyayı oku
        if (fs_cat(filename) >= 0) {
            log_operation("DOSYADAN_VERI_OKUNDU", filename);
        } else {
            printf("\"%s\" dosyasindan veri okunamadi!\n", filename);
        }
    } else if (read_choice == 2) {
        read_file_partial(filename, file_size, input);
    } else {
        printf("Gecersiz secim!\n");
    }
}

void read_file_partial(const char* filename, int file_size, char* input) {
    char buffer[BLOCK_SIZE] = {0};
    printf("Okuma baslangic pozisyonu (0-%d): ", file_size - 1);
    if (fgets(input, 4, stdin) == NULL) {
        printf("Pozisyon okunamadi!\n");
        return;
    }
    int offset = atoi(input);

    printf("Kac byte okumak istiyorsunuz: ");
    if (fgets(input, 4, stdin) == NULL) {
        printf("Okunacak boyut okunamadi!\n");
        return;
    }
    int read_size = atoi(input);

    // Geçerli değer kontrolü
    if (offset < 0 || read_size <= 0 || offset + read_size > file_size) {
        printf("Gecersiz okuma parametreleri!\n");
        return;
    }

    if (fs_read(filename, offset, read_size, buffer) >= 0) {
        log_operation("DOSYADAN_VERI_OKUNDU", filename);
        printf("\"%s\" dosyasindan okunan veri (%d byte):\n", filename, read_size);
        printf("-------------------------------------------\n");
        for (int i = 0; i < read_size; i++) {
            putchar(buffer[i]);
        }
        printf("\n-------------------------------------------\n");
    } else {
        printf("\"%s\" dosyasindan veri okunamadi!\n", filename);
    }
}

void list_files() {
    printf("Dosyaları listeleme secildi.\n");
    log_operation("DOSYALAR_LISTELENDI", NULL);
    fs_ls(true);
}

void format_disk() {
    printf("Diske format atma secildi.\n");
    if (fs_format() >= 0) {
        log_operation("DISK_FORMATLANDI", NULL);
        printf("Disk basariyla formatlandi.\n");
    } else {
        printf("Disk formatlama basarisiz oldu!\n");
    }
}

void rename_file(char* filename, char* filename2) {
    printf("Dosya ismini degistirme secildi.\n");
    fs_ls(false);

    if (!get_filename("Eski dosya adini girin: ", filename)) return;
    if (!get_filename("Yeni dosya adini girin: ", filename2)) return;
    if (fs_rename(filename, filename2) >= 0) {
        log_operation("DOSYA_ISMI_DEGISTIRILDI", filename);
        printf("\"%s\" dosyasinin ismi \"%s\" olarak basariyla degistirildi.\n", filename, filename2);
    } else {
        printf("\"%s\" dosyasinin ismi degistirilemedi!\n", filename);
    }
}

void show_file_size(char* filename) {
    printf("Dosya boyutunu gosterme secildi.\n");
    fs_ls(false);

    if (!get_filename("Boyutunu gormek istediginiz dosya adini girin: ", filename)) return;
    int size = fs_size(filename);
    if (size >= 0) {
        log_operation("DOSYA_BOYUTU_GOSTERILDI", filename);
        printf("\"%s\" dosyasinin boyutu: %d byte\n", filename, size);
    } else {
        printf("\"%s\" dosyasi bulunamadi!\n", filename);
    }
}

void append_to_file(char* filename, char* data) {
    printf("Dosya sonuna veri ekleme secildi.\n");
    fs_ls(false);

    if (!get_filename("Veri eklenecek dosya adini girin: ", filename)) return;

    printf("Eklenecek veriyi girin: ");
    if (fgets(data, BLOCK_SIZE, stdin) == NULL) {
        printf("Veri okunamadi!\n");
        return;
    }

    size_t data_len = strlen(data);
    if (data_len > 0 && data[data_len - 1] == '\n') data[data_len - 1] = '\0';

    if (fs_append(filename, data, (int)strlen(data)) >= 0) {
        log_operation("DOSYAYA_VERI_EKLENDI", filename);
        printf("\"%s\" dosyasina veri basariyla eklendi.\n", filename);
    } else {
        printf("\"%s\" dosyasina veri eklenemedi!\n", filename);
    }
}

void truncate_file(char* filename, char* input) {
    printf("Dosya kirpma secildi\n");
    fs_ls(false);

    if (!get_filename("Kirpma yapilacak dosya adini girin: ", filename)) return;
    printf("Yeni boyutu girin: ");
    if (fgets(input, 4, stdin) == NULL) {
        printf("Yeni boyut okunamadi!\n");
        return;
    }
    int new_size = atoi(input);
    if (fs_truncate(filename, new_size) >= 0) {
        log_operation("DOSYA_KIRPILDI", filename);
        printf("\"%s\" dosyasi basariyla %d byte'a kirpildi.\n", filename, new_size);
    } else {
        printf("\"%s\" dosyasi kirpilamadi!\n", filename);
    }
}

void copy_file(char* filename, char* filename2) {
    printf("Dosya kopyalama secildi.\n");
    fs_ls(false);

    if (!get_filename("Kopyalanacak dosya adini girin: ", filename)) return;
    if (!get_filename("Yeni dosya adini girin: ", filename2)) return;
    if (fs_copy(filename, filename2) >= 0) {
        log_operation("DOSYA_KOPYALANDI", filename);
        printf("\"%s\" dosyasi \"%s\" olarak basariyla kopyalandi.\n", filename, filename2);
    } else {
        printf("\"%s\" dosyasi kopyalanamadi!\n", filename);
    }
}

void move_file(char* filename, char* filename2) {
    printf("Dosya tasima secildi.\n");

    if (!get_filename("Tasinacak dosya adini girin: ", filename)) return;
    if (!get_filename("Hedef dosya adini girin: ", filename2)) return;

    if (fs_mv(filename, filename2) >= 0) {
        log_operation("DOSYA_TASINDI", filename);
        printf("\"%s\" dosyasi \"%s\" olarak basariyla tasindi.\n", filename, filename2);
    } else {
        printf("\"%s\" dosyasi tasinamadi!\n", filename);
    }
}

void compare_files(char* filename, char* filename2) {
    printf("Dosya karsilastirma secildi.\n");
    fs_ls(false);

    if (!get_filename("Birinci dosya adini girin: ", filename)) return;
    if (!get_filename("Ikinci dosya adini girin: ", filename2)) return;

    int result = fs_diff(filename, filename2);
    if (result >= 0) {
        char details[FILENAME_LEN * 2 + 5];
        snprintf(details, sizeof(details), "%s ve %s", filename, filename2);
        log_operation("DOSYALAR_KARSILASTIRILDI", details);
    } else {
        printf("Dosya karsilastirma islemi basarisiz oldu!\n");
    }
}

void defragment_disk() {
    printf("Disk uzerindeki bos alanlari birlestirme secildi.\n");
    if (!(fs_defragment() >= 0)) {
        printf("Disk uzerindeki bos alanlar birlestirilemedi!\n");
    } else {
        log_operation("DISK_BIRLESTIRILDI", NULL);
    }
}

void backup_disk(char* filename) {
    printf("Disk yedekleme secildi.\n");
    if (!get_filename("Yedek dosya adini girin: ", filename)) return;

    if (!(fs_backup(filename) >= 0)) {
        printf("Disk yedeklenemedi!\n");
    } else {
        log_operation("DISK_YEDEKLENDI", NULL);
    }
}

void restore_disk(char* filename) {
    printf("Disk yedegini geri yukleme secildi.\n");
    if (!get_filename("Geri yuklenecek yedek dosya adini girin: ", filename)) return;
    if (!(fs_restore(filename) >= 0)) {
        printf("Disk geri yuklenemedi!\n");
    } else {
        log_operation("DISK_GERI_YUKLENDI", NULL);
    }
}

// Giriş bufferını temizlemek için bir fonksiyon
void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}
