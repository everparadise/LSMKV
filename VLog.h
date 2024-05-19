#include <fstream>
#include <memory>
#include <stdio.h>
#include "skiplist.h"
#include <vector>
#include <tuple>
#include <define.h>
namespace disk
{
    class VLog
    {
        uint64_t header;
        uint64_t tail;
        bool opened;

        std::string path;
        FILE *fp;
        static std::unique_ptr<VLog> instance;
        VLog(std::string &&name)
        {
            path = std::move(name);
            fp = fopen(path.c_str(), "w");

            opened = true;
            initialScan();
        }

        ~VLog()
        {
            fclose(fp);
        }
        // to do
        // 启动时扫描初始化
        void initialScan();
        // 周期执行gc
        void gc();

    public:
        VLog(const VLog &) = delete;
        VLog &operator=(const VLog &) = delete;
        static void initialize(std::string &&name)
        {
            if (!instance)
            {
                VLog::instance = std::make_unique<VLog>(new VLog(name));
            }
        }
        static VLog &getInstance(std::string &&name = NULL)
        {
            if (!instance)
            {
                printf("use after initialize\n");
                exit(0);
            }
            return *instance;
        }

        void log(const std::string &message)
        {
        }

        bool get(std::string queryString, uint64_t offset, uint64_t vlen)
        {
            fseek(fp, offset, SEEK_SET);
            char valueChar[vlen + 1];
            fread(valueChar, 1, vlen, fp);
            queryString.assign(valueChar);
        }

        std::vector<std::tuple<uint64_t, uint64_t, uint32_t>> put(skiplist::skiplist_type &skiplist)
        {
            std::vector<std::tuple<uint64_t, uint64_t, uint32_t>> tuples;
            fseek(fp, header, SEEK_SET);

            skiplist::skiplist_type::node *curr = skiplist.getHeader;
            curr = curr->forward[1];
            off_t start = ftell(fp);
            char magic = MAGIC;
            fseek(fp, 3, SEEK_CUR);

            uint64_t totalLength;
            off64_t offset = -1;
            while (curr)
            {

                uint64_t key = curr->key;
                if (curr->value == "~DELETED~")
                {
                    tuples.emplace_back(key, 0, 0);
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
                offset += length;
                tuples.emplace_back(key, offset, length);
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
            fwrite(&magic, 1, 1, fp);
            // fwrite(reinterpret_cast<char *>(&crc16), 2, 1, vLog);

            fseek(fp, 0, SEEK_END);
            return tuples;
        }
    }
}
