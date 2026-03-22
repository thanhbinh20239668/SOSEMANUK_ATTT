#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

// =====================================================================
// 1. CẤU TRÚC DỮ LIỆU TỔNG THỂ 
// =====================================================================
typedef struct {
    uint32_t s[10];   // 10 thanh ghi 32-bit (s0 đến s9) của LFSR
    uint32_t r1, r2;  // 2 biến trạng thái 32-bit của FSM
    uint32_t sk[100]; // 100 Khóa con (Subkeys) sinh ra từ thuật toán Serpent
} SosemanukCtx;

// =====================================================================
// CÁC "Ổ CẮM" CHỜ CÁC THÀNH VIÊN KHÁC
// =====================================================================
// --- Ổ CẮM CHO THÀNH VIÊN 2 (Khởi tạo Key & IV) ---
void Sosemanuk_KeySetup(SosemanukCtx* ctx, const uint8_t* key) {
    // Thành viên 2 sẽ viết thuật toán Serpent Key Schedule vào đây
}

void Sosemanuk_IVSetup(SosemanukCtx* ctx, const uint8_t* iv) {
    // Thành viên 2 sẽ viết thuật toán nạp IV vào ctx->s và ctx->r1, r2 vào đây
}

// --- Ổ CẮM CHO THÀNH VIÊN 4 & 5 (Sinh dòng khóa) ---
//Ham Kiem tra mang khong rong
int Is_LFSR_Ready(SosemanukCtx* ctx) {
    for (int i = 0; i < 10; i++) {
        if (ctx->s[i] != 0) return 1;
    }
    return 0;
}
//Ham tinh toan phan hoi 
uint32_t Calculate_LFSR_Feedback(uint32_t s0, uint32_t s3, uint32_t s9) {
    return (s0 << 1) ^ (s3 >> 1) ^ s9;
}

void Sosemanuk_GenerateKeystreamBlock(SosemanukCtx* ctx, uint32_t ks[4]) {
    // Thành viên 4 & 5 sẽ viết logic LFSR và FSM để nhả ra 4 word (16 byte) vào mảng ks
    // Tạm thời gán giá trị giả lập để code không bị lỗi khi test
    ks[0] = 0xAAAAAAAA; ks[1] = 0xBBBBBBBB; ks[2] = 0xCCCCCCCC; ks[3] = 0xDDDDDDDD;
}

// =====================================================================
// 2. HÀM MÃ HÓA / GIẢI MÃ LÕI 
// =====================================================================
void Sosemanuk_ProcessData(SosemanukCtx* ctx, uint8_t* data, size_t length) {
    uint32_t keystream[4]; // Mảng chứa 16 byte Keystream sinh ra mỗi vòng
    size_t i = 0;
    
    while (i < length) {
        // Bước 1: Gọi hàm lấy 16 byte Keystream mới
        Sosemanuk_GenerateKeystreamBlock(ctx, keystream);
        
        // Bước 2: Kỹ thuật ép kiểu con trỏ trong C
        uint8_t* ks_bytes = (uint8_t*)keystream;
        
        // Bước 3: Vòng lặp XOR tốc độ cao
        for (int j = 0; j < 16 && i < length; ++j, ++i) {
            data[i] ^= ks_bytes[j]; 
        }
    }
}

// =====================================================================
// HÀM TIỆN ÍCH HIỂN THỊ 
// =====================================================================
void UI_PrintHexDump(const uint8_t* data, size_t length) {
    printf("\n--- HEX DUMP (%zu bytes) ---\n", length);
    for (size_t i = 0; i < length; i++) {
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0) printf("\n");
    }
    printf("\n");
}

// =====================================================================
// 3. HÀM MAIN() - QUẢN LÝ LUỒNG HỆ THỐNG
// =====================================================================
int main() {
    SosemanukCtx cipher;
    
    // Khởi tạo mảng tĩnh 16 byte cho Key và IV trong C
    uint8_t key[16] = {0xAB, 0x01, 0x02}; 
    uint8_t iv[16]  = {0xCD, 0x03, 0x04}; 

    int choice;
    do {
        printf("\n=========================================\n");
        printf("       SOSEMANUK STREAM CIPHER DEMO      \n");
        printf("=========================================\n");
        printf("1. Ma hoa file\n");
        printf("2. Giai ma file\n");
        printf("3. Nhap van ban moi (Ghi de vao input.txt)\n");
        printf("4. Do hieu nang thuat toan (Benchmark 1MB)\n");
        printf("5. Xem file ma hoa duoi dang Hex (Hex Dump)\n");
        printf("0. Thoat chuong trinh\n");
        printf("-----------------------------------------\n");
        printf("Nhap lua chon: ");
        scanf("%d", &choice);

        if (choice == 1) {
            Sosemanuk_KeySetup(&cipher, key); 
            Sosemanuk_IVSetup(&cipher, iv);
            // TODO: Gọi hàm đọc file -> Gọi Sosemanuk_ProcessData -> Gọi hàm ghi file
            printf(">> Dang xu ly ma hoa...\n");
            
        } else if (choice == 2) {
            Sosemanuk_KeySetup(&cipher, key); 
            Sosemanuk_IVSetup(&cipher, iv); 
            // TODO: Gọi hàm đọc file -> Gọi Sosemanuk_ProcessData -> Gọi hàm ghi file
            printf(">> Dang xu ly giai ma...\n");
            
        } else if (choice == 3) {
            // =======================================================
            // CHỨC NĂNG 3: NHẬP VĂN BẢN VÀ GHI RA FILE
            // =======================================================
            printf("\nNhap doan van ban ban muon ma hoa: ");
            
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {} // Dọn bộ đệm

            char inputText[1024];
            if (fgets(inputText, sizeof(inputText), stdin) != NULL) {
                
                size_t len = strlen(inputText);
                if (len > 0 && inputText[len - 1] == '\n') {
                    inputText[len - 1] = '\0';
                }

                FILE *file = fopen("input.txt", "w");
                if (file != NULL) {
                    fputs(inputText, file); 
                    fclose(file);           
                    printf(">> [OK] Da ghi noi dung \"%s\" vao file input.txt!\n", inputText);
                } else {
                    printf(">> [Loi] Khong the tao hoac mo file input.txt!\n");
                }
            }
            
        } else if (choice == 4) {
            Sosemanuk_KeySetup(&cipher, key); 
            Sosemanuk_IVSetup(&cipher, iv);
            
            printf("\n>> Dang chay Benchmark... \n");
            // TODO: Chỗ này bạn sẽ gọi hàm do Thành viên Test (Benchmark) viết
            
        } else if (choice == 5) {
            // Giả lập hiển thị Hex Dump
            uint8_t dummy_data[] = {0xDE, 0xAD, 0xBE, 0xEF, 0x01, 0x02, 0x03};
            UI_PrintHexDump(dummy_data, sizeof(dummy_data));
            
        } else if (choice == 0) {
            printf(">> Thoat chuong trinh.\n");
        } else {
            printf(">> Lua chon khong hop le!\n");
        }
    } while (choice != 0);

    return 0;
}