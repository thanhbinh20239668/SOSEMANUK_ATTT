#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
const char *filename = "input.txt";
const char *filename_out = "output.txt";
// Hàm đọc file trả về mảng uint8_t* và ghi kích thước vào out_size
uint8_t *read_file_simple(const char *filename, size_t *out_size) {
  FILE *f = fopen(filename, "rb");
  if (!f)
    return NULL;

  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  rewind(f);

  uint8_t *buffer = (uint8_t *)malloc(size);
  if (buffer) {
    fread(buffer, 1, size, f);
    *out_size = size; // Lưu lại kích thước chuẩn
  }

  fclose(f);
  return buffer;
}

// Hàm ghi file nhị phân
void write_file_simple(const char *filename_out, uint8_t *data, size_t size) {
  if (data == NULL || size == 0)
    return;

  FILE *f = fopen(filename_out, "wb");
  if (f) {
    fwrite(data, 1, size, f);
    fclose(f);
  }
}

// Macro xoay trái bit (Rotate Left) để dùng cho hàm Trans
#define ROTL32(v, n) (((v) << (n)) | ((v) >> (32 - (n))))

// Hàm Trans: Nhân giá trị với hằng số và xoay bit để khuếch tán dữ liệu
uint32_t Sosemanuk_Trans(uint32_t x) { return ROTL32(x * 0x54655307, 7); }

// =====================================================================
// 1. CẤU TRÚC DỮ LIỆU TỔNG THỂ
// =====================================================================
typedef struct {
  uint32_t s[10];   // 10 thanh ghi 32-bit (s0 đến s9) của LFSR
  uint32_t r1, r2;  // 2 biến trạng thái 32-bit của FSM
  uint32_t sk[100]; // 100 Khóa con (Subkeys) sinh ra từ thuật toán Serpent
} SosemanukCtx;
#define PHI 0x9E3779B9 // Hằng số Tỷ lệ vàng

// 8 Bảng S-Box Hệ 16 của Serpent
static const uint8_t SBOX[8][16] = {
    {3, 8, 15, 1, 10, 6, 5, 11, 14, 13, 4, 2, 7, 0, 9, 12},
    {15, 12, 2, 7, 9, 0, 5, 10, 1, 11, 14, 8, 6, 13, 3, 4},
    {8, 6, 7, 9, 3, 12, 10, 15, 13, 1, 14, 4, 0, 11, 5, 2},
    {0, 15, 11, 8, 12, 9, 6, 3, 13, 1, 2, 4, 10, 7, 5, 14},
    {1, 15, 8, 3, 12, 0, 11, 6, 2, 5, 4, 10, 9, 14, 7, 13},
    {15, 5, 2, 11, 4, 10, 9, 12, 0, 3, 14, 8, 13, 6, 7, 1},
    {7, 2, 12, 5, 8, 4, 6, 11, 14, 9, 1, 15, 13, 3, 10, 0},
    {1, 13, 15, 0, 14, 8, 2, 11, 7, 4, 12, 10, 9, 3, 5, 6}};

// Hàm S-Box dùng bảng tra (Lookup Table) tác động lên 4 từ 32-bit
void Serpent_Sbox(int box_idx, uint32_t *x0, uint32_t *x1, uint32_t *x2,
                  uint32_t *x3) {
  uint32_t y0 = 0, y1 = 0, y2 = 0, y3 = 0;
  for (int bit = 0; bit < 32; bit++) {
    uint8_t input = (((*x0 >> bit) & 1)) | (((*x1 >> bit) & 1) << 1) |
                    (((*x2 >> bit) & 1) << 2) | (((*x3 >> bit) & 1) << 3);
    uint8_t out = SBOX[box_idx][input];
    y0 |= ((out) & 1) << bit;
    y1 |= ((out >> 1) & 1) << bit;
    y2 |= ((out >> 2) & 1) << bit;
    y3 |= ((out >> 3) & 1) << bit;
  }
  *x0 = y0;
  *x1 = y1;
  *x2 = y2;
  *x3 = y3;
}

// Máy Trộn Tuyến tính (Linear Transformation)
void Serpent_LT(uint32_t *x0, uint32_t *x1, uint32_t *x2, uint32_t *x3) {
  *x0 = ROTL32(*x0, 13);
  *x2 = ROTL32(*x2, 3);
  *x1 = *x1 ^ *x0 ^ *x2;
  *x3 = *x3 ^ *x2 ^ (*x0 << 3);
  *x1 = ROTL32(*x1, 1);
  *x3 = ROTL32(*x3, 7);
  *x0 = *x0 ^ *x1 ^ *x3;
  *x2 = *x2 ^ *x3 ^ (*x1 << 7);
  *x0 = ROTL32(*x0, 5);
  *x2 = ROTL32(*x2, 22);
}

// Khởi tạo Key & IV
void Sosemanuk_KeySetup(SosemanukCtx *ctx, const uint8_t *key) {
  uint32_t w[140];
  uint8_t kList[32] = {0}; // Đệm 32 byte trống chuẩn
  for (size_t i = 0; i < 16; i++)
    kList[i] = key[i]; // Giả định Khóa nhập 128-bit

  // 1. Đệm Khóa chuẩn cực đoan lên 256-bit
  w[0] = (kList[0] << 24) | (kList[1] << 16) | (kList[2] << 8) | kList[3];
  w[1] = (kList[4] << 24) | (kList[5] << 16) | (kList[6] << 8) | kList[7];
  w[2] = (kList[8] << 24) | (kList[9] << 16) | (kList[10] << 8) | kList[11];
  w[3] = (kList[12] << 24) | (kList[13] << 16) | (kList[14] << 8) | kList[15];
  w[4] = 0x80000000; // Bit đệm đỉnh, còn lại toàn 0
  w[5] = 0;
  w[6] = 0;
  w[7] = 0;

  // 2. Khai triển 132 Khóa nháp qua Tỷ Lệ Vàng (PHI)
  for (int i = 8; i < 108; i++) {
    uint32_t tmp = w[i - 8] ^ w[i - 5] ^ w[i - 3] ^ w[i - 1] ^ PHI ^ (i - 8);
    w[i] = ROTL32(tmp, 11);
  }

  // 3. Nghiền Khóa qua S-BOX để ra 100 Subkeys Chính thức
  for (int i = 0; i < 25; i++) {
    uint32_t x0 = w[8 + i * 4], x1 = w[9 + i * 4], x2 = w[10 + i * 4],
             x3 = w[11 + i * 4];
    int sbox_id = (3 - i) % 8; // S-BOX chạy giật lùi S3, S2, S1...
    if (sbox_id < 0)
      sbox_id += 8;

    Serpent_Sbox(sbox_id, &x0, &x1, &x2, &x3);

    ctx->sk[i * 4 + 0] = x0;
    ctx->sk[i * 4 + 1] = x1;
    ctx->sk[i * 4 + 2] = x2;
    ctx->sk[i * 4 + 3] = x3;
  }
}

void Sosemanuk_IVSetup(SosemanukCtx *ctx, const uint8_t *iv) {
  uint8_t vList[16] = {0};
  for (size_t i = 0; i < 16; i++)
    vList[i] = iv[i];

  // Ép IV 16-byte vào Khối 4 từ
  uint32_t x0 =
      (vList[0] << 24) | (vList[1] << 16) | (vList[2] << 8) | vList[3];
  uint32_t x1 =
      (vList[4] << 24) | (vList[5] << 16) | (vList[6] << 8) | vList[7];
  uint32_t x2 =
      (vList[8] << 24) | (vList[9] << 16) | (vList[10] << 8) | vList[11];
  uint32_t x3 =
      (vList[12] << 24) | (vList[13] << 16) | (vList[14] << 8) | vList[15];

  // Chạy đủ 24 Vòng Lặp Serpent mã hóa
  for (int i = 0; i < 24; i++) {
    // Trộn Subkeys
    x0 ^= ctx->sk[i * 4 + 0];
    x1 ^= ctx->sk[i * 4 + 1];
    x2 ^= ctx->sk[i * 4 + 2];
    x3 ^= ctx->sk[i * 4 + 3];

    // Lăn qua S-BOX tiến lên S0, S1, S2...
    int sbox_id = (i % 8);
    Serpent_Sbox(sbox_id, &x0, &x1, &x2, &x3);

    // Mở nắp Hứng Trạng Thái Rót Vào LFSR Ở Vòng 12 và 18
    if (i == 11) { // Hết vòng 12 (i từ 0-11)
      ctx->s[7] = x0;
      ctx->s[8] = x1;
      ctx->s[9] = x2;
      ctx->s[0] = x3;
    } else if (i == 17) { // Hết vòng 18
      ctx->s[1] = x0;
      ctx->s[2] = x1;
      ctx->s[3] = x2;
      ctx->s[4] = x3;
    }

    // Chạy Biến đổi Tuyến Tính cho các vòng 1-23
    if (i < 23)
      Serpent_LT(&x0, &x1, &x2, &x3);
  }

  // Nạp Khóa Vòng 24 chót
  x0 ^= ctx->sk[96];
  x1 ^= ctx->sk[97];
  x2 ^= ctx->sk[98];
  x3 ^= ctx->sk[99];

  // Trút hết mẻ cuối chia cho LFSR và FSM
  ctx->s[5] = x0;
  ctx->s[6] = x1;
  ctx->r1 = x2;
  ctx->r2 = x3;
}

// Sinh dòng khóa
// Ham Kiem tra mang khong rong
int Is_LFSR_Ready(SosemanukCtx *ctx) {
  for (int i = 0; i < 10; i++) {
    if (ctx->s[i] != 0)
      return 1;
  }
  return 0;
}

// Ham tinh toan phan hoi
uint32_t mul_alpha(uint32_t c) {
  uint8_t top_byte = (uint8_t)(c >> 24);
  return (c << 8) ^ (top_byte == 0 ? 0 : (uint32_t)0xA9000000);
}

// Hàm chia cho alpha (nhân với alpha^-1) trong GF(2^32)
uint32_t div_alpha(uint32_t c) {
  uint8_t low_byte = (uint8_t)(c & 0xFF);
  return (c >> 8) ^ (low_byte == 0 ? 0 : (uint32_t)0x000000A9);
}

// Hàm tính phản hồi LFSR
uint32_t Calculate_LFSR_Feedback(uint32_t s0, uint32_t s3, uint32_t s9) {
  return s9 ^ div_alpha(s3) ^ mul_alpha(s0);
}

// S-box 2 của Serpent được triển khai bằng kỹ thuật Bitslice (tối ưu hóa các cổng logic) Nó nhận vào 4 từ 32-bit (chính là 4 giá trị f_t từ FSM) và biến đổi chúng
void Sosemanuk_SBox2(uint32_t *w0, uint32_t *w1, uint32_t *w2, uint32_t *w3) {
  uint32_t t0, t1, t2, t3, t4, t5, t6, t7;

  // Các phương trình đại số logic tối ưu cho S-box 2
  t0 = ~(*w0);
  t1 = *w1 ^ *w3;
  t2 = t0 ^ t1;
  t3 = *w2 ^ t1;
  t4 = *w1 & t3;
  t5 = t2 ^ t4;
  t6 = *w3 | t5;
  t7 = *w0 ^ t6;

  // Ghi đè kết quả biến đổi trở lại 4 biến đầu vào
  *w1 = t3 ^ t7;
  *w3 = *w2 | t7;
  *w2 = t5 ^ *w3;
  *w0 = t2 ^ *w3;
}

void Sosemanuk_GenerateKeystreamBlock(SosemanukCtx *ctx, uint32_t ks[4]) {
  if (!Is_LFSR_Ready(ctx))
    return;

  uint32_t f_out[4]; // Mảng tạm để lưu 4 giá trị đầu ra của FSM

  // Lưu lại 4 giá trị LFSR ban đầu trước khi bị dịch chuyển
  uint32_t s_init[4] = {ctx->s[0], ctx->s[1], ctx->s[2], ctx->s[3]};

  for (int step = 0; step < 4; step++) {
    // LOGIC FSM
    uint32_t f_t = (ctx->s[9] + ctx->r1) ^ ctx->r2; // Sinh giá trị f_t
    f_out[step] = f_t; // Lưu f_t vào mảng tạm để lát nữa đưa qua S-box

    uint32_t s1 = ctx->s[1];
    uint32_t s8 = ctx->s[8];
    uint32_t mux_out = (ctx->r1 & 1) ? (s1 ^ s8) : s1;

    uint32_t r1_old = ctx->r1;
    ctx->r1 = ctx->r2 + mux_out; // Phép cộng modulo 2^32
    ctx->r2 = Sosemanuk_Trans(r1_old);

    // LOGIC LFSR
    uint32_t s_new = Calculate_LFSR_Feedback(ctx->s[0], ctx->s[3], ctx->s[9]);
    for (int i = 0; i < 9; i++) {
      ctx->s[i] = ctx->s[i + 1];
    }
    ctx->s[9] = s_new;
  }

  // PHẦN KẾT HỢP S-BOX
  // Đưa 4 giá trị f_t qua S-box 2 của Serpent
  Sosemanuk_SBox2(&f_out[0], &f_out[1], &f_out[2], &f_out[3]);

  // Sinh ra 4 word dòng khóa bằng cách XOR đầu ra S-box với mảng s_init cũ
  ks[0] = f_out[0] ^ s_init[0];
  ks[1] = f_out[1] ^ s_init[1];
  ks[2] = f_out[2] ^ s_init[2];
  ks[3] = f_out[3] ^ s_init[3];
}

// =====================================================================
// 2. HÀM MÃ HÓA / GIẢI MÃ LÕI
// =====================================================================
void Sosemanuk_ProcessData(SosemanukCtx *ctx, uint8_t *data, size_t length) {
  uint32_t keystream[4]; // Mảng chứa 16 byte Keystream sinh ra mỗi vòng
  size_t i = 0;

  while (i < length) {
    // Bước 1: Gọi hàm lấy 16 byte Keystream mới
    Sosemanuk_GenerateKeystreamBlock(ctx, keystream);

    // Bước 2: Kỹ thuật ép kiểu con trỏ trong C
    uint8_t *ks_bytes = (uint8_t *)keystream;

    // Bước 3: Vòng lặp XOR tốc độ cao
    for (int j = 0; j < 16 && i < length; ++j, ++i) {
      data[i] ^= ks_bytes[j];
    }
  }
}

// =====================================================================
// HÀM TIỆN ÍCH HIỂN THỊ
// =====================================================================
void UI_PrintHexDump(const uint8_t *data, size_t length) {
  printf("\n--- HEX DUMP (%zu bytes) ---\n", length);
  for (size_t i = 0; i < length; i++) {
    printf("%02X ", data[i]);
    if ((i + 1) % 16 == 0)
      printf("\n");
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
  uint8_t iv[16] = {0xCD, 0x03, 0x04};

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
      // BƯỚC 1: Setup Key và IV
      Sosemanuk_KeySetup(&cipher, key);
      Sosemanuk_IVSetup(&cipher, iv);
      printf(">> Dang xu ly ma hoa...\n");

      // BƯỚC 2: Đọc dữ liệu từ file văn bản rõ
      size_t content_size = 0;
      uint8_t *content = read_file_simple("input.txt", &content_size);
      
      if (content == NULL) {
          printf(">> [Loi] Khong tim thay hoac khong the doc file 'input.txt'!\n");
      } else {
          // BƯỚC 3: Xử lý mã hóa
          Sosemanuk_ProcessData(&cipher, content, content_size);
          
          // BƯỚC 4: Ghi kết quả ra file nhị phân
          write_file_simple("encrypted.bin", content, content_size);
          printf(">> [OK] Da ma hoa xong %zu bytes. File dau ra: 'encrypted.bin'\n", content_size);
          
          // BƯỚC 5: Giải phóng RAM để tránh rò rỉ bộ nhớ
          free(content); 
      }

    } else if (choice == 2) {
      // BƯỚC 1: Setup Key và IV 
      Sosemanuk_KeySetup(&cipher, key);
      Sosemanuk_IVSetup(&cipher, iv);
      printf(">> Dang xu ly giai ma...\n");

      // BƯỚC 2: Đọc dữ liệu TỪ FILE ĐÃ MÃ HÓA
      size_t content_size = 0;
      uint8_t *content = read_file_simple("encrypted.bin", &content_size);
      
      if (content == NULL) {
          printf(">> [Loi] Khong tim thay file 'encrypted.bin'. Ban phai ma hoa (Chon 1) truoc!\n");
      } else {
          // BƯỚC 3: Xử lý giải mã 
          Sosemanuk_ProcessData(&cipher, content, content_size);
          
          // BƯỚC 4: Ghi kết quả trả lại file văn bản
          write_file_simple("decrypted.txt", content, content_size);
          printf(">> [OK] Da giai ma xong %zu bytes. Kiem tra file: 'decrypted.txt'\n", content_size);
          
          // BƯỚC 5: Giải phóng RAM
          free(content);
      }

    } else if (choice == 3) {
      // =======================================================
      // CHỨC NĂNG 3: NHẬP VĂN BẢN VÀ GHI RA FILE
      // =======================================================
      printf("\nNhap doan van ban ban muon ma hoa: ");

      int c;
      while ((c = getchar()) != '\n' && c != EOF) {
      } // Dọn bộ đệm

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
          printf(">> [OK] Da ghi noi dung \"%s\" vao file input.txt!\n",
                 inputText);
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