// Add by MioDB
// Use pmtable to replace sstable in NVM

#ifndef STORAGE_LEVELDB_DB_PMTABLE_H_
#define STORAGE_LEVELDB_DB_PMTABLE_H_

#include <string>

#include "db/dbformat.h"
#include "db/skiplist.h"
#include "leveldb/db.h"
#include "leveldb/status.h"
#include "util/arena.h"
#include "db/memtable.h"
#include "leveldb/options.h"
#include "util/mergeablebloom.h"

namespace leveldb {

class InternalKeyComparator;
class PmTableIterator;
//typedef SkipList<char*, KeyComparator> dTable;

class PmTable {
 public:
  explicit PmTable(const InternalKeyComparator& comparator, MemTable* mem, const Options& options_);
  explicit PmTable(const InternalKeyComparator& comparator);

  PmTable(const PmTable&) = delete;
  PmTable& operator=(const PmTable&) = delete;

  ~PmTable();	// When compaction is completed, should delete the old pmtable

  // Increase reference count.
  void Ref() { ++refs_; }

  // Drop reference count.  Delete if no more references exist.
  void UnRef() {
    --refs_;
    assert(refs_ >= 0);
    if (refs_ <= 0) {
      delete this;
    }
  }

  // Returns an estimate of the number of bytes of data in use by this
  // data structure. 
  //Is it safe to call when PmTable is being modified???????????????
  //Can be replaced by a number, when compation is completed, we can recalculate it
  //used to judge if the db should compact
  size_t ApproximateMemoryUsage();

  // Return an iterator that yields the contents of the pmtable.
  //
  // while the returned iterator is live.  The keys returned by this
  // iterator are internal keys encoded by AppendInternalKey in the
  // db/format.{h,cc} module.
  Iterator* NewIterator();

  // If pmtable contains a value for key, store it in *value and return true.
  // If pmtable contains a deletion for key, store a NotFound() error
  // in *status and return true.
  // Else, return false.
  // Some get operation will start with the jumpflag node instead of the start of skiplist
  bool Get(const LookupKey& key, std::string* value, Status& s);

  Status Compact(PmTable* smalltable, SequenceNumber snum);

 private:

  friend class PmTableIterator;
  friend class PmTableBackwardIterator;

  KeyComparator comparator_;
  int refs_;

 public:
  Arena arena_;
  MergeableBloom* bloom_;
  mTable table_;
  bool IsLastTable;
};

}	// namespace leveldb

#endif //STORAGE_LEVELDB_DB_PMTABLE_H_
