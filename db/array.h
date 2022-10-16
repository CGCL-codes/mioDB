// meta data numa version
#ifndef STORAGE_LEVELDB_DB_ARRAY_H_
#define STORAGE_LEVELDB_DB_ARRAY_H_

#include "numa.h"
#include <assert.h>
#include "leveldb/options.h"
#define INITIAL_SIZE 512

namespace leveldb {

template <typename T>
class Array {
public:
  Array() : numa_node_(0),
            cap_(0),
            head_(0),
            tail_(0),
            ptr_(0) {}
  Array(const Options *ops) : numa_node_(ops->nvm_node), 
                        cap_(INITIAL_SIZE), 
                        head_(0),
                        tail_(0),
                        ptr_((T*)numa_alloc_onnode(cap_ * sizeof(T), numa_node_)) {}
  ~Array() {
    numa_free(ptr_, cap_ * sizeof(T));
  }
  void init(const Options *ops) {
    numa_node_ = ops->nvm_node;
    cap_ = INITIAL_SIZE;
    ptr_ = (T*)numa_alloc_onnode(cap_ * sizeof(T), numa_node_);
  }
  uint64_t capacity() {
    return cap_;
  }
  uint64_t size() {
    return (tail_ - head_ + cap_) % cap_;
  }
  bool empty() {
    return head_ == tail_;
  }
  bool full() {
    return (tail_ + 1) % cap_ == head_;
  }
  void reserve(uint64_t s) {
    if (s <= cap_) {
      return;
    }
    T* tmp = (T*)numa_alloc_onnode(s * sizeof(T), numa_node_);
    int len = size();
    for (int i = 0; i < len; i++) {
      tmp[i] = ptr_[(head_ + i) % cap_];
    }
    numa_free(ptr_, cap_);
    head_ = 0;
    tail_ = len;
    cap_ = s;
    ptr_ = tmp;
  }
  void pop_front() {
    assert(!empty());
    head_ = (head_ + 1) % cap_;
  }
  void push_back(T t) {
    if (full()) {
      reserve(2 * cap_);
    }
    ptr_[tail_] = t;
    tail_ = (tail_ + 1) % cap_;
  }
  T operator[](uint64_t i) {
    assert((head_ + i) % cap_ < tail_);
    return *(ptr_ + head_ + i);
  }

private:
  int numa_node_;
  uint64_t cap_;
  uint64_t head_;
  uint64_t tail_;
  T* ptr_;
};

} // namespace leveldb

#endif // STORAGE_LEVELDB_DB_ARRAY_H_