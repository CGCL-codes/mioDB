// Add by MioDB
// Mergeable bloom filters can accelerate datatables' query requests

#include <string>
#include <vector>
#include "leveldb/slice.h"
#include "leveldb/options.h"
#include "numa.h"
#include "util/hash.h"

#ifndef STORAGE_LEVELDB_DB_MERGEABLE_BLOOM_H_
#define STORAGE_LEVELDB_DB_MERGEABLE_BLOOM_H_

namespace leveldb{

class MergeableBloom {
 public:
  explicit MergeableBloom(const Options& options_);
  MergeableBloom(const MergeableBloom&) = delete;
  MergeableBloom& operator=(const MergeableBloom&) = delete;

  ~MergeableBloom();

  void AddKey(Slice& key);
  void Finish();
  void Merge(MergeableBloom* bloom);
  const char* GetResult();
  bool KeyMayMatch(Slice& key);
  
 private:
  void GenerateFilter();
  void CreateFilter(const Slice* keys, int n);
  uint32_t BloomHash(const Slice& key);

  size_t bits_per_key_;
  size_t k_;
  std::string keys_;             // Flattened key contents
  std::vector<size_t> start_;    // Starting index in keys_ of each key
  size_t result_size_;
  char* result_;           // Filter data computed so far
  std::vector<Slice> tmp_keys_;
};

}   // namespace leveldb

#endif  //STORAGE_LEVELDB_DB_MERGEABLE_BLOOM_H_