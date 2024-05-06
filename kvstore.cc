#include "kvstore.h"
#include <string>
#include <sstream>

KVStore::KVStore(const std::string &dir, const std::string &vlog) : KVStoreAPI(dir, vlog), filter(HASHNUM, FILTERSIZE)
{
	// stamp and dir need read all files under root path
	// the naive implementation need to be completed
	timeStamp = 1;
	memtab = new skiplist::skiplist_type(PROB);

	rootPath = dir;
	if (!utils::dirExists(rootPath))
	{
		utils::_mkdir(rootPath);
	}
	rootPath.append("/");

	try
	{
		vlogPath = vlog;
		vlogMaintainer = new VLog::VLog(vlog);
	}
	catch (std::string except)
	{
		printf("open vlog fault\n");
		printf("%s\n", vlog.c_str());
		return;
	}

	std::vector<std::string> dirVec;
	std::vector<std::string> fileVec;

	int size = utils::scanDir(rootPath, dirVec);
	uint64_t maxHeader = 0;

	for (int i = 0; i < size; i++)
	{
		if (rootPath + dirVec[i] == vlog)
			continue;
		std::string fileName = rootPath + dirVec[i] + "/";
		int files = utils::scanDir(fileName, fileVec);
		for (int j = 0; j < files; j++)
		{
			FILE *sstable = fopen((fileName + fileVec[j]).c_str(), "r");
			caches.cacheFile(sstable);
			fclose(sstable);
		}
	}

	caches.sortSST();
}

KVStore::~KVStore()
{
	storeMem();
	delete memtab;
}

/**
 * Insert/Update the key-value pair.
 * No return values for simplicity.
 */
void KVStore::put(uint64_t key, const std::string &s)
{
	memtab->put(key, s);
	if (memtab->getNum() == SSTMAX)
	{
		storeMem();
	}
}

void KVStore::storeMem()
{
	storeVLog();
	createSSTable();
	delete memtab;
	memtab = new skiplist::skiplist_type(PROB);
}

void KVStore::storeVLog()
{
	/*
	using buffer write method to optimize the codes below, then profile to see how much it optimized
	*/
	vlogMaintainer->put(memtab, tuples);
}
void KVStore::SSTableHead(FILE *sst)
{
	// 时间戳
	fwrite(&timeStamp, 8, 1, sst);

	// 键的数量
	uint64_t size = tuples.size();
	fwrite(&size, 8, 1, sst);

	// 极小键， 极大键
	auto [minKey, ignore1, ignore2] = tuples[0];
	auto [maxKey, ignore4, ignore3] = tuples[size - 1];
	fwrite(&minKey, 8, 1, sst);
	fwrite(&maxKey, 8, 1, sst);
}

void KVStore::createSSTable()
{
	/*
	using buffer write method to optimize the codes below, then profile the project to see how much it matters
			 = =
	*/
	std::ostringstream oss;
	std::string path = rootPath;
	if (!utils::dirExists(rootPath + "level0"))
		utils::_mkdir(rootPath + "level0");
	oss << path << "level0/" << timeStamp << ".sst";
	FILE *sst = fopen(oss.str().c_str(), "w+");
	if (sst == NULL)
	{
		printf("open sst wrong\n");
		return;
	}

	// fwrite SSTable header
	SSTableHead(sst);
	filter.reset();
	// filter.set(caches.cacheFile(sst), BLOOMSIZE);

	// write SSTable Main Content(tuples)
	fseek(sst, KEYSTART, SEEK_SET);

	for (auto it : tuples)
	{
		auto [key, offset, vlen] = it;
		filter.insert(key);
		fwrite(&key, 8, 1, sst);
		fwrite(&offset, 8, 1, sst);
		fwrite(&vlen, 4, 1, sst);
	}
	fseek(sst, BLOOMSTART, SEEK_SET);

	// store bloomFilter ahead of the content
	fwrite(filter.bloomVec, 1, FILTERSIZE, sst);
	caches.cacheFile(sst);
	// close the sstable file;
	fclose(sst);

	// update timeStamp
	timeStamp++;
}
/**
 * Returns the (string) value of the given key.
 * An empty string indicates not found.
 */
// 如果在memtab中检查到DELETED标识，则直接返回空字符串，检查到其他字符则返回字符
// 如果memtab中未找到目标字符串，则从硬盘中查找
// 递归寻找每个文件夹中的文件
// 找到时间戳最大的记录
std::string KVStore::get(uint64_t key)
{
	std::string value = memtab->get(key);
	if (value == "~DELETED~")
		return "";
	if (value.size())
		return value;
	std::vector<std::string> dirVec;
	std::vector<std::string> fileVec;
	value.clear();
	int size = utils::scanDir(rootPath, dirVec);
	uint64_t maxHeader = 0;

	for (int i = 0; i < size; i++)
	{
		if (rootPath + dirVec[i] == vlogPath)
			continue;
		std::string fileName = rootPath + dirVec[i] + "/";
		int files = utils::scanDir(fileName, fileVec);
		for (int j = 0; j < files; j++)
		{
			FILE *sstable = fopen((fileName + fileVec[j]).c_str(), "r");
			maxHeader = get(sstable, key, maxHeader, value);
			fclose(sstable);
		}
	}
	return value;
}
uint64_t KVStore::get(FILE *sstable, uint64_t key, uint64_t maxHeader, std::string &value)
{
	uint64_t header;
	fread(&header, 8, 1, sstable);
	if (header < maxHeader)
		return maxHeader;

	uint64_t min, max, tupleNum;
	caches.getRange(header, max, min, tupleNum);

	if (key > max || key < min)
		return maxHeader;

	bool *bloomArray;
	caches.fetchData(header, &bloomArray, nullptr);

	filter.set(bloomArray, FILTERSIZE);
	if (filter.query(key))
	{
		filter.bloomVec = nullptr;
		if (get(sstable, key, value, tupleNum))
			return header;
	}
	filter.bloomVec = nullptr;
	return maxHeader;
}
bool KVStore::get(FILE *sstable, uint64_t key, std::string &value, uint64_t tupleNum)
{
	char tuples[tupleNum * TUPLESIZE + 1];
	fseek(sstable, KEYSTART, SEEK_SET);
	fread(tuples, 1, tupleNum * TUPLESIZE, sstable);
	uint64_t lp = 0, rp = tupleNum - 1;
	while (lp <= rp)
	{
		uint64_t mid = (lp + rp) / 2;
		uint64_t currKey = *reinterpret_cast<uint64_t *>(tuples + mid * TUPLESIZE);
		if (currKey == key)
		{
			uint64_t offset = *reinterpret_cast<uint64_t *>(tuples + mid * TUPLESIZE + 8);
			uint32_t vlen = *reinterpret_cast<uint32_t *>(tuples + mid * TUPLESIZE + 16);
			if (vlen == 0)
			{
				value.clear();
				return true;
			}

			get(value, offset, vlen);
			return true;
		}
		else if (currKey < key)
		{
			lp = mid + 1;
			continue;
		}
		else if (currKey > key)
		{
			rp = mid - 1;
			continue;
		}
	}
	return false;
}
void KVStore::get(std::string &value, uint64_t offset, uint64_t vlen)
{
	vlogMaintainer->get(value, offset, vlen);
}
/**
 * Delete the given key-value pair if it exists.
 * Returns false iff the key is not found.
 */
bool KVStore::del(uint64_t key)
{
	std::string value = memtab->get(key);
	if (value == "~DELETED~")
		return false;
	value.clear();
	value = get(key);
	if (!value.size())
		return false;
	memtab->put(key, "~DELETED~");
	return true;
}
/**
 * This resets the kvstore. All key-value pairs should be removed,
 * including memtable and all sstables files.
 */
void KVStore::reset()
{
	std::vector<std::string> vec;
	int size = utils::scanDir(rootPath, vec);
	for (int i = 0; i < size; i++)
	{
		std::string fileName = rootPath + vec[i];
		if (!utils::dirExists(fileName))
		{
			utils::rmfile(fileName);
			continue;
		}
		fileName += "/";
		std::vector<std::string> innerVec;
		int dirSize = utils::scanDir(fileName, innerVec);
		for (int j = 0; j < dirSize; j++)
		{
			utils::rmfile(fileName + innerVec[j]);
		}
		utils::rmdir(fileName);
	}
	delete vlogMaintainer;
	delete memtab;
	memtab = new skiplist::skiplist_type(PROB);

	filter.reset();

	if (!utils::dirExists(rootPath))
	{
		utils::_mkdir(rootPath);
	}

	vlogMaintainer = new VLog::VLog(vlogPath);
}

/**
 * Return a list including all the key-value pair between key1 and key2.
 * keys in the list should be in an ascending order.
 * An empty string indicates not found.
 */
void KVStore::scan(uint64_t key1, uint64_t key2, std::list<std::pair<uint64_t, std::string>> &list)
{
	// corner case when invalid key pair and equal pair
	if (key2 < key1)
		return;
	else if (key2 == key1)
	{
		list.emplace_back(key1, get(key1));
		return;
	}

	// commen case
	// map: arg1 -> key   arg2 -> timeStamp
	// priority_queue
	std::unordered_map<uint64_t, uint64_t> hashMap;
	std::map<uint64_t, std::string> map;
	memtab->scan(key1, key2, map, hashMap);

	std::vector<std::string> dirVec;
	std::vector<std::string> fileVec;
	int size = utils::scanDir(rootPath, dirVec);
	uint64_t maxHeader = 0;

	for (int i = 0; i < size; i++)
	{
		if (rootPath + dirVec[i] == vlogPath)
			continue;
		std::string fileName = rootPath + dirVec[i] + "/";
		int files = utils::scanDir(fileName, fileVec);
		for (int j = 0; j < files; j++)
		{
			FILE *sstable = fopen((fileName + fileVec[j]).c_str(), "r");
			scan(sstable, key1, key2, map, hashMap);
			fclose(sstable);
		}
	}
	for (auto &it : map)
	{
		if (it.second == "~DELETED~")
			continue;
		list.emplace_back(it);
	}
}

void KVStore::scan(FILE *sstable, uint64_t key1, uint64_t key2, KVMap &map, KVHash &hashMap)
{
	uint64_t timeStamp, maxKey, minKey, size;
	fread(&timeStamp, 8, 1, sstable);
	fread(&size, 8, 1, sstable);
	fread(&minKey, 8, 1, sstable);
	fread(&maxKey, 8, 1, sstable);

	// make sure target key may exist in this file
	if (minKey > key2 || maxKey < key1)
		return;
	fseek(sstable, KEYSTART, SEEK_SET);
	char KVPairs[TUPLESIZE * size + 1];
	fread(KVPairs, 1, TUPLESIZE * size, sstable);
	uint64_t lp = 0, rp = size - 1;
	uint64_t mid;
	if (key1 > minKey)
	{
		uint64_t tmprp = rp;
		while (lp < tmprp)
		{
			mid = (lp + tmprp) / 2;
			uint64_t currKey = *reinterpret_cast<uint64_t *>(KVPairs + TUPLESIZE * mid);
			if (currKey < key1)
				lp = mid + 1;
			else if (currKey > key1)
			{
				tmprp = mid - 1;
			}
			else
			{
				lp = mid;
				break;
			}
		}
	}
	if (key2 < maxKey)
	{
		uint64_t tmplp = lp;
		while (tmplp < rp)
		{
			mid = (tmplp + rp) / 2;
			uint64_t currKey = *reinterpret_cast<uint64_t *>(KVPairs + TUPLESIZE * mid);
			if (currKey > key2)
				rp = mid - 1;
			else if (currKey < key2)
			{
				tmplp = mid + 1;
			}
			else
			{
				rp = mid;
				break;
			}
		}
	}
	// curr[rp] <= key2 && curr[lp] >= key1
	if (rp < lp)
		return;

	scan(KVPairs, timeStamp, lp, rp, map, hashMap);
}

void KVStore::scan(char *charArray, uint64_t timeStamp, uint64_t lp, uint64_t rp, KVMap &map, KVHash &hashMap)
{
	uint64_t currKey, keyLen, keyOff, preStamp;
	uint64_t *uintPtr;
	for (; lp <= rp; lp++)
	{
		uintPtr = reinterpret_cast<uint64_t *>(charArray + lp * TUPLESIZE);
		currKey = *uintPtr;
		auto it = hashMap.find(currKey);
		if (it != hashMap.end() && it->second < timeStamp || it == hashMap.end())
		{
			hashMap[currKey] = timeStamp;
			keyOff = uintPtr[1];
			keyLen = *reinterpret_cast<uint32_t *>(uintPtr + 2);
			std::string str;
			get(str, keyOff, keyLen);
			map[currKey] = str;
		}
	}
}
/**
 * This reclaims space from vLog by moving valid value and discarding invalid value.
 * chunk_size is the size in byte you should AT LEAST recycle.
 */
void KVStore::gc(uint64_t chunk_size)
{
}
