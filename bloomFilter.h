
#include "MurmurHash3.h"
#include <iostream>
class bloomFilter
{
    static const int hashNum;
    static const int n;
    static const int size;
    static const int seed;

public:
    bloomFilter(int k_input, int m_input, int seed_input = 1) : hashNum(k_input), n(0), size(m_input), seed(seed_input)
    {
    }
    ~bloomFilter()
    {
    }
    static bool query(bool *bloom, uint64_t target)
    {
        uint32_t hash[4] = {0};
        uint32_t times = 0;
        while (true)
        {
            if (times >= hashNum)
                return true;
            MurmurHash3_x64_128(&target, sizeof(target), seed * hash[0], hash);
            for (int i = 0; i < 4; i++, times++)
            {
                if (!bloomFilter::getBit(bloom, hash[i] % BLOOMBIT))
                    return false;
                else if (times >= hashNum)
                    return true;
            }
        }
    }
    static void insert(bool *bloom, uint64_t target)
    {
        n++;
        uint32_t hash[4] = {0};
        uint32_t times = 0;
        while (true)
        {
            if (times >= hashNum)
                return;
            MurmurHash3_x64_128(&target, sizeof(target), seed * hash[0], hash);
            for (int i = 0; i < 4; i++, times++)
            {
                if (times < hashNum)
                    bloomFilter::setBit(bloom, hash[i] % BLOOMBIT);
                else
                    return;
            }
        }
    }

    static bool getBit(bool *bloomVec, int index)
    {
        int outerIndex = index / 8;
        int innerIndex = index % 8;
        return (bloomVec[outerIndex] >> innerIndex) & BITREADER;
    }
    static void setBit(bool *bloomVec, int index)
    {
        int outerIndex = index / 8;
        int innerIndex = index % 8;
        bloomVec[outerIndex] = bloomVec[outerIndex] | (BITREADER << innerIndex);
    }
};