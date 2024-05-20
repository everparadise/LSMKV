#pragma once

#include <inttypes.h>
#include <string>
#include <fstream>
#include "bloomFilter.h"
#include "skiplist.h"
#include "VLog.h"
#include <vector>
#include <tuple>
#include "template.cc"
#include <cstring>
#include "utils.h"
namespace SST
{

    class Section
    {
        FILE *file;
        bool open;
        void createSSTHead(std::vector<dataTuple> &tuples)
        {
            // 时间戳
            fwrite(&timeStamp, 8, 1, file);

            // 键的数量
            numbers = tuples.size();
            fwrite(&numbers, 8, 1, file);

            // 极小键， 极大键
            uint64_t ignore1, ignore2;
            auto [min, ignore1, ignore2] = tuples[0];
            auto [max, ignore3, ignore4] = tuples[numbers - 1];
            minKey = min;
            maxKey = max;
            fwrite(&minKey, 8, 1, file);
            fwrite(&maxKey, 8, 1, file);
        }
        void createSSTBody(std::vector<dataTuple> &tuples)
        {
            // filter.set(caches.cacheFile(sst), BLOOMSIZE);
            bloom = new bool[BLOOMSIZE];
            // write SSTable Main Content(tuples)
            fseek(file, KEYSTART, SEEK_SET);

            for (auto it : tuples)
            {
                auto [key, offset, vlen] = it;
                bloomFilter::insert(bloom, key);
                fwrite(&key, 8, 1, file);
                fwrite(&offset, 8, 1, file);
                fwrite(&vlen, 4, 1, file);
            }
            fseek(file, BLOOMSTART, SEEK_SET);

            // store bloomFilter ahead of the content
            fwrite(bloom, 1, FILTERSIZE, file);
            // close the sstable file;
        }

        bool innerGet(std::string &queryString, uint64_t key)
        {
            char tuples[numbers * TUPLESIZE + 1];
            fseek(file, KEYSTART, SEEK_SET);
            fread(tuples, 1, numbers * TUPLESIZE, file);
            uint64_t lp = 0, rp = numbers - 1;
            while (lp <= rp)
            {
                uint64_t mid = (lp + rp) / 2;
                uint64_t currKey = *reinterpret_cast<uint64_t *>(tuples + mid * TUPLESIZE);
                if (currKey == key)
                {
                    uint64_t offset = *reinterpret_cast<uint64_t *>(tuples + mid * TUPLESIZE + 8);
                    uint32_t vlen = *reinterpret_cast<uint32_t *>(tuples + mid * TUPLESIZE + 16);
                    if (vlen == 0)
                    {
                        queryString.clear();
                        return true;
                    }
                    auto vlog = disk::VLog::getInstance();
                    vlog.get(queryString, offset, vlen);
                    return true;
                }
                else if (currKey < key)
                {
                    lp = mid + 1;
                    continue;
                }
                else if (currKey > key)
                {
                    rp = mid - 1;
                    continue;
                }
            }
            return false;
        }

        void innerScan(char *charArray, uint64_t timeStamp, uint64_t lp, uint64_t rp, KVMap &map, KVHash &hashMap)
        {
            uint64_t currKey, keyLen, keyOff, preStamp;
            uint64_t *uintPtr;
            auto vlog = disk::VLog::getInstance();
            for (; lp <= rp; lp++)
            {
                uintPtr = reinterpret_cast<uint64_t *>(charArray + lp * TUPLESIZE);
                currKey = *uintPtr;
                auto it = hashMap.find(currKey);
                if (it != hashMap.end() && it->second < timeStamp || it == hashMap.end())
                {
                    hashMap[currKey] = timeStamp;
                    keyOff = uintPtr[1];
                    keyLen = *reinterpret_cast<uint32_t *>(uintPtr + 2);
                    std::string str;
                    vlog.get(str, keyOff, keyLen);
                    get(str, keyOff, keyLen);
                    map[currKey] = str;
                }
            }
        }

    public:
        std::string secName;
        const char *fileName;

        uint64_t maxKey;
        uint64_t minKey;
        uint64_t timeStamp;
        uint64_t numbers;

        bool *bloom;

        void reset()
        {
            delete bloom;
            closeFile();
            utils::rmfile(secName);
        }
        Section(std::string &&name, uint64_t &timeStamp)
        {
            this->secName = std::move(name);
            fileName = secName.c_str();
            file = fopen(fileName, "r");
            scanHeader();

            timeStamp = this->timeStamp;
            fclose(file);
            open = false;
        }

        void scanHeader()
        {
            fread(&timeStamp, 8, 1, file);
            fread(&numbers, 8, 1, file);
            fread(&minKey, 8, 1, file);
            fread(&maxKey, 8, 1, file);
            fread(bloom, BLOOMSIZE, 1, file);
        }

        Section(std::string &&rootName, memtable *list, uint64_t timeStamp)
        {
            this->secName = std::move(name);
            this->timeStamp = timeStamp;
            bloom = nullptr;
            open = false;

            int i = 0;
            while (true)
            {
                std::string loopName = this->secName + std::to_string(i);
                if (!(file = fopen(loopName.c_str(), "r")))
                {
                    this->secName = std::move(loopName);
                    fileName = this->secName.c_str();
                }
                fclose(file);
                i++;
            }

            file = fopen(fileName, "w");
            disk::VLog &vlog = disk::VLog::getInstance();
            auto ret = vlog.put(list);

            createSSTHead(ret);
            createSSTBody(ret);
            fclose(file);
        }

        bool get(std::string &queryString, uint64_t key)
        {
            if (key > maxKey || key < minKey)
                return false;
            if (bloom)
            {
                if (!bloomFilter::query(bloom, key))
                    return false;

                openFile();
                bool flag = innerGet(queryString, key);
                closeFile();
                return flag;
            }
            openFile();
            bool flag = innerGet(queryString, key);
            closeFile();
            return flag;
        }

        void scan(uint64_t key1, uint64_t key2, KVMap &map, KVHash &hashMap)
        {
            openFile();

            // make sure target key may exist in this file
            if (minKey > key2 || maxKey < key1)
                return;
            fseek(file, KEYSTART, SEEK_SET);
            char KVPairs[TUPLESIZE * numbers + 1];
            fread(KVPairs, 1, TUPLESIZE * numbers, file);
            uint64_t lp = 0, rp = numbers - 1;
            uint64_t mid;
            if (key1 > minKey)
            {
                uint64_t tmprp = rp;
                while (lp < tmprp)
                {
                    mid = (lp + tmprp) / 2;
                    uint64_t currKey = *reinterpret_cast<uint64_t *>(KVPairs + TUPLESIZE * mid);
                    if (currKey < key1)
                        lp = mid + 1;
                    else if (currKey > key1)
                    {
                        tmprp = mid - 1;
                    }
                    else
                    {
                        lp = mid;
                        break;
                    }
                }
            }
            if (key2 < maxKey)
            {
                uint64_t tmplp = lp;
                while (tmplp < rp)
                {
                    mid = (tmplp + rp) / 2;
                    uint64_t currKey = *reinterpret_cast<uint64_t *>(KVPairs + TUPLESIZE * mid);
                    if (currKey > key2)
                        rp = mid - 1;
                    else if (currKey < key2)
                    {
                        tmplp = mid + 1;
                    }
                    else
                    {
                        rp = mid;
                        break;
                    }
                }
            }
            // curr[rp] <= key2 && curr[lp] >= key1
            if (rp < lp)
                return;

            scan(KVPairs, timeStamp, lp, rp, map, hashMap);
            closeFile();
        }

        static bool compareSection(Section &sec1, Section &sec2)
        {
            if (sec1.timeStamp != sec2.timeStamp)
            {
                return sec1.timeStamp < sec2.timeStamp;
            }

            if (sec1.minKey != sec2.minKey)
            {
                return sec1.minKey < sec2.minKey;
            }

            return sec1.maxKey < sec2.maxKey;
        }

        void closeFile()
        {
            if (open)
            {
                fclose(file);
                open = false;
            }
        }

        void openFile()
        {
            if (!open)
            {
                fopen(fileName, "w+");
                open = true;
            }
        }

        FILE *getFile()
        {
            if (!open)
            {
                fopen(fileName, "w+");
                open = true;
            }
            return file;
        }
    };
}