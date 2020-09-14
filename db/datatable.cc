// Add by MioDB
// Use datatable to replace sstable in NVM

#include "db/datatable.h"
#include "db/dbformat.h"
#include "leveldb/comparator.h"
#include "leveldb/env.h"
#include "leveldb/iterator.h"
#include "util/coding.h"

namespace leveldb {

static Slice GetLengthPrefixedSlice(const char* data) {
  uint32_t len;
  const char* p = data;
  p = GetVarint32Ptr(p, p + 5, &len);  // +5: we assume "p" is not corrupted
  return Slice(p, len);
}

DataTable::DataTable(const InternalKeyComparator& comparator, MemTable* mem, const Options& options_)
  : arena_(&(mem->arena_)),
  comparator_(comparator),
  bloom_(options_.use_datatable_bloom?  new MergeableBloom(options_) : nullptr),
	table_(comparator_, &arena_, &(mem->table_), options_, bloom_),
  IsLastTable(false),
  refs_(0) {}

DataTable::DataTable(const InternalKeyComparator& comparator)
  : comparator_(comparator),
    bloom_(nullptr),
    table_(comparator_),
    IsLastTable(true),
    refs_(0) {}

DataTable::~DataTable() {
  assert(refs_ == 0);
  if (bloom_ != nullptr) {
    delete bloom_;
  }
}

size_t DataTable::ApproximateMemoryUsage() { return table_.GetSize(); }

// Encode a suitable internal key target for "target" and return it.
// Uses *scratch as scratch space, and the returned pointer will point
// into this scratch space.
static const char* EncodeKey(std::string* scratch, const Slice& target) {
  scratch->clear();
  PutVarint32(scratch, target.size());
  scratch->append(target.data(), target.size());
  return scratch->data();
}

class DataTableIterator : public Iterator {
 public:
  explicit DataTableIterator(mTable* table) : iter_(table) {}

  DataTableIterator(const DataTableIterator&) = delete;
  DataTableIterator& operator=(const DataTableIterator&) = delete;

  ~DataTableIterator() override = default;

  bool Valid() const override { return iter_.Valid(); }
  void Seek(const Slice& k) override { iter_.Seek(EncodeKey(&tmp_, k)); }
  void SeekToFirst() override { iter_.SeekToFirst(); }
  void SeekToLast() override { iter_.SeekToLast(); }
  void Next() override { iter_.Next(); }
  void Prev() override { iter_.Prev(); }
  Slice key() const override { return GetLengthPrefixedSlice(iter_.key()); }
  Slice value() const override {
    Slice key_slice = GetLengthPrefixedSlice(iter_.key());
    return GetLengthPrefixedSlice(key_slice.data() + key_slice.size());
  }

  Status status() const override { return Status::OK(); }

 private:
  mTable::Iterator iter_;
  std::string tmp_;  // For passing to EncodeKey
};

Iterator* DataTable::NewIterator() { return new DataTableIterator(&table_); }

bool DataTable::Get(const LookupKey& key, std::string* value, Status& s) {
  if (bloom_ != nullptr) {
    Slice tmpkey = key.user_key();
    if(!(bloom_->KeyMayMatch(tmpkey))) {
      return false;
    }
  }
  Slice memkey = key.memtable_key();
  mTable::Iterator iter(&table_);
  iter.Seek(memkey.data());
  if (iter.Valid()) {
    // entry format is:
    //    klength  varint32
    //    userkey  char[klength]
    //    tag      uint64
    //    vlength  varint32
    //    value    char[vlength]
    // Check that it belongs to same user key.  We do not check the
    // sequence number since the Seek() call above should have skipped
    // all entries with overly large sequence numbers.
    const char* entry = iter.key();
    uint32_t key_length;
    const char* key_ptr = GetVarint32Ptr(entry, entry + 5, &key_length);
    if (comparator_.comparator.user_comparator()->Compare(
            Slice(key_ptr, key_length - 8), key.user_key()) == 0) {
      // Correct user key
      const uint64_t tag = DecodeFixed64(key_ptr + key_length - 8);
      switch (static_cast<ValueType>(tag & 0xff)) {
        case kTypeValue: {
          Slice v = GetLengthPrefixedSlice(key_ptr + key_length);
          value->assign(v.data(), v.size());
          return true;
        }
        case kTypeDeletion:
          s = Status::NotFound(Slice());
          return true;
      }
    }
  }
  return false;
}

Status DataTable::Compact(DataTable* dtable, SequenceNumber snum) {
	if(dtable != nullptr) {
    if (IsLastTable) {
      table_.LastTableCompact(&(dtable->table_), snum);
    } else {
      if (bloom_ != nullptr) {
        bloom_->Merge(dtable->bloom_);
      }
      table_.Compact(&(dtable->table_), snum);
    }
		return Status::OK();
	} else {
		return Status::Corruption("Compaction has sth wrong!");
	}
}

}	//namespace leveldb