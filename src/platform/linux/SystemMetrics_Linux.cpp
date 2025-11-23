#include "core/SystemMetrics.h"
#include <fstream>
#include <sstream>
#include <thread>
#include <unistd.h>

namespace DiagIDE {
namespace Core {

class SystemMetrics_Linux : public SystemMetrics {
public:
    SystemMetrics_Linux()
        : m_prevIdleTime(0)
        , m_prevTotalTime(0)
    {
        Update();
    }

    ~SystemMetrics_Linux() override = default;

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
        std::ifstream statFile("/proc/stat");
        if (!statFile.is_open()) {
            return;
        }

        std::string line;
        std::getline(statFile, line);
        statFile.close();

        if (line.substr(0, 3) != "cpu") {
            return;
        }

        std::istringstream iss(line);
        std::string cpu;
        unsigned long long user, nice, system, idle, iowait, irq, softirq;

        iss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq;

        unsigned long long idleTime = idle + iowait;
        unsigned long long totalTime = user + nice + system + idle + iowait + irq + softirq;

        unsigned long long idleDelta = idleTime - m_prevIdleTime;
        unsigned long long totalDelta = totalTime - m_prevTotalTime;

        if (totalDelta > 0) {
            m_cpuMetrics.totalUsage = 100.0f * (1.0f - static_cast<float>(idleDelta) / totalDelta);
            m_cpuMetrics.userTime = 100.0f * static_cast<float>(user - m_prevUserTime) / totalDelta;
            m_cpuMetrics.systemTime = 100.0f * static_cast<float>(system - m_prevSystemTime) / totalDelta;
        }

        m_cpuMetrics.coreCount = std::thread::hardware_concurrency();

        m_prevIdleTime = idleTime;
        m_prevTotalTime = totalTime;
        m_prevUserTime = user;
        m_prevSystemTime = system;
    }

    void UpdateMemory() {
        std::ifstream meminfoFile("/proc/meminfo");
        if (!meminfoFile.is_open()) {
            return;
        }

        std::string line;
        while (std::getline(meminfoFile, line)) {
            std::istringstream iss(line);
            std::string key;
            uint64_t value;

            iss >> key >> value;

            if (key == "MemTotal:") {
                m_memoryMetrics.totalPhysical = value * 1024; // Convert KB to bytes
            } else if (key == "MemAvailable:") {
                m_memoryMetrics.availablePhysical = value * 1024;
            }
        }

        meminfoFile.close();

        m_memoryMetrics.usedPhysical = m_memoryMetrics.totalPhysical - m_memoryMetrics.availablePhysical;
        m_memoryMetrics.usagePercent = 100.0f * static_cast<float>(m_memoryMetrics.usedPhysical) /
                                       m_memoryMetrics.totalPhysical;

        // Virtual memory (simplified)
        m_memoryMetrics.totalVirtual = m_memoryMetrics.totalPhysical * 2;
        m_memoryMetrics.availableVirtual = m_memoryMetrics.availablePhysical * 2;
        m_memoryMetrics.usedVirtual = m_memoryMetrics.usedPhysical;
    }

    void UpdateDiskIO() {
        std::ifstream diskstatsFile("/proc/diskstats");
        if (!diskstatsFile.is_open()) {
            m_diskIOMetrics.readBytesPerSec = 0;
            m_diskIOMetrics.writeBytesPerSec = 0;
            m_diskIOMetrics.readOpsPerSec = 0;
            m_diskIOMetrics.writeOpsPerSec = 0;
            return;
        }

        // This is a simplified implementation
        // In a real scenario, we'd accumulate stats for all disks and calculate delta
        m_diskIOMetrics.readBytesPerSec = 0;
        m_diskIOMetrics.writeBytesPerSec = 0;
        m_diskIOMetrics.readOpsPerSec = 0;
        m_diskIOMetrics.writeOpsPerSec = 0;

        diskstatsFile.close();
    }

    void UpdateNetwork() {
        std::ifstream netFile("/proc/net/dev");
        if (!netFile.is_open()) {
            m_networkMetrics.bytesSentPerSec = 0;
            m_networkMetrics.bytesRecvPerSec = 0;
            m_networkMetrics.packetsSentPerSec = 0;
            m_networkMetrics.packetsRecvPerSec = 0;
            return;
        }

        // Skip header lines
        std::string line;
        std::getline(netFile, line);
        std::getline(netFile, line);

        uint64_t totalRecvBytes = 0;
        uint64_t totalSentBytes = 0;
        uint64_t totalRecvPackets = 0;
        uint64_t totalSentPackets = 0;

        while (std::getline(netFile, line)) {
            std::istringstream iss(line);
            std::string iface;
            uint64_t recvBytes, recvPackets, sentBytes, sentPackets;
            uint64_t dummy;

            iss >> iface >> recvBytes >> recvPackets;
            for (int i = 0; i < 6; ++i) iss >> dummy;
            iss >> sentBytes >> sentPackets;

            // Skip loopback
            if (iface.find("lo") != std::string::npos) {
                continue;
            }

            totalRecvBytes += recvBytes;
            totalSentBytes += sentBytes;
            totalRecvPackets += recvPackets;
            totalSentPackets += sentPackets;
        }

        netFile.close();

        // Calculate rate (simplified - would need time delta for accuracy)
        m_networkMetrics.bytesRecvPerSec = totalRecvBytes - m_prevRecvBytes;
        m_networkMetrics.bytesSentPerSec = totalSentBytes - m_prevSentBytes;
        m_networkMetrics.packetsRecvPerSec = totalRecvPackets - m_prevRecvPackets;
        m_networkMetrics.packetsSentPerSec = totalSentPackets - m_prevSentPackets;

        m_prevRecvBytes = totalRecvBytes;
        m_prevSentBytes = totalSentBytes;
        m_prevRecvPackets = totalRecvPackets;
        m_prevSentPackets = totalSentPackets;
    }

private:
    CPUMetrics m_cpuMetrics{};
    MemoryMetrics m_memoryMetrics{};
    DiskIOMetrics m_diskIOMetrics{};
    NetworkMetrics m_networkMetrics{};

    // Previous values for delta calculation
    unsigned long long m_prevIdleTime;
    unsigned long long m_prevTotalTime;
    unsigned long long m_prevUserTime{};
    unsigned long long m_prevSystemTime{};

    uint64_t m_prevRecvBytes{};
    uint64_t m_prevSentBytes{};
    uint64_t m_prevRecvPackets{};
    uint64_t m_prevSentPackets{};
};

std::unique_ptr<SystemMetrics> SystemMetrics::Create() {
    return std::make_unique<SystemMetrics_Linux>();
}

} // namespace Core
} // namespace DiagIDE
