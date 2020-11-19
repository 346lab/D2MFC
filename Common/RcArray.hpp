#pragma once

#include "Common.hpp"

template<class Elem>
class RcArray {
public:
  constexpr RcArray() noexcept = default;

  RcArray(const RcArray& Another) noexcept { *this = Another; }

  RcArray(RcArray&& Another) noexcept = default;

  RcArray(size_t R, size_t C) noexcept { Resize(R, C); }

  RcArray& operator =(const RcArray& Another) noexcept {
    Resize(Another.NR, Another.NC);
    copy(Another.Raw(), Another.Raw() + NR * NC, Raw());
    return *this;
  }

  RcArray& operator =(RcArray&& Another) noexcept = default;

  constexpr size_t NRow() const noexcept { return NR; }
  constexpr size_t NCol() const noexcept { return NC; }
  constexpr size_t Count() const noexcept { return NR * NC; }

  Elem* Raw() noexcept { return Data.get();  }
  const Elem* Raw() const noexcept { return Data.get(); }

  void Resize(size_t R, size_t C) noexcept {
    if (NR * NC < R * C)
      Data.reset(new Elem[R * C]);
    NR = R;
    NC = C;
  }

  void Fill(const Elem& Val) { fill(Raw(), Raw() + Count(), Val); }
  
  Elem* operator [](size_t R) noexcept { return Raw() + R * NC; }
  const Elem* operator [](size_t R) const noexcept { return Raw() + R * NC; }
private:
  size_t NR = 0;
  size_t NC = 0;
  unique_ptr<Elem[]> Data;
};
