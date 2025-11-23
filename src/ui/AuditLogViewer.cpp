#include "ui/AuditLogViewer.h"
#include "core/Application.h"
#include <imgui.h>
#include <cstring>

namespace DiagIDE {
namespace UI {

AuditLogViewer::AuditLogViewer()
    : m_filterLevel(Core::AuditLevel::Info)
    , m_filterEnabled(false)
    , m_refreshInterval(2.0f)
    , m_timeSinceRefresh(0.0f)
{
    std::memset(m_searchBuffer, 0, sizeof(m_searchBuffer));
}

AuditLogViewer::~AuditLogViewer() = default;

bool AuditLogViewer::Initialize() {
    return true;
}

void AuditLogViewer::Shutdown() {
}

void AuditLogViewer::Update(float deltaTime) {
    m_timeSinceRefresh += deltaTime;

    if (m_timeSinceRefresh >= m_refreshInterval) {
        auto* app = Core::Application::GetInstance();
        if (app && app->GetAuditLogger()) {
            m_entries = app->GetAuditLogger()->GetRecentEntries(1000);
        }
        m_timeSinceRefresh = 0.0f;
    }
}

void AuditLogViewer::Render() {
    ImGui::Begin("Audit Log", &m_visible);

    RenderFilterControls();
    ImGui::Separator();
    RenderExportControls();
    ImGui::Separator();
    RenderLogTable();

    ImGui::End();
}

void AuditLogViewer::RenderFilterControls() {
    ImGui::Checkbox("Filter by Level", &m_filterEnabled);

    if (m_filterEnabled) {
        ImGui::SameLine();
        const char* levelNames[] = { "Info", "Warning", "Critical", "Security" };
        int currentLevel = static_cast<int>(m_filterLevel);
        if (ImGui::Combo("Level", &currentLevel, levelNames, IM_ARRAYSIZE(levelNames))) {
            m_filterLevel = static_cast<Core::AuditLevel>(currentLevel);
        }
    }

    ImGui::SetNextItemWidth(200);
    ImGui::InputText("Search", m_searchBuffer, sizeof(m_searchBuffer));

    ImGui::SameLine();
    if (ImGui::Button("Refresh Now")) {
        auto* app = Core::Application::GetInstance();
        if (app && app->GetAuditLogger()) {
            m_entries = app->GetAuditLogger()->GetRecentEntries(1000);
        }
    }
}

void AuditLogViewer::RenderExportControls() {
    static char exportPath[256] = "audit_report";

    ImGui::InputText("Export Base Path", exportPath, sizeof(exportPath));

    if (ImGui::Button("Export to JSON")) {
        auto* app = Core::Application::GetInstance();
        if (app && app->GetAuditLogger()) {
            std::string filepath = std::string(exportPath) + ".json";
            app->GetAuditLogger()->ExportToJson(filepath);
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Export to HTML")) {
        auto* app = Core::Application::GetInstance();
        if (app && app->GetAuditLogger()) {
            std::string filepath = std::string(exportPath) + ".html";
            app->GetAuditLogger()->ExportToHtml(filepath);
        }
    }
}

void AuditLogViewer::RenderLogTable() {
    ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                            ImGuiTableFlags_ScrollY | ImGuiTableFlags_Resizable;

    if (ImGui::BeginTable("AuditLogTable", 5, flags)) {
        ImGui::TableSetupColumn("Timestamp", ImGuiTableColumnFlags_WidthFixed, 180.0f);
        ImGui::TableSetupColumn("Level", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("User", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Message", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (const auto& entry : m_entries) {
            // Apply filters
            if (m_filterEnabled && entry.level != m_filterLevel) {
                continue;
            }

            if (m_searchBuffer[0] != '\0') {
                std::string search = m_searchBuffer;
                if (entry.message.find(search) == std::string::npos &&
                    entry.category.find(search) == std::string::npos) {
                    continue;
                }
            }

            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("%s", entry.timestamp.c_str());

            ImGui::TableSetColumnIndex(1);
            ImVec4 color;
            const char* levelText;
            switch (entry.level) {
                case Core::AuditLevel::Info:
                    color = ImVec4(0.5f, 0.5f, 1.0f, 1.0f);
                    levelText = "INFO";
                    break;
                case Core::AuditLevel::Warning:
                    color = ImVec4(1.0f, 0.8f, 0.0f, 1.0f);
                    levelText = "WARNING";
                    break;
                case Core::AuditLevel::Critical:
                    color = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
                    levelText = "CRITICAL";
                    break;
                case Core::AuditLevel::Security:
                    color = ImVec4(1.0f, 0.0f, 1.0f, 1.0f);
                    levelText = "SECURITY";
                    break;
            }
            ImGui::TextColored(color, "%s", levelText);

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", entry.category.c_str());

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%s", entry.user.c_str());

            ImGui::TableSetColumnIndex(4);
            ImGui::TextWrapped("%s", entry.message.c_str());
        }

        ImGui::EndTable();
    }
}

} // namespace UI
} // namespace DiagIDE
