#pragma once
#include <string>
#include <thread>
#include <mutex>
#include "config.hpp"
#include "template.hpp"
#include <sstream>
#include <fstream>
#include <iostream>
class logger
{
    std::string path;
    const char *filePath;
    std::fstream file;
    std::mutex mtx;
    static logger *logInstance;

    logger(const std::string &logPath)
    {
        path = logPath;
        file.open(path, std::ios::in | std::ios::out);
    }

    ~logger()
    {
        file.close();
        delete logger::logInstance;
        logger::logInstance = nullptr;
    }

public:
    logger(logger &) = delete;
    logger &operator=(logger &) = delete;

    static logger *getInstance()
    {
        if (!logInstance)
        {
            logInstance = new logger(Config::logPath);
        }
        return logInstance;
    }

    void flush()
    {
        file.close();
        file.open(path, std::ios::in | std::ios::out | std::ios::trunc);
    }

    void log(std::string &&message)
    {
        if (!file.is_open())
        {
            std::cerr << "Error opening file\n"
                      << std::endl;
            exit(0);
        }

        file << message << std::endl;
    }

    std::vector<KVPair> reflog()
    {

        std::vector<KVPair> tuples;
        while (!file.eof())
        {
            uint64_t key;
            std::string value;
            file >> key;
            file >> value;

            if (value.empty())
                break;
            tuples.emplace_back(std::make_pair(key, std::move(value)));
        }

        flush();
        return tuples;
    }
};