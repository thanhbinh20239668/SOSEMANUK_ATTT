#include "gf_tables.h"

// Da thuc rut gon (bo bit x^8) cua  x^8 + x^7 + x^5 + x^3 + 1
#define GF_POLY 0xA9

// Nhan 2 phan tu trong GF(2^8)
static uint8_t gf_mul(uint8_t a, uint8_t b) {
  uint8_t p = 0;
  for (int i = 0; i < 8; i++) {
    if (b & 1)
      p ^= a;
    uint8_t hi = a & 0x80;
    a = (uint8_t)(a << 1);
    if (hi)
      a ^= GF_POLY;
    b >>= 1;
  }
  return p;
}

// Luy thua a^n trong GF(2^8)
static uint8_t gf_pow(uint8_t a, int n) {
  uint8_t r = 1;
  for (int i = 0; i < n; i++)
    r = gf_mul(r, a);
  return r;
}

// Tim nghich dao nhan cua a trong GF(2^8) (a != 0)
static uint8_t gf_inv(uint8_t a) {
  for (int x = 1; x < 256; x++) {
    if (gf_mul(a, (uint8_t)x) == 1)
      return (uint8_t)x;
  }
  return 0; // khong xay ra voi a != 0
}

// Nghich dao ma tran 4x4 tren GF(2^8) bang khu Gauss-Jordan
static void gf_mat_inverse4(uint8_t M[4][4], uint8_t Minv[4][4]) {
  uint8_t A[4][8];

  // Ghep ma tran mo rong [M | I]
  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++)
      A[i][j] = M[i][j];
    for (int j = 0; j < 4; j++)
      A[i][4 + j] = (i == j) ? 1 : 0;
  }

  for (int col = 0; col < 4; col++) {
    // Tim dong pivot khac 0
    int piv = -1;
    for (int r = col; r < 4; r++) {
      if (A[r][col] != 0) {
        piv = r;
        break;
      }
    }
    if (piv != col) {
      for (int k = 0; k < 8; k++) {
        uint8_t tmp = A[col][k];
        A[col][k] = A[piv][k];
        A[piv][k] = tmp;
      }
    }

    // Chuan hoa dong pivot ve 1
    uint8_t inv = gf_inv(A[col][col]);
    for (int k = 0; k < 8; k++)
      A[col][k] = gf_mul(A[col][k], inv);

    // Khu cac dong con lai
    for (int r = 0; r < 4; r++) {
      if (r != col && A[r][col] != 0) {
        uint8_t factor = A[r][col];
        for (int k = 0; k < 8; k++)
          A[r][k] ^= gf_mul(factor, A[col][k]);
      }
    }
  }

  for (int i = 0; i < 4; i++)
    for (int j = 0; j < 4; j++)
      Minv[i][j] = A[i][4 + j];
}

void GF_GenerateTables(uint32_t mul_table[256], uint32_t div_table[256]) {
  const uint8_t beta = 0x02;

  // 4 hang so dinh nghia phep rut gon alpha^4 = b23*a^3+b245*a^2+b48*a+b239
  uint8_t b23  = gf_pow(beta, 23);
  uint8_t b245 = gf_pow(beta, 245);
  uint8_t b48  = gf_pow(beta, 48);
  uint8_t b239 = gf_pow(beta, 239);

  // ---- Bang MUL (nhan voi alpha) ----
  // Byte cao nhat (c3) "roi ra" khi dich trai duoc nhan voi 4 hang so tren
  for (int c3 = 0; c3 < 256; c3++) {
    mul_table[c3] =
        ((uint32_t)gf_mul(b23,  (uint8_t)c3) << 24) |
        ((uint32_t)gf_mul(b245, (uint8_t)c3) << 16) |
        ((uint32_t)gf_mul(b48,  (uint8_t)c3) << 8)  |
         (uint32_t)gf_mul(b239, (uint8_t)c3);
  }

  // ---- Bang DIV (nhan voi alpha^-1) ----
  // Dung ma tran M bieu dien phep "nhan voi alpha" theo co so {a^3,a^2,a,1}
  uint8_t M[4][4] = {
      {b23,  1, 0, 0},
      {b245, 0, 1, 0},
      {b48,  0, 0, 1},
      {b239, 0, 0, 0},
  };
  uint8_t Minv[4][4];
  gf_mat_inverse4(M, Minv);

  // Cot cuoi cua Minv la toa do cua alpha^-1 trong co so {a^3,a^2,a,1}
  uint8_t d3 = Minv[0][3];
  uint8_t d2 = Minv[1][3];
  uint8_t d1 = Minv[2][3];
  uint8_t d0 = Minv[3][3];

  // Byte thap nhat (c0) "roi ra" khi dich phai duoc nhan voi 4 hang so tren
  for (int c0 = 0; c0 < 256; c0++) {
    div_table[c0] =
        ((uint32_t)gf_mul(d3, (uint8_t)c0) << 24) |
        ((uint32_t)gf_mul(d2, (uint8_t)c0) << 16) |
        ((uint32_t)gf_mul(d1, (uint8_t)c0) << 8)  |
         (uint32_t)gf_mul(d0, (uint8_t)c0);
  }
}
