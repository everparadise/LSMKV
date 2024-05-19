#include "Section.cc"
#include "Level.cc"
#include "skiplist.h"
#include "utils.h"
#include <list>
#include <map>
#include <unordered_map>
#include "template.cc"
namespace SST
{
    class SSTManager
    {
        std::string path;
        std::vector<Level> levels;
        int level;
        int currTimeStamp;

    public:
        SSTable(std::string &&path)
        {
            currTimeStamp = 0;
            level = 0;
            this->path = std::move(path);
            levels.emplace(level++);

            mkdir(path.c_str());
        }
        void createSection(memtable &list)
        {
            Level level0 = *levels.begin();
            level0.createSection(list);
        }

        bool get(std::string &queryString, uint64_t key)
        {
            for (auto it : levels)
            {
                if (it.get(key, queryString))
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
    }

}