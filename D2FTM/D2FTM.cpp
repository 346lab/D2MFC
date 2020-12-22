#define _SILENCE_CXX17_CODECVT_HEADER_DEPRECATION_WARNING

#include <algorithm>
#include <codecvt>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <list>
#include <locale>
#include <string>
#include <utility>
#include <vector>

namespace fs = ::std::filesystem;

using namespace std;

const char* Program;

[[noreturn]]
void ShowHelp() {
  wcout << '\n';
  wcout << Program << ": Diablo II Font Table Master\n";
  wcout << "    by EAirPeter, CaiMiao\n";
  wcout << '\n';
  wcout << "Usage: " << Program << " <Input>.tbl <Output>.txt\n";
  wcout << "       " << Program << " <Input>.txt <Output>.tbl\n";
  wcout << "       " << Program << " <Input>.tbl\n";
  wcout << "       " << Program << " <Input>.txt\n";
  wcout << '\n';
  wcout << "Convert from TBL file to TXT file or vice versa. (The ";
  wcout << "direction of the conversion is determined by the extension ";
  wcout << "of the input file)\n";
  wcout << flush;
  exit(EXIT_FAILURE);
}

constexpr uint32_t TblHeaderVal = 0x216f6f57;
constexpr uint16_t TblOneVal = 0x0001;
constexpr wchar_t TblHeaderStr[] = L"Woo!";

struct TblHeader {
  uint32_t Header;    // +00 - 0x216f6f57 (Woo!)
  uint16_t One;       // +04 - 0x0001
  uint16_t UnkHZ;     // +06 - mostly 0x0000
  uint16_t TotalChar; // +08
  uint8_t LnSpacing;  // +0A
  uint8_t CapHeight;  // +0B
};
static_assert(sizeof(TblHeader) == 12);

struct TblChar {
  char16_t Char;          // +00
  uint8_t UnkCZ1;         // +02 - mostly 0x00
  uint8_t Width;          // +03
  uint8_t Height;         // +04
  bool UnkTwo;            // +05 - mostly 1, seldomly 0
  uint16_t UnkCZ2;        // +06 - mostly 0x0000
  uint16_t Dc6ImageIndex; // +08
  uint16_t ZPad1;         // +0A - 0x0000
  uint16_t ZPad2;         // +0A - 0x0000
};
static_assert(sizeof(TblChar) == 14);

bool ShouldSkip(int Ch) {
  return Ch == '\t' || Ch == '\n' || Ch == '\r';
}

wistream& Skip(wistream& WI) {
  while (WI && ShouldSkip(WI.peek()))
    WI.ignore();
  return WI;
}

struct U8O {
  explicit constexpr U8O(uint8_t U_) : U(U_) {}
  uint8_t U;
};

wostream& operator <<(wostream& WO, U8O P) {
  return WO << (unsigned) P.U;
}

struct U8I {
  explicit constexpr U8I(uint8_t& U_) : U(U_) {}
  uint8_t& U;
};

wistream& operator >>(wistream& WI, U8I P) {
  unsigned U;
  if (!(WI >> U))
    return WI;
  if ((uint8_t) U != U) {
    WI.setstate(ios_base::failbit);
    return WI;
  }
  P.U = (uint8_t) U;
  return WI;
}

struct C16O {
  explicit constexpr C16O(char16_t Chr_) : Chr(Chr_) {}
  char16_t Chr;
};

wostream& operator <<(wostream& WO, C16O P) {
  switch (P.Chr) {
    case u'\t': WO << "\\t"; break;
    case u'\n': WO << "\\n"; break;
    case u'\r': WO << "\\r"; break;
    default: WO.write((wchar_t*) &P.Chr, 1); break;
  }
  return WO;
}

struct C16I {
  explicit constexpr C16I(char16_t& Chr_) : Chr(Chr_) {}
  char16_t& Chr;
};

wistream& operator >>(wistream& WI, C16I P) {
  Skip(WI);
  char16_t Chr;
  if (!WI.read((wchar_t*) &Chr, 1)) {
    WI.setstate(ios_base::failbit);
    return WI;
  }
  if (Chr == u'\\') {
    if (!WI.read((wchar_t*) &Chr, 1)) {
      WI.setstate(ios_base::failbit);
      return WI;
    }
    switch (Chr) {
      case u't': Chr = u'\t'; break;
      case u'n': Chr = u'\n'; break;
      case u'r': Chr = u'\r'; break;
      case u'\t': Chr = u'\\'; break;
      default:
                  wcerr << "Unknown escape sequence [\\" << C16O{Chr};
                  wcerr << "]." << endl;
                  WI.setstate(ios_base::failbit);
                  return WI;
    }
  }
  P.Chr = Chr;
  return WI;
}

template<class UInt>
struct ImplGet {
  wistream& operator ()(wistream& FI, UInt& U) {
    return FI >> U;
  }
};

template<>
struct ImplGet<uint8_t> {
  wistream& operator ()(wistream& FI, uint8_t& U) {
    return FI >> U8I{U};
  }
};

  template<class UInt>
bool GetUInt(wifstream& FI, UInt& Res,  TblChar* Table[],
    UInt TblChar::* Ptr)
{
  Skip(FI);
  if (FI.peek() == '#') {
    FI.ignore();
    char16_t Chr;
    if (!(FI >> C16I{Chr}))
      return false;
    if (!Table[(int) Chr]) {
      wcerr << "No [" << C16O{Chr} << "] found." << endl;
      return false;
    }
    Res = Table[(int) Chr]->*Ptr;
    return true;
  }
  if (!ImplGet<UInt>{}(FI, Res))
    return false;
  return true;
}

int TxtToTbl(const fs::path& PI, const fs::path& PO) {
  auto FI = wifstream(PI, ios_base::binary);
  FI.imbue(locale(FI.getloc(),
        new codecvt_utf16<wchar_t, 0xffff, little_endian>));
  if (!FI) {
    wcerr << "Failed to open [" << PI << "]." << endl;
    return EXIT_FAILURE;
  }
  wstring SHeader;
  if (!(FI >> SHeader) || SHeader != TblHeaderStr) {
    wcerr << "Invalid TXT file." << endl;
    return EXIT_FAILURE;
  }
  TblHeader Hdr;
  Hdr.Header = TblHeaderVal;
  Hdr.One = TblOneVal;
  FI >> Hdr.UnkHZ >> Hdr.TotalChar;
  FI >> U8I{Hdr.LnSpacing} >> U8I{Hdr.CapHeight};
  if (!FI) {
    wcerr << "Failed to read header." << endl;
    return EXIT_FAILURE;
  }
  static TblChar* Table[65536] {};
  list<TblChar> Chrs;
  while (!FI.eof()) {
    TblChar Chr;
    if (!(FI >> C16I{Chr.Char})) {
      wcerr << "Failed to read a character." << endl;
      return EXIT_FAILURE;
    }
    Chr.UnkCZ1 = 0x00;
    if (!GetUInt(FI, Chr.Width, Table, &TblChar::Width)) {
      wcerr << "Failed to read Width of [";
      wcerr << C16O{Chr.Char} << "]." << endl;
      return EXIT_FAILURE;
    }
    if (!GetUInt(FI, Chr.Height, Table, &TblChar::Height)) {
      wcerr << "Failed to read Height of [";
      wcerr << C16O{Chr.Char} << "]." << endl;
      return EXIT_FAILURE;
    }
    if (!GetUInt(FI, Chr.UnkTwo, Table, &TblChar::UnkTwo)) {
      wcerr << "Failed to read UnkTwo of [";
      wcerr << C16O{Chr.Char} << "]." << endl;
      return EXIT_FAILURE;
    }
    Chr.UnkCZ2 = 0x0000;
    if (!GetUInt(FI, Chr.Dc6ImageIndex, Table, &TblChar::Dc6ImageIndex)) {
      wcerr << "Failed to read Dc6ImageIndex of [";
      wcerr << C16O{Chr.Char} << "]." << endl;
      return EXIT_FAILURE;
    }
    Chr.ZPad1 = 0x0000;
    Chr.ZPad2 = 0x0000;
    if (Table[Chr.Char]) {
      wcerr << "WARNING: Found duplicated [" << C16O{Chr.Char};
      wcerr << "], both of them will be kept and #reference to ";
      wcerr << "this character will be using the latter one." << endl;
    }
    Chrs.emplace_back(Chr);
    Table[Chrs.back().Char] = &Chrs.back();
    Skip(FI);
  }
  FI.close();
  if (Chrs.size() != Hdr.TotalChar) {
    wcerr << "WARNING: TotalChar in header (" << Hdr.TotalChar;
    wcerr << ") not matching with the number of characters in file (";
    wcerr << Chrs.size() << "), will use the latter one when generating ";
    wcerr << "TBL file." << endl;
    if ((uint16_t) Chrs.size() != Chrs.size()) {
      wcerr << "WARNING: The number of characters in file is greater ";
      wcerr << "than 65535, 65536th and latter characters will be ";
      wcerr << "ignored." << endl;
    }
    Hdr.TotalChar = (uint16_t) Chrs.size();
  }
  auto FO = ofstream(PO, ios_base::binary | ios_base::trunc);
  if (!FO) {
    wcerr << "Failed to create [" << PO << "]." << endl;
    return EXIT_FAILURE;
  }
  FO.write((char*) &Hdr, sizeof(Hdr));
  for (auto& Chr : Chrs)
    FO.write((char*) &Chr, sizeof(Chr));
  if (!FO) {
    wcerr << "Failed to write data." << endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int TblToTxt(const fs::path& PI, const fs::path& PO) {
  auto FI = ifstream(PI, ios_base::binary);
  if (!FI) {
    wcerr << "Failed to open [" << PI << "]." << endl;
    return EXIT_FAILURE;
  }
  TblHeader Hdr;
  if (!FI.read((char*) &Hdr, sizeof(Hdr))) {
    wcerr << "Failed to read file header." << endl;
    return EXIT_FAILURE;
  }
  if (Hdr.Header != TblHeaderVal || Hdr.One != TblOneVal) {
    wcerr << "Invalid TBL file." << endl;
    return EXIT_FAILURE;
  }
  uint16_t MaxDII = 0;
  vector<TblChar> Chrs(Hdr.TotalChar);
  for (auto& Chr : Chrs) {
    if (!FI.read((char*) &Chr, sizeof(Chr))) {
      wcerr << "Failed to read char data." << endl;
      return EXIT_FAILURE;
    }
    MaxDII = max(MaxDII, Chr.Dc6ImageIndex);
  }
  FI.close();
  auto Width = MaxDII < 10000 ? 4 : 5;
  auto FO = wofstream(PO, ios_base::binary | ios_base::trunc);
  FO.imbue(locale(FO.getloc(),
        new codecvt_utf16<wchar_t, 0xffff, little_endian>));
  if (!FO) {
    wcerr << "Failed to create [" << PO << "]." << endl;
    return EXIT_FAILURE;
  }
  auto Bom = (wchar_t) 0xfeff;
  FO.write(&Bom, 1);
  FO << "Woo!\t" << Hdr.UnkHZ << '\t' << Hdr.TotalChar << '\t';
  FO << U8O{Hdr.LnSpacing} << '\t' << U8O{Hdr.CapHeight} << "\r\n";
  for (auto& Chr : Chrs) {
    if (0xd800 <= Chr.Char && Chr.Char <= 0xdfff) {
      wcerr << "WARNING: Found invalid codepoint (" << (uint32_t) Chr.Char <<
        "); It is in surrogate area." << endl;
      continue;
    }
    FO << C16O{Chr.Char} << '\t';
    FO << Chr.Width << '\t' << Chr.Height << '\t';
    FO << (int) Chr.UnkTwo << '\t';
    FO << setfill(L'0') << setw(Width) << Chr.Dc6ImageIndex << "\r\n";
    if (!FO) {
      wcerr << "Failed to write (" << (uint32_t) Chr.Char << ")." << endl;
      break;
    }
  }
  if (!FO) {
    wcerr << "Failed to write data." << endl;
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int main(int NArg, char* Args[]) {
  Program = Args[0];
  if (NArg != 3 && NArg != 2) {
    wcerr << "Incorrect command line." << endl;
    ShowHelp();
  }
  auto InputPath = fs::path{Args[1]};
  if (InputPath.extension() == u".txt") {
    return TxtToTbl(InputPath, NArg == 3 ? Args[2] :
        fs::path{Args[1]}.replace_extension(".tbl"));
  }
  if (InputPath.extension() == u".tbl") {
    return TblToTxt(InputPath, NArg == 3 ? Args[2] :
        fs::path{Args[1]}.replace_extension(".txt"));
  }
  wcerr << "Unrecognized input file extension." << endl;
  return EXIT_FAILURE;
}
