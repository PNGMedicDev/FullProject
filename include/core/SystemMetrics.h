#pragma once

#include <cstdint>
#include <memory>

namespace DiagIDE {
namespace Core {

struct CPUMetrics {
    float totalUsage;      // 0-100%
    float userTime;        // 0-100%
    float systemTime;      // 0-100%
    uint32_t coreCount;
};

struct MemoryMetrics {
    uint64_t totalPhysical;
    uint64_t availablePhysical;
    uint64_t usedPhysical;
    float usagePercent;

    uint64_t totalVirtual;
    uint64_t availableVirtual;
    uint64_t usedVirtual;
};

struct DiskIOMetrics {
    uint64_t readBytesPerSec;
    uint64_t writeBytesPerSec;
    uint64_t readOpsPerSec;
    uint64_t writeOpsPerSec;
};

struct NetworkMetrics {
    uint64_t bytesSentPerSec;
    uint64_t bytesRecvPerSec;
    uint64_t packetsSentPerSec;
    uint64_t packetsRecvPerSec;
};

// Platform-specific system metrics collector
class SystemMetrics {
public:
    static std::unique_ptr<SystemMetrics> Create();
    virtual ~SystemMetrics() = default;

    virtual CPUMetrics GetCPUMetrics() = 0;
    virtual MemoryMetrics GetMemoryMetrics() = 0;
    virtual DiskIOMetrics GetDiskIOMetrics() = 0;
    virtual NetworkMetrics GetNetworkMetrics() = 0;

    virtual void Update() = 0;

protected:
    SystemMetrics() = default;
};

} // namespace Core
} // namespace DiagIDE
