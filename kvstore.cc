#include "kvstore.h"
#include <string>
#include <sstream>

KVStore::KVStore(const std::string &dir, const std::string &vlog) : KVStoreAPI(dir, vlog)
{
	// stamp and dir need read all files under root path
	// the naive implementation need to be completed
	timeStamp = 1;
	memtab = new skiplist::skiplist_type(Config::prob);

	rootPath = dir;
	vlogPath = vlog;
	if (!utils::dirExists(rootPath))
	{
		utils::_mkdir(rootPath);
	}
	try
	{
		sstManager.initialize(rootPath + "/");
		disk::VLog::initialize(vlogPath, true);
	}
	catch (std::string message)
	{
		printf("path fault\n");
		printf("%s\n", message.c_str());
		exit(0);
	}
}

KVStore::~KVStore()
{
	if (memtab->getNum() != 0)
		sstManager.createSection(memtab);
	delete memtab;
}

/**
 * Insert/Update the key-value pair.
 * No return values for simplicity.
 */
void KVStore::put(uint64_t key, const std::string &s)
{
	memtab->put(key, s);
	if (memtab->getNum() == Config::sstMax)
	{
		storeMem();
	}
}

void KVStore::storeMem()
{
	sstManager.createSection(memtab);
	delete memtab;
	memtab = new memtable(Config::prob);
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

	sstManager.get(value, key);
	return value;
}

/**
 * Delete the given key-value pair if it exists.
 * Returns false iff the key is not found.
 */
bool KVStore::del(uint64_t key)
{
	std::string value = get(key);
	if (value.empty())
		return false;
	put(key, "~DELETED~");
	return true;
}
/**
 * This resets the kvstore. All key-value pairs should be removed,
 * including memtable and all sstables files.
 */
void KVStore::reset()
{
	delete memtab;
	memtab = new memtable(Config::prob);
	sstManager.reset();
	auto vlogInstance = disk::VLog::getInstance();
	vlogInstance->reset();
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
	sstManager.scan(key1, key2, map, hashMap);

	for (auto &it : map)
	{
		if (it.second == "~DELETED~")
			continue;
		list.emplace_back(it);
	}
}

/**
 * This reclaims space from vLog by moving valid value and discarding invalid value.
 * chunk_size is the size in byte you should AT LEAST recycle.
 */
void KVStore::gc(uint64_t chunk_size)
{
}
