#include "db/global.h"
// because nvm_node_size() is very slow, we use global variables to record KV size and change nvm_node
// just used to test more KVs
// in beta version, we support 2 nvm numa nodes
int nvm_node = 2;
int nvm_next_node = 4;
size_t nvm_free_space = 384L * 1024 * 1024 * 1024;
bool nvm_node_has_changed = false;
size_t nvm_actual_use = 0L;
size_t nvm_use_max = 0L;
std::ofstream ofs;
long long nvm_total = 0L;
long long ops_num = 0L;

// init nvm_free_space
void NvmNodeSizeInit(const Options& options_) {
    nvm_node = options_.nvm_node;
    nvm_next_node = options_.nvm_next_node;
    long tmp;
    numa_node_size(nvm_node, &tmp);
    nvm_free_space = (size_t)tmp - 16L * 1024 * 1024 * 1024;   // when nvm is full, it will impact performance, so we have 16GB left
	//std::cout << "init nvm node size: " << nvm_free_space << std::endl;
}

// we do not consider the released memory in beta version
void NvmNodeSizeRecord(size_t s) {
    nvm_actual_use += s;
    if (nvm_use_max < nvm_actual_use) {
        nvm_use_max = nvm_actual_use;
    }
    if (nvm_node_has_changed || nvm_next_node == -1) {
        return;
    }
    size_t tmp = (s + 4095) / 4096 * 4096; //  numa_alloc_onnode() allocates memory blocks of 4KB
    if (tmp > nvm_free_space) {
        nvm_node = nvm_next_node;
        nvm_node_has_changed = true;
    } else {
        nvm_free_space -= tmp;
    }
}

void NvmFreeRecord(size_t s) {
    nvm_actual_use -= s;
}

void NvmUsagePrint() {
    std::cout << "Final Use NVM: " << 1.0 * nvm_actual_use / 1024 / 1024 / 1024 << " GB" << std::endl;
    std::cout << "Max Use NVM: " << 1.0 * nvm_use_max / 1024 / 1024 / 1024 << " GB" << std::endl;
    std::cout << "Average Use NVM: " << 1.0 * nvm_total / 1024 / 1024 / 1024 / ops_num << " GB" << std::endl;
}

void OpenRecordFile() {
    ofs.open("./nvm_record.txt");
    if (!ofs.is_open()) {
        std::cout << "Open file error!" << std::endl;
    }
}

void CloseRecordFile() {
    ofs.close();
}

void RecordNvmSeries() {
    // ofs << nvm_actual_use << std::endl;
    nvm_total += nvm_actual_use;
    ops_num++;
}
