#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
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
static const uint32_t mul_table[256] = {
        0x00000000, 0xE19FCF13, 0x6B973726, 0x8A08F835, 0xD6876E4C, 0x3718A15F, 0xBD10596A, 0x5C8F9679,
        0x05A7DC98, 0xE438138B, 0x6E30EBBE, 0x8FAF24AD, 0xD320B2D4, 0x32BF7DC7, 0xB8B785F2, 0x59284AE1,
        0x0AE71199, 0xEB78DE8A, 0x617026BF, 0x80EFE9AC, 0xDC607FD5, 0x3DFFB0C6, 0xB7F748F3, 0x566887E0,
        0x0F40CD01, 0xEEDF0212, 0x64D7FA27, 0x85483534, 0xD9C7A34D, 0x38586C5E, 0xB250946B, 0x53CF5B78,
        0x1467229B, 0xF5F8ED88, 0x7FF015BD, 0x9E6FDAAE, 0xC2E04CD7, 0x237F83C4, 0xA9777BF1, 0x48E8B4E2,
        0x11C0FE03, 0xF05F3110, 0x7A57C925, 0x9BC80636, 0xC747904F, 0x26D85F5C, 0xACD0A769, 0x4D4F687A,
        0x1E803302, 0xFF1FFC11, 0x75170424, 0x9488CB37, 0xC8075D4E, 0x2998925D, 0xA3906A68, 0x420FA57B,
        0x1B27EF9A, 0xFAB82089, 0x70B0D8BC, 0x912F17AF, 0xCDA081D6, 0x2C3F4EC5, 0xA637B6F0, 0x47A879E3,
        0x28CE449F, 0xC9518B8C, 0x435973B9, 0xA2C6BCAA, 0xFE492AD3, 0x1FD6E5C0, 0x95DE1DF5, 0x7441D2E6,
        0x2D699807, 0xCCF65714, 0x46FEAF21, 0xA7616032, 0xFBEEF64B, 0x1A713958, 0x9079C16D, 0x71E60E7E,
        0x22295506, 0xC3B69A15, 0x49BE6220, 0xA821AD33, 0xF4AE3B4A, 0x1531F459, 0x9F390C6C, 0x7EA6C37F,
        0x278E899E, 0xC611468D, 0x4C19BEB8, 0xAD8671AB, 0xF109E7D2, 0x109628C1, 0x9A9ED0F4, 0x7B011FE7,
        0x3CA96604, 0xDD36A917, 0x573E5122, 0xB6A19E31, 0xEA2E0848, 0x0BB1C75B, 0x81B93F6E, 0x6026F07D,
        0x390EBA9C, 0xD891758F, 0x52998DBA, 0xB30642A9, 0xEF89D4D0, 0x0E161BC3, 0x841EE3F6, 0x65812CE5,
        0x364E779D, 0xD7D1B88E, 0x5DD940BB, 0xBC468FA8, 0xE0C919D1, 0x0156D6C2, 0x8B5E2EF7, 0x6AC1E1E4,
        0x33E9AB05, 0xD2766416, 0x587E9C23, 0xB9E15330, 0xE56EC549, 0x04F10A5A, 0x8EF9F26F, 0x6F663D7C,
        0x50358897, 0xB1AA4784, 0x3BA2BFB1, 0xDA3D70A2, 0x86B2E6DB, 0x672D29C8, 0xED25D1FD, 0x0CBA1EEE,
        0x5592540F, 0xB40D9B1C, 0x3E056329, 0xDF9AAC3A, 0x83153A43, 0x628AF550, 0xE8820D65, 0x091DC276,
        0x5AD2990E, 0xBB4D561D, 0x3145AE28, 0xD0DA613B, 0x8C55F742, 0x6DCA3851, 0xE7C2C064, 0x065D0F77,
        0x5F754596, 0xBEEA8A85, 0x34E272B0, 0xD57DBDA3, 0x89F22BDA, 0x686DE4C9, 0xE2651CFC, 0x03FAD3EF,
        0x4452AA0C, 0xA5CD651F, 0x2FC59D2A, 0xCE5A5239, 0x92D5C440, 0x734A0B53, 0xF942F366, 0x18DD3C75,
        0x41F57694, 0xA06AB987, 0x2A6241B2, 0xCBFD8EA1, 0x977218D8, 0x76EDD7CB, 0xFCE52FFE, 0x1D7AE0ED,
        0x4EB5BB95, 0xAF2A7486, 0x25228CB3, 0xC4BD43A0, 0x9832D5D9, 0x79AD1ACA, 0xF3A5E2FF, 0x123A2DEC,
        0x4B12670D, 0xAA8DA81E, 0x2085502B, 0xC11A9F38, 0x9D950941, 0x7C0AC652, 0xF6023E67, 0x179DF174,
        0x78FBCC08, 0x9964031B, 0x136CFB2E, 0xF2F3343D, 0xAE7CA244, 0x4FE36D57, 0xC5EB9562, 0x24745A71,
        0x7D5C1090, 0x9CC3DF83, 0x16CB27B6, 0xF754E8A5, 0xABDB7EDC, 0x4A44B1CF, 0xC04C49FA, 0x21D386E9,
        0x721CDD91, 0x93831282, 0x198BEAB7, 0xF81425A4, 0xA49BB3DD, 0x45047CCE, 0xCF0C84FB, 0x2E934BE8,
        0x77BB0109, 0x9624CE1A, 0x1C2C362F, 0xFDB3F93C, 0xA13C6F45, 0x40A3A056, 0xCAAB5863, 0x2B349770,
        0x6C9CEE93, 0x8D032180, 0x070BD9B5, 0xE69416A6, 0xBA1B80DF, 0x5B844FCC, 0xD18CB7F9, 0x301378EA,
        0x693B320B, 0x88A4FD18, 0x02AC052D, 0xE333CA3E, 0xBFBC5C47, 0x5E239354, 0xD42B6B61, 0x35B4A472,
        0x667BFF0A, 0x87E43019, 0x0DECC82C, 0xEC73073F, 0xB0FC9146, 0x51635E55, 0xDB6BA660, 0x3AF46973,
        0x63DC2392, 0x8243EC81, 0x084B14B4, 0xE9D4DBA7, 0xB55B4DDE, 0x54C482CD, 0xDECC7AF8, 0x3F53B5EB
};
static const uint32_t div_table[256] = {
        0x00000000, 0x180F40CD, 0x301E8033, 0x2811C0FE, 0x603CA966, 0x7833E9AB, 0x50222955, 0x482D6998,
        0xC078FBCC, 0xD877BB01, 0xF0667BFF, 0xE8693B32, 0xA04452AA, 0xB84B1267, 0x905AD299, 0x88559254,
        0x29F05F31, 0x31FF1FFC, 0x19EEDF02, 0x01E19FCF, 0x49CCF657, 0x51C3B69A, 0x79D27664, 0x61DD36A9,
        0xE988A4FD, 0xF187E430, 0xD99624CE, 0xC1996403, 0x89B40D9B, 0x91BB4D56, 0xB9AA8DA8, 0xA1A5CD65,
        0x5249BE62, 0x4A46FEAF, 0x62573E51, 0x7A587E9C, 0x32751704, 0x2A7A57C9, 0x026B9737, 0x1A64D7FA,
        0x923145AE, 0x8A3E0563, 0xA22FC59D, 0xBA208550, 0xF20DECC8, 0xEA02AC05, 0xC2136CFB, 0xDA1C2C36,
        0x7BB9E153, 0x63B6A19E, 0x4BA76160, 0x53A821AD, 0x1B854835, 0x038A08F8, 0x2B9BC806, 0x339488CB,
        0xBBC11A9F, 0xA3CE5A52, 0x8BDF9AAC, 0x93D0DA61, 0xDBFDB3F9, 0xC3F2F334, 0xEBE333CA, 0xF3EC7307,
        0xA492D5C4, 0xBC9D9509, 0x948C55F7, 0x8C83153A, 0xC4AE7CA2, 0xDCA13C6F, 0xF4B0FC91, 0xECBFBC5C,
        0x64EA2E08, 0x7CE56EC5, 0x54F4AE3B, 0x4CFBEEF6, 0x04D6876E, 0x1CD9C7A3, 0x34C8075D, 0x2CC74790,
        0x8D628AF5, 0x956DCA38, 0xBD7C0AC6, 0xA5734A0B, 0xED5E2393, 0xF551635E, 0xDD40A3A0, 0xC54FE36D,
        0x4D1A7139, 0x551531F4, 0x7D04F10A, 0x650BB1C7, 0x2D26D85F, 0x35299892, 0x1D38586C, 0x053718A1,
        0xF6DB6BA6, 0xEED42B6B, 0xC6C5EB95, 0xDECAAB58, 0x96E7C2C0, 0x8EE8820D, 0xA6F942F3, 0xBEF6023E,
        0x36A3906A, 0x2EACD0A7, 0x06BD1059, 0x1EB25094, 0x569F390C, 0x4E9079C1, 0x6681B93F, 0x7E8EF9F2,
        0xDF2B3497, 0xC724745A, 0xEF35B4A4, 0xF73AF469, 0xBF179DF1, 0xA718DD3C, 0x8F091DC2, 0x97065D0F,
        0x1F53CF5B, 0x075C8F96, 0x2F4D4F68, 0x37420FA5, 0x7F6F663D, 0x676026F0, 0x4F71E60E, 0x577EA6C3,
        0xE18D0321, 0xF98243EC, 0xD1938312, 0xC99CC3DF, 0x81B1AA47, 0x99BEEA8A, 0xB1AF2A74, 0xA9A06AB9,
        0x21F5F8ED, 0x39FAB820, 0x11EB78DE, 0x09E43813, 0x41C9518B, 0x59C61146, 0x71D7D1B8, 0x69D89175,
        0xC87D5C10, 0xD0721CDD, 0xF863DC23, 0xE06C9CEE, 0xA841F576, 0xB04EB5BB, 0x985F7545, 0x80503588,
        0x0805A7DC, 0x100AE711, 0x381B27EF, 0x20146722, 0x68390EBA, 0x70364E77, 0x58278E89, 0x4028CE44,
        0xB3C4BD43, 0xABCBFD8E, 0x83DA3D70, 0x9BD57DBD, 0xD3F81425, 0xCBF754E8, 0xE3E69416, 0xFBE9D4DB,
        0x73BC468F, 0x6BB30642, 0x43A2C6BC, 0x5BAD8671, 0x1380EFE9, 0x0B8FAF24, 0x239E6FDA, 0x3B912F17,
        0x9A34E272, 0x823BA2BF, 0xAA2A6241, 0xB225228C, 0xFA084B14, 0xE2070BD9, 0xCA16CB27, 0xD2198BEA,
        0x5A4C19BE, 0x42435973, 0x6A52998D, 0x725DD940, 0x3A70B0D8, 0x227FF015, 0x0A6E30EB, 0x12617026,
        0x451FD6E5, 0x5D109628, 0x750156D6, 0x6D0E161B, 0x25237F83, 0x3D2C3F4E, 0x153DFFB0, 0x0D32BF7D,
        0x85672D29, 0x9D686DE4, 0xB579AD1A, 0xAD76EDD7, 0xE55B844F, 0xFD54C482, 0xD545047C, 0xCD4A44B1,
        0x6CEF89D4, 0x74E0C919, 0x5CF109E7, 0x44FE492A, 0x0CD320B2, 0x14DC607F, 0x3CCDA081, 0x24C2E04C,
        0xAC977218, 0xB49832D5, 0x9C89F22B, 0x8486B2E6, 0xCCABDB7E, 0xD4A49BB3, 0xFCB55B4D, 0xE4BA1B80,
        0x17566887, 0x0F59284A, 0x2748E8B4, 0x3F47A879, 0x776AC1E1, 0x6F65812C, 0x477441D2, 0x5F7B011F,
        0xD72E934B, 0xCF21D386, 0xE7301378, 0xFF3F53B5, 0xB7123A2D, 0xAF1D7AE0, 0x870CBA1E, 0x9F03FAD3,
        0x3EA637B6, 0x26A9777B, 0x0EB8B785, 0x16B7F748, 0x5E9A9ED0, 0x4695DE1D, 0x6E841EE3, 0x768B5E2E,
        0xFEDECC7A, 0xE6D18CB7, 0xCEC04C49, 0xD6CF0C84, 0x9EE2651C, 0x86ED25D1, 0xAEFCE52F, 0xB6F3A5E2
};
int Is_LFSR_Ready(SosemanukCtx *ctx) {
  for (int i = 0; i < 10; i++) {
    if (ctx->s[i] != 0)
      return 1;
  }
  return 0;
}

// Hàm nhân alpha
uint32_t mul_alpha(uint32_t c) {
    uint8_t top_byte = (uint8_t)(c >> 24); // Lấy byte văng ra ở đầu
    return (c << 8) ^ mul_table[top_byte]; // Tra bảng lấy mặt nạ chuẩn
}

// Hàm chia alpha 
uint32_t div_alpha(uint32_t c) {
    uint8_t low_byte = (uint8_t)(c & 0xFF); // Lấy byte văng ra ở đuôi
    return (c >> 8) ^ div_table[low_byte]; // Tra bảng lấy mặt nạ chuẩn
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
      printf("\n>> Dang chay Benchmark... Xin vui long doi trong giay lat.\n");

      size_t mb_size = 1024 * 1024;
      uint8_t *dummy_buffer = (uint8_t *)malloc(mb_size);
      
      if (dummy_buffer == NULL) {
        printf(">> [Loi] Khong the cap phat bo nho!\n");
      } else {
        memset(dummy_buffer, 0, mb_size);
        Sosemanuk_KeySetup(&cipher, key);
        Sosemanuk_IVSetup(&cipher, iv);

        int iterations = 100; // Chạy 100MB để đo cho chuẩn
        clock_t start_time = clock();

        for (int i = 0; i < iterations; i++) {
          Sosemanuk_ProcessData(&cipher, dummy_buffer, mb_size);
        }

        clock_t end_time = clock();
        double time_taken = (double)(end_time - start_time) / CLOCKS_PER_SEC;

        if (time_taken > 0) {
          printf(">> [Thanh cong] Da xu ly xong %d MB.\n", iterations);
          printf(">> [Hieu nang] Toc do: %.2f MB/s\n", (double)iterations / time_taken);
        }
        free(dummy_buffer);
      }

} else if (choice == 5) {
      // Chức năng Hex Dump ĐỌC FILE THẬT
      size_t enc_size = 0;
      uint8_t *enc_data = read_file_simple("encrypted.bin", &enc_size);
      
      if (enc_data == NULL) {
        printf(">> [Loi] Chua co file 'encrypted.bin'. Ban hay ma hoa file truoc (Chon 1)!\n");
      } else {
        // Chỉ in tối đa 256 byte đầu tiên để tránh tràn màn hình Console
        size_t print_size = (enc_size > 256) ? 256 : enc_size; 
        printf("\n>> Hien thi Hex Dump cua file 'encrypted.bin':\n");
        UI_PrintHexDump(enc_data, print_size);
        free(enc_data); // Giải phóng RAM
      }

    } else if (choice == 0) {
      printf(">> Thoat chuong trinh.\n");

    } else {
      printf(">> Lua chon khong hop le!\n");
    }

  } while (choice != 0); // Đóng vòng lặp do-while

  return 0;
} 