#include "ui/DiagnosticsDashboard.h"
#include "core/SystemMetrics.h"
#include <imgui.h>
#include <implot.h>

namespace DiagIDE {
namespace UI {

DiagnosticsDashboard::DiagnosticsDashboard()
    : m_systemMetrics(Core::SystemMetrics::Create())
    , m_timeWindow(60.0f)
    , m_maxHistorySize(300)
    , m_historyIndex(0)
    , m_updateInterval(0.5f)
    , m_timeSinceUpdate(0.0f)
{
    m_cpuHistory.resize(m_maxHistorySize, 0.0f);
    m_memoryHistory.resize(m_maxHistorySize, 0.0f);
    m_diskReadHistory.resize(m_maxHistorySize, 0.0f);
    m_diskWriteHistory.resize(m_maxHistorySize, 0.0f);
    m_netSentHistory.resize(m_maxHistorySize, 0.0f);
    m_netRecvHistory.resize(m_maxHistorySize, 0.0f);
}

DiagnosticsDashboard::~DiagnosticsDashboard() = default;

bool DiagnosticsDashboard::Initialize() {
    ImPlot::CreateContext();
    UpdateMetrics();
    return true;
}

void DiagnosticsDashboard::Shutdown() {
    ImPlot::DestroyContext();
}

void DiagnosticsDashboard::Update(float deltaTime) {
    m_timeSinceUpdate += deltaTime;

    if (m_timeSinceUpdate >= m_updateInterval) {
        UpdateMetrics();
        m_timeSinceUpdate = 0.0f;
    }
}

void DiagnosticsDashboard::Render() {
    ImGui::Begin("Diagnostics Dashboard", &m_visible);

    ImGui::Text("System Performance Monitoring");
    ImGui::Separator();

    // Time window control
    ImGui::SetNextItemWidth(150);
    if (ImGui::SliderFloat("Time Window (s)", &m_timeWindow, 10.0f, 300.0f)) {
        size_t newSize = static_cast<size_t>(m_timeWindow / m_updateInterval);
        if (newSize != m_maxHistorySize) {
            m_maxHistorySize = newSize;
            m_cpuHistory.resize(m_maxHistorySize, 0.0f);
            m_memoryHistory.resize(m_maxHistorySize, 0.0f);
            m_diskReadHistory.resize(m_maxHistorySize, 0.0f);
            m_diskWriteHistory.resize(m_maxHistorySize, 0.0f);
            m_netSentHistory.resize(m_maxHistorySize, 0.0f);
            m_netRecvHistory.resize(m_maxHistorySize, 0.0f);
        }
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(150);
    ImGui::SliderFloat("Update Interval (s)", &m_updateInterval, 0.1f, 2.0f);

    ImGui::Separator();

    // Render graphs
    RenderCpuGraph();
    RenderMemoryGraph();
    RenderDiskIOGraph();
    RenderNetworkGraph();

    ImGui::End();
}

void DiagnosticsDashboard::RenderCpuGraph() {
    if (ImPlot::BeginPlot("CPU Usage", ImVec2(-1, 200))) {
        ImPlot::SetupAxes("Time", "Usage (%)", 0, 0);
        ImPlot::SetupAxisLimits(ImAxis_X1, 0, m_maxHistorySize, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 100, ImGuiCond_Always);

        ImPlot::PlotLine("CPU", m_cpuHistory.data(), m_cpuHistory.size());

        ImPlot::EndPlot();
    }

    auto cpuMetrics = m_systemMetrics->GetCPUMetrics();
    ImGui::Text("Current CPU: %.1f%% (Cores: %u)", cpuMetrics.totalUsage, cpuMetrics.coreCount);
}

void DiagnosticsDashboard::RenderMemoryGraph() {
    if (ImPlot::BeginPlot("Memory Usage", ImVec2(-1, 200))) {
        ImPlot::SetupAxes("Time", "Usage (%)", 0, 0);
        ImPlot::SetupAxisLimits(ImAxis_X1, 0, m_maxHistorySize, ImGuiCond_Always);
        ImPlot::SetupAxisLimits(ImAxis_Y1, 0, 100, ImGuiCond_Always);

        ImPlot::PlotLine("Memory", m_memoryHistory.data(), m_memoryHistory.size());

        ImPlot::EndPlot();
    }

    auto memMetrics = m_systemMetrics->GetMemoryMetrics();
    double totalGB = memMetrics.totalPhysical / (1024.0 * 1024.0 * 1024.0);
    double usedGB = memMetrics.usedPhysical / (1024.0 * 1024.0 * 1024.0);
    ImGui::Text("Memory: %.2f / %.2f GB (%.1f%%)", usedGB, totalGB, memMetrics.usagePercent);
}

void DiagnosticsDashboard::RenderDiskIOGraph() {
    if (ImPlot::BeginPlot("Disk I/O", ImVec2(-1, 200))) {
        ImPlot::SetupAxes("Time", "Bytes/sec", 0, 0);
        ImPlot::SetupAxisLimits(ImAxis_X1, 0, m_maxHistorySize, ImGuiCond_Always);

        ImPlot::PlotLine("Read", m_diskReadHistory.data(), m_diskReadHistory.size());
        ImPlot::PlotLine("Write", m_diskWriteHistory.data(), m_diskWriteHistory.size());

        ImPlot::EndPlot();
    }

    auto diskMetrics = m_systemMetrics->GetDiskIOMetrics();
    double readMB = diskMetrics.readBytesPerSec / (1024.0 * 1024.0);
    double writeMB = diskMetrics.writeBytesPerSec / (1024.0 * 1024.0);
    ImGui::Text("Disk I/O - Read: %.2f MB/s, Write: %.2f MB/s", readMB, writeMB);
}

void DiagnosticsDashboard::RenderNetworkGraph() {
    if (ImPlot::BeginPlot("Network I/O", ImVec2(-1, 200))) {
        ImPlot::SetupAxes("Time", "Bytes/sec", 0, 0);
        ImPlot::SetupAxisLimits(ImAxis_X1, 0, m_maxHistorySize, ImGuiCond_Always);

        ImPlot::PlotLine("Sent", m_netSentHistory.data(), m_netSentHistory.size());
        ImPlot::PlotLine("Received", m_netRecvHistory.data(), m_netRecvHistory.size());

        ImPlot::EndPlot();
    }

    auto netMetrics = m_systemMetrics->GetNetworkMetrics();
    double sentKB = netMetrics.bytesSentPerSec / 1024.0;
    double recvKB = netMetrics.bytesRecvPerSec / 1024.0;
    ImGui::Text("Network - Sent: %.2f KB/s, Received: %.2f KB/s", sentKB, recvKB);
}

void DiagnosticsDashboard::UpdateMetrics() {
    m_systemMetrics->Update();

    auto cpuMetrics = m_systemMetrics->GetCPUMetrics();
    auto memMetrics = m_systemMetrics->GetMemoryMetrics();
    auto diskMetrics = m_systemMetrics->GetDiskIOMetrics();
    auto netMetrics = m_systemMetrics->GetNetworkMetrics();

    // Update ring buffers
    m_cpuHistory[m_historyIndex] = cpuMetrics.totalUsage;
    m_memoryHistory[m_historyIndex] = memMetrics.usagePercent;
    m_diskReadHistory[m_historyIndex] = static_cast<float>(diskMetrics.readBytesPerSec);
    m_diskWriteHistory[m_historyIndex] = static_cast<float>(diskMetrics.writeBytesPerSec);
    m_netSentHistory[m_historyIndex] = static_cast<float>(netMetrics.bytesSentPerSec);
    m_netRecvHistory[m_historyIndex] = static_cast<float>(netMetrics.bytesRecvPerSec);

    m_historyIndex = (m_historyIndex + 1) % m_maxHistorySize;
}

} // namespace UI
} // namespace DiagIDE
