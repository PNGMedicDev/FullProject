#ifdef PLATFORM_WINDOWS

#include "core/SystemMetrics.h"
#include <windows.h>
#include <pdh.h>
#include <thread>

#pragma comment(lib, "pdh.lib")

namespace DiagIDE {
namespace Core {

class SystemMetrics_Windows : public SystemMetrics {
public:
    SystemMetrics_Windows() {
        Update();
    }

    ~SystemMetrics_Windows() override = default;

    CPUMetrics GetCPUMetrics() override {
        return m_cpuMetrics;
    }

    MemoryMetrics GetMemoryMetrics() override {
        return m_memoryMetrics;
    }

    DiskIOMetrics GetDiskIOMetrics() override {
        return m_diskIOMetrics;
    }

    NetworkMetrics GetNetworkMetrics() override {
        return m_networkMetrics;
    }

    void Update() override {
        UpdateCPU();
        UpdateMemory();
        UpdateDiskIO();
        UpdateNetwork();
    }

private:
    void UpdateCPU() {
        FILETIME idleTime, kernelTime, userTime;
        if (!GetSystemTimes(&idleTime, &kernelTime, &userTime)) {
            return;
        }

        auto FileTimeToULL = [](const FILETIME& ft) -> unsigned long long {
            return (static_cast<unsigned long long>(ft.dwHighDateTime) << 32) | ft.dwLowDateTime;
        };

        unsigned long long idle = FileTimeToULL(idleTime);
        unsigned long long kernel = FileTimeToULL(kernelTime);
        unsigned long long user = FileTimeToULL(userTime);

        unsigned long long total = kernel + user;

        unsigned long long idleDelta = idle - m_prevIdleTime;
        unsigned long long totalDelta = total - m_prevTotalTime;

        if (totalDelta > 0) {
            m_cpuMetrics.totalUsage = 100.0f * (1.0f - static_cast<float>(idleDelta) / totalDelta);
        }

        m_cpuMetrics.coreCount = std::thread::hardware_concurrency();

        m_prevIdleTime = idle;
        m_prevTotalTime = total;
    }

    void UpdateMemory() {
        MEMORYSTATUSEX memStatus;
        memStatus.dwLength = sizeof(memStatus);

        if (!GlobalMemoryStatusEx(&memStatus)) {
            return;
        }

        m_memoryMetrics.totalPhysical = memStatus.ullTotalPhys;
        m_memoryMetrics.availablePhysical = memStatus.ullAvailPhys;
        m_memoryMetrics.usedPhysical = m_memoryMetrics.totalPhysical - m_memoryMetrics.availablePhysical;
        m_memoryMetrics.usagePercent = static_cast<float>(memStatus.dwMemoryLoad);

        m_memoryMetrics.totalVirtual = memStatus.ullTotalVirtual;
        m_memoryMetrics.availableVirtual = memStatus.ullAvailVirtual;
        m_memoryMetrics.usedVirtual = m_memoryMetrics.totalVirtual - m_memoryMetrics.availableVirtual;
    }

    void UpdateDiskIO() {
        // Simplified - would use PDH counters for real implementation
        m_diskIOMetrics.readBytesPerSec = 0;
        m_diskIOMetrics.writeBytesPerSec = 0;
        m_diskIOMetrics.readOpsPerSec = 0;
        m_diskIOMetrics.writeOpsPerSec = 0;
    }

    void UpdateNetwork() {
        // Simplified - would use PDH counters or GetIfTable2 for real implementation
        m_networkMetrics.bytesSentPerSec = 0;
        m_networkMetrics.bytesRecvPerSec = 0;
        m_networkMetrics.packetsSentPerSec = 0;
        m_networkMetrics.packetsRecvPerSec = 0;
    }

private:
    CPUMetrics m_cpuMetrics{};
    MemoryMetrics m_memoryMetrics{};
    DiskIOMetrics m_diskIOMetrics{};
    NetworkMetrics m_networkMetrics{};

    unsigned long long m_prevIdleTime{};
    unsigned long long m_prevTotalTime{};
};

std::unique_ptr<SystemMetrics> SystemMetrics::Create() {
    return std::make_unique<SystemMetrics_Windows>();
}

} // namespace Core
} // namespace DiagIDE

#endif // PLATFORM_WINDOWS
