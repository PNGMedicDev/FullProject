#pragma once

#include "ui/IPanel.h"
#include "core/SystemMetrics.h"
#include <vector>
#include <memory>

namespace DiagIDE {
namespace UI {

class DiagnosticsDashboard : public IPanel {
public:
    DiagnosticsDashboard();
    ~DiagnosticsDashboard() override;

    bool Initialize() override;
    void Shutdown() override;
    void Render() override;
    void Update(float deltaTime) override;
    const char* GetName() const override { return "Diagnostics Dashboard"; }

private:
    void RenderCpuGraph();
    void RenderMemoryGraph();
    void RenderDiskIOGraph();
    void RenderNetworkGraph();
    void RenderProcessMetrics();

    void UpdateMetrics();

private:
    std::unique_ptr<Core::SystemMetrics> m_systemMetrics;

    // Time window for graphs (in seconds)
    float m_timeWindow;

    // Ring buffers for historical data
    std::vector<float> m_cpuHistory;
    std::vector<float> m_memoryHistory;
    std::vector<float> m_diskReadHistory;
    std::vector<float> m_diskWriteHistory;
    std::vector<float> m_netSentHistory;
    std::vector<float> m_netRecvHistory;

    size_t m_maxHistorySize;
    size_t m_historyIndex;

    float m_updateInterval;
    float m_timeSinceUpdate;
};

} // namespace UI
} // namespace DiagIDE
