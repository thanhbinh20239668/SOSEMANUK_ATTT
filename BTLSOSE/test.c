#include "serpent.h"
char XORchar1(char a, char b) { return a ^ b; }
int main() {
  char a = 'A';
  char b = 0x20;
  printf("%c\n", XORchar1(a, XORchar1(a, b)));
  return 0;
}
