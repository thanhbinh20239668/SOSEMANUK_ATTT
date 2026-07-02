#ifndef GF_TABLES_H
#define GF_TABLES_H

#include <stdint.h>

/*
 * Sinh 2 bang tra cuu 256 phan tu dung cho Sosemanuk / SNOW 2.0:
 *   - mul_table: dung trong mul_alpha()  (nhan voi alpha trong GF(2^32))
 *   - div_table: dung trong div_alpha()  (chia cho alpha, tuc nhan alpha^-1)
 *
 * Ca 2 bang duoc tinh tu truong GF(2^8) sinh boi da thuc
 *   x^8 + x^7 + x^5 + x^3 + 1
 * va truong mo rong GF(2^32) sinh boi alpha, nghiem cua
 *   x^4 + beta^23 x^3 + beta^245 x^2 + beta^48 x + beta^239
 *
 * Ket qua sinh ra khop CHINH XAC voi mul_table/div_table hardcode
 * trong main.c goc (da kiem chung tung phan tu).
 */
void GF_GenerateTables(uint32_t mul_table[256], uint32_t div_table[256]);

#endif
