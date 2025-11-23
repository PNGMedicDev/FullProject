#pragma once

#include <string>
#include <cstdint>
#include <vector>

namespace DiagIDE {
namespace Core {

struct ThreadInfo {
    uint32_t threadId;
    std::string state;
    float cpuUsage;
};

struct ModuleInfo {
    std::string name;
    std::string path;
    uintptr_t baseAddress;
    size_t size;
};

struct MemoryRegion {
    uintptr_t baseAddress;
    size_t size;
    uint32_t protection;
    std::string type;
    bool readable;
    bool writable;
    bool executable;
};

struct ProcessInfo {
    uint32_t pid;
    std::string name;
    std::string path;
    std::string userName;
    float cpuUsage;
    uint64_t memoryUsage;  // in bytes
    uint32_t threadCount;
    std::string status;

    // Extended info (loaded on demand)
    std::vector<ThreadInfo> threads;
    std::vector<ModuleInfo> modules;
    std::vector<MemoryRegion> memoryRegions;
};

} // namespace Core
} // namespace DiagIDE
