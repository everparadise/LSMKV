#pragma once
#include "cachedData.h"
#include "Section.cc"
#include <set>
#include "template.cc"
#include "utils.h"
#include "string"
#include "vector"
#include <cmath>
namespace SST
{
    class Level
    {
        std::string path;
        int level;
        int numbers;
        std::set<Section, Section::compareSection> sections;

    public:
        void reset()
        {
            for (auto it = sections.begin(); it != sections.end();)
            {
                (*it).reset();
                it = sections.erase(it);
            }

            utils::rmdir(path);
        }

        Level(int levelInput, std::string &&path, bool needScan = false, uint64_t &maxTimeStamp = NULL)
        {
            this.level = levelInput;
            numbers = 1;
            for (int i = 0; i <= levelInput; i++)
            {
                numbers = numbers << 1;
            }
            this.path = path;
            if (needScan)
            {
                if (utils::dirExists(this->path))
                {
                    maxTimeStamp = -1;
                    uint64_t timeStamp;
                    std::vector<std::string> fileVec;
                    int files = utils::scanDir(this->path, fileVec);
                    for (int j = 0; j < files; j++)
                    {
                        sections.emplace(path + fileVec[j], timeStamp);
                        if (timeStamp > maxTimeStamp)
                            maxTimeStamp = timeStamp;
                    }
                    return;
                }
                else
                    mkdir(this->path);
            }
        }
        int getLevel()
        {
            return level;
        }
        bool createSection(memtable *list, uint64_t timeStamp)
        {
            sections.emplace(list, timeStamp);
            return sections.size() > numbers;
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