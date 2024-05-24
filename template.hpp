#pragma once
#include "skiplist.h"
#include <unordered_map>
#include <map>
#include <string>
#include <tuple>
using memtable = skiplist::skiplist_type;
using KVMap = std::map<uint64_t, std::string>;
using KVHash = std::unordered_map<uint64_t, uint64_t>;
using dataTuple = std::tuple<uint64_t, uint64_t, uint32_t>;
using KOVec = std::vector<std::pair<uint64_t, uint64_t>>;
using KVPair = std::pair<uint64_t, std::string>;