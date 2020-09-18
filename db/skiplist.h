// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#ifndef STORAGE_LEVELDB_DB_SKIPLIST_H_
#define STORAGE_LEVELDB_DB_SKIPLIST_H_

// Thread safety
// -------------
//
// Writes require external synchronization, most likely a mutex.
// Reads require a guarantee that the SkipList will not be destroyed
// while the read is in progress.  Apart from that, reads progress
// without any internal locking or synchronization.
//
// Invariants:
//
// (1) Allocated nodes are never deleted until the SkipList is
// destroyed.  This is trivially guaranteed by the code since we
// never delete any skip list nodes.
//
// (2) The contents of a Node except for the next/prev pointers are
// immutable after the Node has been linked into the SkipList.
// Only Insert() modifies the list, and it is careful to initialize
// a node and use release-stores to publish the nodes in one or
// more lists.
//
// ... prev vs. next pointer ordering ...

#include <atomic>
#include <cassert>
#include <cstdlib>
#include <string.h>
#include <iostream>

#include "util/arena.h"
#include "util/random.h"
#include "db/dbformat.h"
#include "leveldb/options.h"
#include "util/mergeablebloom.h"
#include "sys/time.h"
#include "db/global.h"

namespace leveldb {

class Arena;

template <typename Key, class Comparator>
class SkipList {
 private:
  enum { kMaxHeight = 22, kLastHeight = 32 };
 public:  // modify by mio
  struct Node;

 public:
  // Create a new SkipList object that will use "cmp" for comparing keys,
  // and will allocate memory using "*arena".  Objects allocated in the arena
  // must remain allocated for the lifetime of the skiplist object.
  explicit SkipList(Comparator cmp, Arena* arena);

  SkipList(const SkipList&) = delete;
  SkipList& operator=(const SkipList&) = delete;

  // Insert key into the list.
  // REQUIRES: nothing that compares equal to key is currently in the list.
  void Insert(const Key& key, const size_t& len);

  // Returns true iff an entry that compares equal to key is in the list.
  bool Contains(const Key& key) const;

  // Iteration over the contents of a skip list
  class Iterator {
   public:
    // Initialize an iterator over the specified list.
    // The returned iterator is not valid.
    explicit Iterator(const SkipList* list);

    // Returns true iff the iterator is positioned at a valid node.
    bool Valid() const;

    // Returns the key at the current position.
    // REQUIRES: Valid()
    const Key& key() const;

    // Advances to the next position.
    // REQUIRES: Valid()
    void Next();

    // Advances to the previous position.
    // REQUIRES: Valid()
    void Prev();

    // Advance to the first entry with a key >= target
    void Seek(const Key& target);

    // Position at the first entry in list.
    // Final state of iterator is Valid() iff list is not empty.
    void SeekToFirst();

    // Position at the last entry in list.
    // Final state of iterator is Valid() iff list is not empty.
    void SeekToLast();

   private:
    const SkipList* list_;
    Node* node_;
    // Intentionally copyable
  };

  // ---------------------------------------------------------------------
  // Add by mio
  // public parameter
  Arena* arena_;  // Arena used for allocations of nodes
  Node* const head_;
  Node* smallest;
  Node* largest[kMaxHeight];
  std::atomic<Node*> insertingnode;
  size_t wa;
  uint64_t dumptime;

  // public function
  Node* Insert(const Key& key, const size_t& len, Node** prev, bool max);
  explicit SkipList(Comparator cmp, Arena* arena, const SkipList<Key, Comparator>* list, const Options& options_, MergeableBloom* bloom_);
  void PreNext(Node** pre, int height);
  bool Compact(SkipList<Key, Comparator>* list, SequenceNumber snum);
  bool Compact(SkipList<Key, Comparator>* list, bool frontlink);

  void Insert(SkipList<Key, Comparator>::Node* n, Node** prev);
  void DeleteNode(Node** pre, Node* n);

  explicit SkipList(Comparator cmp);
  Node* LastTableNewNode(const Key& key, int height, const size_t& len);
  void LastTableDeleteNode(Node** pre, Node* n);
  Node* LastTableInsert(const Key& key, const size_t& len, Node** prev);
  bool LastTableCompact(SkipList<Key, Comparator>* list, SequenceNumber snum);

  // modify from private to public
  inline int GetMaxHeight() const {
    return max_height_.load(std::memory_order_relaxed);
  }

  // Get the size of skiplist
  size_t GetSize() const {
    if (IsLastTable) {
      return sizesum;
    } else {
      return arena_->MemoryUsage();
    }
  }
  // Add end
  // --------------------------------------------------------------------

 private:

  /* change to public by mio
  inline int GetMaxHeight() const {
    return max_height_.load(std::memory_order_relaxed);
  } */

  // add parameter len by mio
  Node* NewNode(const Key& key, int height, const size_t& len);
  int RandomHeight();
  bool Equal(const Key& a, const Key& b) const { return (compare_(a, b) == 0); }

  // Return true if key is greater than the data stored in "n"
  bool KeyIsAfterNode(const Key& key, Node* n) const;

  // Return the earliest node that comes at or after key.
  // Return nullptr if there is no such node.
  //
  // If prev is non-null, fills prev[level] with pointer to previous
  // node at "level" for every level in [0..max_height_-1].
  Node* FindGreaterOrEqual(const Key& key, Node** prev) const;

  // Return the latest node with a key < key.
  // Return head_ if there is no such node.
  Node* FindLessThan(const Key& key) const;

  // Return the last node in the list.
  // Return head_ if list is empty.
  Node* FindLast() const;

  // Immutable after construction
  Comparator const compare_;
  /* change to public by mio
  Arena* const arena_;  // Arena used for allocations of nodes

  Node* const head_;*/
  
  // Modified only by Insert().  Read racily by readers, but stale
  // values are ok.
  std::atomic<int> max_height_;  // Height of the entire list

  // Read/written only by Insert().
  Random rnd_;

  // ------------------------------------------------------------------------------------
  // Add by mio
  // private parameter
  bool UseBloomFilter;
  bool IsLastTable;
  size_t sizesum;

  // private function
  int NewCompare(const Node* a, const Node* b, bool hasseq, SequenceNumber snum) const;
  bool NewCompare(const Node* a, const Node* b) const;
  int LastRandomHeight();
  // Add end
  // ------------------------------------------------------------------------------------
};

// Implementation details follow
template <typename Key, class Comparator>
struct SkipList<Key, Comparator>::Node {
  // add parameter len by mio 2020/5/30
  explicit Node(const Key& k, const size_t& l, const int h) : key(k), len(l), height(h) {}

  // modify by mio 2020/5/29
  // Key const -> key;
  Key key;
  // add by mio 2020/5/29
  size_t len;
  int height;

  // Accessors/mutators for links.  Wrapped in methods so we can
  // add the appropriate barriers as necessary.
  Node* Next(int n) {
    assert(n >= 0);
    // Use an 'acquire load' so that we observe a fully initialized
    // version of the returned Node.
    return next_[n].load(std::memory_order_acquire);
  }
  void SetNext(int n, Node* x) {
    assert(n >= 0);
    // Use a 'release store' so that anybody who reads through this
    // pointer observes a fully initialized version of the inserted node.
    next_[n].store(x, std::memory_order_release);
  }

  // No-barrier variants that can be safely used in a few locations.
  Node* NoBarrier_Next(int n) {
    assert(n >= 0);
    return next_[n].load(std::memory_order_relaxed);
  }
  void NoBarrier_SetNext(int n, Node* x) {
    assert(n >= 0);
    next_[n].store(x, std::memory_order_relaxed);
  }

 private:
  // Array of length equal to the node height.  next_[0] is lowest level link.
  std::atomic<Node*> next_[1];
};

// modify by mio 2020/5/20
template <typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node* SkipList<Key, Comparator>::NewNode(
    const Key& key, int height, const size_t& len) {
  char* node_memory = arena_->AllocateAligned(
                            sizeof(Node) + sizeof(std::atomic<Node*>) * (height - 1));
  return new (node_memory) Node(key, len, height);
}

template <typename Key, class Comparator>
inline SkipList<Key, Comparator>::Iterator::Iterator(const SkipList* list) {
  list_ = list;
  node_ = nullptr;
}

template <typename Key, class Comparator>
inline bool SkipList<Key, Comparator>::Iterator::Valid() const {
  return node_ != nullptr;
}

template <typename Key, class Comparator>
inline const Key& SkipList<Key, Comparator>::Iterator::key() const {
  assert(Valid());
  return node_->key;
}

template <typename Key, class Comparator>
inline void SkipList<Key, Comparator>::Iterator::Next() {
  assert(Valid());
  while (list_->insertingnode.load(std::memory_order_acquire) != nullptr);
  node_ = node_->Next(0);
}

template <typename Key, class Comparator>
inline void SkipList<Key, Comparator>::Iterator::Prev() {
  // Instead of using explicit "prev" links, we just search for the
  // last node that falls before key.
  assert(Valid());
  node_ = list_->FindLessThan(node_->key);
  if (node_ == list_->head_) {
    node_ = nullptr;
  }
}

template <typename Key, class Comparator>
inline void SkipList<Key, Comparator>::Iterator::Seek(const Key& target) {
  node_ = list_->FindGreaterOrEqual(target, nullptr);
  Node* tmp = list_->insertingnode.load(std::memory_order_acquire);
  if (tmp != nullptr && !list_->KeyIsAfterNode(target, tmp)) {
    if (node_ == nullptr) {
      node_ = tmp;
    } else if (list_->NewCompare(node_, tmp)) { // node_->key = ins->key && node_->snum < ins_snum
      node_ = tmp;
    }
  }
}

template <typename Key, class Comparator>
inline void SkipList<Key, Comparator>::Iterator::SeekToFirst() {
  node_ = list_->head_->Next(0);
}

template <typename Key, class Comparator>
inline void SkipList<Key, Comparator>::Iterator::SeekToLast() {
  node_ = list_->FindLast();
  if (node_ == list_->head_) {
    node_ = nullptr;
  }
}

template <typename Key, class Comparator>
int SkipList<Key, Comparator>::RandomHeight() {
  // Increase height with probability 1 in kBranching
  static const unsigned int kBranching = 4;
  int height = 1;
  while (height < kMaxHeight && ((rnd_.Next() % kBranching) == 0)) {
    height++;
  }
  assert(height > 0);
  assert(height <= kMaxHeight);
  return height;
}

// n->userkey < k->userkey, return true
// n->userkey = k->userkey, n->num > k->num return true
template <typename Key, class Comparator>
bool SkipList<Key, Comparator>::KeyIsAfterNode(const Key& key, Node* n) const {
  // null n is considered infinite
  return (n != nullptr) && (compare_(n->key, key) < 0);
}

template <typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node*
SkipList<Key, Comparator>::FindGreaterOrEqual(const Key& key,
                                              Node** prev) const {
  Node* x = head_;
  int level = GetMaxHeight() - 1;
  while (true) {
    Node* next = x->Next(level);
    if (KeyIsAfterNode(key, next)) {
      // Keep searching in this list
      x = next;
    } else {
      if (prev != nullptr) prev[level] = x;
      if (level == 0) {
        return next;
      } else {
        // Switch to next list
        level--;
      }
    }
  }
}

template <typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node*
SkipList<Key, Comparator>::FindLessThan(const Key& key) const {
  Node* x = head_;
  int level = GetMaxHeight() - 1;
  while (true) {
    assert(x == head_ || compare_(x->key, key) < 0);
    Node* next = x->Next(level);
    if (next == nullptr || compare_(next->key, key) >= 0) {
      if (level == 0) {
        return x;
      } else {
        // Switch to next list
        level--;
      }
    } else {
      x = next;
    }
  }
}

template <typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node* SkipList<Key, Comparator>::FindLast()
    const {
  Node* x = head_;
  int level = GetMaxHeight() - 1;
  while (true) {
    Node* next = x->Next(level);
    if (next == nullptr) {
      if (level == 0) {
        return x;
      } else {
        // Switch to next list
        level--;
      }
    } else {
      x = next;
    }
  }
}

// modify by mio
template <typename Key, class Comparator>
SkipList<Key, Comparator>::SkipList(Comparator cmp, Arena* arena)
    : UseBloomFilter(false),
      IsLastTable(false),
      compare_(cmp),
      arena_(arena),
      head_(NewNode(0 /* any key will do */, kMaxHeight, 0 /*add by mio*/)),
      max_height_(1),
      rnd_(0xdeadbeef) {
  for (int i = 0; i < kMaxHeight; i++) {
    head_->SetNext(i, nullptr);
  }
  insertingnode.store(nullptr, std::memory_order_relaxed);
}

// add parameter len by mio 2020/5/30
template <typename Key, class Comparator>
void SkipList<Key, Comparator>::Insert(const Key& key, const size_t& len) {
  // TODO(opt): We can use a barrier-free variant of FindGreaterOrEqual()
  // here since Insert() is externally synchronized.
  Node* prev[kMaxHeight];
  Node* x = FindGreaterOrEqual(key, prev);

  // Our data structure does not allow duplicate insertion
  assert(x == nullptr || !Equal(key, x->key));

  int height = RandomHeight();
  if (height > GetMaxHeight()) {
    for (int i = GetMaxHeight(); i < height; i++) {
      prev[i] = head_;
    }
    // It is ok to mutate max_height_ without any synchronization
    // with concurrent readers.  A concurrent reader that observes
    // the new value of max_height_ will see either the old value of
    // new level pointers from head_ (nullptr), or a new value set in
    // the loop below.  In the former case the reader will
    // immediately drop to the next level since nullptr sorts after all
    // keys.  In the latter case the reader will use the new node.
    max_height_.store(height, std::memory_order_relaxed);
  }

  x = NewNode(key, height, len);
  for (int i = 0; i < height; i++) {
    // NoBarrier_SetNext() suffices since we will add a barrier when
    // we publish a pointer to "x" in prev[i].
    x->NoBarrier_SetNext(i, prev[i]->NoBarrier_Next(i));
    prev[i]->SetNext(i, x);
  }
}

template <typename Key, class Comparator>
bool SkipList<Key, Comparator>::Contains(const Key& key) const {
  Node* x = FindGreaterOrEqual(key, nullptr);
  if (x != nullptr && Equal(key, x->key)) {
    return true;
  } else {
    return false;
  }
}

// Add by mio, 2020/5/14
// serve for small datatable
template <typename Key, class Comparator>
SkipList<Key, Comparator>::SkipList(Comparator cmp, Arena* arena,
                                    const SkipList<Key, Comparator>* list,
                                    const Options& options_,
                                    MergeableBloom* bloom_)
    : UseBloomFilter(bloom_ != nullptr),
      IsLastTable(false),
      compare_(cmp),
      arena_(arena),
      head_((Node*)arena->GetHead()),
      max_height_(list->GetMaxHeight()),
      rnd_(0xdeadbeef) {

  wa = GetSize();
  dumptime = 0;
  struct timeval start, end;
  gettimeofday(&start, nullptr);
  Node* x;
  for (int level = 0; level < GetMaxHeight(); level++) {
    x = head_;
    do {
      // The pointer of key
      if (x != head_ && level == 0) {
        x->key = x->key - (Key)(list->head_) + (Key)head_;
	    wa += 8;
        if (UseBloomFilter) {
          uint32_t len;
          const char* p = x->key;
          p = GetVarint32Ptr(p, p + 5, &len);  // +5: we assume "p" is not corrupted
          Slice tmpkey = Slice(p, len - 8);
          bloom_->AddKey(tmpkey);
        }
      }
      // The pointer of next node
      x->NoBarrier_SetNext(level, x->NoBarrier_Next(level) - list->head_ + head_);
	  wa += 8;
      x = x->NoBarrier_Next(level);
    } while (x->NoBarrier_Next(level) != nullptr);
    if (level == 0) {
      x->key = x->key - (Key)(list->head_) + (Key)head_;
	  wa += 8;
      if (UseBloomFilter) {
        uint32_t len;
        const char* p = x->key;
        p = GetVarint32Ptr(p, p + 5, &len);  // +5: we assume "p" is not corrupted
        Slice tmpkey = Slice(p, len - 8);
        bloom_->AddKey(tmpkey);
      }
      largest[level] = x;
    } else {
      largest[level] = x;
    }
  }
  gettimeofday(&end, nullptr);
  dumptime = (end.tv_sec - start.tv_sec) * 1000000L + (end.tv_usec - start.tv_usec);
  if (UseBloomFilter) {
    bloom_->Finish();
  }

  smallest = head_->NoBarrier_Next(0);
  insertingnode.store(nullptr, std::memory_order_relaxed);
}

template <typename Key, class Comparator>
void SkipList<Key, Comparator>::PreNext(Node** pre, int height) {
  Node* n = pre[0]->Next(0);
  for (int level = 0; level < height; level++) {
    if (pre[level]->Next(level) == n) {
      pre[level] = n;
    } else {
      break;
    }
  }
}

// Comparing node a with b use both key and sequence number
template <typename Key, class Comparator>
int SkipList<Key, Comparator>::NewCompare(const Node* a, const Node* b, bool hasseq, SequenceNumber snum) const {
  if (a == nullptr || b == nullptr) {
    return 0;
  } else {
    return compare_.NewCompare(a->key, b->key, hasseq, snum);
  }
}

template <typename Key, class Comparator>
bool SkipList<Key, Comparator>::NewCompare(const Node* a, const Node* b) const {
  if (a == nullptr || b == nullptr) {
    return false;
  } else {
    return compare_.NewCompare(a->key, b->key);
  }
}

template <typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node* SkipList<Key, Comparator>::Insert(const Key& key, const size_t& len, Node** prev, bool max) {
  int height = RandomHeight();
  if (height > GetMaxHeight()) {
    for (int i = GetMaxHeight(); i < height; i++) {
      prev[i] = head_;
    }
    max_height_.store(height, std::memory_order_relaxed);
  } else if (max) {
    height = GetMaxHeight();
  }

  Node* x = NewNode(key, height, len);
  for (int i = 0; i < height; i++) {
    // NoBarrier_SetNext() suffices since we will add a barrier when
    // we publish a pointer to "x" in prev[i].
    x->NoBarrier_SetNext(i, prev[i]->NoBarrier_Next(i));
    prev[i]->SetNext(i, x);
  }
  return x;
}

template <typename Key, class Comparator>
void SkipList<Key, Comparator>::Insert(SkipList<Key, Comparator>::Node* n, Node** prev) {
  Node* x = FindGreaterOrEqual(n->key, prev);

  assert(x == nullptr || !Equal(n->key, x->key));

  int height = n->height;
  if (height > GetMaxHeight()) {
    for (int i = GetMaxHeight(); i < height; i++) {
      prev[i] = head_;
    }
    max_height_.store(height, std::memory_order_relaxed);
  }

  for (int i = 0; i < height; i++) {
    n->NoBarrier_SetNext(i, prev[i]->NoBarrier_Next(i));
    prev[i]->SetNext(i, n);
  }
}

template <typename Key, class Comparator>
void SkipList<Key, Comparator>::DeleteNode(Node** pre, Node* n) {
  for (int i = 0; i < n->height; i++) {
    pre[i]->SetNext(i, n->Next(i));
  }
}

// only compact overlapping skiplists
// insert all old table's nodes into new table(this table)
/*
template <typename Key, class Comparator>
bool SkipList<Key, Comparator>::Compact(SkipList<Key, Comparator>* list, SequenceNumber snum) {
  Node *x, *y, *xpre[kMaxHeight];
  x = head_;
  y = list->head_;

  for (int i = 0; i < kMaxHeight; i++) {
    if (i < GetMaxHeight()) {
      xpre[i] = x;
    } else {
      xpre[i] = nullptr;
    }
  }

  x = x->Next(0);
  y = y->Next(0);
  
  // front, y->key < head_->Next(0)->key
  while (y != nullptr && NewCompare(x, y, false, 0) == 0b11) { // xkey > ykey

    if (xpre[0] == head_) { // first insert node
      char* copykey = arena_->Allocate(y->len);
      memcpy(copykey, y->key, y->len);
      smallest = Insert(copykey, y->len, xpre, false);
      PreNext(xpre, GetMaxHeight());

    } else {
      if (NewCompare(xpre[0], y, true, snum) == 0b0010) { // xkey = ykey, xnum <= snum, ynum <= snum, y should be deleted
        //do nothing

      } else {  // insert y
        char* copykey = arena_->Allocate(y->len);
        memcpy(copykey, y->key, y->len);
        Insert(copykey, y->len, xpre, false);
        PreNext(xpre, GetMaxHeight());
      }
    }
    y = y->Next(0);
  }

  // mid and back
  bool firstinsert = true;
  while (y != nullptr) {
    if (x == nullptr || NewCompare(x, y, false, 0) == 0b11) { // x = nullptr or xkey > ykey means y may be inserted before nullptr or x
      if (NewCompare(xpre[0], y, true, snum) == 0b0010) { // y is obsolescent
        y = y->Next(0);
      } else {  // insert
        char* copykey = arena_->Allocate(y->len);
        memcpy(copykey, y->key, y->len);

        if (firstinsert) {  // In mid phase, the first inserted node's inserted height should be max
          Node* tmp = Insert(copykey, y->len, xpre, true);
          firstinsert = false;
        } else {
          Insert(copykey, y->len, xpre, false);
        }
        PreNext(xpre, GetMaxHeight());
        y = y->Next(0);
      }
    } else {  // xkey <= ykey
      x = x->Next(0);
      PreNext(xpre, GetMaxHeight());
    }
  }

  if (NewCompare(xpre[0], largest[0], false ,0) == 0b11) {
    for (int i = 0; i < GetMaxHeight(); i++) {
      largest[i] = xpre[i];
    }
  }

  return true;
}*/

// new->old
template <typename Key, class Comparator>
bool SkipList<Key, Comparator>::Compact(SkipList<Key, Comparator>* list, SequenceNumber snum) {
  wa = 0;
  Node *x = list->head_->Next(0);
  Node *y, *ypre[kMaxHeight], *xpre[kMaxHeight];
  for (int i = 0; i < kMaxHeight; i++) {
    if (i < list->GetMaxHeight()) {
      xpre[i] = list->head_;
    } else {
      xpre[i] = nullptr;
    }
  }

  bool first = true;
  while (x != nullptr) {
    insertingnode.store(x, std::memory_order_release);
    DeleteNode(xpre, x);
    Insert(x, ypre);
	wa += (3 * 8 * x->height);
    y = x;
    PreNext(ypre, y->height);

    // Set smallest
    if (first) {
      if (smallest == nullptr || NewCompare(y, smallest, false, 0) == 0b01) {
        smallest = y;
      }
      first = false;
    }

    // LargeTable duplication
    while (y->Next(0) != nullptr) {
      int r = NewCompare(y, y->Next(0), true, snum);
      if (r == 0b0010) {
        for (int i = 0; i < GetMaxHeight(); i++) {
          if (largest[i] = y->Next(0)) {
            largest[i] = ypre[i];
          } else {
            break;
          }
        }
        DeleteNode(ypre, y->Next(0));
      } else if ((r & 0b11) == 0b10) {
        y = y->Next(0);
        PreNext(ypre, y->height);
      } else {
        break;
      }
    }

    // Jump obsolescent node in small table
    x = xpre[0]->Next(0);
    if (x == nullptr) {

    } else {
      bool flag = true;
      int r;
      do {
        if (flag) {
          r = NewCompare(insertingnode.load(std::memory_order_relaxed), x, true, snum);
          flag = false;
        } else {
          r = NewCompare(xpre[0], x, true, snum);
        }
        if (r == 0b0010) {
          PreNext(xpre, x->height);
          x = x->Next(0);
        } else {
          break;
        }
      } while (x != nullptr);
    }
  }
  insertingnode.store(nullptr, std::memory_order_release);

  // Set Largest
  for (int i = 0; i < GetMaxHeight(); i++) {
    if (NewCompare(ypre[i], largest[i], false, 0) == 0b11) {
      largest[i] = ypre[i];
    }
  }
  
  arena_->ReceiveArena(list->arena_);
  list->arena_->SetTransfer();
  return true;
}

// no overlaping skiplists compaction
template <typename Key, class Comparator>
bool SkipList<Key, Comparator>::Compact(SkipList<Key, Comparator>* list, bool frontlink) {
  Node *pre[kMaxHeight];
  int xheight = GetMaxHeight();
  int yheight = list->GetMaxHeight();

  if (frontlink) {
    for (int i = 0; i < yheight; i++) {
      list->largest[i]->SetNext(i, head_->Next(i));
      head_->SetNext(i, list->head_->Next(i));
    }
    if (yheight > xheight) {
      max_height_.store(yheight, std::memory_order_relaxed);
    }
    smallest = list->smallest;

  } else {
    for (int i = 0; i < xheight; i++) {
      largest[i]->SetNext(i, list->head_->Next(i));
    }
    if (yheight > xheight) {
      max_height_.store(yheight, std::memory_order_relaxed);
    }
    for (int i = 0; i < yheight; i++) {
      largest[i] = list->largest[i];
    }
  }
  
  arena_->ReceiveArena(list->arena_);
  list->arena_->SetTransfer();
  return true;
}

// serve for last large datatable
template <typename Key, class Comparator>
SkipList<Key, Comparator>::SkipList(Comparator cmp)
    : UseBloomFilter(false),
      IsLastTable(true),
      sizesum(0),
      compare_(cmp),
      arena_(nullptr),
      head_(LastTableNewNode(0 /* any key will do */, kLastHeight, 0 /*add by mio*/)),
      max_height_(1),
      rnd_(0xdeadbeef) {
  for (int i = 0; i < kLastHeight; i++) {
    head_->SetNext(i, nullptr);
  }
  smallest = nullptr;
  largest[0] = nullptr;
  insertingnode.store(nullptr, std::memory_order_relaxed);
}

template <typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node* SkipList<Key, Comparator>::LastTableNewNode(
    const Key& key, int height, const size_t& len) {
  NvmNodeSizeRecord(len);
  char* copykey = (char*)numa_alloc_onnode(len, nvm_node);
  wa += len;
  sizesum += len;
  memcpy(copykey, key, len);
  size_t tmp = sizeof(Node) + sizeof(std::atomic<Node*>) * (height - 1);
  NvmNodeSizeRecord(tmp);
  char* node_memory = (char*)numa_alloc_onnode(tmp, nvm_node);
  wa += tmp;
  sizesum += tmp;
  return new (node_memory) Node(copykey, len, height);
}

template <typename Key, class Comparator>
void SkipList<Key, Comparator>::LastTableDeleteNode(Node** pre, Node* n) {
  for (int i = 0; i < n->height; i++) {
    pre[i]->SetNext(i, n->Next(i));
  }
  wa += (8 * n->height);
  numa_free(const_cast<char*>(n->key), n->len);
  sizesum -= n->len;
  size_t tmp = sizeof(Node) + sizeof(std::atomic<Node*>) * (n->height - 1);
  numa_free(n, tmp);
  sizesum -= tmp;
}

template <typename Key, class Comparator>
int SkipList<Key, Comparator>::LastRandomHeight() {
  // Increase height with probability 1 in kBranching
  static const unsigned int kBranching = 4;
  int height = 1;
  while (height < kLastHeight && ((rnd_.Next() % kBranching) == 0)) {
    height++;
  }
  assert(height > 0);
  assert(height <= kLastHeight);
  return height;
}

template <typename Key, class Comparator>
typename SkipList<Key, Comparator>::Node* SkipList<Key, Comparator>::LastTableInsert(const Key& key, const size_t& len, Node** prev) {
  Node* x = FindGreaterOrEqual(key, prev);

  assert(x == nullptr || !Equal(key, x->key));

  int height = LastRandomHeight();
  if (height > GetMaxHeight()) {
    for (int i = GetMaxHeight(); i < height; i++) {
      prev[i] = head_;
    }
    max_height_.store(height, std::memory_order_relaxed);
  }

  x = LastTableNewNode(key, height, len); // different from Insert()
  for (int i = 0; i < height; i++) {
    x->NoBarrier_SetNext(i, prev[i]->NoBarrier_Next(i));
    prev[i]->SetNext(i, x);
  }
  wa += (2 * 8 * height);
  return x;
}

// Final compaction, newtable -> oldtable
template <typename Key, class Comparator>
bool SkipList<Key, Comparator>::LastTableCompact(SkipList<Key, Comparator>* list, SequenceNumber snum) {
  wa = 0;
  Node *x = list->head_->Next(0);
  Node *pre[kLastHeight];
  Node *y;
  bool first = true;
  while (x != nullptr) {
    y = LastTableInsert(x->key, x->len, pre);
    PreNext(pre, y->height);

    // Set smallest
    if (first) {
      if (smallest == nullptr || NewCompare(y, smallest, false, 0) == 0b01) {
        smallest = y;
      }
      first = false;
    }

    // LargeTable duplication
    while (y->Next(0) != nullptr) {
      int r = NewCompare(y, y->Next(0), true, snum);
      if (r == 0b0010) {
        if (y->Next(0) == largest[0]) {
          largest[0] = y;
        }
        LastTableDeleteNode(pre, y->Next(0));
      } else if ((r & 0b11) == 0b10) {
        y = y->Next(0);
        PreNext(pre, y->height);
      } else {
        break;
      }
    }

    // Jump obsolescent node in small table
    if (x->Next(0) == nullptr) {
      x = nullptr;
    } else {
      do {
        int r = NewCompare(x, x->Next(0), true, snum);
        if (r == 0b0010) {
          x = x->Next(0);
        } else {
          x = x->Next(0);
          break;
        }
      } while (x->Next(0) != nullptr);
    }
  }

  // Set Largest
  if (largest[0] == nullptr || NewCompare(y, largest[0], false, 0) == 0b11) {
    largest[0] = y;
  }
  return true;
}
}  // namespace leveldb
#endif  // STORAGE_LEVELDB_DB_SKIPLIST_H_
