#include <iostream>
#include <vector>
#include <cstdint>
#include <string>
#include <iomanip>

using namespace std;

// =====================================================================
// 1. CẤU TRÚC DỮ LIỆU TỔNG THỂ 
// =====================================================================
struct SosemanukState {
    uint32_t s[10];  // 10 thanh ghi 32-bit (s0 đến s9) của LFSR
    uint32_t r1, r2; // 2 biến trạng thái 32-bit của FSM
};

class SosemanukCipher {
protected:
    SosemanukState state;
    uint32_t sk[100]; // 100 Khóa con (Subkeys) sinh ra từ thuật toán Serpent

public:
    // --- Ổ CẮM CHO THÀNH VIÊN 2 (Khởi tạo Key & IV) ---
    void KeySetup(const vector<uint8_t>& key) {
        // Thành viên 2 sẽ viết thuật toán Serpent Key Schedule vào đây
    }

    void IVSetup(const vector<uint8_t>& iv) {
        // Thành viên 2 sẽ viết thuật toán nạp IV vào state.s và state.r1, r2 vào đây
    }
};

// =====================================================================
// 2. HÀM MÃ HÓA / GIẢI MÃ LÕI 
// =====================================================================
class SosemanukCore : public SosemanukCipher {
public:
    // --- Ổ CẮM CHO THÀNH VIÊN 4 & 5 (Sinh dòng khóa) ---
    void GenerateKeystreamBlock(uint32_t ks[4]) {
        // Thành viên 4 & 5 sẽ viết logic LFSR và FSM để nhả ra 4 word (16 byte) vào mảng ks
    }

    // ---> ĐÂY LÀ ĐOẠN CODE "ĂN TIỀN" CỦA BẠN <---
    // Hàm thực hiện XOR bản rõ với dòng khóa (Dùng chung cho cả Mã hóa và Giải mã)
    void ProcessData(vector<uint8_t>& data) {
        uint32_t keystream[4]; // Mảng chứa 16 byte Keystream sinh ra mỗi vòng
        size_t i = 0;
        
        while (i < data.size()) {
            // Bước 1: Gọi hàm của TV 4&5 để lấy 16 byte Keystream mới
            GenerateKeystreamBlock(keystream);
            
            // Bước 2: Kỹ thuật ép kiểu con trỏ (Giải thích chi tiết bên dưới)
            uint8_t* ks_bytes = reinterpret_cast<uint8_t*>(keystream);
            
            // Bước 3: Vòng lặp XOR tốc độ cao (Xử lý tối đa 16 byte mỗi nhịp)
            for (int j = 0; j < 16 && i < data.size(); ++j, ++i) {
                data[i] ^= ks_bytes[j]; // Phép toán XOR sinh ra bản mã
            }
        }
    }
};

// =====================================================================
// HÀM TIỆN ÍCH HIỂN THỊ 
// =====================================================================
class UIHelper {
public:
    static void PrintHexDump(const vector<uint8_t>& data) {
        cout << "\n--- HEX DUMP (" << data.size() << " bytes) ---\n";
        for (size_t i = 0; i < data.size(); i++) {
            cout << hex << uppercase << setw(2) << setfill('0') << (int)data[i] << " ";
            if ((i + 1) % 16 == 0) cout << endl;
        }
        cout << dec << endl; 
    }
};

// =====================================================================
// 3. HÀM MAIN() - QUẢN LÝ LUỒNG HỆ THỐNG 
// =====================================================================
// =====================================================================
// 3. HÀM MAIN() - QUẢN LÝ LUỒNG HỆ THỐNG (Do bạn điều phối)
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
            // PHẦN CODE CỦA BẠN NẰM Ở ĐÂY
            // =======================================================
            printf("\nNhap doan van ban ban muon ma hoa: ");
            
            // Lời khuyên nhỏ: Khi dùng scanf("%d") ở trên, ký tự Enter (\n) vẫn còn kẹt trong bộ đệm.
            // Bạn nên dùng hàm getchar() hoặc fflush(stdin) để dọn dẹp bộ đệm trước khi đọc chuỗi nhé!
            
            /* Viết code nhập chuỗi (vd: dùng fgets) và ghi ra file input.txt tại đây */
            
            
            // =======================================================
            printf(">> [OK] Da ghi noi dung moi vao file input.txt!\n");
            
        } else if (choice == 4) {
            // Khởi tạo lại hệ thống trước khi đo hiệu năng
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