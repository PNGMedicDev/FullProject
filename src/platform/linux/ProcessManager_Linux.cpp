#include "platform/ProcessManager.h"
#include <fstream>
#include <sstream>
#include <cstring>
#include <algorithm>
#include <set>

#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <signal.h>
#include <errno.h>

namespace DiagIDE {
namespace Platform {

class ProcessManager_Linux : public ProcessManager {
public:
    ProcessManager_Linux() = default;
    ~ProcessManager_Linux() override = default;

    std::vector<Core::ProcessInfo> EnumerateProcesses() override {
        std::vector<Core::ProcessInfo> processes;

        DIR* dir = opendir("/proc");
        if (!dir) {
            return processes;
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            // Check if directory name is a number (PID)
            if (entry->d_type == DT_DIR) {
                std::string dirname = entry->d_name;
                if (std::all_of(dirname.begin(), dirname.end(), ::isdigit)) {
                    uint32_t pid = std::stoul(dirname);
                    Core::ProcessInfo info;
                    if (GetProcessInfo(pid, info)) {
                        processes.push_back(info);
                    }
                }
            }
        }

        closedir(dir);
        return processes;
    }

    bool GetProcessInfo(uint32_t pid, Core::ProcessInfo& info) override {
        info.pid = pid;

        // Read /proc/[pid]/stat
        std::string statPath = "/proc/" + std::to_string(pid) + "/stat";
        std::ifstream statFile(statPath);
        if (!statFile.is_open()) {
            return false;
        }

        std::string line;
        std::getline(statFile, line);
        statFile.close();

        // Parse stat file
        size_t nameStart = line.find('(');
        size_t nameEnd = line.rfind(')');
        if (nameStart == std::string::npos || nameEnd == std::string::npos) {
            return false;
        }

        info.name = line.substr(nameStart + 1, nameEnd - nameStart - 1);

        // Read cmdline for full path
        std::string cmdlinePath = "/proc/" + std::to_string(pid) + "/cmdline";
        std::ifstream cmdlineFile(cmdlinePath);
        if (cmdlineFile.is_open()) {
            std::getline(cmdlineFile, info.path, '\0');
            cmdlineFile.close();
        }

        // Read status for more details
        std::string statusPath = "/proc/" + std::to_string(pid) + "/status";
        std::ifstream statusFile(statusPath);
        if (statusFile.is_open()) {
            std::string statusLine;
            while (std::getline(statusFile, statusLine)) {
                if (statusLine.find("Uid:") == 0) {
                    // Extract username (simplified)
                    info.userName = "user";
                } else if (statusLine.find("Threads:") == 0) {
                    std::istringstream iss(statusLine.substr(8));
                    iss >> info.threadCount;
                } else if (statusLine.find("VmRSS:") == 0) {
                    std::istringstream iss(statusLine.substr(6));
                    uint64_t kbytes;
                    iss >> kbytes;
                    info.memoryUsage = kbytes * 1024; // Convert to bytes
                }
            }
            statusFile.close();
        }

        // CPU usage (simplified - would need historical data for accurate calculation)
        info.cpuUsage = 0.0f;
        info.status = "Running";

        return true;
    }

    bool AttachToProcess(uint32_t pid) override {
        if (m_attachedProcesses.find(pid) != m_attachedProcesses.end()) {
            return true; // Already attached
        }

        // Check if we have permission
        if (!CanAttachToProcess(pid)) {
            return false;
        }

        // Use ptrace to attach
        if (ptrace(PTRACE_ATTACH, pid, nullptr, nullptr) == -1) {
            return false;
        }

        // Wait for process to stop
        int status;
        if (waitpid(pid, &status, 0) == -1) {
            ptrace(PTRACE_DETACH, pid, nullptr, nullptr);
            return false;
        }

        m_attachedProcesses.insert(pid);
        return true;
    }

    bool DetachFromProcess(uint32_t pid) override {
        if (m_attachedProcesses.find(pid) == m_attachedProcesses.end()) {
            return false;
        }

        if (ptrace(PTRACE_DETACH, pid, nullptr, nullptr) == -1) {
            return false;
        }

        m_attachedProcesses.erase(pid);
        return true;
    }

    bool IsAttached(uint32_t pid) const override {
        return m_attachedProcesses.find(pid) != m_attachedProcesses.end();
    }

    std::vector<Core::ThreadInfo> GetThreads(uint32_t pid) override {
        std::vector<Core::ThreadInfo> threads;

        std::string taskPath = "/proc/" + std::to_string(pid) + "/task";
        DIR* dir = opendir(taskPath.c_str());
        if (!dir) {
            return threads;
        }

        struct dirent* entry;
        while ((entry = readdir(dir)) != nullptr) {
            if (entry->d_type == DT_DIR) {
                std::string dirname = entry->d_name;
                if (std::all_of(dirname.begin(), dirname.end(), ::isdigit)) {
                    Core::ThreadInfo tinfo;
                    tinfo.threadId = std::stoul(dirname);
                    tinfo.state = "Running";
                    tinfo.cpuUsage = 0.0f;
                    threads.push_back(tinfo);
                }
            }
        }

        closedir(dir);
        return threads;
    }

    std::vector<Core::ModuleInfo> GetModules(uint32_t pid) override {
        std::vector<Core::ModuleInfo> modules;

        std::string mapsPath = "/proc/" + std::to_string(pid) + "/maps";
        std::ifstream mapsFile(mapsPath);
        if (!mapsFile.is_open()) {
            return modules;
        }

        std::string line;
        while (std::getline(mapsFile, line)) {
            // Parse maps format: address perms offset dev inode pathname
            std::istringstream iss(line);
            std::string addressRange, perms, offset, dev, inode, pathname;

            iss >> addressRange >> perms >> offset >> dev >> inode;
            std::getline(iss, pathname);

            // Skip anonymous mappings
            if (pathname.empty() || pathname[0] != ' ') {
                continue;
            }

            pathname = pathname.substr(1); // Remove leading space

            // Only add executable modules
            if (perms.find('x') != std::string::npos) {
                Core::ModuleInfo minfo;
                minfo.path = pathname;

                // Extract base address
                size_t dashPos = addressRange.find('-');
                if (dashPos != std::string::npos) {
                    std::string baseAddrStr = addressRange.substr(0, dashPos);
                    minfo.baseAddress = std::stoull(baseAddrStr, nullptr, 16);
                }

                // Extract module name
                size_t lastSlash = pathname.rfind('/');
                if (lastSlash != std::string::npos) {
                    minfo.name = pathname.substr(lastSlash + 1);
                } else {
                    minfo.name = pathname;
                }

                minfo.size = 0; // Would need to calculate from maps

                // Avoid duplicates
                bool found = false;
                for (const auto& existing : modules) {
                    if (existing.path == minfo.path) {
                        found = true;
                        break;
                    }
                }

                if (!found) {
                    modules.push_back(minfo);
                }
            }
        }

        mapsFile.close();
        return modules;
    }

    std::vector<Core::MemoryRegion> GetMemoryRegions(uint32_t pid) override {
        std::vector<Core::MemoryRegion> regions;

        std::string mapsPath = "/proc/" + std::to_string(pid) + "/maps";
        std::ifstream mapsFile(mapsPath);
        if (!mapsFile.is_open()) {
            return regions;
        }

        std::string line;
        while (std::getline(mapsFile, line)) {
            std::istringstream iss(line);
            std::string addressRange, perms;

            iss >> addressRange >> perms;

            Core::MemoryRegion region;

            // Parse address range
            size_t dashPos = addressRange.find('-');
            if (dashPos != std::string::npos) {
                uintptr_t start = std::stoull(addressRange.substr(0, dashPos), nullptr, 16);
                uintptr_t end = std::stoull(addressRange.substr(dashPos + 1), nullptr, 16);

                region.baseAddress = start;
                region.size = end - start;
            }

            // Parse permissions
            region.readable = (perms[0] == 'r');
            region.writable = (perms[1] == 'w');
            region.executable = (perms[2] == 'x');

            region.protection = 0;
            if (region.readable) region.protection |= 0x1;
            if (region.writable) region.protection |= 0x2;
            if (region.executable) region.protection |= 0x4;

            region.type = "Unknown";

            regions.push_back(region);
        }

        mapsFile.close();
        return regions;
    }

    bool ReadMemory(uint32_t pid, uintptr_t address, void* buffer, size_t size) override {
        if (!IsAttached(pid)) {
            return false;
        }

        std::string memPath = "/proc/" + std::to_string(pid) + "/mem";
        std::ifstream memFile(memPath, std::ios::binary);
        if (!memFile.is_open()) {
            return false;
        }

        memFile.seekg(address);
        memFile.read(static_cast<char*>(buffer), size);

        bool success = memFile.gcount() == static_cast<std::streamsize>(size);
        memFile.close();

        return success;
    }

    bool WriteMemory(uint32_t pid, uintptr_t address, const void* buffer, size_t size) override {
        if (!IsAttached(pid)) {
            return false;
        }

        std::string memPath = "/proc/" + std::to_string(pid) + "/mem";
        std::fstream memFile(memPath, std::ios::binary | std::ios::in | std::ios::out);
        if (!memFile.is_open()) {
            return false;
        }

        memFile.seekp(address);
        memFile.write(static_cast<const char*>(buffer), size);

        bool success = memFile.good();
        memFile.close();

        return success;
    }

    bool CanAttachToProcess(uint32_t pid) override {
        // Check if process exists
        std::string procPath = "/proc/" + std::to_string(pid);
        struct stat st;
        if (stat(procPath.c_str(), &st) != 0) {
            return false;
        }

        // Check if we're the same user or root
        uid_t myUid = geteuid();
        return (myUid == 0 || myUid == st.st_uid);
    }

    bool RequiresElevation(uint32_t pid) override {
        std::string procPath = "/proc/" + std::to_string(pid);
        struct stat st;
        if (stat(procPath.c_str(), &st) != 0) {
            return true;
        }

        uid_t myUid = geteuid();
        return (myUid != 0 && myUid != st.st_uid);
    }

private:
    std::set<uint32_t> m_attachedProcesses;
};

std::unique_ptr<ProcessManager> ProcessManager::Create() {
    return std::make_unique<ProcessManager_Linux>();
}

} // namespace Platform
} // namespace DiagIDE
