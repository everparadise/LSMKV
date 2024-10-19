#include <chrono>
#include "kvstore.h"
#include "zipf.hpp"
#include "config.hpp"
#include "fstream"

const uint64_t SIMPLE_TEST_MAX = 512;
const uint64_t LARGE_TEST_MAX = 1024 * 64;
const uint64_t GC_TEST_MAX = 1024 * 48;
const uint64_t MAX_KEY = 1024 * 1024;
class Timer
{
    std::chrono::high_resolution_clock::time_point start_time;

public:
    void start()
    {
        start_time = std::chrono::high_resolution_clock::now();
    }

    uint64_t end()
    {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = end_time - start_time;
        return duration.count();
        // auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>
    }
};

int main()
{
    KVStore store("./data", "./data/vlog");
    store.reset();
    Timer timer;

    uint64_t keys[LARGE_TEST_MAX];

    uint64_t putCost[LARGE_TEST_MAX];

    uint64_t sequencialGetCost[LARGE_TEST_MAX];
    uint64_t OutOfOrderGetCost[LARGE_TEST_MAX];
    uint64_t randomGetCost[LARGE_TEST_MAX];

    uint64_t delCost[LARGE_TEST_MAX];

    uint64_t scanCost[100];
    for (int i = 0; i < LARGE_TEST_MAX; i++)
    {
        keys[i] = zipf(0.5, MAX_KEY);
        timer.start();
        store.put(keys[i], "HI,THERE");
        putCost[i] = timer.end();

        timer.start();
        store.get(keys[i]);
        sequencialGetCost[i] = timer.end();
    }
    printf("finish put and sequencial get\n");
    for (int i = 1; i < LARGE_TEST_MAX; i++)
    {
        int index = rand() % i;
        uint64_t tmp = keys[index];
        keys[index] = keys[i];
        keys[i] = tmp;
    }
    printf("finish out of order init\n");
    std::list<KVPair> lists;
    for (int i = 0; i < 100; i++)
    {
        uint64_t minKey = rand() % MAX_KEY;
        timer.start();
        store.scan(minKey, minKey + MAX_KEY / 3, lists);
        scanCost[i] = timer.end();
    }
    printf("finish scan\n");

    for (int i = 0; i < LARGE_TEST_MAX; i++)
    {
        timer.start();
        store.get(keys[i]);
        OutOfOrderGetCost[i] = timer.end();
    }
    printf("finish out of order get\n");

    for (int i = 0; i < LARGE_TEST_MAX; i++)
    {
        uint64_t targetKey = zipf(0.5, MAX_KEY);
        timer.start();
        store.get(targetKey);
        randomGetCost[i] = timer.end();
    }
    printf("finish random get\n");

    for (int i = 0; i < LARGE_TEST_MAX; i++)
    {
        timer.start();
        store.del(i);
        delCost[i] = timer.end();
    }
    // printf("finish del\n");
    // printf("--------------start statistic output----------------\n");
    {

        // put
        std::ofstream of(Config::put);
        uint64_t totalLatency = 0;
        for (int i = 0; i < LARGE_TEST_MAX; i++)
        {
            of << i << "," << putCost[i] << "\n";
            totalLatency += putCost[i];
        }
        totalLatency /= LARGE_TEST_MAX;
        printf("operation put:\n");
        printf("avg latency: %ld ns\n", totalLatency);
        printf("avg throughput: %ld\n\n", 1000000000 / totalLatency);
        of.close();

        // scan
        of.open(Config::scan);
        totalLatency = 0;
        for (int i = 0; i < 100; i++)
        {
            of << i << "," << scanCost[i] << "\n";
            totalLatency += scanCost[i];
        }
        totalLatency /= LARGE_TEST_MAX;
        printf("operation scan\n");
        printf("avg latency: %ld ns\n", totalLatency);
        printf("avg throughput: %ld\n\n", 1000000000 / totalLatency);
        of.close();

        // sequencial get
        of.open(Config::sequencialGet);
        totalLatency = 0;
        for (int i = 0; i < LARGE_TEST_MAX; i++)
        {
            of << i << "," << sequencialGetCost[i] << "\n";
            totalLatency += sequencialGetCost[i];
        }
        totalLatency /= LARGE_TEST_MAX;
        printf("operation get\n");
        printf("avg latency: %ld ns\n", totalLatency);
        printf("avg throughput: %ld\n\n", 1000000000 / totalLatency);

        of.close();

        // out of order get
        of.open(Config::OutOfOrderGet);
        for (int i = 0; i < LARGE_TEST_MAX; i++)
        {
            of << i << "," << OutOfOrderGetCost[i] << "\n";
        }

        of.close();

        // random get
        totalLatency = 0;
        for (int i = 0; i < LARGE_TEST_MAX; i++)
        {
            of << i << "," << randomGetCost[i] << "\n";
            totalLatency += randomGetCost[i];
        }
        totalLatency /= LARGE_TEST_MAX;
        printf("operation random get\n");
        printf("avg latency: %ld ns\n", totalLatency);
        printf("avg throughput: %ld\n\n", 1000000000 / totalLatency);

        of.close();

        // del
        totalLatency = 0;
        of.open(Config::del);
        for (int i = 0; i < LARGE_TEST_MAX; i++)
        {
            of << i << "," << delCost[i] << "\n";
            totalLatency += delCost[i];
        }

        totalLatency /= LARGE_TEST_MAX;
        printf("operation del\n");
        printf("avg latency: %ld ns\n", totalLatency);
        printf("avg throughput: %ld\n\n", 1000000000 / totalLatency);
    }
    printf("finish static output\n");
}