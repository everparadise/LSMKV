
#include "MurmurHash3.h"
#include <iostream>
class bloomFilter
{
    int hashNum;
    int n;
    int size;
    int seed;

public:
    bool *bloomVec;
    bloomFilter(int k_input, int m_input, int seed_input = 1) : hashNum(k_input), n(0), size(m_input)
    {
        seed = seed_input;
        bloomVec = nullptr;
    }
    ~bloomFilter()
    {
        delete[] bloomVec;
    }
    void reset()
    {
        n = 0;
        delete[] bloomVec;
        bloomVec = new bool[size];
    }
    bool query(uint64_t target)
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
                if (!getBit(hash[i] % BLOOMBIT))
                    return false;
                else if (times >= hashNum)
                    return true;
            }
        }
    }
    void insert(uint64_t target)
    {
        if (!bloomVec)
            bloomVec = new bool[size];
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
                    setBit(hash[i] % BLOOMBIT);
                else
                    return;
            }
        }
    }

    void set(bool *array, uint64_t size)
    {
        n = 0;
        if (bloomVec)
            delete[] bloomVec;
        bloomVec = array;
        bloomFilter::size = size;
    }

    bool getBit(int index)
    {
        int outerIndex = index / 8;
        int innerIndex = index % 8;
        return (bloomVec[outerIndex] >> innerIndex) & BITREADER;
    }
    void setBit(int index)
    {
        int outerIndex = index / 8;
        int innerIndex = index % 8;
        bloomVec[outerIndex] = bloomVec[outerIndex] | (BITREADER << innerIndex);
    }
};