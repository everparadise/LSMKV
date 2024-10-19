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
#include <cassert>
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
            }
            else
                utils::mkdir(path);

            std::sort(levels.begin(), levels.end(), [](Level &level1, Level &level2)
                      { return level1.getLevel() < level2.getLevel(); });
        }

        void createSection(memtable *list)
        {
            if (level == 0)
            {
                utils::mkdir(path + "level-0");
                levels.emplace_back(level, path + "level-0/");
                level++;
            }

            int currLevel = 0;
            if (levels[currLevel].createSection(list, currTimeStamp))
            {
                currTimeStamp++;

                while (true)
                {
                    int nextLevel = currLevel + 1;
                    if (nextLevel == levels.size())
                    {
                        levels.emplace_back(level, path + "level-" + std::to_string(level) + "/");
                        level++;
                    }

                    if (!compact(levels[currLevel], levels[nextLevel]))
                    {
                        break;
                    }
                    currLevel++;
                }

                return;
            }

            currTimeStamp++;
        }

        bool get(std::string &queryString, uint64_t key)
        {
            for (auto &it : levels)
            {
                if (it.get(queryString, key))
                    return true;
            }
            return false;
        }
        uint64_t getOffset(uint64_t key)
        {
            for (auto &it : levels)
            {
                uint64_t offset = it.getOffset(key);
                if (offset != -1)
                    return offset;
            }
            return -1;
        }
        void scan(uint64_t key1, uint64_t key2, KVMap &map, KVHash &hashMap)
        {
            for (auto &it : levels)
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

        static bool compareLevel(SST::Section *sec1, SST::Section *sec2)
        {
            if (sec1->timeStamp == sec2->timeStamp)
            {
                return sec1->minKey < sec2->minKey;
            }
            else
                return sec1->timeStamp < sec2->timeStamp;
        }

        bool compact(SST::Level &levelUp, SST::Level &levelDown)
        {
            // std::vector<SST::Section *> sections;
            std::list<SST::Section *> sections;
            uint64_t newTimeStamp = selectSection(levelUp, levelDown, sections);

            // 可能会出现下层 小timestamp数据在后的情况
            // std::sort(sections.begin(), sections.end(), compareLevel);

            // 需要验证取出的section严格按照timeStamp升序
            std::vector<std::vector<dataTuple>> datas = extractData(sections);

            bool bottomLevel = false;
            if (levelDown.getLevel() == level)
                bottomLevel = true;
            merge(datas, bottomLevel);
            eraseOriginSection(levelUp, levelDown);

            levelDown.createSection(datas[0], newTimeStamp);
            // split(datas[0], newTimeStamp, levelDown);
            return levelDown.sections.size() > levelDown.getNumber();
        }

        void eraseOriginSection(SST::Level &levelUp, SST::Level &levelDown)
        {
            for (auto it = levelUp.sections.begin(); it != levelUp.sections.end();)
            {
                if (!it->getValid())
                {
                    it->rmfile();
                    it = levelUp.sections.erase(it);
                }

                else
                    it++;
            }
            for (auto it = levelDown.sections.begin(); it != levelDown.sections.end();)
            {
                if (!it->getValid())
                {
                    it->rmfile();
                    it = levelDown.sections.erase(it);
                }

                else
                    it++;
            }
        }

        void split(std::vector<dataTuple> &mergedTuple, uint64_t timeStamp, SST::Level &target)
        {
            std::string root = target.getName();

            target.createSection(mergedTuple, timeStamp);

            // assert Section in levels are sorted
            for (int i = 0; i < target.sections.size() - 1; i++)
            {
                auto tmp1 = target.sections[i];
                auto tmp2 = target.sections[i + 1];

                assert(tmp1.timeStamp <= tmp2.timeStamp);
                assert(tmp1.minKey <= tmp2.minKey);
            }
        }

        void merge(std::vector<std::vector<dataTuple>> &tuples, bool bottomLevel)
        {
            // ensure element in tuples strictly rising
            while (tuples.size() != 1)
            {
                int size = tuples.size();
                std::vector<std::vector<dataTuple>> mergeTmp;
                int index;
                for (index = 0; index < size - 1; index += 2)
                {
                    mergeTmp.emplace_back(std::move(merge(tuples[index], tuples[index + 1], bottomLevel)));
                }
                if (index == size - 1)
                {
                    mergeTmp.emplace_back(std::move(tuples[index]));
                }
                tuples = std::move(mergeTmp);
            }
        }

        std::vector<dataTuple> merge(std::vector<dataTuple> &tuple1, std::vector<dataTuple> &tuple2, bool bottomLevel)
        {
            // ensure tuple1 has less timeStamp than tuple2
            // key offset vlen
            std::vector<dataTuple> res;

            auto it1 = tuple1.begin();
            auto it2 = tuple2.begin();
            while (!(it1 == tuple1.end() && it2 == tuple2.end()))
            {
                if (it1 == tuple1.end())
                {
                    if (!bottomLevel || std::get<1>(*it2) != 0)
                        res.emplace_back(std::move(*it2));
                    it2++;
                    continue;
                }
                if (it2 == tuple2.end())
                {
                    if (!bottomLevel || std::get<1>(*it1) != 0)
                        res.emplace_back(std::move(*it1));
                    it1++;
                    continue;
                }
                auto [key1, offset1, vlen1] = *it1;
                auto [key2, offset2, vlen2] = *it2;

                if (key1 < key2)
                {
                    if (!bottomLevel || offset1 != 0)
                        res.emplace_back(key1, offset1, vlen1);
                    it1++;
                }
                else
                {
                    if (!bottomLevel || offset2 != 0)
                        res.emplace_back(key2, offset2, vlen2);
                    if (key1 == key2)
                    {
                        it1++;
                    }
                    it2++;
                }
            }

            return res;
        }

        std::vector<std::vector<dataTuple>> extractData(std::list<SST::Section *> &sections)
        {
            std::vector<std::vector<dataTuple>> extractedVecs;
            char *tmpChar = new char[Config::sstMax * Config::tupleSize + 1];
            for (auto &it : sections)
            {
                std::vector<dataTuple> sectionData;
                FILE *file = it->getFile();
                fseek(file, Config::keyStart, SEEK_SET);
                uint64_t tupleNumbers = it->numbers;
                fread(tmpChar, Config::tupleSize, tupleNumbers, file);

                for (int index = 0; index < tupleNumbers; index++)
                {
                    uint64_t *tuple = (uint64_t *)(tmpChar + index * Config::tupleSize);
                    // tuple = (uint64_t *)tuple;
                    //  printf("index: %d, tuple: %p, calculated address: %p\n", index, tuple, (tmpChar + index * Config::tupleSize));
                    sectionData.emplace_back(tuple[0], tuple[1], *(uint32_t *)(tuple + 2));
                }

                extractedVecs.emplace_back(std::move(sectionData));
                it->closeFile();
            }

            delete tmpChar;
            return extractedVecs;
        }

        uint64_t selectSection(SST::Level &levelUp, SST::Level &levelDown, std::list<SST::Section *> &res)
        {
            uint64_t maxTimeStamp = 0;

            if (levelUp.getLevel() == 0)
            {
                for (auto &it : levelUp.sections)
                {
                    res.emplace_back(&it);
                    it.revertValid();
                    maxTimeStamp = it.timeStamp > maxTimeStamp ? it.timeStamp : maxTimeStamp;
                }
            }
            else
            {
                int cut = levelUp.sections.size() - levelUp.getNumber();
                for (int i = 0; i < cut; i++)
                {
                    levelUp.sections[i].revertValid();
                    res.emplace_back(&levelUp.sections[i]);
                    uint64_t currTimeStamp = levelUp.sections[i].timeStamp;
                    maxTimeStamp = currTimeStamp > maxTimeStamp ? currTimeStamp : maxTimeStamp;
                }
            }
            std::list<SST::Section *> tmp;
            for (auto &it : levelDown.sections)
            {
                for (auto &secondIt : res)
                {
                    if ((it.maxKey >= secondIt->minKey && it.maxKey <= secondIt->maxKey) ||
                        (it.minKey >= secondIt->minKey && it.minKey <= secondIt->maxKey) ||
                        (secondIt->minKey >= it.minKey && secondIt->minKey <= it.maxKey) ||
                        (secondIt->maxKey >= it.minKey && secondIt->maxKey <= it.maxKey))
                    {
                        it.revertValid();
                        tmp.emplace_back(&it);
                    }
                }
            }

            res.splice(res.begin(), tmp);
            return maxTimeStamp;
        }
    };

}