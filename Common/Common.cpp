#include "Common.hpp"

void Warn(const char* Fmt, ...) {
  fputs("[WARN] ", stderr);
  va_list Args;
  va_start(Args, Fmt);
  vfprintf(stderr, Fmt, Args);
  va_end(Args);
  fputc('\n', stderr);
}

[[noreturn]]
void AbortV(const char* Fmt, va_list Args) {
  fputs("[ABORT] ", stderr);
  vfprintf(stderr, Fmt, Args);
  fputc('\n', stderr);
  exit(EXIT_FAILURE);
}

[[noreturn]]
void Abort(const char* Fmt, ...) {
  va_list Args;
  va_start(Args, Fmt);
  AbortV(Fmt, Args);
  va_end(Args);
}
