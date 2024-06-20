#pragma once
#include "utils.h"
#include <string>
namespace Config
{
    static constexpr const bool cacheBody = true;
    static constexpr const bool cacheBloom = true;

    static constexpr const char *VLogPath = "./";
    static constexpr const char *SSTRootPath = "./data";

    // memtable
    static constexpr double prob = 0.25;
    static constexpr uint64_t maxTimeStamp = __UINT64_MAX__;

    // bloomFilter
    static constexpr int hashnum = 8;
    static constexpr int filterSize = 8192 + 4096;
    static constexpr int seed = 1;
    static constexpr int bloomBit = filterSize * 8;
    static constexpr unsigned char bitReader = 0x01;
    static constexpr int bloomStart = 32;
    static constexpr int keyStart = filterSize + bloomStart;

    // vlog
    static constexpr unsigned char magic = 0xFF;
    // magic + checksum
    static constexpr int entryHeadLength = 3;
    // key + vlen
    static constexpr int entryKVLength = 12;
    static constexpr int entryMetaData = entryHeadLength + entryKVLength;
    static constexpr int tupleSize = 20;
    static constexpr int vlenSize = 4;
    static constexpr int keySize = 8;
    static constexpr int crcCheckSize = 2;
    static constexpr int metaVlenPos = entryHeadLength + keySize;

    // sst
    static constexpr int sstSize = 16 * 1024;
    static constexpr uint32_t sstMax = (sstSize - filterSize - bloomStart) / tupleSize;

    // initial scan
    static constexpr const int scanSize = 4096;

    // log
    static const std::string logPath = "./data/log";

    // performance
    static const std::string sequencialGet = "./performanceStas/get_sequence.csv";
    static const std::string OutOfOrderGet = "./performanceStas/get_outOrder.csv";
    static const std::string randomGet = "./performanceStas/get_random.csv";
    static const std::string put = "./performanceStas/put.csv";
    static const std::string scan = "./performanceStas/scan.csv";
    static const std::string del = "./performanceStas/del.csv";
}