#pragma once

#include <string>
#include <vector>
#include <fstream>
#include <mutex>
#include <cstdint>

namespace DiagIDE {
namespace Core {

enum class AuditLevel {
    Info,
    Warning,
    Critical,
    Security
};

struct AuditEntry {
    std::string timestamp;
    AuditLevel level;
    std::string category;
    std::string message;
    std::string user;
    uint32_t processId;
};

class AuditLogger {
public:
    AuditLogger(const std::string& logFilePath);
    ~AuditLogger();

    // Logging methods
    void Log(AuditLevel level, const std::string& category, const std::string& message);
    void LogProcessAttach(uint32_t pid, const std::string& processName, bool success);
    void LogMemoryRead(uint32_t pid, uintptr_t address, size_t size, bool success);
    void LogMemoryWrite(uint32_t pid, uintptr_t address, size_t size, bool success);
    void LogExport(const std::string& exportType, const std::string& filepath);

    // Retrieval
    std::vector<AuditEntry> GetRecentEntries(size_t count = 100);
    std::vector<AuditEntry> GetEntriesByLevel(AuditLevel level);

    // Export
    bool ExportToJson(const std::string& filepath);
    bool ExportToHtml(const std::string& filepath);

private:
    void WriteEntry(const AuditEntry& entry);
    std::string GetCurrentTimestamp();
    std::string GetCurrentUser();

private:
    std::string m_logFilePath;
    std::ofstream m_logFile;
    std::vector<AuditEntry> m_entries;
    std::mutex m_mutex;
};

} // namespace Core
} // namespace DiagIDE
