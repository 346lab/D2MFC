#pragma once

#include "Common.hpp"
#include "RcArray.hpp"

struct Pixel {
  uint8_t R;
  uint8_t G;
  uint8_t B;
#ifdef BMP_ALPHA
  uint8_t A;
#endif

  Pixel() noexcept = default;
  constexpr Pixel(uint8_t R_, uint8_t G_, uint8_t B_) noexcept :
    R(R_), G(G_), B(B_) {}
  constexpr Pixel(uint32_t Val) : R((uint8_t) (Val >> 16)), G((uint8_t) (Val >> 8)), B((uint8_t) Val)
#ifdef BMP_ALPHA
    , A((uint8_t) (Val >> 24))
#endif
  {}

  constexpr uint32_t Rgb() const noexcept {
    return (uint32_t) R << 16 | (uint32_t) G << 8 | B;
  }
};

class Palette : public array<Pixel, 256> {
public:
  using array::array;

  uint8_t Encode(const Pixel& Pix) const noexcept;

  void ReadDat(const char* Path);
};

class PalEncoder {
public:
  PalEncoder(const Palette& Pal) noexcept;
  uint8_t Encode(const Pixel& Pix) noexcept;
private:
  const Palette* Pal;
  unordered_map<uint32_t, uint8_t> Map;
};

class Bitmap : public RcArray<Pixel> {
public:
  constexpr Bitmap() noexcept = default;
  Bitmap(const Bitmap&) noexcept = default;
  Bitmap(Bitmap&&) noexcept = default;
  Bitmap(size_t W, size_t H) noexcept : RcArray(H, W) {}

  Bitmap& operator =(const Bitmap&) noexcept = default;
  Bitmap& operator =(Bitmap&&) noexcept = default;

  constexpr size_t Width() const noexcept { return NCol(); }
  constexpr size_t Height() const noexcept { return NRow(); }

  void Resize(size_t W, size_t H) { RcArray::Resize(H, W); }

  void SavePng(const char* Path);

#ifdef BMP_ALPHA
  void Draw(const Bitmap& Bmp, int32_t X, int32_t Y);
#else
  void Draw(const Bitmap& Bmp, int32_t X, int32_t Y, uint32_t Mask = 0x000000);
#endif
private:
  using RcArray::NRow;
  using RcArray::NCol;
};
