#include "../Common/Common.hpp"
#include "../Common/Font.hpp"

template<class T>
T Parse(const char* S, const char* Desc) {
  uintmax_t Res{};
  while (*S) {
    if (!isdigit(*S))
      Abort("Failed to parse %s", Desc);
    auto Tmp = Res * 10 + (*S++ & 0xf);
    if (Tmp < Res)
      Abort("Integer overflow when parsing %s", Desc);
    Res = Tmp;
  }
  return Cast<T>(Res, "The %s is too large (%" PRIuMAX ")", Desc, Res);
}

int main(int NArg, char* Args[]) {
  if (NArg != 8) {
    fprintf(stderr, "Incorrect command line.\n");
    fprintf(stderr,
      "\n"
      "Create DC6 and TBL according to given font and codepoint range\n"
      "\n"
      "Usage: %s <FirstCodePoint> <LastCodePoint> <FontFacePath> <PointSize> <Palette>.dat <Out>.dc6 <Out>.tbl\n"
      "Construct DC6 and TBL file using the specified font face and point size.\n"
      "Note: The font must be supported by FreeType.\n"
      "Use null as the palatte to encode as grayscale images.\n",
      Args[0]
    );
    return EXIT_FAILURE;
  }
  Font Fnt;
  auto First = Parse<uint16_t>(Args[1], "<FirstCodePoint>");
  auto Last = Parse<uint16_t>(Args[2], "<LastCodePoint>");
  auto FacePath = Args[3];
  auto Size = Parse<uint16_t>(Args[4], "<PointSize>");
  auto PalPath = Args[5];
  auto Dc6Path = Args[6];
  auto TblPath = Args[7];
  printf("Preparing glyphs...\n");
  Fnt.Size = Size;
  Fnt.Faces.emplace_back(FacePath);
  for (auto Ch = (unsigned) First; Ch <= Last; ++Ch) {
    auto& G = Fnt.Glyphs[Ch];
    G.reset(new FontGlyph);
    G->Char = Ch;
    G->AntiAliasing = false;
    G->Size = Size;
    G->FaceIdx = 0;
    G->HasBmp = false;
  }
  printf("Rendering glyphs...\n");
  Fnt.RenderGlyphs();
  printf("Reading palette...\n");
  Palette Pal;
  Pal.ReadDat(PalPath);
  printf("Dumping font...\n");
  Sprite Spr;
  FontTable Tbl;
  Fnt.Dump(Spr, Tbl);
  printf("Saving DC6...\n");
  Spr.SaveDc6(Dc6Path, Pal);
  printf("Saving TBL...\n");
  Tbl.SaveTbl(TblPath);
  printf("All done\n");
  return 0;
}