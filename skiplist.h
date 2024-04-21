#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <cstdint>
// #include <optional>
// #include <vector>
#include <string>
#include "define.h"
#include <map>
#include <unordered_map>
namespace VLog
{
    class VLog;
}

class KVStore;

namespace skiplist
{

    using key_type = uint64_t;
    // using value_type = std::vector<char>;
    using value_type = std::string;

    class skiplist_type
    {
        friend class VLog::VLog;
        friend class ::KVStore;
        // add something here
        int totalLevel;
        int maxLevel;
        int nodeNum;
        double p;

        struct node
        {
            node **forward;
            int level;
            key_type key;
            value_type value;
            node(key_type keyInput, value_type valueInput, int level) : key(keyInput), value(valueInput)
            {
                forward = new node *[level + 1];
                for (int i = 0; i <= level; i++)
                {
                    forward[i] = nullptr;
                }
            }
            ~node()
            {
                if (forward)
                    delete[] forward;
                forward = nullptr;
            }
        };
        node *header;
        int randomLevel();

    public:
        explicit skiplist_type(double p = 0.5);
        void put(key_type key, const value_type &val);
        // std::optional<value_type> get(key_type key) const;
        std::string get(key_type key) const;

        int getNum();
        void scan(key_type key1, key_type key2, std::map<uint64_t, std::string> &map, std::unordered_map<uint64_t, uint64_t> &hashMap);
        ~skiplist_type();
    };

} // namespace skiplist

#endif // SKIPLIST_H
