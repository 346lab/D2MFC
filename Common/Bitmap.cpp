#include "AutoFile.hpp"
#include "Bitmap.hpp"

#include <png.h>

namespace {
  constexpr uint32_t Dis2(uint32_t A, uint32_t B) {
    auto Diff = (int32_t) A - (int32_t) B;
    return (uint32_t) (Diff * Diff);
  }

  constexpr uint32_t Dis2(const Pixel& A, const Pixel& B) {
    return Dis2(A.R, B.R) + Dis2(A.G, B.G) + Dis2(A.B, B.B);
  }

#ifdef BMP_ALPHA
  constexpr Pixel AlphaBlend(const Pixel& Src, const Pixel& Dst) {
    auto Sx = Src.A * 255u;
    auto Dx = Dst.A * (255u - Src.A);
    auto Ax = Sx + Dx;
    if (!Ax)
      return {};
    auto R = (uint8_t) ((Sx * Src.R + Dx * Dst.R) / Ax);
    auto G = (uint8_t) ((Sx * Src.G + Dx * Dst.G) / Ax);
    auto B = (uint8_t) ((Sx * Src.B + Dx * Dst.B) / Ax);
    auto A = (uint8_t) (Ax / 255);
    return {R, G, B, A};
  }
#endif
}

void Bitmap::SavePng(const char* Path) {
  auto File = AutoFile(Path, "wb");
  auto Png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
  if (!Png)
    Abort("Failed to create png write struct");
  auto Info = png_create_info_struct(Png);
  if (!Info)
    Abort("Failed to create png info struct");
  if (setjmp(png_jmpbuf(Png)))
    Abort("Failed to write png");
  png_set_IHDR(Png, Info, (uint32_t) Width(), (uint32_t) Height(), 8,
#ifdef BMP_ALPHA
    PNG_COLOR_TYPE_RGBA,
#else
    PNG_COLOR_TYPE_RGB,
#endif
    PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
  vector<png_byte*> Rows(Height());
  for (auto y = 0u; y < Height(); ++y)
    Rows[y] = (png_byte*) (*this)[y];
  png_init_io(Png, File.Raw());
  png_set_rows(Png, Info, Rows.data());
  png_write_png(Png, Info, PNG_TRANSFORM_IDENTITY, nullptr);
  png_destroy_write_struct(&Png, &Info);
}

#ifdef BMP_ALPHA
void Bitmap::Draw(const Bitmap& Bmp, int32_t X, int32_t Y) {
#else
void Bitmap::Draw(const Bitmap& Bmp, int32_t X, int32_t Y, uint32_t Mask) {
#endif
  auto XD = X < 0 ? 0 : X;
  auto YD = Y < 0 ? 0 : Y;
  auto XS = X < 0 ? -X : 0;
  auto YS = Y < 0 ? -Y : 0;
  auto W = min((int32_t) Width() - XD, (int32_t) Bmp.Width() - XS);
  auto H = min((int32_t) Height() - YD, (int32_t) Bmp.Height() - YS);
  if (W <= 0 || H <= 0) {
    Warn("No bitmap is drawn at (%d,%d)->(%d,%d); canvas size is ($zux%zu)",
      X, Y, X + (int32_t) Bmp.Width(), Y + (int32_t) Bmp.Width(), Width(), Height());
    return;
  }
  if (W != Bmp.Width() || H != Bmp.Height())
    Warn("Bitmap is cropped from (%zu,%zu) to (%d,%d)", Bmp.Width(), Bmp.Height(), W, H);
  for (auto y = 0; y < H; ++y)
    for (auto x = 0; x < W; ++x) {
#ifdef BMP_ALPHA
      (*this)[y + Y][x + X] = AlphaBlend(Bmp[y][x], (*this)[y + Y][x + X]);
#else
      if (Bmp[y + YS][x + XS].Rgb() != Mask)
        (*this)[y + YD][x + XD] = Bmp[y + YS][x + XS];
#endif
    }
}

uint8_t Palette::Encode(const Pixel& Pix) const noexcept {
  auto Res = ~0u;
  auto MinDiff = ~0u;
  for (auto i = 0u; i < 256; ++i) {
    auto Diff = Dis2(Pix, (*this)[i]);
    if (Diff < MinDiff) {
      MinDiff = Diff;
      Res = i;
    }
  }
  return (uint8_t) Res;
}

void Palette::ReadDat(const char* Path) {
  if (!strcmp(Path, "null")) {
    for (auto i = 0; i < 256; ++i) {
      (*this)[i].R = i;
      (*this)[i].G = i;
      (*this)[i].B = i;
#ifdef BMP_ALPHA
      (*this)[i].A = 255;
#endif
    }
    return;
  }
  auto File = AutoFile(Path, "rb");
  auto Size = File.Size();
  if (Size != 768)
    Abort("Incorrect file size: expected 768 bytes, but %zu bytes", Size);
  uint8_t Vals[3];
  for (auto i = 0; i < 256; ++i) {
    File.Get(Vals, 3);
    (*this)[i].R = Vals[2];
    (*this)[i].G = Vals[1];
    (*this)[i].B = Vals[0];
#ifdef BMP_ALPHA
    (*this)[i].A = 255;
#endif
  }
}

PalEncoder::PalEncoder(const Palette& Pal) noexcept : Pal(&Pal) {}

uint8_t PalEncoder::Encode(const Pixel& Pix) noexcept {
  auto Res = Map.find(Pix.Rgb());
  if (Res != Map.end())
    return Res->second;
  auto Idx = Pal->Encode(Pix);
  Map.emplace(Pix.Rgb(), Idx);
  return Idx;
}
