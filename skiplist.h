#ifndef SKIPLIST_H
#define SKIPLIST_H

#include <cstdint>
// #include <optional>
// #include <vector>
#include <string>
#include "config.hpp"
#include <map>
#include <unordered_map>

namespace skiplist
{
    using key_type = uint64_t;
    using value_type = std::string;
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
    class skiplist_type
    {
        // add something here
        int totalLevel;
        int maxLevel;
        int nodeNum;
        double p;

        node *header;
        int randomLevel();

    public:
        explicit skiplist_type(double p = 0.5);
        void put(key_type key, const value_type &val);
        std::string get(key_type key) const;

        node *getHeader()
        {
            return header;
        }
        int getNum();
        void scan(key_type key1, key_type key2, std::map<uint64_t, std::string> &map, std::unordered_map<uint64_t, uint64_t> &hashMap);
        void flush();
        bool query(uint64_t key);
        ~skiplist_type();
    };

}

#endif // SKIPLIST_H
