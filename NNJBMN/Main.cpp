#include "../Common/Common.hpp"
#include "../Common/Bitmap.hpp"

int main() {
  Bitmap Bmp(1, 1);
  Bmp.Fill(0xffffff);
  Bmp.SavePng("white.png");
  Bmp.Fill(0x8080ff);
  Bmp.SavePng("normal.png");
  return 0;
}