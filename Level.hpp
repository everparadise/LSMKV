#pragma once
#include "Section.hpp"
#include <set>
#include "template.hpp"
#include "utils.h"
#include "string"
#include "vector"
#include <cmath>
#include <algorithm>
namespace SST
{
    bool checkSectionValid(std::string &fileName);

    class Level
    {
        // 与根目录相对路径，以/结尾
        std::string path;
        int level;
        int numbers;

        struct Compare
        {
            bool operator()(const Section &sec1, const Section &sec2) const
            {
                return Section::compareSection(sec1, sec2);
            }
        };
        std::vector<Section> sections;

    public:
        void reset()
        {
            for (auto it = sections.begin(); it != sections.end();)
            {
                it->reset();
                it = sections.erase(it);
            }
            utils::rmdir(path);
            level = 0;
            numbers = 0;
        }

        Level(int levelInput, std::string &&path, bool needScan = false, uint64_t *maxTimeStamp = nullptr)
        {
            this->level = levelInput;
            numbers = 1;
            for (int i = 0; i <= levelInput; i++)
            {
                numbers = numbers << 1;
            }
            this->path = path;
            if (needScan)
            {
                if (utils::dirExists(this->path))
                {
                    *maxTimeStamp = -1;
                    uint64_t timeStamp;
                    std::vector<std::string> fileVec;
                    int files = utils::scanDir(this->path, fileVec);
                    for (int j = 0; j < files; j++)
                    {
                        if (!checkSectionValid(fileVec[j]))
                            continue;
                        sections.emplace_back(path + fileVec[j], timeStamp);
                        if (timeStamp > *maxTimeStamp)
                            *maxTimeStamp = timeStamp;
                    }
                    std::sort(sections.begin(), sections.end(), Section::compareSection);

                    return;
                }
                else
                    utils::mkdir(this->path);
            }
        }
        int getLevel()
        {
            return level;
        }
        bool createSection(memtable *list, uint64_t timeStamp)
        {
            sections.emplace_back(path + std::to_string(timeStamp), list, timeStamp);
            return sections.size() > numbers;
        }

        bool get(std::string &queryString, uint64_t key)
        {
            for (std::vector<Section>::reverse_iterator it = sections.rbegin(); it != sections.rend(); it++)
            {
                if (it->get(queryString, key))
                    return true;
            }
            return false;
        }

        void scan(uint64_t key1, uint64_t key2, KVMap &map, KVHash &hashMap)
        {
            for (std::vector<Section>::reverse_iterator it = sections.rbegin(); it != sections.rend(); it++)
            {
                it->scan(key1, key2, map, hashMap);
            }
        }

        static bool compareLevel(Level &level1, Level &level2)
        {
            return level1.level < level2.level;
        }

        bool checkSectionValid(std::string &fileName)
        {
            size_t index;
            if ((index = fileName.find_first_of('-')) == std::string::npos)
                return false;

            return true;
        }
    };

}