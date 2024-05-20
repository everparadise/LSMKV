#pragma once
#include "Section.hpp"
#include "Level.hpp"
#include "skiplist.h"
#include "utils.h"
#include <list>
#include <map>
#include <unordered_map>
#include <vector>
#include <string>
#include "template.hpp"
#include "compaction.hpp"
namespace SST
{

    class SSTManager
    {
        std::string path;
        std::vector<Level> levels;
        int level;
        int currTimeStamp;

    public:
        void reset()
        {
            currTimeStamp = 0;
            level = 0;
            for (auto it = levels.begin(); it != levels.end();)
            {
                it->reset();
                it = levels.erase(it);
            }
        }

        SSTManager()
        {
            currTimeStamp = 0;
            level = 0;
        }
        void initialize(std::string &&path)
        {
            this->path = std::move(path);
            if (utils::dirExists(this->path))
            {
                std::vector<std::string> fileVec;
                int size = utils::scanDir(this->path, fileVec);
                uint64_t maxHeader = 0;
                for (int i = 0; i < size; i++)
                {
                    uint64_t currNumber;
                    int levelNum;
                    if ((levelNum = checkLevelValid(fileVec[i])) == -1)
                        continue;
                    levels.emplace_back(levelNum, this->path + fileVec[i] + "/", true, &currNumber);
                    currTimeStamp = currNumber > currTimeStamp ? currNumber : currTimeStamp;
                    level = i;
                }
                return;
            }
            else
                utils::mkdir(path);
        }

        void createSection(memtable *list)
        {
            if (level == 0)
            {
                utils::mkdir(path + "level-0");
                levels.emplace_back(level, path + "level-0/");
                level++;
            }

            std::vector<Level>::iterator it = levels.begin();
            if (it->createSection(list, currTimeStamp))
            {
                while (true)
                {
                    auto next = std::next(it);
                    if (next == levels.end())
                    {
                        levels.emplace_back(level, path + std::to_string(level) + "/");
                        level++;
                    }

                    if (!Compact::compactionContext::compact(*it, *next))
                    {
                        break;
                    }
                    it = next;
                }
            }

            currTimeStamp++;
        }

        bool get(std::string &queryString, uint64_t key)
        {
            for (auto it : levels)
            {
                if (it.get(queryString, key))
                    return true;
            }
            return false;
        }

        void scan(uint64_t key1, uint64_t key2, KVMap &map, KVHash &hashMap)
        {
            for (auto it : levels)
            {
                it.scan(key1, key2, map, hashMap);
            }
        }

        int checkLevelValid(std::string &dirName)
        {
            if (dirName.size() < 7)
                return -1;
            try
            {
                int res = std::stoi(dirName.substr(6));
                return res;
            }
            catch (const std::invalid_argument &e)
            {
                return -1;
            }
        }
    };

}