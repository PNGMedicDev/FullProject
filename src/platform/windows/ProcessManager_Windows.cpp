#ifdef PLATFORM_WINDOWS

#include "platform/ProcessManager.h"
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <sstream>
#include <set>
#include <map>

#pragma comment(lib, "psapi.lib")

namespace DiagIDE {
namespace Platform {

class ProcessManager_Windows : public ProcessManager {
public:
    ProcessManager_Windows() = default;
    ~ProcessManager_Windows() override {
        // Detach from all processes
        for (auto pid : m_attachedProcesses) {
            DetachFromProcess(pid);
        }
    }

    std::vector<Core::ProcessInfo> EnumerateProcesses() override {
        std::vector<Core::ProcessInfo> processes;

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            return processes;
        }

        PROCESSENTRY32W pe32;
        pe32.dwSize = sizeof(PROCESSENTRY32W);

        if (Process32FirstW(snapshot, &pe32)) {
            do {
                Core::ProcessInfo info;
                if (GetProcessInfo(pe32.th32ProcessID, info)) {
                    processes.push_back(info);
                }
            } while (Process32NextW(snapshot, &pe32));
        }

        CloseHandle(snapshot);
        return processes;
    }

    bool GetProcessInfo(uint32_t pid, Core::ProcessInfo& info) override {
        info.pid = pid;

        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
        if (!hProcess) {
            return false;
        }

        // Get process name
        WCHAR exePath[MAX_PATH];
        if (GetModuleFileNameExW(hProcess, nullptr, exePath, MAX_PATH)) {
            // Convert wide string to narrow
            char narrowPath[MAX_PATH];
            WideCharToMultiByte(CP_UTF8, 0, exePath, -1, narrowPath, MAX_PATH, nullptr, nullptr);
            info.path = narrowPath;

            // Extract process name from path
            std::string pathStr = info.path;
            size_t lastSlash = pathStr.rfind('\\');
            if (lastSlash != std::string::npos) {
                info.name = pathStr.substr(lastSlash + 1);
            } else {
                info.name = pathStr;
            }
        }

        // Get memory info
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(hProcess, (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
            info.memoryUsage = pmc.WorkingSetSize;
        }

        // Get thread count
        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snapshot != INVALID_HANDLE_VALUE) {
            THREADENTRY32 te32;
            te32.dwSize = sizeof(THREADENTRY32);
            info.threadCount = 0;

            if (Thread32First(snapshot, &te32)) {
                do {
                    if (te32.th32OwnerProcessID == pid) {
                        info.threadCount++;
                    }
                } while (Thread32Next(snapshot, &te32));
            }

            CloseHandle(snapshot);
        }

        // CPU usage (simplified)
        info.cpuUsage = 0.0f;
        info.status = "Running";
        info.userName = "user"; // Would need to query token for real username

        CloseHandle(hProcess);
        return true;
    }

    bool AttachToProcess(uint32_t pid) override {
        if (m_attachedProcesses.find(pid) != m_attachedProcesses.end()) {
            return true; // Already attached
        }

        if (!CanAttachToProcess(pid)) {
            return false;
        }

        HANDLE hProcess = OpenProcess(
            PROCESS_ALL_ACCESS,
            FALSE,
            pid
        );

        if (!hProcess) {
            return false;
        }

        m_processHandles[pid] = hProcess;
        m_attachedProcesses.insert(pid);
        return true;
    }

    bool DetachFromProcess(uint32_t pid) override {
        auto it = m_processHandles.find(pid);
        if (it == m_processHandles.end()) {
            return false;
        }

        CloseHandle(it->second);
        m_processHandles.erase(it);
        m_attachedProcesses.erase(pid);
        return true;
    }

    bool IsAttached(uint32_t pid) const override {
        return m_attachedProcesses.find(pid) != m_attachedProcesses.end();
    }

    std::vector<Core::ThreadInfo> GetThreads(uint32_t pid) override {
        std::vector<Core::ThreadInfo> threads;

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
        if (snapshot == INVALID_HANDLE_VALUE) {
            return threads;
        }

        THREADENTRY32 te32;
        te32.dwSize = sizeof(THREADENTRY32);

        if (Thread32First(snapshot, &te32)) {
            do {
                if (te32.th32OwnerProcessID == pid) {
                    Core::ThreadInfo tinfo;
                    tinfo.threadId = te32.th32ThreadID;
                    tinfo.state = "Running";
                    tinfo.cpuUsage = 0.0f;
                    threads.push_back(tinfo);
                }
            } while (Thread32Next(snapshot, &te32));
        }

        CloseHandle(snapshot);
        return threads;
    }

    std::vector<Core::ModuleInfo> GetModules(uint32_t pid) override {
        std::vector<Core::ModuleInfo> modules;

        auto it = m_processHandles.find(pid);
        if (it == m_processHandles.end()) {
            return modules;
        }

        HMODULE hModules[1024];
        DWORD cbNeeded;

        if (EnumProcessModules(it->second, hModules, sizeof(hModules), &cbNeeded)) {
            for (unsigned int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++) {
                WCHAR szModName[MAX_PATH];
                if (GetModuleFileNameExW(it->second, hModules[i], szModName, MAX_PATH)) {
                    Core::ModuleInfo minfo;

                    char narrowPath[MAX_PATH];
                    WideCharToMultiByte(CP_UTF8, 0, szModName, -1, narrowPath, MAX_PATH, nullptr, nullptr);
                    minfo.path = narrowPath;

                    // Extract module name
                    std::string pathStr = minfo.path;
                    size_t lastSlash = pathStr.rfind('\\');
                    if (lastSlash != std::string::npos) {
                        minfo.name = pathStr.substr(lastSlash + 1);
                    } else {
                        minfo.name = pathStr;
                    }

                    minfo.baseAddress = reinterpret_cast<uintptr_t>(hModules[i]);

                    MODULEINFO modInfo;
                    if (GetModuleInformation(it->second, hModules[i], &modInfo, sizeof(modInfo))) {
                        minfo.size = modInfo.SizeOfImage;
                    }

                    modules.push_back(minfo);
                }
            }
        }

        return modules;
    }

    std::vector<Core::MemoryRegion> GetMemoryRegions(uint32_t pid) override {
        std::vector<Core::MemoryRegion> regions;

        auto it = m_processHandles.find(pid);
        if (it == m_processHandles.end()) {
            return regions;
        }

        MEMORY_BASIC_INFORMATION mbi;
        uintptr_t address = 0;

        while (VirtualQueryEx(it->second, reinterpret_cast<LPCVOID>(address), &mbi, sizeof(mbi)) == sizeof(mbi)) {
            if (mbi.State == MEM_COMMIT) {
                Core::MemoryRegion region;
                region.baseAddress = reinterpret_cast<uintptr_t>(mbi.BaseAddress);
                region.size = mbi.RegionSize;
                region.protection = mbi.Protect;

                region.readable = (mbi.Protect & (PAGE_READONLY | PAGE_READWRITE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE)) != 0;
                region.writable = (mbi.Protect & (PAGE_READWRITE | PAGE_WRITECOPY | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0;
                region.executable = (mbi.Protect & (PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY)) != 0;

                region.type = "Unknown";
                if (mbi.Type == MEM_IMAGE) region.type = "Image";
                else if (mbi.Type == MEM_MAPPED) region.type = "Mapped";
                else if (mbi.Type == MEM_PRIVATE) region.type = "Private";

                regions.push_back(region);
            }

            address = reinterpret_cast<uintptr_t>(mbi.BaseAddress) + mbi.RegionSize;
        }

        return regions;
    }

    bool ReadMemory(uint32_t pid, uintptr_t address, void* buffer, size_t size) override {
        auto it = m_processHandles.find(pid);
        if (it == m_processHandles.end()) {
            return false;
        }

        SIZE_T bytesRead;
        return ReadProcessMemory(
            it->second,
            reinterpret_cast<LPCVOID>(address),
            buffer,
            size,
            &bytesRead
        ) && bytesRead == size;
    }

    bool WriteMemory(uint32_t pid, uintptr_t address, const void* buffer, size_t size) override {
        auto it = m_processHandles.find(pid);
        if (it == m_processHandles.end()) {
            return false;
        }

        SIZE_T bytesWritten;
        return WriteProcessMemory(
            it->second,
            reinterpret_cast<LPVOID>(address),
            buffer,
            size,
            &bytesWritten
        ) && bytesWritten == size;
    }

    bool CanAttachToProcess(uint32_t pid) override {
        HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
        if (!hProcess) {
            return false;
        }

        CloseHandle(hProcess);
        return true;
    }

    bool RequiresElevation(uint32_t pid) override {
        // Check if we need admin rights
        HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
        if (!hProcess) {
            // If we can't open with all access, we might need elevation
            return GetLastError() == ERROR_ACCESS_DENIED;
        }

        CloseHandle(hProcess);
        return false;
    }

private:
    std::set<uint32_t> m_attachedProcesses;
    std::map<uint32_t, HANDLE> m_processHandles;
};

std::unique_ptr<ProcessManager> ProcessManager::Create() {
    return std::make_unique<ProcessManager_Windows>();
}

} // namespace Platform
} // namespace DiagIDE

#endif // PLATFORM_WINDOWS
