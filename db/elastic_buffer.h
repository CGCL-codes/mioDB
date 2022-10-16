#ifndef STORAGE_LEVELDB_DB_ELASTIC_BUFFER_H_
#define STORAGE_LEVELDB_DB_ELASTIC_BUFFER_H_

#include <set>
#include <utility>
#include <vector>

#include "db/dbformat.h"
#include "db/pmtable.h"
#include "numa.h"
#include "leveldb/status.h"
#include "leveldb/slice.h"
#include "port/port.h"
#include "db/array.h"
#include "table/two_level_iterator.h"
#include <iostream>
#include "db/global.h"

namespace leveldb {
struct PmMetaData {
  PmMetaData() : refs(0),  file_size(0), mustquery(false), pt(nullptr) {}
  void Ref() {
    refs++;
  }
  void UnRef() {
    refs--;
    if (refs <= 0) {
      numa_free(this, sizeof(PmMetaData));
    }
  }
  int refs;
  uint64_t file_size;    // File size in bytes
  InternalKey smallest;  // Smallest internal key served by table
  InternalKey largest;   // Largest internal key served by table
  bool mustquery;
  PmTable* pt;
};

class ElasticBuffer {
public:
  ElasticBuffer(const Options* options, const InternalKeyComparator* cmp);
  ElasticBuffer(const ElasticBuffer&) = delete;
  ElasticBuffer& operator=(const ElasticBuffer&) = delete;
  ~ElasticBuffer();

  void AddFile(int level, uint64_t file_size,
               const InternalKey& smallest, const InternalKey& largest, PmTable* addtable);
  void RemoveFile(int level, int removenums);

  Status DoCompactionWork(int level, uint64_t smallest_snapshot, port::Mutex* mu);

  Status Get(const ReadOptions& options, const LookupKey& lkey, std::string* value);

  bool NeedsCompaction(int level);

  PmTable* GetCompactablePmTable();

  void AddIterators(const ReadOptions&, std::vector<Iterator*>* iters);

private:
  const Options* const options_;
  const InternalKeyComparator icmp_;
  // std::vector<PmMetaData*> files_[config::kNumLevels];
  Array<PmMetaData*>** files_;
};

} // namespace leveldb

#endif // STORAGE_LEVELDB_DB_ELASTIC_BUFFER_H_
