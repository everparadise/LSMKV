#pragma once
#include <inttypes.h>
#include <string>
#include <fstream>
#include "bloomFilter.h"
#include "skiplist.h"
#include "VLog.hpp"
#include <vector>
#include <tuple>
#include "template.hpp"
#include <cstring>
#include "utils.h"
namespace SST
{

    class Section
    {
        FILE *file;
        bool open;
        // 用来执行compact时的erase 但是不代表这个section不能用来读取
        bool valid;
        void createSSTHead(std::vector<dataTuple> &tuples)
        {
            // 时间戳
            fwrite(&timeStamp, 8, 1, file);

            // 键的数量
            numbers = tuples.size();
            if (numbers > Config::sstMax)
                numbers = Config::sstMax;
            fwrite(&numbers, 8, 1, file);

            // 极小键， 极大键
            auto min = std::get<0>(tuples[0]);
            auto max = std::get<0>(tuples[numbers - 1]);
            minKey = min;
            maxKey = max;
            fwrite(&minKey, 8, 1, file);
            fwrite(&maxKey, 8, 1, file);
        }
        void createSSTBody(std::vector<dataTuple> &tuples)
        {
            // filter.set(caches.cacheFile(sst), BLOOMSIZE);
            bloom = new unsigned char[Config::filterSize];
            // write SSTable Main Content(tuples)
            fseek(file, Config::keyStart, SEEK_SET);

            for (uint64_t i = 0; i < numbers; i++)
            {
                auto [key, offset, vlen] = tuples[0];
                bloomFilter::insert(bloom, key);
                fwrite(&key, 8, 1, file);
                fwrite(&offset, 8, 1, file);
                fwrite(&vlen, 4, 1, file);
                tuples.erase(tuples.begin());
            }
            fseek(file, Config::bloomStart, SEEK_SET);

            // store bloomFilter ahead of the content
            fwrite(bloom, Config::filterSize, 1, file);
            if (Config::cacheBody)
            {
                cacheBody = new unsigned char[numbers * Config::tupleSize + 1];
                uint64_t read = fread(cacheBody, 1, numbers * Config::tupleSize, file);
                if (read != numbers * Config::tupleSize)
                {
                    printf("read wrong\n");
                }
            }
            if (!Config::cacheBloom)
            {
                delete bloom;
                bloom = nullptr;
            }
            // close the sstable file;
        }

        bool innerGet(std::string &queryString, uint64_t key)
        {
            unsigned char *tuples;
            if (cacheBody)
            {
                tuples = cacheBody;
            }
            else
            {
                tuples = new unsigned char[numbers * Config::tupleSize + 1];
                fseek(file, Config::keyStart, SEEK_SET);
                fread(tuples, 1, numbers * Config::tupleSize, file);
            }

            uint64_t lp = 0, rp = numbers - 1;
            while (lp <= rp)
            {
                uint64_t mid = (lp + rp) / 2;
                uint64_t currKey = *reinterpret_cast<uint64_t *>(tuples + mid * Config::tupleSize);
                if (currKey == key)
                {
                    uint64_t offset = *reinterpret_cast<uint64_t *>(tuples + mid * Config::tupleSize + 8);
                    uint32_t vlen = *reinterpret_cast<uint32_t *>(tuples + mid * Config::tupleSize + 16);
                    if (vlen == 0)
                    {
                        if (!cacheBody)
                            delete tuples;
                        return true;
                    }
                    auto vlog = disk::VLog::getInstance();
                    vlog->get(queryString, offset, vlen);
                    if (!cacheBody)
                        delete tuples;
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
            if (!cacheBody)
                delete tuples;
            return false;
        }

        uint64_t innerGetOffset(uint64_t key)
        {
            unsigned char *tuples;
            if (cacheBody)
            {
                tuples = cacheBody;
            }
            else
            {
                tuples = new unsigned char[numbers * Config::tupleSize + 1];
                fseek(file, Config::keyStart, SEEK_SET);
                fread(tuples, 1, numbers * Config::tupleSize, file);
            }
            uint64_t lp = 0, rp = numbers - 1;
            while (lp <= rp)
            {
                uint64_t mid = (lp + rp) / 2;
                uint64_t currKey = *reinterpret_cast<uint64_t *>(tuples + mid * Config::tupleSize);
                if (currKey == key)
                {
                    uint64_t offset = *reinterpret_cast<uint64_t *>(tuples + mid * Config::tupleSize + 8);
                    if (!cacheBody)
                        delete tuples;
                    return offset;
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
            if (!cacheBody)
                delete tuples;
            return -1;
        }
        void innerScan(unsigned char *charArray, uint64_t timeStamp, uint64_t lp, uint64_t rp, KVMap &map, KVHash &hashMap)
        {
            uint64_t currKey, keyLen, keyOff;
            uint64_t *uintPtr;
            auto vlog = disk::VLog::getInstance();
            for (; lp <= rp; lp++)
            {
                uintPtr = reinterpret_cast<uint64_t *>(charArray + lp * Config::tupleSize);
                currKey = *uintPtr;
                auto it = hashMap.find(currKey);
                if ((it != hashMap.end() && it->second < timeStamp) || it == hashMap.end())
                {
                    hashMap[currKey] = timeStamp;
                    keyOff = uintPtr[1];
                    keyLen = *reinterpret_cast<uint32_t *>(uintPtr + 2);
                    std::string str;
                    vlog->get(str, keyOff, keyLen);
                    map[currKey] = std::move(str);
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

        unsigned char *bloom;
        unsigned char *cacheBody;

        bool getValid()
        {
            return valid;
        }

        void revertValid()
        {
            valid = false;
        }
        void reset()
        {
            if (bloom)
            {
                delete[] bloom;
                bloom = nullptr;
            }
            if (cacheBody)
            {
                delete[] cacheBody;
                cacheBody = nullptr;
            }
            closeFile();
            utils::rmfile(secName);
        }
        void rmfile()
        {
            utils::rmfile(fileName);
            closeFile();
        }
        // initial scan时扫描vlog
        Section(std::string &&name, uint64_t &timeStamp)
        {
            bloom = nullptr;
            this->secName = std::move(name);
            fileName = secName.c_str();
            file = fopen(fileName, "rb");
            scanCache();

            timeStamp = this->timeStamp;
            fclose(file);
            file = nullptr;
            open = false;
            valid = true;
        }
        void scanCache()
        {
            fread(&timeStamp, 8, 1, file);
            fread(&numbers, 8, 1, file);
            fread(&minKey, 8, 1, file);
            fread(&maxKey, 8, 1, file);
            if (Config::cacheBloom)
            {
                bloom = new unsigned char[Config::filterSize];
                fread(bloom, Config::filterSize, 1, file);
            }
            else
                bloom = nullptr;
            if (Config::cacheBody)
            {
                cacheBody = new unsigned char[numbers * Config::keySize];
                fseek(file, Config::keyStart, SEEK_SET);
                fread(cacheBody, numbers * Config::keySize, 1, file);
            }
            else
                cacheBody = nullptr;
        }

        // Level中新建的 Section，写vlog
        Section(std::string &&rootName, memtable *list, uint64_t timeStamp)
        {
            // rootName格式为相对根目录路径 + "/timeStamp"
            this->secName = std::move(rootName);
            this->timeStamp = timeStamp;
            bloom = nullptr;
            open = false;
            valid = true;
            cacheBody = nullptr;

            int i = 0;
            while (true)
            {
                std::string loopName = this->secName + "-" + std::to_string(i);
                if (!(file = fopen(loopName.c_str(), "rb")))
                {
                    this->secName = std::move(loopName);
                    fileName = this->secName.c_str();
                    break;
                }
                else
                    fclose(file);
                i++;
            }

            file = fopen(fileName, "wb+");
            disk::VLog *vlog = disk::VLog::getInstance();
            auto ret = vlog->put(list);

            createSSTHead(ret);
            createSSTBody(ret);
            fclose(file);
            file = nullptr;
        }

        // 在compaction时直接通过数据进行初始化，不需要写vlog
        Section(std::string &&rootName, std::vector<dataTuple> &tuples, uint64_t timeStamp)
        {
            this->secName = std::move(rootName);
            this->timeStamp = timeStamp;
            cacheBody = nullptr;

            valid = true;
            int i = 0;
            while (true)
            {
                std::string loopName = this->secName + "-" + std::to_string(i);
                if (!(file = fopen(loopName.c_str(), "rb")))
                {
                    this->secName = std::move(loopName);
                    fileName = this->secName.c_str();
                    break;
                }
                else
                    fclose(file);
                i++;
            }

            file = fopen(fileName, "wb+");

            createSSTHead(tuples);
            createSSTBody(tuples);
            fclose(file);
            file = nullptr;
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
            bloom = new unsigned char[Config::filterSize];
            fseek(file, Config::bloomStart, SEEK_SET);
            fread(bloom, Config::filterSize, 1, file);
            if (!bloomFilter::query(bloom, key))
            {
                delete bloom;
                bloom = nullptr;
                closeFile();
                return false;
            }
            bool flag = innerGet(queryString, key);
            closeFile();
            delete bloom;
            bloom = nullptr;
            return flag;
        }

        uint64_t getOffset(uint64_t key)
        {
            if (key > maxKey || key < minKey)
                return -1;
            if (bloom)
            {
                if (!bloomFilter::query(bloom, key))
                    return -1;

                openFile();
                uint64_t offset = innerGetOffset(key);
                closeFile();
                return offset;
            }

            openFile();
            bloom = new unsigned char[Config::filterSize];
            fseek(file, Config::bloomStart, SEEK_SET);
            fread(bloom, Config::filterSize, 1, file);
            if (!bloomFilter::query(bloom, key))
            {
                delete bloom;
                bloom = nullptr;
                closeFile();
                return -1;
            }
            uint64_t offset = innerGetOffset(key);
            closeFile();
            delete bloom;
            bloom = nullptr;
            return offset;
        }
        void scan(uint64_t key1, uint64_t key2, KVMap &map, KVHash &hashMap)
        {
            openFile();

            // make sure target key may exist in this file
            if (minKey > key2 || maxKey < key1)
                return;

            unsigned char *KVPairs;
            if (!cacheBody)
            {
                KVPairs = new unsigned char[Config::tupleSize * numbers + 1];
                fseek(file, Config::keyStart, SEEK_SET);
                fread(KVPairs, 1, Config::tupleSize * numbers, file);
            }
            else
                KVPairs = cacheBody;

            uint64_t lp = 0, rp = numbers - 1;
            uint64_t mid;
            if (key1 > minKey)
            {
                uint64_t tmprp = rp;
                while (lp < tmprp)
                {
                    mid = (lp + tmprp) / 2;
                    uint64_t currKey = *reinterpret_cast<uint64_t *>(KVPairs + Config::tupleSize * mid);
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
                    uint64_t currKey = *reinterpret_cast<uint64_t *>(KVPairs + Config::tupleSize * mid);
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
            {
                if (!cacheBody)
                {
                    delete KVPairs;
                    KVPairs = nullptr;
                }

                return;
            }

            innerScan(KVPairs, timeStamp, lp, rp, map, hashMap);
            closeFile();
            if (!cacheBody)
            {
                delete KVPairs;
                KVPairs = nullptr;
            }
        }

        static bool compareSection(const Section &sec1, const Section &sec2)
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
            if (file)
            {
                fclose(file);
                file = nullptr;
            }
        }

        void openFile()
        {
            if (!file)
            {
                file = fopen(fileName, "rb");
            }
            else
                fseek(file, 0, SEEK_SET);
        }

        FILE *getFile()
        {
            if (!file)
            {
                file = fopen(fileName, "rb");
            }
            else
                fseek(file, 0, SEEK_SET);
            return file;
        }
    };
}