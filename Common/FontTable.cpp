#include "AutoFile.hpp"
#include "FontTable.hpp"
#include "Sprite.hpp"

void FontTable::ReadTbl(const char* Path) {
  auto File = AutoFile(Path, "rb");
  File.Get(&Hdr, 1);
  if (Hdr.Sign != TblSign)
    Abort("TBL file should start with %.8x instead of %.8x", TblSign, Hdr.Sign);
  if (!Hdr.NChar)
    Abort("TBL file contains no characters");
  Chrs.reset(new TblChar[Hdr.NChar]);
  File.Get(Chrs.get(), Hdr.NChar);
  auto NeedSort = false;
  for (auto i = 1u; i < Hdr.NChar; ++i)
    if (Chrs[i - 1].Char >= Chrs[i].Char)
      Warn("The %u-th char (%u) is not less than %u-th char (%u)", i - 1, Chrs[i - 1].Char, i, Chrs[i]);
  if (!NeedSort)
    return;
  Warn("Will sort the font table");
  stable_sort(Chrs.get(), Chrs.get() + Hdr.NChar,
    [](const TblChar& A, const TblChar& B) { return A.Char < B.Char; }
  );
  auto N = 0u;
  for (auto i = 0u; i < Hdr.NChar; ) {
    auto j = i;
    while (i < Hdr.NChar && Chrs[i].Char == Chrs[j].Char)
      ++i;
    if (i - j > 1)
      Warn("Found %u occurrences of char (%u), only the last occurrence will be kept", i - j, Chrs[j].Char);
    Chrs[N++] = Chrs[i - 1];
  }
  if (Hdr.NChar != N) {
    Warn("The number of chars are shrinked from %u to %u due to duplicates", Hdr.NChar, N);
    Hdr.NChar = N;
  }
}

void FontTable::SaveTbl(const char* Path) {
  auto File = AutoFile(Path, "wb");
  File.Put(Hdr);
  File.Put(Chrs.get(), Hdr.NChar);
}
