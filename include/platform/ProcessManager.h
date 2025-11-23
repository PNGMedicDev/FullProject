#pragma once

#include "core/ProcessInfo.h"
#include <vector>
#include <memory>
#include <cstdint>

namespace DiagIDE {
namespace Platform {

// Platform abstraction for process operations
class ProcessManager {
public:
    static std::unique_ptr<ProcessManager> Create();
    virtual ~ProcessManager() = default;

    // Process enumeration
    virtual std::vector<Core::ProcessInfo> EnumerateProcesses() = 0;
    virtual bool GetProcessInfo(uint32_t pid, Core::ProcessInfo& info) = 0;

    // Process inspection (requires authorization)
    virtual bool AttachToProcess(uint32_t pid) = 0;
    virtual bool DetachFromProcess(uint32_t pid) = 0;
    virtual bool IsAttached(uint32_t pid) const = 0;

    // Thread/Module enumeration (for attached processes)
    virtual std::vector<Core::ThreadInfo> GetThreads(uint32_t pid) = 0;
    virtual std::vector<Core::ModuleInfo> GetModules(uint32_t pid) = 0;
    virtual std::vector<Core::MemoryRegion> GetMemoryRegions(uint32_t pid) = 0;

    // Memory operations (requires attachment and user consent)
    virtual bool ReadMemory(uint32_t pid, uintptr_t address, void* buffer, size_t size) = 0;
    virtual bool WriteMemory(uint32_t pid, uintptr_t address, const void* buffer, size_t size) = 0;

    // Permission checking
    virtual bool CanAttachToProcess(uint32_t pid) = 0;
    virtual bool RequiresElevation(uint32_t pid) = 0;

protected:
    ProcessManager() = default;
};

} // namespace Platform
} // namespace DiagIDE
