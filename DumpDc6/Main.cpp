#include "../Common/Common.hpp"
#include "../Common/Bitmap.hpp"
#include "../Common/Sprite.hpp"

int main(int NArg, char* Args[]) {
  if (NArg != 4) {
    fprintf(stderr, "Incorrect command line.\n");
    fprintf(stderr,
      "\n"
      "Dump DC6 File\n"
      "\n"
      "Usage: %s <Input>.dc6 <Palette>.dat <OutputDir>\n"
      "Read DC6 file and extract all images.\n"
      "Use null as the second argument to output grayscale images.\n",
      Args[0]
    );
    return EXIT_FAILURE;
  }
  printf("Reading palette: %s...\n", Args[2]);
  Palette Pal;
  Pal.ReadDat(Args[2]);
  printf("Done palette reading\n");
  printf("Reading DC6: %s...\n", Args[1]);
  Sprite Spr;
  Spr.ReadDc6(Args[1], Pal);
  printf("Done DC6 reading\n");
  printf("Saving extracted images...\n");
  for (auto Dir = 0u; Dir < Spr.NDir(); ++Dir)
    for (auto Frm = 0u; Frm < Spr.NFrm(); ++Frm) {
      ostringstream OutPath;
      OutPath << Args[3];
      OutPath << '/' << setfill('0') << setw(2) << Dir;
      OutPath << '-' << setfill('0') << setw(4) << Frm;
      OutPath << ".png";
      Spr[Dir][Frm].SavePng(OutPath.str().c_str());
    }
  printf("Done image saving...\n");
  return 0;
}