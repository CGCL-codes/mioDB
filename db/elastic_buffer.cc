#include "elastic_buffer.h"

namespace leveldb {
  ElasticBuffer::ElasticBuffer(const Options* options, 
                const InternalKeyComparator* cmp)
        : options_(options),
          icmp_(*cmp),
          files_((Array<PmMetaData*>**)numa_alloc_onnode(config::kNumLevels * sizeof(Array<PmMetaData*>*), options_->nvm_node)) {
    for (int i = 0; i < config::kNumLevels; i++) {
      char *fmemory = (char*)numa_alloc_onnode(sizeof(Array<PmMetaData*>), options_->nvm_node);
      files_[i] = new (fmemory) Array<PmMetaData*>(options_);
    }
  }

  // memory leak
  ElasticBuffer::~ElasticBuffer() {}

  void ElasticBuffer::AddFile(int level, uint64_t file_size,
               const InternalKey& smallest, const InternalKey& largest, PmTable* addtable) {
    char* fmemory = (char*)numa_alloc_onnode(sizeof(PmMetaData), options_->nvm_node);
    NvmNodeSizeRecord(sizeof(PmMetaData));
    PmMetaData* f = new (fmemory) PmMetaData();
    f->file_size = file_size;
    f->smallest = smallest;
    f->largest = largest;
    f->pt = addtable;
    f->mustquery = false;
    f->Ref();
    files_[level]->push_back(f);
  }

  void ElasticBuffer::RemoveFile(int level, int removenums) {
    // files_[level].erase(files_[level].begin(), files_[level].begin() + removenums - 1);
    // std::cout << "level " << level << " before delete: " << files_[level].size() << std::endl;
    for (int i = 0; i < removenums; i++) {
      if (removenums == 1) {
        NvmFreeRecord((*(files_[level]))[0]->file_size + sizeof(PmMetaData));
      }
      (*(files_[level]))[0]->UnRef();
      // files_[level].erase(files_[level].begin());
      files_[level]->pop_front();
    }
    // std::cout << "level " << level << " after delete: " << files_[level].size() << std::endl;
  }

  Status ElasticBuffer::DoCompactionWork(int level, uint64_t smallest_snapshot, port::Mutex* mu) {
    mu->AssertHeld();
    assert(level < config::kNumLevels - 1);
    int num = files_[level]->size();
    if (num < 2) {
      // do nothing
    } else {
      PmMetaData* oldmeta = (*(files_[level]))[0];
      PmMetaData* newmeta = (*(files_[level]))[1];

      mu->Unlock();

      oldmeta->mustquery = true;

      PmTable* oldpt = oldmeta->pt;
      PmTable* newpt = newmeta->pt;

      oldpt->Compact(newpt, smallest_snapshot);

      InternalKey smallest, largest;
      uint32_t len;
      const char* p = oldpt->table_.smallest->key;
      p = GetVarint32Ptr(p, p + 5, &len);
      smallest.DecodeFrom(Slice(p, len));

      p = oldpt->table_.largest[0]->key;
      p = GetVarint32Ptr(p, p + 5, &len);
      largest.DecodeFrom(Slice(p, len));

      mu->Lock();

      AddFile(level + 1, oldpt->ApproximateMemoryUsage(), smallest, largest, oldpt);
      RemoveFile(level, 2);
    }
    return Status::OK();
  }

  Status ElasticBuffer::Get(const ReadOptions& options, const LookupKey& lkey, std::string* value) {
    const Comparator* ucmp = icmp_.user_comparator();
    Slice user_key = lkey.user_key();
    Status s;
    bool not_found = true;
    for (int level = 0; level < config::kNumLevels; level++) {
      std::vector<PmMetaData*> tmp;
      tmp.reserve(files_[level]->size());
      for (uint32_t i = 0; i < files_[level]->size(); i++) {
        PmMetaData* f = (*(files_[level]))[i];
        if (f->mustquery) {
          f->Ref();
          // std::cout << "A compacting PMTable is read" << std::endl;
          tmp.push_back(f);
        } else if (ucmp->Compare(user_key, f->smallest.user_key()) >= 0 &&
                  ucmp->Compare(user_key, f->largest.user_key()) <= 0) {
          f->Ref();
          // std::cout << "smallest: " << f->smallest.user_key().ToString() << "  largest: " << f->largest.user_key().ToString() << std::endl;
          tmp.push_back(f);
        }
      }
      if (!tmp.empty()) {
        // std::cout << "level " << level << " start search, file number: " << tmp.size() << std::endl;
        for (int i = tmp.size() - 1; i >= 0; i--) {
          if (not_found && tmp[i]->pt->Get(lkey, value, s)) {
            not_found = false;
          }
          tmp[i]->UnRef();
        }
        tmp.clear();
      }
      if (!not_found) {
        break;
      }
    }
    return not_found? Status::NotFound(Slice()) : Status::OK();
  }

  // arrival level
  bool ElasticBuffer::NeedsCompaction(int level) {
    assert(level > 0 && level <= config::kNumLevels);
    int file_num = files_[level - 1]->size();
    if (level < config:: kNumLevels) {
      return file_num >= 2;
    } else {
      return file_num >= 1;
    }
  }

  // provide pmtable in level N which flushes to sstable
  PmTable* ElasticBuffer::GetCompactablePmTable() {
    assert(!files_[config::kNumLevels - 1]->empty());
    return (*(files_[config::kNumLevels - 1]))[0]->pt;
  }

  void ElasticBuffer::AddIterators(const ReadOptions& options,
                           std::vector<Iterator*>* iters) {
    for (int level = 0; level < config::kNumLevels; level++) {
      if (!files_[level]->empty()) {
        for (size_t i = 0; i < files_[level]->size(); i++) {
          iters->push_back((*(files_[level]))[i]->pt->NewIterator());
        }
      }
    }
  }
} // namespace leveldb
