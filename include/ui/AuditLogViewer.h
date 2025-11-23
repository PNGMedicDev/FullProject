#pragma once

#include "ui/IPanel.h"
#include "core/AuditLogger.h"
#include <vector>
#include <string>

namespace DiagIDE {
namespace UI {

class AuditLogViewer : public IPanel {
public:
    AuditLogViewer();
    ~AuditLogViewer() override;

    bool Initialize() override;
    void Shutdown() override;
    void Render() override;
    void Update(float deltaTime) override;
    const char* GetName() const override { return "Audit Log"; }

private:
    void RenderLogTable();
    void RenderFilterControls();
    void RenderExportControls();

private:
    std::vector<Core::AuditEntry> m_entries;
    Core::AuditLevel m_filterLevel;
    bool m_filterEnabled;
    char m_searchBuffer[256];
    float m_refreshInterval;
    float m_timeSinceRefresh;
};

} // namespace UI
} // namespace DiagIDE
