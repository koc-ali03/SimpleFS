#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h> // Terminal ayarları için
#include <unistd.h>

// Giriş bufferını temizlemek için bir fonksiyon
void clear_input_buffer() {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// Enter'a basmadan tek karakter okumak için fonksiyon
int wait_for_user_input() {
    struct termios old_terminal, new_terminal;
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

int main() {
    int choice;
    int is_first_run = 1; // İlk çalışma olup olmadığını kontrol etmek için bayrak
    char input[4];        // Kullanıcı girişini geçici olarak saklamak için

    do {
        if (!is_first_run) { // Eğer ilk çalışma değilse
            printf("\nDevam etmek icin lutfen bir tusa basin...");
            wait_for_user_input(); // Herhangi bir tuşa basılmasını bekle
            printf("\n");
        }

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
        printf("13. Disk uzerindeki bos alanlari birlestir\n");
        printf("14. Diski yedekle\n");
        printf("15. Varolan disk yedegini geri yukle\n");
        printf("16. Cikis\n");
        puts("==============================================");
        printf("Seciminizi girin: ");

        // Kullanıcı girişini metin olarak al
        if (fgets(input, sizeof(input), stdin) == NULL) {
            system("clear");
            return 0;
        }

        // fgets'in sonuna eklediği satır sonu karakterini kaldır
        size_t len = strlen(input);
        int newline_found = 0;

        if (len > 0 && input[len - 1] == '\n') {
            input[len - 1] = '\0';
            newline_found = 1; // Newline bulundu ve kaldırıldı
        }

        // Eğer newline bulunamazsa, giriş bufferında hala karakter var demektir
        if (!newline_found) clear_input_buffer();

        // Giriş sadece sayılardan mı oluşuyor kontrol et
        int is_valid = 1;
        for (int i = 0; input[i] != '\0'; i++) {
            if (input[i] < '0' || input[i] > '9') {
                is_valid = 0;
                break;
            }
        }

        if (!is_valid) {
            system("clear");
            printf("Gecersiz secim. Lütfen sadece bir sayi girin.\n");
            choice = 0;
            is_first_run = 0;
            continue;
        }

        // Geçerli bir sayı olduğu için şimdi int'e çevir
        choice = atoi(input);
        system("clear");
        switch (choice) {
            case 1:
                printf("Dosya olusturma secildi.\n");
                break;
            case 2:
                printf("Dosya silme secildi.\n");
                break;
            case 3:
                printf("Dosyaya veri yazma secildi.\n");
                break;
            case 4:
                printf("Dosyadan veri okuma secildi.\n");
                break;
            case 5:
                printf("Dosyaları listeleme secildi.\n");
                break;
            case 6:
                printf("Diske format atma secildi.\n");
                break;
            case 7:
                printf("Dosya ismini degistirme secildi.\n");
                break;
            case 8:
                printf("Dosya boyutunu gosterme secildi.\n");
                break;
            case 9:
                printf("Dosya sonuna veri ekleme secildi.\n");
                break;
            case 10:
                printf("Dosya kirpma secildi\n");
                break;
            case 11:
                printf("Dosya kopyalama secildi.\n");
                break;
            case 12:
                printf("Dosya tasima secildi\n");
                break;
            case 13:
                printf("Disk uzerindeki bos alanlari birlestirme secildi.\n");
                break;
            case 14:
                printf("Disk yedekleme secildi.\n");
                break;
            case 15:
                printf("Disk yedegini geri yukleme secildi.\n");
                break;
            case 16:
                printf("Cikis yapiliyor...\n");
                break;
            default:
                printf("Gecersiz secim. Lutfen (1-16) arasi bir secim yapin.\n");
                break;
        }
        is_first_run = 0;
    } while (choice != 16);

    return 0;
}
