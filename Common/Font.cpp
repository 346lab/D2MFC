#include "Font.hpp"

#include <math.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#define FtAss(e_) ((void) (!(e_) || (Abort("FreeType call failed: " # e_ ""), 0)))

void Font::Clear() {
  fill(Glyphs.begin(), Glyphs.end(), nullptr);
  Pals.clear();
  Faces.clear();
  Size = 0;
  LnSpacingOff = 2;
  CapHeightOff = 0;
  DescentPadding = -1;
  LnSpacing = 0;
  CapHeight = 0;
  UnkHZ = 0;
}

void Font::FromSprTbl(Sprite& Spr, FontTable& Tbl) {
  if (Spr.NDir() != 1)
    Abort("The number of directions should be 1 instead of %zu", Spr.NDir());
  if (Tbl.Hdr.NChar != Spr.NFrm())
    Abort("The number of images (%zu) should be the same with characters (%u)", Spr.NFrm(), Tbl.Hdr.NChar);
  Clear();
  Size = Tbl.Hdr.LnSpacing;
  LnSpacing = Tbl.Hdr.LnSpacing;
  CapHeight = Tbl.Hdr.CapHeight;
  UnkHZ = Tbl.Hdr.UnkHZ;
  for (auto i = 0u; i < Spr.NFrm(); ++i) {
    auto& C = Tbl.Chrs[i];
    auto& G = Glyphs[C.Char];
    Assert(!G);
    G.reset(new FontGlyph);
    G->Char = C.Char;
    G->Size = Tbl.Hdr.LnSpacing;
    G->UnkTwo = C.UnkTwo;
    G->HasBmp = 2;
    G->BearY = C.Height;
    G->Advance = C.Width;
    if (C.Dc6Index >= Spr.NFrm())
      Abort("DC6 index (%u) is too large for char (%u): should be less than %zu", C.Dc6Index, G->Char, Spr.NFrm());
    G->Bmp = move(Spr[0][C.Dc6Index]);
  }
}

void Font::RenderGlyphs() {
  vector<FontGlyph*> ToRender;
  for (auto Ch = 0u; Ch < Glyphs.size(); ++Ch) {
    auto& G = Glyphs[Ch];
    if (!G)
      continue;
    Assert(G->Char == Ch);
    if (G->HasBmp)
      continue;
    if (G->FaceIdx < 0)
      Abort("No font face specified for char (%u)", Ch);
    if ((size_t) G->FaceIdx >= Faces.size())
      Abort("Face index for char (%u) is too large: %d > %zu", Ch, G->FaceIdx, Faces.size());
    if (!Glyphs[Ch]->Size)
      Abort("The size of char (%u) should not be 0", Ch);
    ToRender.emplace_back(Glyphs[Ch].get());
  }
  sort(ToRender.begin(), ToRender.end(),
    [](FontGlyph* A, FontGlyph* B) {
      return A->FaceIdx != B->FaceIdx ? A->FaceIdx < B->FaceIdx : A->Size < B->Size;
    }
  );
  int32_t LastFace{-1};
  uint32_t LastSize{};
  FT_Library Lib{};
  FtAss(FT_Init_FreeType(&Lib));
  FT_Face Face{};
  for (auto& G : ToRender) {
    if (G->FaceIdx != LastFace) {
      if (Face)
        FtAss(FT_Done_Face(Face));
      FtAss(FT_New_Face(Lib, Faces[G->FaceIdx].c_str(), 0, &Face));
      LastFace = G->FaceIdx;
      LastSize = 0;
    }
    if (G->Size != LastSize) {
      FtAss(FT_Set_Pixel_Sizes(Face, 0, G->Size));
      LastSize = G->Size;
    }
    auto FtgIdx = FT_Get_Char_Index(Face, G->Char);
    if (!FtgIdx) {
      Warn("No glyph found for char (%u), a dummy (1x1) bitmap will be generated", G->Char);
      G->BearX = 0;
      G->BearY = 1;
      G->Advance = 1;
      G->HasBmp = 1;
      G->Bmp.Resize(1, 1);
      G->Bmp.Fill({});
    }
    else {
      FtAss(FT_Load_Glyph(Face, FtgIdx, G->AntiAliasing ? FT_LOAD_DEFAULT : FT_LOAD_TARGET_MONO | FT_LOAD_MONOCHROME));
      if (Face->glyph->format != FT_GLYPH_FORMAT_BITMAP)
        FtAss(FT_Render_Glyph(Face->glyph, G->AntiAliasing ? FT_RENDER_MODE_NORMAL : FT_RENDER_MODE_MONO));
      auto& Ftg = Face->glyph;
      auto& Ftb = Face->glyph->bitmap;
      if (!Ftb.width || !Ftb.rows) {
        Warn("Empty bitmap generated for char (%u), a dummy (1x1) bitmap will be generated", G->Char);
        G->BearX = 0;
        G->BearY = 1;
        G->Advance = Ftg->advance.x >> 6;
        G->HasBmp = 1;
        G->Bmp.Resize(1, 1);
        G->Bmp.Fill({});
      }
      else {
        G->BearX = Ftg->bitmap_left;
        G->BearY = Ftg->bitmap_top;
        G->Advance = Ftg->advance.x >> 6;
        G->HasBmp = 2;
        G->Bmp.Resize(Ftb.width, Ftb.rows);
        if (G->AntiAliasing) {
          for (auto i = 0u; i < Ftb.rows; ++i)
            for (auto j = 0u; j < Ftb.width; ++j) {
              auto Col = Ftb.buffer[i * Ftb.pitch + j];
              G->Bmp[i][j].R = Col;
              G->Bmp[i][j].G = Col;
              G->Bmp[i][j].B = Col;
#ifdef BMP_ALPHA
              G->Bmp[i][j].A = Col ? 255 : 0;
#endif
            }
        }
        else {
          for (auto i = 0u; i < Ftb.rows; ++i)
            for (auto j = 0u; j < Ftb.width; ++j) {
              auto Col = Ftb.buffer[i * Ftb.pitch + (j >> 3)] & (1u << ((j & 7) ^ 7));
              G->Bmp[i][j].R = Col ? 255 : 0;
              G->Bmp[i][j].G = Col ? 255 : 0;
              G->Bmp[i][j].B = Col ? 255 : 0;
#ifdef BMP_ALPHA
              G->Bmp[i][j].A = Col ? 255 : 0;
#endif
            }
        }
      }
    }
  }
  if (Face)
    FtAss(FT_Done_Face(Face));
  FtAss(FT_Done_FreeType(Lib));
  auto MaxDescent = int32_t{};
  for (auto& G : ToRender)
    if (G->HasBmp == 2)
      MaxDescent = max(MaxDescent, G->Descent());
  auto MaxPadding = ~DescentPadding ? DescentPadding : MaxDescent;
  auto MaxH = size_t{};
  for (auto& G : ToRender) {
    if (G->HasBmp != 2)
      continue;
    if (G->BearX < 0) {
      Warn("BearX is negative (%d) for char (%u), set it to 0", G->BearX, G->Char);
      G->BearX = 0;
    }
    if (G->BearX || G->Descent() != MaxPadding) {
      auto W = G->BearX + (int32_t) G->Bmp.Width();
      auto H = G->BearY + MaxPadding;
      if (W <= 0 || H <= 0) {
        Warn("The bitmap of char (%u) is completely cropped out, a dummy (1x1) bitmap will be generated", G->Char);
        G->HasBmp = 1;
        G->Bmp.Resize(1, 1);
        G->Bmp.Fill({});
        continue;
      }
      auto Bmp = Bitmap(W, H);
      Bmp.Fill({});
      Bmp.Draw(G->Bmp, G->BearX, 0);
      G->Bmp = move(Bmp);
    }
    MaxH = max(MaxH, G->Bmp.Height());
  }
  if (!LnSpacing)
    LnSpacing = ceil((float)(MaxH - MaxDescent * 10 / HeightConstant)) + LnSpacingOff;
    auto ActualSpacing = HeightConstant * LnSpacing / 10;
  if (ActualSpacing < MaxH)
    Warn("The maximum height (%zu) of newly generated glyphs is larger than Actual Spacing (%u)", MaxH, ActualSpacing);
  if (!CapHeight)
    CapHeight = (Size + CapHeightOff) / 2;
}

pair<size_t, size_t> Font::Extent(wstring_view Str) {
  auto NLine = (uint32_t) count(Str.begin(), Str.end(), L'\n') + 1;
  auto W = size_t{0};
  auto H = size_t{NLine * LnSpacing};
  auto X = size_t{0};
  auto XMax = size_t{0};
  auto HCur = H - LnSpacing;
  for (auto Ch : Str) {
    if (Ch == L'\n') {
      --NLine;
      W = max(W, XMax);
      XMax = 0u;
      HCur -= LnSpacing;
      X = 0u;
      continue;
    }
    auto& G = Glyphs[Ch];
    if (!G->HasBmp)
      Abort("No bitmap for char (%d)", (int) Ch);
    H = max(H, HCur + G->Bmp.Height());
    XMax = max(XMax, X + G->BearX + G->Bmp.Width());
    X += G->Advance;
  }
  W = max(W, XMax);
  return {W, H};
}

Bitmap Font::Render(wstring_view Str) {
  auto [W, H] = Extent(Str);
  auto NLine_ = (int32_t) count(Str.begin(), Str.end(), L'\n');
  auto X = (int32_t) 0;
  auto Y = (int32_t) (H - NLine_ * LnSpacing);
  Bitmap Bmp(W, H);
  Bmp.Fill({});
  for (auto Ch : Str) {
    if (Ch == L'\n') {
      X = 0u;
      Y += LnSpacing;
      continue;
    }
    auto& G = Glyphs[Ch];
    if (!G->HasBmp)
      Abort("No bitmap for char (%d)", (int) Ch);
    Bmp.Draw(G->Bmp, X + G->BearX, Y - G->BearY);
    X += G->Advance;
  }
  return Bmp;
}

void Font::Dump(Sprite& Spr, FontTable& Tbl) {
  auto NChar = 0u;
  for (auto Ch = 0u; Ch < Glyphs.size(); ++Ch)
    if (Glyphs[Ch])
      ++NChar;
  Tbl.Hdr.Sign = TblSign;
  Tbl.Hdr.One = 1;
  Tbl.Hdr.UnkHZ = UnkHZ;
  Tbl.Hdr.NChar = Cast<uint16_t>(NChar, "Too many chars (%u)", NChar);
  Tbl.Hdr.LnSpacing = LnSpacing;
  Tbl.Hdr.CapHeight = CapHeight;
  Tbl.Chrs.reset(new TblChar[NChar]);
  Spr.Resize(1, NChar);
  auto Id = 0u;
  for (auto Ch = 0u; Ch < Glyphs.size(); ++Ch)
    if (Glyphs[Ch]) {
      auto& C = Tbl.Chrs[Id];
      auto& G = Glyphs[Ch];
      if (!G->HasBmp)
        Abort("No bitmap for char (%u)", Ch);
      C.Char = G->Char;
      C.UnkCZ1 = 0;
      C.Width = Cast<uint8_t>(G->Advance, "The advance of char (%u) is too large (%u)", Ch, G->Advance);
      C.Height = Cast<uint8_t>(G->Bmp.Height(), "The height of char (%u) is too large (%zu)", Ch, G->Bmp.Height());
      C.UnkTwo = G->UnkTwo;
      C.UnkCZ2 = 0;
      C.Dc6Index = (uint16_t) Id;
      C.ZPad1 = 0;
      C.ZPad2 = 0;
      Spr[0][Id] = move(G->Bmp);
      ++Id;
    }
  Assert(Id == NChar);
}
