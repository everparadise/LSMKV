#pragma once
#include <fstream>
#include <memory>
#include <stdio.h>
#include "skiplist.h"
#include <vector>
#include <tuple>
#include "template.hpp"
#include "config.hpp"

namespace disk
{
    class VLog
    {
        uint64_t header;
        uint64_t tail;
        bool opened;

        std::string path;
        FILE *fp;
        static VLog *instance;
        VLog(std::string &name)
        {

            path = name;
            fp = fopen(path.c_str(), "w");

            opened = true;
        }

        ~VLog()
        {
            fclose(fp);
        }
        // to do
        // scan file when initializing
        void initialScan()
        {
        }
        // invoke periodicly to clean garbage data
        void gc()
        {
        }
        // check crc to ensure data completeness
        bool crcCheck()
        {
            return true;
        }

    public:
        VLog(const VLog &) = delete;
        VLog &operator=(const VLog &) = delete;
        static void initialize(std::string &name, bool needScan = false)
        {
            if (instance == nullptr)
            {
                instance = new disk::VLog(name);
                instance->initialScan();
            }
        }
        static VLog *getInstance(std::string *stringPtr = nullptr)
        {
            if (instance == nullptr)
            {
                printf("get VLog before initialize\n");
                exit(0);
            }
            return instance;
        }

        void log(const std::string &message)
        {
        }

        void reset()
        {
            fclose(fp);
            fp = fopen(path.c_str(), "w+");
            header = 0;
            tail = 0;
        }

        bool get(std::string &queryString, uint64_t offset, uint64_t vlen)
        {
            fseek(fp, offset, SEEK_SET);
            char valueChar[vlen + 1];
            fread(valueChar, 1, vlen, fp);
            valueChar[vlen] = '\0';
            queryString.assign(valueChar);
            return true;
        }

        std::vector<dataTuple> put(memtable *skiplist)
        {
            std::vector<dataTuple> tuples;
            fseek(fp, header, SEEK_SET);

            skiplist::node *curr = skiplist->getHeader();
            curr = curr->forward[1];
            off_t start = ftell(fp);
            fseek(fp, 3, SEEK_CUR);

            uint64_t totalLength;

            off64_t offset = -1;
            while (curr)
            {

                uint64_t key = curr->key;
                if (curr->value == "~DELETED~")
                {
                    tuples.emplace_back(key, 0, 0);
                    curr = curr->forward[1];
                    continue;
                }

                uint32_t length = curr->value.length();

                fwrite(&key, 8, 1, fp);
                fwrite(&length, 4, 1, fp);
                if (offset == -1)
                {
                    offset = ftell(fp);
                }
                else
                {
                    offset += 12;
                }
                fwrite(curr->value.c_str(), length, 1, fp);
                tuples.emplace_back(key, offset, length);
                offset += length;
                totalLength += 12 + length;
                curr = curr->forward[1];
            }
            header = (offset == -1 ? ftell(fp) : offset);
            fseek(fp, start + 3, SEEK_SET);
            // char ptr[totalLength + 1];
            // fread(ptr, totalLength, 1, vLog);
            // std::vector<unsigned char> crcData(ptr, ptr + totalLength + 1);
            // uint16_t crc16 = utils::crc16(crcData);
            fseek(fp, start, SEEK_SET);
            fwrite(&Config::magic, 1, 1, fp);
            // fwrite(reinterpret_cast<char *>(&crc16), 2, 1, vLog);

            fseek(fp, 0, SEEK_END);
            return tuples;
        }
    };
}
