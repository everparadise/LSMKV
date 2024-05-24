#pragma once
#include "kvstore_api.h"
#include "skiplist.h"
#include "utils.h"
#include <fstream>
#include <stdio.h>
#include "VLog.hpp"
#include "bloomFilter.h"
#include <unordered_map>
#include <map>
#include "SST.hpp"
#include <string>
#include "template.hpp"
#include "logger.hpp"
// the max num of memtab node when it reaches 16kb after transfering to SSTable

class KVStore : public KVStoreAPI
{
	std::string rootPath;
	std::string vlogPath;
	uint64_t timeStamp;
	// You can add your implementation here
	memtable *memtab;
	SST::SSTManager sstManager;
	void storeMem();
	uint64_t getOffset(uint64_t key);
	logger *log;

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
