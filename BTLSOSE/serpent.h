#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct w {
  char a0;
  char a1;
  char a2;
  char a3;
} W;
W *XORchar(W *a, W *b) {
  W *c = new W;
  c->a0 = a->a0 ^ b->a0;
  c->a1 = a->a1 ^ b->a1;
  c->a2 = a->a2 ^ b->a2;
  c->a3 = a->a3 ^ b->a3;
  return c;
} // phép xor từng kí tự trong cụm 4 kí tự

void wkey(char s[], W *w) { // gom 4 từ lại thành một cụm =>
  // Lấy độ dài chuỗi ban đầu
  int len = strlen(s);
  if (len < 32) {
    for (int i = len; i < 32; i++) {
      s[i] = '0';
    }
    s[32] = '\0'; // Kí tự kết thúc chuỗi an toàn
  }

  // vòng lặp gán giá trị vào w , 4 kí tự vào 1 w , w chạy từ -8 đến -1
  for (int i = -8; i < 0; i++) {
    for (int j = 0; j < 32; j++) {
      w[i].a0 = s[j];
      w[i].a1 = s[j + 1];
      w[i].a2 = s[j + 2];
      w[i].a3 = s[j + 3];
      j += 3;
    }
  }
}
