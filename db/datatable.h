// Add by MioDB
// Use datatable to replace sstable in NVM

#ifndef STORAGE_LEVELDB_DB_DATATABLE_H_
#define STORAGE_LEVELDB_DB_DATATABLE_H_

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
class DataTableIterator;
//typedef SkipList<char*, KeyComparator> dTable;

class DataTable {
 public:
  explicit DataTable(const InternalKeyComparator& comparator, MemTable* mem, const Options& options_);
  explicit DataTable(const InternalKeyComparator& comparator);

  DataTable(const DataTable&) = delete;
  DataTable& operator=(const DataTable&) = delete;

  ~DataTable();	// When compaction is completed, should delete the old datatable

  // Increase reference count.
  void Ref() { ++refs_; }

  // Drop reference count.  Delete if no more references exist.
  void Unref() {
    --refs_;
    assert(refs_ >= 0);
    if (refs_ <= 0) {
      delete this;
    }
  }

  // Returns an estimate of the number of bytes of data in use by this
  // data structure. 
  //Is it safe to call when DataTable is being modified???????????????
  //Can be replaced by a number, when compation is completed, we can recalculate it
  //used to judge if the db should compact
  size_t ApproximateMemoryUsage();

  // Return an iterator that yields the contents of the datatable.
  //
  // while the returned iterator is live.  The keys returned by this
  // iterator are internal keys encoded by AppendInternalKey in the
  // db/format.{h,cc} module.
  Iterator* NewIterator();

  // If datatable contains a value for key, store it in *value and return true.
  // If datatable contains a deletion for key, store a NotFound() error
  // in *status and return true.
  // Else, return false.
  // Some get operation will start with the jumpflag node instead of the start of skiplist
  bool Get(const LookupKey& key, std::string* value, Status& s);

  Status Compact(DataTable* smalltable, SequenceNumber snum);

 private:

  friend class DataTableIterator;
  friend class DataTableBackwardIterator;

  KeyComparator comparator_;
  int refs_;

 public:
  Arena arena_;
  MergeableBloom* bloom_;
  mTable table_;
  bool IsLastTable;
};

}	// namespace leveldb

#endif //STORAGE_LEVELDB_DB_DATATABLE_H_
