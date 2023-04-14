#pragma once

#include "Bitmap.hpp"
#include "Common.hpp"

struct Dc6Header {
  uint32_t Version;   // +00 - 0x00000006
  uint32_t Unk1;      // +04 - 0x00000001
  uint32_t UnkZ;      // +08 - 0x00000000
  uint32_t Term;      // +0c - 0xeeeeeeee or 0xcdcdcdcd
  uint32_t NDir;      // +10 - #Direction
  uint32_t NFrm;      // +14 - #Frame per Direction
};

struct Dc6FrameHeader {
  uint32_t Flip;      // +00 - 1 if Flipped, 0 else
  uint32_t Width;     // +04
  uint32_t Height;    // +08
  int32_t OffsetX;   // +0c
  int32_t OffsetY;   // +10
  uint32_t Unk;       // +14 - 0x00000000
  uint32_t NextBlock; // +18
  uint32_t Length;    // +1c
};

class Sprite : public RcArray<Bitmap> {
public:
  using RcArray::RcArray;

  constexpr size_t NDir() const noexcept { return NRow(); }
  constexpr size_t NFrm() const noexcept { return NCol(); }

#ifdef BMP_ALPHA
  void ReadDc6(const char* Path, const Palette& Pal);
  void SaveDc6(const char* Path, const Palette& Pal);
#else
  void ReadDc6(const char* Path, const Palette& Pal, uint32_t Mask = 0x000000);
  void SaveDc6(const char* Path, const Palette& Pal, int32_t Dc6OffsetY = 0, uint32_t Mask = 0x000000);
#endif
private:
  using RcArray::NRow;
  using RcArray::NCol;
};
