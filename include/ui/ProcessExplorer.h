#pragma once

#include "ui/IPanel.h"
#include "core/ProcessInfo.h"
#include <vector>
#include <string>
#include <memory>

namespace DiagIDE {
namespace UI {

class ProcessExplorer : public IPanel {
public:
    ProcessExplorer();
    ~ProcessExplorer() override;

    bool Initialize() override;
    void Shutdown() override;
    void Render() override;
    void Update(float deltaTime) override;
    const char* GetName() const override { return "Process Explorer"; }

private:
    void RefreshProcessList();
    void RenderProcessTable();
    void RenderProcessDetails();
    void RenderContextMenu();

private:
    std::vector<Core::ProcessInfo> m_processes;
    int m_selectedProcessIndex;
    float m_refreshInterval;
    float m_timeSinceRefresh;
    char m_searchBuffer[256];

    // Sort state
    int m_sortColumn;
    bool m_sortAscending;
};

} // namespace UI
} // namespace DiagIDE
