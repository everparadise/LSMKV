#pragma once
#include "cachedData.h"
#include "Section.cc"
#include <set>
#include "template.cc"
namespace SST
{
    int currLevel;

    class Level
    {
        int level;
        int numbers;
        std::set<Section, Section::compareSection> sections;

    public:
        Level(int levelInput)
        {
            this.level = levelInput;
            currLevel++;
        }

        Section &createSection(std::string &&name)
        {
            sections.emplace(name);
            numbers++;
        }

        bool get(std::string &queryString, uint64_t key)
        {
            for (std::set<Section>::iterator it = sections.rbegin(); it != sections.rend(); it++)
            {
                if (it->get(queryString, key))
                    return true;
            }
            return false;
        }

        void scan(uint64_t key1, uint64_t key2, KVMap &map, KVHash &hashMap)
        {
            for (std::set<Section>::iterator it = sections.rbegin(); it != sections.rend(); it++)
            {
                it->scan(key1, key2, map, hashMap);
            }
        }

        static bool compareLevel(Level &level1, Level &level2)
        {
            return level1.level < level2.level;
        }
    };
}