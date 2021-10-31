// Copyright (c) 2019 Microsoft Corporation.

#ifndef SRC_MEDIA_BLINK_TRIPLE_BUFFER_H_
#define SRC_MEDIA_BLINK_TRIPLE_BUFFER_H_

#include <atomic>

#include "base/macros.h"

// This implementation must behave exactly the same as
// https://skype.visualstudio.com/SCC/_git/client-shared_electron_slimcore?path=%2Fsrc%2Futils%2FTripleBuffer.h&version=GBmaster

namespace microsoft {

class TripleBuffer {
 public:
  using flags_t = unsigned int;
  using atomic_t = std::atomic<flags_t>;

  TripleBuffer() = default;
  explicit TripleBuffer(atomic_t* ptr);

  bool IsValid() const;

  void Attach(atomic_t* ptr);
  void Create(atomic_t* ptr);

  unsigned int GetRead() const;   // get current Read buffer
  unsigned int GetWrite() const;  // get current Write buffer
  bool IsWrite() const;           // returns true if Write flag is set

  bool SwapRead();   // swap Read and Ready if there was new Write
  void SwapWrite();  // swap Write and Ready

  static const size_t FLAG_SIZE = sizeof(flags_t);

 private:
  flags_t SwapReadWithReady(flags_t flags);  // swap Read and Ready indexes
  flags_t SetWriteAndSwapReadyWithWrite(
      flags_t flags);  // set newWrite to 1 and swap Ready and Write indexes

  // 8 bit flags are (unused) (new write) (2x write) (2x ready) (2x read)
  // newWrite   = (flags & 0b01000000)
  // WriteIndex = (flags & 0b00110000) >> 4
  // ReadyIndex = (flags & 0b00001100) >> 2
  // ReadIndex  = (flags & 0b00000011) >> 0
  atomic_t* m_flags = nullptr;

  DISALLOW_COPY_AND_ASSIGN(TripleBuffer);
};

}  // namespace microsoft

#endif  // SRC_MEDIA_BLINK_TRIPLE_BUFFER_H_
