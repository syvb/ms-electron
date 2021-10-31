// Copyright (c) 2019 Microsoft Corporation.

#include "microsoft/src/media_blink/triple_buffer.h"

namespace microsoft {

constexpr auto READ_MASK = 0b00000011;
constexpr auto READY_MASK = 0b00001100;
constexpr auto WRITE_MASK = 0b00110000;
constexpr auto WRITE_FLAG = 0b01000000;

TripleBuffer::TripleBuffer(atomic_t* ptr) {
  Create(ptr);
}

bool TripleBuffer::IsValid() const {
  return !!m_flags;
}

void TripleBuffer::Attach(atomic_t* ptr) {
  m_flags = ptr;
}

void TripleBuffer::Create(atomic_t* ptr) {
  m_flags = ptr;
  // initially Write = 0, Ready = 1 and Read = 2
  m_flags->store(0b00000110, std::memory_order_relaxed);
}

unsigned int TripleBuffer::GetRead() const {
  return (m_flags->load(std::memory_order_consume) & READ_MASK) >> 0;
}

unsigned int TripleBuffer::GetWrite() const {
  return (m_flags->load(std::memory_order_consume) & WRITE_MASK) >> 4;
}

bool TripleBuffer::IsWrite() const {
  return ((m_flags->load(std::memory_order_consume) & WRITE_FLAG) != 0);
}

bool TripleBuffer::SwapRead() {
  flags_t flagsNow(m_flags->load(std::memory_order_consume));
  do {
    if ((flagsNow & WRITE_FLAG) == 0)  // nothing new, no need to swap
      return false;
  } while (!m_flags->compare_exchange_weak(
      flagsNow, SwapReadWithReady(flagsNow), std::memory_order_release,
      std::memory_order_consume));

  return true;
}

void TripleBuffer::SwapWrite() {
  flags_t flagsNow(m_flags->load(std::memory_order_consume));
  while (!m_flags->compare_exchange_weak(
      flagsNow, SetWriteAndSwapReadyWithWrite(flagsNow),
      std::memory_order_release, std::memory_order_consume)) {
  }
}

TripleBuffer::flags_t TripleBuffer::SwapReadWithReady(flags_t flags) {
  // swap Read with Ready
  return (flags & WRITE_MASK) | ((flags & READ_MASK) << 2) |
         ((flags & READY_MASK) >> 2);
}

TripleBuffer::flags_t TripleBuffer::SetWriteAndSwapReadyWithWrite(
    flags_t flags) {
  // set newWrite bit to 1 and swap Write with Ready
  return WRITE_FLAG | ((flags & READY_MASK) << 2) |
         ((flags & WRITE_MASK) >> 2) | (flags & READ_MASK);
}

}  // namespace microsoft
