#include <tuple>
#include <vector>
#include "define.h"
#include <stdint.h>
#include <fstream>
#include <algorithm>

using element = std::tuple<uint64_t *, bool *>;
namespace cached
{

    // 一个bloomFilter占据8K内存
    // 这个类将SSTable中的信息进行缓存，并通过接口进行文件信息判断
    class CachedSST
    {
        // 使用元组将SST header元数据进行缓存
        static bool Compare(element &ele1, element &ele2);
        std::vector<std::tuple<uint64_t *, bool *>> cachedHead;
        std::tuple<uint64_t *, bool *> *staged;

    public:
        bool remove(uint64_t timeStamp);
        bool *cacheFile(FILE *stream);
        bool getRange(uint64_t timeStamp, uint64_t &max, uint64_t &min, uint64_t &size); // 返回值为是否缓存了这个文件头
        void sortSST();
        bool fetchData(uint64_t timeStamp, bool *bloom, uint64_t *infoPtr);
        ~CachedSST();
    };
}