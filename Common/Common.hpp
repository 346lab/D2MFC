#pragma once

#include <algorithm>
#include <array>
#include <cctype>
#include <cinttypes>
#include <cstdarg>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

using namespace std;

void Warn(const char* Fmt, ...);

[[noreturn]]
void AbortV(const char* Fmt, va_list Args);

[[noreturn]]
void Abort(const char* Fmt, ...);

template<class T, class U>
T Cast(U Val, const char* Fmt, ...) {
  if (static_cast<T>(Val) != Val) {
    va_list Args;
    va_start(Args, Fmt);
    AbortV(Fmt, Args);
    va_end(Args);
  }
  return static_cast<T>(Val);
}

#define Assert(e_) ((void) ((e_) || (Abort("Assertion failed: " # e_ "\n"), 0)))
