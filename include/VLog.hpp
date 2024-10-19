#pragma once
#include <fstream>
#include <memory>
#include <stdio.h>
#include "skiplist.h"
#include <vector>
#include <tuple>
#include "template.hpp"
#include "config.hpp"
#include <iostream>
class KVStore;
namespace disk
{

    class VLog
    {
        friend class ::KVStore;

        uint64_t header;
        uint64_t tail;
        bool opened;

        std::string path;
        FILE *fp;
        static VLog *instance;
        VLog(std::string &name)
        {

            path = name;
            if (!(fp = fopen(path.c_str(), "r+")))
            {
                fp = fopen(path.c_str(), "w+");
            }

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
            fseek(fp, 0, SEEK_END);
            header = ftell(fp);
            if (header == 0)
            {
                tail = 0;
                return;
            }

            off_t pos = utils::seek_data_block(path);
            beginScan(pos);
        }
        // check crc to ensure data completeness
        void beginScan(uint64_t pos)
        {
            fseek(fp, pos, SEEK_SET);

            unsigned char *buffer = new unsigned char[Config::scanSize];
            int index = 0;
            bool find = false;
            while (!find && !feof(fp))
            {
                fseek(fp, pos + index * Config::scanSize, SEEK_SET);
                fread(buffer, Config::scanSize, 1, fp);

                for (uint64_t i = 0; i < Config::scanSize; i++)
                {
                    if (buffer[i] == Config::magic)
                    {
                        if (crcCheck(index * Config::scanSize + i + pos))
                        {
                            tail = index * Config::scanSize + i + pos;
                            find = true;
                            break;
                        }
                    }
                }
                index++;
            }
            delete[] buffer;

            if (feof(fp) && !find)
            {
                std::cerr << "vlog wrong format!\n"
                          << std::endl;

                exit(0);
            }
        }

        bool crcCheck(uint64_t pos)
        {
            fseek(fp, pos, SEEK_SET);
            unsigned char metaData[Config::entryMetaData];
            fread(metaData, Config::entryMetaData, 1, fp);
            uint16_t vlogCrc = *(uint16_t *)(metaData + 1);

            uint32_t vlen = *(uint32_t *)(metaData + 11);

            unsigned char *valueChar = new unsigned char[vlen];
            fread(valueChar, vlen, 1, fp);

            std::vector<unsigned char> vec(metaData + 3, metaData + 15);
            vec.insert(vec.end(), valueChar, valueChar + vlen);
            uint16_t calculateCrc = utils::crc16(vec);
            delete valueChar;
            return calculateCrc == vlogCrc;
        }

    public:
        VLog(const VLog &) = delete;
        VLog &operator=(const VLog &) = delete;
        static void initialize(std::string &name)
        {
            if (instance == nullptr)
            {
                instance = new disk::VLog(name);
                instance->initialScan();
            }
        }
        static VLog *getInstance()
        {
            if (instance == nullptr)
            {
                printf("get VLog before initialize\n");
                exit(0);
            }
            return instance;
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
            char *valueChar = new char[vlen + 1];
            size_t result = fread(valueChar, 1, vlen, fp);
            if (result != vlen)
            {
                if (ferror(fp))
                    printf("wrong file\n");
            }
            valueChar[vlen] = '\0';
            queryString.assign(valueChar);
            delete[] valueChar;
            return true;
        }

        std::vector<dataTuple> put(memtable *skiplist)
        {
            std::vector<dataTuple> tuples;
            fseek(fp, header, SEEK_SET);

            skiplist::node *curr = skiplist->getHeader();
            curr = curr->forward[1];

            off64_t offset = header;
            while (curr)
            {

                uint64_t key = curr->key;
                if (curr->value == "~DELETED~")
                {
                    tuples.emplace_back(key, 0, 0);
                    curr = curr->forward[1];
                    continue;
                }

                uint32_t currLength;
                std::vector<unsigned char> data = catKVData(key, curr->value, currLength);
                uint16_t crcCheck = utils::crc16(data);

                tuples.emplace_back(key, offset + Config::entryMetaData, currLength - Config::entryKVLength);
                fwrite(&Config::magic, 1, 1, fp);
                fwrite(&crcCheck, Config::crcCheckSize, 1, fp);
                fwrite(data.data(), currLength, 1, fp);

                offset += currLength + Config::entryHeadLength;
                curr = curr->forward[1];
            }
            header = offset;
            return tuples;
        }

        std::vector<unsigned char> catKVData(uint64_t key, std::string &value, uint32_t &totalLength)
        {
            uint32_t size = value.size();
            unsigned char *res = new unsigned char[Config::entryKVLength + size + 1];
            *(uint64_t *)res = key;
            *(uint32_t *)(res + Config::keySize) = size;
            memcpy(res + Config::entryKVLength, value.data(), size);
            res[Config::entryKVLength + size] = '\0';
            totalLength = Config::entryKVLength + size;

            std::vector<unsigned char> data(res, res + totalLength);
            delete[] res;
            return data;
        }
    };
}
