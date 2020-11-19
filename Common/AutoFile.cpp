#include "AutoFile.hpp"

AutoFile::AutoFile(AutoFile&& Another) noexcept : File(exchange(Another.File, nullptr)) {}

AutoFile::AutoFile(const char* Path, const char* Mode) noexcept {
  Open(Path, Mode);
}

AutoFile::~AutoFile() {
  Close();
}

AutoFile& AutoFile::operator=(AutoFile&& Another) noexcept {
  Another.Swap(*this);
  Another.Close();
  return *this;
}

void AutoFile::Swap(AutoFile& Another) noexcept {
  swap(File, Another.File);
}

void AutoFile::Open(const char* Path, const char* Mode) noexcept {
  File = fopen(Path, Mode);
  if (!File)
    Abort("Failed to open %s as [%s]", Path, Mode);
}

void AutoFile::Close() noexcept {
  if (File) {
    fclose(File);
    File = nullptr;
  }
}

size_t AutoFile::Size() noexcept {
  auto Pos = ftell(File);
  fseek(File, 0, SEEK_END);
  auto Size = (size_t) ftell(File);
  fseek(File, Pos, SEEK_SET);
  return Size;
}

size_t AutoFile::Tell() {
  return (size_t) ftell(File);
}

void AutoFile::Seek(size_t Off) {
  if (fseek(File, (long) Off, SEEK_SET))
    Abort("Failed to set the file pointer to %zu bytes", Off);
}

void AutoFile::Advance(ptrdiff_t Off) {
  if (fseek(File, (long) Off, SEEK_CUR))
    Abort("Failed to advance for %zd bytes", Off);
}

string AutoFile::ReadAll() noexcept {
  auto NByte = Size();
  fseek(File, 0, SEEK_SET);
  string Res(NByte, '\0');
  Get(Res.data(), NByte);
  return Res;
}
