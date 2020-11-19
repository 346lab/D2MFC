#include "../Common/Common.hpp"
#include "../Common/Bitmap.hpp"
#include "../Common/Font.hpp"
#include "../Common/FontTable.hpp"
#include "../Common/Sprite.hpp"

int main(int NArg, char* Args[]) {
  if (NArg != 5) {
    fprintf(stderr, "Incorrect command line.\n");
    fprintf(stderr,
      "\n"
      "Dump DC6 File\n"
      "\n"
      "Usage: %s <Input>.dc6 <Input>.tbl <Palette>.dat <Output>.png\n"
      "Render text using provided dc6 and tbl.\n"
      "Text should be given in standard input.\n",
      Args[0]
    );
    return EXIT_FAILURE;
  }
  printf("Reading palette...\n");
  Palette Pal;
  Pal.ReadDat(Args[3]);
  printf("Reading DC6...\n");
  Sprite Spr;
  Spr.ReadDc6(Args[1], Pal);
  printf("Reading TBL...\n");
  FontTable Tbl;
  Tbl.ReadTbl(Args[2]);
  printf("LnSpacing=%u\n", Tbl.Hdr.LnSpacing);
  printf("CapHeight=%u\n", Tbl.Hdr.CapHeight);
  printf("Constructing font...\n");
  Font Fnt;
  Fnt.FromSprTbl(Spr, Tbl);
  wstring Str;
  printf("Ready, type some text below:\n");
  auto Ch = (wchar_t) getwchar();
  while (Ch != WEOF) {
    Str += Ch;
    Ch = getwchar();
  }
  while (!Str.empty() && Str.back() == '\n')
    Str.pop_back();
  auto Bmp = Fnt.Render(Str);
  printf("Saving PNG...\n");
  Bmp.SavePng(Args[4]);
  printf("All done\n");
  return 0;
}