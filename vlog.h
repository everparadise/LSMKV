#ifndef __VLOG_H__
#define __VLOG_H__

#include <string>
#include <fstream>
#include "skiplist.h"
#include "define.h"
#include <vector>
namespace skiplist
{
    class skiplist_type;
}

namespace VLog
{
    class VLog
    {
        uint64_t header;
        uint64_t tail;
        FILE *vLogFile;

        std::string vlogPath;
        bool opened;

    public:
        VLog(const std::string &);
        ~VLog();
        void get(std::string &value, uint64_t offset, uint64_t vlen);
        void put(skiplist::skiplist_type *, std::vector<std::tuple<uint64_t, uint64_t, uint32_t>> &);
        void close();
        bool crcCheck(uint64_t target);
    };
}

#endif //__VLOG_H__