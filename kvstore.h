#pragma once
#include "vlog.h"
#include "kvstore_api.h"
#include "skiplist.h"
#include "utils.h"
#include <fstream>
#include <stdio.h>
#include "bloomFilter.h"
#include "define.h"
#include <unordered_map>
#include <map>
#include "cachedData.h"

// the max num of memtab node when it reaches 16kb after transfering to SSTable

using KVMap = std::map<uint64_t, std::string>;
using KVHash = std::unordered_map<uint64_t, uint64_t>;
using dataTuple = std::tuple<uint64_t, uint64_t, uint32_t>;

class KVStore : public KVStoreAPI
{
	uint64_t timeStamp;
	// You can add your implementation here
	skiplist::skiplist_type *memtab;
	bloomFilter filter;

	std::string rootPath;
	std::string vlogPath;

	VLog::VLog *vlogMaintainer;
	cached::CachedSST caches;

	std::vector<dataTuple> tuples;
	void storeMem();
	void storeVLog();
	void createSSTable();

	void SSTableHead(FILE *sst);
	// check if sstable has target key
	uint64_t get(FILE *sstable, uint64_t key, uint64_t maxHeader, std::string &value);
	// bloomFilter says target key here, check target key by divide algorithm
	bool get(FILE *sstable, uint64_t key, std::string &value, uint64_t tupleNum);
	// get value from vlog file
	void get(std::string &value, uint64_t offset, uint64_t vlen);

	void scan(FILE *sstable, uint64_t key1, uint64_t key2,
			  KVMap &map, KVHash &hashmap);
	void scan(char *charArray, uint64_t timeStamp, uint64_t lp, uint64_t rp, KVMap &map, KVHash &hashMap);

public:
	KVStore(const std::string &dir, const std::string &vlog);

	~KVStore();

	void put(uint64_t key, const std::string &s) override;

	std::string get(uint64_t key) override;

	bool del(uint64_t key) override;

	void reset() override;

	void scan(uint64_t key1, uint64_t key2, std::list<std::pair<uint64_t, std::string>> &list) override;

	void gc(uint64_t chunk_size) override;
};
