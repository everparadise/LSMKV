#pragma once
#include "MurmurHash3.h"
#include <iostream>
#include "config.hpp"
class bloomFilter
{
    static const int hashNum = Config::hashnum;
    static const int size = Config::filterSize;
    static const int seed = Config::seed;

public:
    bloomFilter(bloomFilter &) = delete;
    bloomFilter operator=(bloomFilter &) = delete;
    ~bloomFilter()
    {
    }
    static bool query(unsigned char *bloom, uint64_t target)
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
                if (!bloomFilter::getBit(bloom, hash[i] % Config::bloomBit))
                    return false;
                else if (times >= hashNum)
                    return true;
            }
        }
    }
    static void insert(unsigned char *bloom, uint64_t target)
    {
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
                    bloomFilter::setBit(bloom, hash[i] % Config::bloomBit);
                else
                    return;
            }
        }
    }

    static bool getBit(unsigned char *bloomVec, int index)
    {
        int outerIndex = index / 8;
        int innerIndex = index % 8;
        return (bloomVec[outerIndex] >> innerIndex) & Config::bitReader;
    }
    static void setBit(unsigned char *bloomVec, int index)
    {
        int outerIndex = index / 8;
        int innerIndex = index % 8;
        bloomVec[outerIndex] = bloomVec[outerIndex] | (Config::bitReader << innerIndex);
    }
};