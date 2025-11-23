#include "ui/ProcessExplorer.h"
#include "platform/ProcessManager.h"
#include "core/Application.h"
#include "core/AuditLogger.h"
#include <imgui.h>
#include <algorithm>
#include <cstring>

namespace DiagIDE {
namespace UI {

ProcessExplorer::ProcessExplorer()
    : m_selectedProcessIndex(-1)
    , m_refreshInterval(3.0f)  // Increase to 3 seconds for better performance
    , m_timeSinceRefresh(0.0f)
    , m_sortColumn(0)
    , m_sortAscending(true)
{
    std::memset(m_searchBuffer, 0, sizeof(m_searchBuffer));
}

ProcessExplorer::~ProcessExplorer() = default;

bool ProcessExplorer::Initialize() {
    // Defer initial process list load to first render to speed up startup
    return true;
}

void ProcessExplorer::Shutdown() {
    m_processes.clear();
}

void ProcessExplorer::Update(float deltaTime) {
    m_timeSinceRefresh += deltaTime;

    if (m_timeSinceRefresh >= m_refreshInterval) {
        RefreshProcessList();
        m_timeSinceRefresh = 0.0f;
    }
}

void ProcessExplorer::Render() {
    ImGui::Begin("Process Explorer", &m_visible);

    // Lazy load: refresh on first render if list is empty
    if (m_processes.empty()) {
        RefreshProcessList();
    }

    // Control bar
    if (ImGui::Button("Refresh")) {
        RefreshProcessList();
    }

    ImGui::SameLine();
    ImGui::SetNextItemWidth(100);
    ImGui::SliderFloat("Auto-refresh (s)", &m_refreshInterval, 0.5f, 10.0f);

    ImGui::SameLine();
    ImGui::SetNextItemWidth(200);
    ImGui::InputText("Search", m_searchBuffer, sizeof(m_searchBuffer));

    ImGui::Separator();

    // Process table
    RenderProcessTable();

    // Details panel
    if (m_selectedProcessIndex >= 0 && m_selectedProcessIndex < static_cast<int>(m_processes.size())) {
        ImGui::Separator();
        RenderProcessDetails();
    }

    ImGui::End();
}

void ProcessExplorer::RefreshProcessList() {
    auto processManager = Platform::ProcessManager::Create();
    m_processes = processManager->EnumerateProcesses();

    // Apply search filter if set
    if (m_searchBuffer[0] != '\0') {
        std::string search = m_searchBuffer;
        std::transform(search.begin(), search.end(), search.begin(), ::tolower);

        m_processes.erase(
            std::remove_if(m_processes.begin(), m_processes.end(),
                [&search](const Core::ProcessInfo& p) {
                    std::string name = p.name;
                    std::transform(name.begin(), name.end(), name.begin(), ::tolower);
                    return name.find(search) == std::string::npos;
                }),
            m_processes.end()
        );
    }

    // Apply sorting
    if (m_sortColumn >= 0) {
        std::sort(m_processes.begin(), m_processes.end(),
            [this](const Core::ProcessInfo& a, const Core::ProcessInfo& b) {
                bool result = false;
                switch (m_sortColumn) {
                    case 0: result = a.name < b.name; break;
                    case 1: result = a.pid < b.pid; break;
                    case 2: result = a.cpuUsage < b.cpuUsage; break;
                    case 3: result = a.memoryUsage < b.memoryUsage; break;
                    case 4: result = a.threadCount < b.threadCount; break;
                    default: break;
                }
                return m_sortAscending ? result : !result;
            }
        );
    }
}

void ProcessExplorer::RenderProcessTable() {
    ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
                            ImGuiTableFlags_Sortable | ImGuiTableFlags_ScrollY |
                            ImGuiTableFlags_Resizable;

    if (ImGui::BeginTable("ProcessTable", 5, flags)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 200.0f);
        ImGui::TableSetupColumn("PID", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("CPU %", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Memory", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Threads", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableHeadersRow();

        // Handle sorting
        ImGuiTableSortSpecs* sortSpecs = ImGui::TableGetSortSpecs();
        if (sortSpecs && sortSpecs->SpecsDirty) {
            if (sortSpecs->SpecsCount > 0) {
                m_sortColumn = sortSpecs->Specs[0].ColumnIndex;
                m_sortAscending = sortSpecs->Specs[0].SortDirection == ImGuiSortDirection_Ascending;
                RefreshProcessList();
            }
            sortSpecs->SpecsDirty = false;
        }

        ImGuiListClipper clipper;
        clipper.Begin(m_processes.size());

        while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
                const auto& process = m_processes[row];

                ImGui::TableNextRow();

                // Selectable row
                ImGui::TableSetColumnIndex(0);
                bool isSelected = (m_selectedProcessIndex == row);
                if (ImGui::Selectable(process.name.c_str(), isSelected,
                    ImGuiSelectableFlags_SpanAllColumns | ImGuiSelectableFlags_AllowItemOverlap)) {
                    m_selectedProcessIndex = row;
                }

                // Context menu
                if (ImGui::BeginPopupContextItem()) {
                    RenderContextMenu();
                    ImGui::EndPopup();
                }

                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%u", process.pid);

                ImGui::TableSetColumnIndex(2);
                ImGui::Text("%.1f%%", process.cpuUsage);

                ImGui::TableSetColumnIndex(3);
                double memMB = process.memoryUsage / (1024.0 * 1024.0);
                ImGui::Text("%.1f MB", memMB);

                ImGui::TableSetColumnIndex(4);
                ImGui::Text("%u", process.threadCount);
            }
        }

        ImGui::EndTable();
    }
}

void ProcessExplorer::RenderProcessDetails() {
    const auto& process = m_processes[m_selectedProcessIndex];

    ImGui::Text("Process Details");
    ImGui::Separator();

    ImGui::Text("Name: %s", process.name.c_str());
    ImGui::Text("PID: %u", process.pid);
    ImGui::Text("Path: %s", process.path.c_str());
    ImGui::Text("User: %s", process.userName.c_str());
    ImGui::Text("Status: %s", process.status.c_str());
}

void ProcessExplorer::RenderContextMenu() {
    if (m_selectedProcessIndex < 0 || m_selectedProcessIndex >= static_cast<int>(m_processes.size())) {
        return;
    }

    const auto& process = m_processes[m_selectedProcessIndex];

    if (ImGui::MenuItem("Inspect Process")) {
        // TODO: Open memory inspector for this process
        auto* app = Core::Application::GetInstance();
        if (app && app->GetAuditLogger()) {
            app->GetAuditLogger()->Log(
                Core::AuditLevel::Info,
                "ProcessExplorer",
                "User requested inspection of process: " + process.name
            );
        }
    }

    if (ImGui::MenuItem("View Modules")) {
        // TODO: Show modules dialog
    }

    if (ImGui::MenuItem("View Threads")) {
        // TODO: Show threads dialog
    }
}

} // namespace UI
} // namespace DiagIDE
