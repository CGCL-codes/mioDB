// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "util/arena.h"
#include "string.h"
#include "db/global.h"

namespace leveldb {

static const int kBlockSize = 4096;
//static const int kMemSize = 6 * 1024 * 1024;

Arena::Arena()
    : alloc_ptr_(nullptr), alloc_bytes_remaining_(0), memory_usage_(0), IsMemTable(true), Transfer(false), kMemSize(0) {}

Arena::Arena(const size_t size)
    : alloc_ptr_(nullptr), alloc_bytes_remaining_(0), memory_usage_(0), IsMemTable(true), Transfer(false), kMemSize(size) {}

Arena::Arena(const Arena* a): memory_usage_(0), IsMemTable(false), Transfer(false) {
  assert(a->blocks_.size() == 1); //memtable only has one block

  alloc_ptr_ = AllocateNewBlock(a->MemoryUsage());
  
  memcpy(alloc_ptr_, a->blocks_[0], a->MemoryUsage());
  alloc_ptr_ += a->MemoryUsage();
  alloc_bytes_remaining_ = 0;
} 

Arena::~Arena() {
  if (IsMemTable) {
    for (size_t i = 0; i < blocks_.size(); i++) {
      delete[] blocks_[i];
    }

  } else if (!Transfer) {
    int j = 0;
    for (size_t i = 0; i < blocks_.size(); i++) {
        numa_free(blocks_[i], block_size_[i]);
    }

  } else {
    blocks_.clear();
  }
}

char* Arena::AllocateFallback(size_t bytes) {
  // MemTable only alloc one large block (6MB)
  if (IsMemTable) {
    alloc_ptr_ = AllocateNewBlock(kMemSize);
    alloc_bytes_remaining_ = kMemSize;

    char* result = alloc_ptr_;
    alloc_ptr_ += bytes;
    alloc_bytes_remaining_ -= bytes;
    if (IsMemTable) {
      memory_usage_.fetch_add(bytes, std::memory_order_relaxed);
    }
    return result;

  // DataTale alloc smaller block after initialization
  } else {
    if (bytes > kBlockSize / 4) {
    // Object is more than a quarter of our block size.  Allocate it separately
    // to avoid wasting too much space in leftover bytes.
    char* result = AllocateNewBlock(bytes);
    return result;
    }
    alloc_ptr_ = AllocateNewBlock(kBlockSize);
    alloc_bytes_remaining_ = kBlockSize;

    char* result = alloc_ptr_;
    alloc_ptr_ += bytes;
    alloc_bytes_remaining_ -= bytes;
    return result;
  }
}

char* Arena::AllocateAligned(size_t bytes) {
  const int align = (sizeof(void*) > 8) ? sizeof(void*) : 8;
  static_assert((align & (align - 1)) == 0,
                "Pointer size should be a power of 2");
  size_t current_mod = reinterpret_cast<uintptr_t>(alloc_ptr_) & (align - 1);
  size_t slop = (current_mod == 0 ? 0 : align - current_mod);
  size_t needed = bytes + slop;
  char* result;
  if (needed <= alloc_bytes_remaining_) {
    result = alloc_ptr_ + slop;
    alloc_ptr_ += needed;
    alloc_bytes_remaining_ -= needed;

    if (IsMemTable) {
      memory_usage_.fetch_add(needed, std::memory_order_relaxed);
    }
  } else {
    // AllocateFallback always returned aligned memory
    result = AllocateFallback(bytes);
  }
  assert((reinterpret_cast<uintptr_t>(result) & (align - 1)) == 0);
  return result;
}

char* Arena::AllocateNewBlock(size_t block_bytes) {
  char* result;
  if (IsMemTable) {
    result = new char[block_bytes];
  } else {
    NvmNodeSizeRecord(block_bytes);
    result = (char*)numa_alloc_onnode(block_bytes, nvm_node);
    memory_usage_.fetch_add(block_bytes + sizeof(char*),
                              std::memory_order_relaxed);
    block_size_.push_back(block_bytes);
  }
  blocks_.push_back(result);
  return result;
}

void Arena::ReceiveArena(Arena* a) {
  int j = 0;
  for (int i = 0; i < a->blocks_.size(); i++) {
    blocks_.push_back(a->blocks_[i]);
    block_size_.push_back(a->block_size_[i]);
  }
}

}  // namespace leveldb
