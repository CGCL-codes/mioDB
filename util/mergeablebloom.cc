// Add by MioDB
// Mergeable bloom filters can accelerate datatables' query requests

#include "util/mergeablebloom.h"

namespace leveldb {

MergeableBloom::MergeableBloom(const Options& options_): bits_per_key_(options_.bits_per_key) {    // n = highest level DataTable size / KV size, it should be a constant in miodb
  int n = options_.keys_per_datatable;

  size_t bits = n * bits_per_key_;
  if (bits < 64) bits = 64;
  result_size_ = (bits + 7) / 8;

  // We intentionally round down to reduce probing cost a little bit
  k_ = static_cast<size_t>(bits_per_key_ * 0.69);  // 0.69 =~ ln(2)
  if (k_ < 1) k_ = 1;
  if (k_ > 30) k_ = 30;

  result_ = (char*)numa_alloc_onnode(result_size_, options_.dram_node);
}

MergeableBloom::~MergeableBloom() {
  numa_free(result_, result_size_);
}

void MergeableBloom::AddKey(Slice& key) {
  Slice k = key;
  start_.push_back(keys_.size());
  keys_.append(k.data(), k.size());
}

void MergeableBloom::Finish() {
  if (!start_.empty()) {
    GenerateFilter();
  }
}

void MergeableBloom::Merge(MergeableBloom* bloom) {
  const char* c = bloom->GetResult();
  for (int i = 0; i < result_size_; i++) {
    result_[i] |= c[i];
  }
}

const char* MergeableBloom::GetResult() {
  return result_;
}

bool MergeableBloom::KeyMayMatch(Slice& key) {
  if (result_size_ < 2) return false;

  const size_t bits = result_size_ * 8;

  uint32_t h = BloomHash(key);
  const uint32_t delta = (h >> 17) | (h << 15);  // Rotate right 17 bits
  for (size_t j = 0; j < k_; j++) {
    const uint32_t bitpos = h % bits;
    if ((result_[bitpos / 8] & (1 << (bitpos % 8))) == 0) return false;
    h += delta;
  }
  return true;
}

void MergeableBloom::GenerateFilter() {
  const size_t num_keys = start_.size();
  if (num_keys == 0) {
    return;
  }

  // Make list of keys from flattened key structure
  start_.push_back(keys_.size());  // Simplify length computation
  tmp_keys_.resize(num_keys);
  for (size_t i = 0; i < num_keys; i++) {
    const char* base = keys_.data() + start_[i];
    size_t length = start_[i + 1] - start_[i];
    tmp_keys_[i] = Slice(base, length);
  }

  // Generate filter for current set of keys and append to result_.
  CreateFilter(&tmp_keys_[0], static_cast<int>(num_keys));
  
  tmp_keys_.clear();
  keys_.clear();
  start_.clear();
}

void MergeableBloom::CreateFilter(const Slice* keys, int n) {
  size_t bits = result_size_ * 8;

  for (int i = 0; i < n; i++) {
    // Use double-hashing to generate a sequence of hash values.
    // See analysis in [Kirsch,Mitzenmacher 2006].
    uint32_t h = BloomHash(keys[i]);
    const uint32_t delta = (h >> 17) | (h << 15);  // Rotate right 17 bits
    for (size_t j = 0; j < k_; j++) {
      const uint32_t bitpos = h % bits;
      result_[bitpos / 8] |= (1 << (bitpos % 8));
      h += delta;
    }
  }
}

uint32_t MergeableBloom::BloomHash(const Slice& key) {
  return Hash(key.data(), key.size(), 0xbc9f1d34);
}

}   // namespace
