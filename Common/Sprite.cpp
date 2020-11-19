#include "AutoFile.hpp"
#include "Sprite.hpp"

namespace {
  constexpr uint32_t Dc6HdrVer = 0x00000006;
  constexpr uint32_t Dc6HdrUnk1 = 0x00000001;

#ifdef BMP_ALPHA
  void ReadDc6Frame(AutoFile& File, Bitmap& Bmp, const Dc6FrameHeader& Frm, const Palette& Pal) {
#else
  void ReadDc6Frame(AutoFile& File, Bitmap& Bmp, const Dc6FrameHeader& Frm, const Palette& Pal, uint32_t Mask) {
#endif
    Bmp.Resize(Frm.Width, Frm.Height);
#ifdef BMP_ALPHA
    Bmp.Fill({});
#else
    Bmp.Fill(Mask);
#endif
    auto y = Bmp.Height() - 1;
    auto x = size_t{0};
    for (auto i = 0u; i < Frm.Length; ++i) {
      auto b = File.Get<uint8_t>();
      if (b == 0x80) {
        x = 0;
        --y;
      }
      else if (b & 0x80)
        x += b & 0x7f;
      else {
        for (auto j = 0u; j < b; ++j, ++i) {
          auto c = File.Get<uint8_t>();
          if (c > 255)
            Abort("Invalid color index: %u", c);
          if (y >= Bmp.Height() || x >= Bmp.Width())
            Abort("Invalid position (%zu,%zu)", x, y);
          Bmp[y][x++] = Pal[c];
        }
      }
    }
  }

#ifdef BMP_ALPHA
  void WriteDc6Frame(AutoFile& File, const Bitmap& Bmp, PalEncoder& Enc) {
#else
  void WriteDc6Frame(AutoFile& File, const Bitmap& Bmp, PalEncoder& Enc, uint32_t Mask) {
#endif
    if (Bmp.Count()) {
      auto Done = false;
      auto y = Bmp.Height() - 1;
      auto x = size_t{0};
      auto n = 0u;
      for (;;) {
#ifdef BMP_ALPHA
        while (x + n < Bmp.Width() && !Bmp[y][x + n].A)
          ++n;
#else
        while (x + n < Bmp.Width() && Bmp[y][x + n].Rgb() == Mask)
          ++n;
#endif
        if (x + n == Bmp.Width()) {
          // End of Line
          File.Put<uint8_t>(0x80);
          x = 0;
          n = 0;
          if (!y--)
            break;
          continue;
        }
        if (n) {
          // Transparent
          while (n > 0x7f) {
            File.Put<uint8_t>(0xff);
            x += 0x7f;
            n -= 0x7f;
          }
          File.Put<uint8_t>(n | 0x80);
          x += n;
          n = 0;
        }
#ifdef BMP_ALPHA
        while (x + n < Bmp.Width() && Bmp[y][x + n].A)
          ++n;
#else
        while (x + n < Bmp.Width() && Bmp[y][x + n].Rgb() != Mask)
          ++n;
#endif
        if (n) {
          // Colors
          while (n > 0x7f) {
            File.Put<uint8_t>(0x7f);
            for (auto i = 0u; i < 0x7f; ++i)
              File.Put<uint8_t>(Enc.Encode(Bmp[y][x + i]));
            x += 0x7f;
            n -= 0x7f;
          }
          File.Put<uint8_t>(n);
          for (auto i = 0u; i < n; ++i)
            File.Put<uint8_t>(Enc.Encode(Bmp[y][x + i]));
          x += n;
          n = 0;
        }
      }
    }
    uint8_t Term[3]{0xee, 0xee, 0xee};
    File.Put(Term, 3);
  }
}

#ifdef BMP_ALPHA
void Sprite::ReadDc6(const char* Path, const Palette& Pal) {
#else
void Sprite::ReadDc6(const char* Path, const Palette& Pal, uint32_t Mask) {
#endif
  auto File = AutoFile(Path, "rb");
  auto Hdr = File.Get<Dc6Header>();
  if (Hdr.Version != Dc6HdrVer)
    Abort("DC6 file should start with %.8x instead of %.8x", Dc6HdrVer, Hdr.Version);
  Resize(Hdr.NDir, Hdr.NFrm);
  auto Offs = RcArray<uint32_t>(Hdr.NDir, Hdr.NFrm);
  File.Get(Offs.Raw(), Offs.Count());
  for (uint32_t IDir = 0; IDir < Hdr.NDir; ++IDir)
    for (uint32_t IFrm = 0; IFrm < Hdr.NFrm; ++IFrm) {
      auto Frm = File.GetAt<Dc6FrameHeader>(Offs[IDir][IFrm]);
#ifdef BMP_ALPHA
      ReadDc6Frame(File, (*this)[IDir][IFrm], Frm, Pal);
#else
      ReadDc6Frame(File, (*this)[IDir][IFrm], Frm, Pal, Mask);
#endif
    }
}

#ifdef BMP_ALPHA
void Sprite::SaveDc6(const char* Path, const Palette& Pal) {
#else
void Sprite::SaveDc6(const char* Path, const Palette& Pal, uint32_t Mask) {
#endif
  Dc6Header Hdr;
  Hdr.Version = Dc6HdrVer;
  Hdr.Unk1 = Dc6HdrUnk1;
  Hdr.UnkZ = 0;
  Hdr.Term = 0xeeeeeeee;
  Hdr.NDir = Cast<uint32_t>(NDir(), "Too many directions (%zu)", NDir());
  Hdr.NFrm = Cast<uint32_t>(NFrm(), "Too many frames (%zu)", NFrm());
  RcArray<uint32_t> Offs(NDir(), NFrm());
  auto File = AutoFile(Path, "wb");
  File.Put(Hdr);
  auto FpOffs = File.Tell();
  File.Advance(sizeof(uint32_t) * Offs.Count());
  PalEncoder Enc(Pal);
  for (auto IDir = 0u; IDir < NDir(); ++IDir)
    for (auto IFrm = 0u; IFrm < NFrm(); ++IFrm) {
      auto& Bmp = (*this)[IDir][IFrm];
      Dc6FrameHeader Frm;
      Frm.Flip = 0;
      Frm.Width = (uint32_t) Bmp.Width();
      Frm.Height = (uint32_t) Bmp.Height();
      Frm.OffsetX = 0;
      Frm.OffsetY = 0;
      Frm.Unk = 0;
      Offs[IDir][IFrm] = Cast<uint32_t>(File.Tell(), "The resulted DC6 file is too large (%zu bytes)", File.Size());
      File.Advance(sizeof(Dc6FrameHeader));
      auto FpBeg = File.Tell();
#ifdef BMP_ALPHA
      WriteDc6Frame(File, Bmp, Enc);
#else
      WriteDc6Frame(File, Bmp, Enc, Mask);
#endif
      auto FpEnd = File.Tell();
      Frm.NextBlock = (uint32_t) FpEnd;
      Frm.Length = (uint32_t) (FpEnd - FpBeg - 3);
      File.PutAt(Frm, Offs[IDir][IFrm]);
      File.Seek(FpEnd);
    }
  File.PutAt(Offs.Raw(), FpOffs, Offs.Count());
}
