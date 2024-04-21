#include "vlog.h"
namespace VLog
{
    VLog::VLog(const std::string &vlogPath)
    {
        opened = false;
        vLogFile = fopen(vlogPath.c_str(), "w+");
        this->vlogPath = vlogPath;

        char reader[TAILREADER + 1];
        int times = 0;

        bool flag = false;
        while (!flag)
        {
            if (feof(vLogFile))
                break;
            fread(reader, TAILREADER, 1, vLogFile);
            for (int i = 0; i < TAILREADER; i++)
            {
                if (reader[i] == MAGIC)
                {
                    uint64_t pos = times * TAILREADER + i;
                    // if(!crcCheck(pos)) continue;
                    tail = pos;
                    flag = true;
                    opened = true;
                    break;
                }
            }
        }

        fseek(vLogFile, 0, SEEK_END);
        header = ftell(vLogFile);
    }
    VLog::~VLog()
    {
        if (opened)
            fclose(vLogFile);
    }
    bool VLog::crcCheck(uint64_t target)
    {
        // crcCheck here
        return true;
    }
    void VLog::get(std::string &value, uint64_t offset, uint64_t vlen)
    {
        fseek(vLogFile, offset, SEEK_SET);
        char valueChar[vlen + 1];
        fread(valueChar, 1, vlen, vLogFile);
        value.clear();
        value.append(valueChar, vlen);
    }

    void VLog::put(skiplist::skiplist_type *skiplist, std::vector<std::tuple<uint64_t, uint64_t, uint32_t>> &tuples)
    {
        fseek(vLogFile, header, SEEK_SET);

        skiplist::skiplist_type::node *curr = skiplist->header;
        curr = curr->forward[1];
        off_t start = ftell(vLogFile);
        char magic = MAGIC;
        fseek(vLogFile, 3, SEEK_CUR);

        uint64_t totalLength;
        while (curr)
        {

            uint64_t key = curr->key;
            if (curr->value == "~DELETED~")
            {
                tuples.emplace_back(key, 0, 0);
            }

            uint32_t length = curr->value.length();

            fwrite(&key, 8, 1, vLogFile);
            fwrite(&length, 4, 1, vLogFile);
            off64_t offset = ftell(vLogFile);
            fwrite(curr->value.c_str(), length, 1, vLogFile);

            tuples.emplace_back(key, offset, length);
            totalLength += 12 + length;
            curr = curr->forward[1];
        }
        fseek(vLogFile, start + 3, SEEK_SET);
        // char ptr[totalLength + 1];
        // fread(ptr, totalLength, 1, vLog);
        // std::vector<unsigned char> crcData(ptr, ptr + totalLength + 1);
        // uint16_t crc16 = utils::crc16(crcData);
        fseek(vLogFile, start, SEEK_SET);
        fwrite(&magic, 1, 1, vLogFile);
        // fwrite(reinterpret_cast<char *>(&crc16), 2, 1, vLog);

        fseek(vLogFile, 0, SEEK_END);
    }

    void VLog::close()
    {
        fclose(vLogFile);
        opened = false;
    }
}
