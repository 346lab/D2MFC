#include "../Common/Common.hpp"
#include "../Common/Font.hpp"

int main() {
  constexpr char Face[] = R"?(C:\Windows\Fonts\consola.ttf)?";
  unsigned z, a, b;
  int d, x;
  scanf("%u%d%d%u%u", &z, &d, &x, &a, &b);
  Font Fnt;
  Fnt.Size = z;
  Fnt.DescentPadding = d;
  Fnt.Faces.emplace_back(Face);
  for (auto i = a; i < b; ++i) {
    auto& G = Fnt.Glyphs[i];
    G.reset(new FontGlyph{});
    G->Char = i;
    G->AntiAliasing = x;
    G->Size = z;
    G->FaceIdx = 0;
    G->HasBmp = false;
  }
  printf("Rendering glyphs...\n");
  Fnt.RenderGlyphs();
  printf("Reading palette...\n");
  Palette Pal;
  Pal.ReadDat("pal.dat");
  printf("Dumping font...\n");
  Sprite Spr;
  FontTable Tbl;
  Fnt.Dump(Spr, Tbl);
  printf("Saving DC6...\n");
  Spr.SaveDc6("x.DC6", Pal);
  printf("Saving TBL...\n");
  Tbl.SaveTbl("x.tbl");
#if 1
  printf("Saving PNGs...\n");
  for (auto Dir = 0u; Dir < Spr.NDir(); ++Dir)
    for (auto Frm = 0u; Frm < Spr.NFrm(); ++Frm) {
      ostringstream OutPath;
      OutPath << 'y';
      OutPath << '/' << setfill('0') << setw(2) << Dir;
      OutPath << '-' << setfill('0') << setw(4) << Frm;
      OutPath << ".png";
      Spr[Dir][Frm].SavePng(OutPath.str().c_str());
    }
#endif
  printf("All done\n");
  return 0;
}