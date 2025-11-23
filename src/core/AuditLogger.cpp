#include "core/AuditLogger.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#include <lmcons.h>
#else
#include <unistd.h>
#include <pwd.h>
#endif

using json = nlohmann::json;

namespace DiagIDE {
namespace Core {

AuditLogger::AuditLogger(const std::string& logFilePath)
    : m_logFilePath(logFilePath)
{
    m_logFile.open(m_logFilePath, std::ios::app);
    if (!m_logFile.is_open()) {
        throw std::runtime_error("Failed to open audit log file: " + m_logFilePath);
    }
}

AuditLogger::~AuditLogger() {
    if (m_logFile.is_open()) {
        m_logFile.close();
    }
}

void AuditLogger::Log(AuditLevel level, const std::string& category, const std::string& message) {
    std::lock_guard<std::mutex> lock(m_mutex);

    AuditEntry entry;
    entry.timestamp = GetCurrentTimestamp();
    entry.level = level;
    entry.category = category;
    entry.message = message;
    entry.user = GetCurrentUser();
    entry.processId = 0;

    m_entries.push_back(entry);
    WriteEntry(entry);
}

void AuditLogger::LogProcessAttach(uint32_t pid, const std::string& processName, bool success) {
    std::ostringstream oss;
    oss << "Attempt to attach to process '" << processName << "' (PID: " << pid << ") - "
        << (success ? "SUCCESS" : "FAILED");
    Log(AuditLevel::Security, "Process", oss.str());
}

void AuditLogger::LogMemoryRead(uint32_t pid, uintptr_t address, size_t size, bool success) {
    std::ostringstream oss;
    oss << "Memory read from PID " << pid << " at 0x" << std::hex << address
        << " size " << std::dec << size << " bytes - "
        << (success ? "SUCCESS" : "FAILED");
    Log(AuditLevel::Security, "Memory", oss.str());
}

void AuditLogger::LogMemoryWrite(uint32_t pid, uintptr_t address, size_t size, bool success) {
    std::ostringstream oss;
    oss << "Memory write to PID " << pid << " at 0x" << std::hex << address
        << " size " << std::dec << size << " bytes - "
        << (success ? "SUCCESS" : "FAILED");
    Log(AuditLevel::Critical, "Memory", oss.str());
}

void AuditLogger::LogExport(const std::string& exportType, const std::string& filepath) {
    std::ostringstream oss;
    oss << "Exported " << exportType << " to file: " << filepath;
    Log(AuditLevel::Info, "Export", oss.str());
}

std::vector<AuditEntry> AuditLogger::GetRecentEntries(size_t count) {
    std::lock_guard<std::mutex> lock(m_mutex);

    if (m_entries.size() <= count) {
        return m_entries;
    }

    return std::vector<AuditEntry>(
        m_entries.end() - count,
        m_entries.end()
    );
}

std::vector<AuditEntry> AuditLogger::GetEntriesByLevel(AuditLevel level) {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::vector<AuditEntry> filtered;
    for (const auto& entry : m_entries) {
        if (entry.level == level) {
            filtered.push_back(entry);
        }
    }
    return filtered;
}

bool AuditLogger::ExportToJson(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(m_mutex);

    json j = json::array();

    for (const auto& entry : m_entries) {
        json jEntry;
        jEntry["timestamp"] = entry.timestamp;
        jEntry["level"] = static_cast<int>(entry.level);
        jEntry["category"] = entry.category;
        jEntry["message"] = entry.message;
        jEntry["user"] = entry.user;
        jEntry["processId"] = entry.processId;
        j.push_back(jEntry);
    }

    std::ofstream outFile(filepath);
    if (!outFile.is_open()) {
        return false;
    }

    outFile << j.dump(2);
    outFile.close();

    Log(AuditLevel::Info, "Export", "Audit log exported to JSON: " + filepath);
    return true;
}

bool AuditLogger::ExportToHtml(const std::string& filepath) {
    std::lock_guard<std::mutex> lock(m_mutex);

    std::ofstream outFile(filepath);
    if (!outFile.is_open()) {
        return false;
    }

    outFile << "<!DOCTYPE html>\n<html>\n<head>\n";
    outFile << "<title>Audit Log Report</title>\n";
    outFile << "<style>\n";
    outFile << "body { font-family: Arial, sans-serif; margin: 20px; }\n";
    outFile << "table { border-collapse: collapse; width: 100%; }\n";
    outFile << "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n";
    outFile << "th { background-color: #4CAF50; color: white; }\n";
    outFile << "tr:nth-child(even) { background-color: #f2f2f2; }\n";
    outFile << ".info { color: blue; }\n";
    outFile << ".warning { color: orange; }\n";
    outFile << ".critical { color: red; font-weight: bold; }\n";
    outFile << ".security { color: purple; font-weight: bold; }\n";
    outFile << "</style>\n</head>\n<body>\n";
    outFile << "<h1>Diagnostic IDE - Audit Log Report</h1>\n";
    outFile << "<table>\n<tr><th>Timestamp</th><th>Level</th><th>Category</th><th>User</th><th>Message</th></tr>\n";

    for (const auto& entry : m_entries) {
        std::string levelClass;
        std::string levelText;

        switch (entry.level) {
            case AuditLevel::Info: levelClass = "info"; levelText = "INFO"; break;
            case AuditLevel::Warning: levelClass = "warning"; levelText = "WARNING"; break;
            case AuditLevel::Critical: levelClass = "critical"; levelText = "CRITICAL"; break;
            case AuditLevel::Security: levelClass = "security"; levelText = "SECURITY"; break;
        }

        outFile << "<tr><td>" << entry.timestamp << "</td>";
        outFile << "<td class=\"" << levelClass << "\">" << levelText << "</td>";
        outFile << "<td>" << entry.category << "</td>";
        outFile << "<td>" << entry.user << "</td>";
        outFile << "<td>" << entry.message << "</td></tr>\n";
    }

    outFile << "</table>\n</body>\n</html>";
    outFile.close();

    Log(AuditLevel::Info, "Export", "Audit log exported to HTML: " + filepath);
    return true;
}

void AuditLogger::WriteEntry(const AuditEntry& entry) {
    if (!m_logFile.is_open()) {
        return;
    }

    std::string levelStr;
    switch (entry.level) {
        case AuditLevel::Info: levelStr = "INFO"; break;
        case AuditLevel::Warning: levelStr = "WARNING"; break;
        case AuditLevel::Critical: levelStr = "CRITICAL"; break;
        case AuditLevel::Security: levelStr = "SECURITY"; break;
    }

    m_logFile << "[" << entry.timestamp << "] "
              << "[" << levelStr << "] "
              << "[" << entry.category << "] "
              << "[" << entry.user << "] "
              << entry.message << std::endl;

    m_logFile.flush();
}

std::string AuditLogger::GetCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto time = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;

    std::ostringstream oss;
    oss << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S");
    oss << '.' << std::setfill('0') << std::setw(3) << ms.count();

    return oss.str();
}

std::string AuditLogger::GetCurrentUser() {
#ifdef PLATFORM_WINDOWS
    char username[UNLEN + 1];
    DWORD username_len = UNLEN + 1;
    if (GetUserNameA(username, &username_len)) {
        return std::string(username);
    }
#else
    uid_t uid = geteuid();
    struct passwd* pw = getpwuid(uid);
    if (pw) {
        return std::string(pw->pw_name);
    }
#endif
    return "unknown";
}

} // namespace Core
} // namespace DiagIDE
