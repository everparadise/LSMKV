# LSMKV
**LSM Tree**(Log-Structured Merge Tree) is a kind of Key-Value Storage Structure designed to handle a high volumn of write operations efficiently. It was introduced in 1996 by Patrick O'Neil and others in a seminal paper.

Today, this data structure is widely used in DBSM. Google’s LevelDB and Facebook’s RocksDB are both built around the LSM Tree as their core data structure. 

In 2016, a paper by Lanyue Lu and colleagues, titled Wisckey, proposed an optimization for the LSM Tree, addressing the long-standing issue of write amplification through a technique called **key-value separation**. 

In recent years, several LSM Tree storage engines utilizing key-value separation have emerged in the industry, such as TerarkDB and Titan.
## Table of Contents
- [Features](#features)
- [Build](#build)
- [Future](#future)
- [Performance](#performance)
- [Apology](#apology)

## Features
- **Efficient Write Performance**
  LSM Trees are optimized for high write throughput. By **batching** writes into memory(in-memory buffers skiplist) and periodically flushed into disk(key in SSTable, value in vLog), LSM Trees avoid frequently I/O Operation which is quite expensive.
  The implementation of **Key-value Separation** strategy reduced the problems of write amplification, further improving write performance
  What's more, LSM Tree perform **sequential writes** when flushing data to disk instead of random access, which is much faster on disks.
- **High Read Performance(with Caching)**
  In this project, LSM Trees are optimized by **caching** different parts of SSTable in memory, bringing orders-of magnitude enhancement of read operations. 
- **Peridic Garbage Collection**
  Periodic garbage collection is employed to remove obsolete data, reducing storage space usage and maintaining storage efficiency
- **Enhanced Configurability**
  To ensure that LSM Tree engine can adapt to various use cases and workloads, many core components and functionalities are exposed as configurable parameters. This design allows users to adjust system behavior based on specific requirement, providing **flexibility** to accomodate
  - Cache method: (1) cache nothing (2) cache bloom filter (3) cache entries (4) Both caching
  - bloom filter arguments: (1) hash number (2) filter size
  - skiplist arguments (1) probabiility
- **Redo Log maintain Duration**
  Because of the vulnerability of RAM, the project utilizes a redo log to ensure data durability in case of system crashes and unexpected shutdowns. When restart the engine, it scan the disk to reconstruct system and redo the operation in redo log.

## Build
``` bash
# please adopt release mode when using
cmake -B build -DCMAKE_BUILD_TYPE=Release
# adopt debug mode when debug
cmake -B build -DCMAKE_BUILD_TYPE=Debug
```

release mode adopt O3 level compiler optimization, which provides better performance.

Debug mode integrated Address Sanitizer to detect memory leakage, adopt no compiler optimization to keep logic as imagined, enables full error/warning output. Those configurations lead to worse performance.

## Future
- [] adopt a thread pool to concurrently flush data to memory

## Performance
source file is **src/benchmark/performance.cc**

### Latency
| workload size | Put | Get | Scan | Del |
| --- | --- | --- | --- | --- |
| 1024*16 | 16039ns | 7117 ns | 15434 ns | 7805ns |
| 1024*64 | 122108ns | 7163 ns | 18432ns | 11180ns |
| 1024*256 | 131338ns | 6922ns | 19373ns | 13585ns |

### Throughput
| size | Put | Get | Scan | Del |
| --- | --- | --- | --- | --- |
| 1024 * 16 | 63759 | 149857 | 60960 | 128353 |
| 1024 * 64 | 7909 | 142106 | 48911 | 88276 |
| 1024 * 256 | 5603 | 140016 | 50730 | 67024 |

### Performance of different caching method
<p align="center">
<img src="https://github.com/user-attachments/assets/7e1efa08-4296-4d6d-b6e2-113cc027ea76" width=100% height=100% 
class="center">
</p>

### Performance of different bloom filter size
<p align="center">
<img src="https://github.com/user-attachments/assets/01984777-672a-4458-99c8-419db59756aa" width=100% height=100% 
class="center">
</p>


## apology
During the development, I changed several code styles, which makes the structure a little dirty. Sorry for that, and best wishes for you!
