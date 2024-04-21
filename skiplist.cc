#include "skiplist.h"
#include <optional>
#include <cstdlib>
#include <iostream>
#include <map>
#include <unordered_map>
namespace skiplist
{
    skiplist_type::skiplist_type(double p)
    {
        srand(time(NULL));
        maxLevel = 100;
        header = new node(0, "", maxLevel);
        this->p = p;
        totalLevel = 0;
        nodeNum = 0;
    }
    int skiplist_type::randomLevel()
    {
        int level = 1;

        while ((double)(rand() % 100) / 99 < p && level < maxLevel)
            level++;
        return level;
    }
    void skiplist_type::put(key_type key, const value_type &val)
    {
        node *curr = header;
        node **update = new node *[maxLevel];

        for (int i = totalLevel; i > 0; i--)
        {

            while (curr->forward[i] && curr->forward[i]->key < key)
            {
                curr = curr->forward[i];
            }
            update[i] = curr;
            // update[i]->forward[i]->key >= key
        }
        // curr->key <= curr->forward[1]->key;
        if (curr->forward[1] && curr->forward[1]->key == key)
        {
            curr->forward[1]->value = val;
            delete[] update;
            return;
        }
        int level = randomLevel();
        // std::cout << level << "\n";
        node *newNode = new node(key, val, level);
        nodeNum++;
        if (level > totalLevel)
        {
            for (int i = totalLevel + 1; i <= level; i++)
            {
                update[i] = header;
            }
            totalLevel = level;
        }

        for (int i = 1; i <= level; i++)
        {
            newNode->forward[i] = update[i]->forward[i];
            update[i]->forward[i] = newNode;
        }
        delete[] update;
    }
    std::string skiplist_type::get(key_type key) const
    {
        node *curr = header;
        for (int i = totalLevel; i > 0; i--)
        {
            while (curr->forward[i] && curr->forward[i]->key < key)
            {
                curr = curr->forward[i];
            }
        }
        if (curr->forward[1] && curr->forward[1]->key == key)
        {
            return curr->forward[1]->value;
        }
        return "";
    }
    int skiplist_type::getNum()
    {
        return nodeNum;
    }

    skiplist_type::~skiplist_type()
    {
        node *curr = header;
        do
        {
            node *tmp = curr;
            curr = curr->forward[1];
            delete tmp;
        } while (curr);
        return;
    }
    void skiplist_type::scan(key_type key1, key_type key2, std::map<uint64_t, std::string> &map, std::unordered_map<uint64_t, uint64_t> &hashMap)
    {
        node *curr = header;
        for (int i = totalLevel; i > 0; i--)
        {
            while (curr->forward[i] && curr->forward[i]->key < key1)
            {
                curr = curr->forward[i];
            }
        }
        if (!curr->forward[1])
            return;
        curr = curr->forward[1];
        while (curr && curr->key <= key2)
        {
            map[curr->key] = curr->value;
            hashMap[curr->key] = MAXSTAMP;
            curr = curr->forward[1];
        }
    }
} // namespace skiplist
