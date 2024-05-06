// define.h
#ifndef DEFINE_H
#define DEFINE_H

#define SSTMAX 408

// prob of memtab skiplist level up
#define PROB 0.25

// bloomfilter arguement
#define HASHNUM 8
#define FILTERSIZE 8192

// sstable tuple start
#define KEYSTART 8192 + 32
#define BLOOMSTART 32
#define BLOOMSIZE 8192
#define TUPLESIZE 20
#define BLOOMBIT BLOOMSIZE * 8
#define BITREADER 1

#define MAGIC 0xFF
#define MAXSTAMP __UINT64_MAX__
#define TAILREADER 8192

#endif