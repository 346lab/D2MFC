#pragma once

#include "Common.hpp"

struct TblHeader {
    uint32_t Sign;      // +00 - 0x216f6f57 (Woo!)
    uint16_t One;       // +04 - 0x0001
    uint16_t UnkHZ;     // +06 - mostly 0x0000
    uint16_t NChar;     // +08
    uint8_t  LnSpacing; // +0A
    uint8_t  CapHeight; // +0B
};

struct TblChar {
    uint16_t Char;           // +00
    uint8_t  UnkCZ1;         // +02 - mostly 0x00
    uint8_t  Width;          // +03
    uint8_t  Height;         // +04
    uint8_t  UnkTwo;         // +05 - mostly 1, seldomly 0
    uint16_t UnkCZ2;         // +06 - mostly 0x0000
    uint16_t Dc6Index;       // +08
    uint16_t ZPad1;          // +0A - 0x0000
    uint16_t ZPad2;          // +0A - 0x0000
};

struct FontTable {
  TblHeader Hdr;
  unique_ptr<TblChar[]> Chrs;
  void ReadTbl(const char* Path);
  void SaveTbl(const char* Path);
};

constexpr uint32_t TblSign = 0x216f6f57;
