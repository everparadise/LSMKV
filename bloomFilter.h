#include "MurmurHash3.h"
#include <iostream>

// bloomFilter类只进行filter操作，不对指针进行操作
// 对bloomVec的创建、删除均在CachedData中完成
// 在需要filter时从CachedData中得到bloomVec，在bloomFilter中判断

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
        bloomVec = new bool[size]{false};
    }
    ~bloomFilter()
    {
    }
    void reset()
    {
        bloomVec = nullptr;
    }
    bool query(uint64_t target)
    {
        for (uint32_t i = 0; i < hashNum; i++)
        {
            int index = filterHash(target, i + 1);
            if (!bloomVec[index])
                return false;
        }
        return true;
    }
    void insert(uint64_t target)
    {
        if (!bloomVec)
            bloomVec = new bool[size];
        n++;
        for (uint32_t i = 0; i < hashNum; i++)
        {
            int index = filterHash(target, i + 1);
            bloomVec[index] = true;
        }
    }
    int filterHash(uint64_t key, uint32_t seedInput)
    {
        uint64_t hash[2] = {0};
        MurmurHash3_x64_128(&key, sizeof(key), seed * seedInput, hash);
        return hash[0] % size;
    }
    void set(bool *array, uint64_t size)
    {
        bloomVec = array;
        bloomFilter::size = size;
    }
};