#pragma once
#include "utils.h"
#include <string>
namespace Config
{
    static constexpr const char *VLogPath = "./";
    static constexpr const char *SSTRootPath = "./data";

    // memtable
    static constexpr double prob = 0.25;
    static constexpr uint64_t maxTimeStamp = __UINT64_MAX__;

    // sst
    static constexpr uint32_t sstMax = 408;

    // bloomFilter
    static constexpr int hashnum = 8;
    static constexpr int filterSize = 8192;
    static constexpr int seed = 1;
    static constexpr int bloomBit = filterSize * 8;
    static constexpr unsigned char bitReader = 0x01;
    static constexpr int bloomStart = 32;
    static constexpr int keyStart = filterSize + bloomStart;

    // vlog
    static constexpr unsigned char magic = 0xFF;
    static constexpr int tupleSize = 20;

    // disk::VLog *disk::VLog::instance = nullptr;
    //      class FileManager
    //  {

    //     static constexpr std::string rootPath = "";
    //     FileManager()
    //     {
    //     }

    // public:
    //     FileManager(const FileManager &) = delete;
    //     FileManager &operator=(const FileManager &) = delete;

    //     static FileManager &getInstance()
    //     {
    //         static FileManager instance;
    //     }
    // };
}