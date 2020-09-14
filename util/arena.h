// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_UTIL_ARENA_H_
#define STORAGE_LEVELDB_UTIL_ARENA_H_

#include <atomic>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <numa.h>
#include "leveldb/options.h"

namespace leveldb {

class Arena {
 public:
  Arena();
  Arena(const size_t size);
  Arena(const Arena*);
  
  Arena(const Arena&) = delete;
  Arena& operator=(const Arena&) = delete;

  ~Arena();

  // Return a pointer to a newly allocated memory block of "bytes" bytes.
  char* Allocate(size_t bytes);

  // Allocate memory with the normal alignment guarantees provided by malloc.
  char* AllocateAligned(size_t bytes);

  // Returns an estimate of the total memory usage of data allocated
  // by the arena.
  size_t MemoryUsage() const {
	if (IsMemTable) {
		return memory_usage_.load(std::memory_order_relaxed);
	} else {
		size_t tmp = 0;
		for (int i = 0; i < block_size_.size(); i++) {
			tmp += block_size_[i];
		}
		return tmp;
	}
  }

  char* GetHead() {
    assert(blocks_.size() > 0);
    return blocks_[0];
  }

  void SetTransfer() {
    Transfer = true;
  }

  void ReceiveArena(Arena* a);

 private:
  bool IsMemTable;
  bool Transfer;
  char* AllocateFallback(size_t bytes);
  char* AllocateNewBlock(size_t block_bytes);

  // Allocation state
  char* alloc_ptr_;
  size_t alloc_bytes_remaining_;

 public:
  // Array of new[] allocated memory blocks
  std::vector<char*> blocks_;
  // mark large block
  std::vector<size_t> block_size_;

  // Total memory usage of the arena.
  //
  // TODO(costan): This member is accessed via atomics, but the others are
  //               accessed without any locking. Is this OK?
 private:
  std::atomic<size_t> memory_usage_;
  int kMemSize;
};

inline char* Arena::Allocate(size_t bytes) {
  // The semantics of what to return are a bit messy if we allow
  // 0-byte allocations, so we disallow them here (we don't need
  // them for our internal use).
  assert(bytes > 0);
  if (bytes <= alloc_bytes_remaining_) {
    char* result = alloc_ptr_;
    alloc_ptr_ += bytes;
    alloc_bytes_remaining_ -= bytes;
    if (IsMemTable) {
      memory_usage_.fetch_add(bytes, std::memory_order_relaxed);
    }
    return result;
  }
  return AllocateFallback(bytes);
}

}  // namespace leveldb

#endif  // STORAGE_LEVELDB_UTIL_ARENA_H_
