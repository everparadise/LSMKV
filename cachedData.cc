#include "cachedData.h"

namespace cached
{
    bool *CachedSST::cacheFile(FILE *stream)
    {
        fseek(stream, 0, SEEK_SET);

        uint64_t *info = new uint64_t[4];
        bool *bloomVec = new bool[BLOOMSIZE];

        fread(info, 8, 4, stream);
        fread(bloomVec, 1, BLOOMSIZE, stream);

        cachedHead.emplace_back(std::make_tuple(info, bloomVec));
        return bloomVec;
    }

    bool CachedSST::remove(uint64_t timeStamp)
    {

        for (std::vector<element>::iterator it = cachedHead.begin(); it != cachedHead.end(); it++)
        {

            if (std::get<0>(*it)[0] == timeStamp)
            {
                if (staged == &(*it))
                    staged = nullptr;
                auto [ptr1, ptr2] = *it;
                delete[] ptr1;
                delete[] ptr2;

                cachedHead.erase(it);
                return true;
            }
        }
        return false;
    }

    bool CachedSST::getRange(uint64_t timeStamp, uint64_t &max, uint64_t &min, uint64_t &size)
    {
        for (std::vector<element>::iterator it = cachedHead.begin(); it != cachedHead.end(); it++)
        {
            uint64_t *info = std::get<0>(*it);
            if (info[0] == timeStamp)
            {
                size = info[1];
                min = info[2];
                max = info[3];

                staged = &(*it);
                return true;
            }
        }
        return false;
    }

    bool CachedSST::fetchData(uint64_t timeStamp, bool **bloom, uint64_t *infoPtr)
    {
        if (staged && std::get<0>(*staged)[0] == timeStamp)
        {
            infoPtr = std::get<0>(*staged);
            *bloom = std::get<1>(*staged);
            return true;
        }
        for (auto it : cachedHead)
        {
            uint64_t *info = std::get<0>(it);
            if (info[0] == timeStamp)
            {
                infoPtr = info;
                *bloom = std::get<1>(it);
                return true;
            }
        }
        return false;
    }

    CachedSST::~CachedSST()
    {
        for (std::vector<element>::iterator it = cachedHead.begin(); it != cachedHead.end(); it++)
        {
            auto [ptr1, ptr2] = *it;
            delete[] ptr1;
            delete[] ptr2;
            it = cachedHead.erase(it);
        }
    }
    bool CachedSST::Compare(element &ele1, element &ele2)
    {
        return std::get<0>(ele1)[0] > std::get<0>(ele2)[0];
    }
    void CachedSST::sortSST()
    {
        std::sort(cachedHead.begin(), cachedHead.end(), Compare);
    }
}
