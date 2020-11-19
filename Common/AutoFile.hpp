#pragma once

#include "Common.hpp"

class AutoFile final {
public:
  constexpr AutoFile() noexcept = default;
  AutoFile(const AutoFile&) = delete;
  AutoFile(AutoFile&& Another) noexcept;
  AutoFile(const char* Path, const char* Mode) noexcept;
  ~AutoFile();

  AutoFile& operator =(const AutoFile&) = delete;
  AutoFile& operator =(AutoFile&& Another) noexcept;

  void Swap(AutoFile& Another) noexcept;

  constexpr FILE* Raw() noexcept { return File; }

  void Open(const char* Path, const char* Mode) noexcept;
  void Close() noexcept;
  size_t Size() noexcept;

  size_t Tell();
  void Seek(size_t Off);
  void Advance(ptrdiff_t Off);

  template<class T>
  T Get() noexcept {
    T Res;
    Get(&Res, 1);
    return Res;
  }

  template<class T>
  void Put(const T& Obj) noexcept {
    Put(&Obj, 1);
  }

  template<class T>
  T GetAt(size_t Offset) noexcept {
    T Res;
    GetAt(&Res, Offset, 1);
    return Res;
  }

  template<class T>
  void PutAt(const T& Obj, size_t Offset) noexcept {
    PutAt(&Obj, Offset, 1);
  }


  template<class T>
  void Get(T* Ptr, size_t Count) noexcept {
    auto Size = sizeof(T) * Count;
    if (Size != fread(Ptr, 1, Size, File))
      Abort("Failed to read %zu bytes\n", Size);
  }

  template<class T>
  void Put(const T* Ptr, size_t Count) noexcept {
    auto Size = sizeof(T) * Count;
    if (Size != fwrite(Ptr, 1, Size, File))
      Abort("Failed to write %zu bytes\n", Size);
  }

  template<class T>
  void GetAt(T* Ptr, size_t Offset, size_t Count) noexcept {
    if (fseek(File, (long) Offset, SEEK_SET))
      Abort("Failed to set the file pointer to %zu bytes\n", Offset);
    auto Size = sizeof(T) * Count;
    if (Size != fread(Ptr, 1, Size, File))
      Abort("Failed to read %zu bytes at %zu bytes\n", Size, Offset);
  }

  template<class T>
  void PutAt(const T* Ptr, size_t Offset, size_t Count) noexcept {
    if (fseek(File, (long) Offset, SEEK_SET))
      Abort("Failed to set the file pointer to %zu bytes\n", Offset);
    auto Size = sizeof(T) * Count;
    if (Size != fwrite(Ptr, 1, Size, File))
      Abort("Failed to write %zu bytes at %zu bytes\n", Size, Offset);
  }

  string ReadAll() noexcept;
private:
  FILE* File = nullptr;
};
