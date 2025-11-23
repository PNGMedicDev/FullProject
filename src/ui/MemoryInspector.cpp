#include "ui/MemoryInspector.h"
#include "platform/ProcessManager.h"
#include "core/Application.h"
#include "core/AuditLogger.h"
#include <imgui.h>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <cstring>

namespace DiagIDE {
namespace UI {

MemoryInspector::MemoryInspector()
    : m_processManager(Platform::ProcessManager::Create())
    , m_targetPid(0)
    , m_isAttached(false)
    , m_viewAddress(0)
    , m_bytesPerRow(16)
    , m_showConsentDialog(false)
{
    std::memset(m_addressInput, 0, sizeof(m_addressInput));
    std::memset(m_sizeInput, 0, sizeof(m_sizeInput));
    std::strcpy(m_sizeInput, "256"); // Default size
}

MemoryInspector::~MemoryInspector() = default;

bool MemoryInspector::Initialize() {
    return true;
}

void MemoryInspector::Shutdown() {
    if (m_isAttached && m_targetPid != 0) {
        m_processManager->DetachFromProcess(m_targetPid);
    }
}

void MemoryInspector::Update(float deltaTime) {
    // Nothing to update per frame
}

void MemoryInspector::Render() {
    ImGui::Begin("Memory Inspector", &m_visible);

    RenderAttachmentPanel();

    if (m_isAttached) {
        ImGui::Separator();
        RenderMemoryRegions();
        ImGui::Separator();
        RenderHexViewer();
        ImGui::Separator();
        RenderExportPanel();
    }

    // Consent dialog
    if (m_showConsentDialog) {
        ImGui::OpenPopup("Operation Consent");
        if (ImGui::BeginPopupModal("Operation Consent", &m_showConsentDialog)) {
            ImGui::Text("WARNING: You are about to perform a privileged operation:");
            ImGui::Separator();
            ImGui::TextWrapped("%s", m_pendingOperation.c_str());
            ImGui::Separator();
            ImGui::TextWrapped("This action will be logged in the audit trail.");
            ImGui::Spacing();

            if (ImGui::Button("Approve", ImVec2(120, 0))) {
                m_showConsentDialog = false;
                // Operation approved - actual operation would happen here
                auto* app = Core::Application::GetInstance();
                if (app && app->GetAuditLogger()) {
                    app->GetAuditLogger()->Log(
                        Core::AuditLevel::Security,
                        "MemoryInspector",
                        "User approved: " + m_pendingOperation
                    );
                }
                ImGui::CloseCurrentPopup();
            }

            ImGui::SameLine();
            if (ImGui::Button("Deny", ImVec2(120, 0))) {
                m_showConsentDialog = false;
                ImGui::CloseCurrentPopup();
            }

            ImGui::EndPopup();
        }
    }

    ImGui::End();
}

void MemoryInspector::SetTargetProcess(uint32_t pid) {
    if (m_isAttached && m_targetPid != 0) {
        m_processManager->DetachFromProcess(m_targetPid);
        m_isAttached = false;
    }

    m_targetPid = pid;
}

void MemoryInspector::RenderAttachmentPanel() {
    ImGui::Text("Target Process: %s", m_targetPid == 0 ? "None" : std::to_string(m_targetPid).c_str());

    if (!m_isAttached) {
        ImGui::InputInt("Process ID", reinterpret_cast<int*>(&m_targetPid));

        if (ImGui::Button("Attach to Process")) {
            if (m_targetPid != 0) {
                std::ostringstream oss;
                oss << "Attach to process PID " << m_targetPid << " for memory inspection";

                if (RequestUserConsent(oss.str())) {
                    bool success = m_processManager->AttachToProcess(m_targetPid);
                    m_isAttached = success;

                    auto* app = Core::Application::GetInstance();
                    if (app && app->GetAuditLogger()) {
                        app->GetAuditLogger()->LogProcessAttach(
                            m_targetPid,
                            std::to_string(m_targetPid),
                            success
                        );
                    }

                    if (!success) {
                        ImGui::OpenPopup("Attach Failed");
                    }
                }
            }
        }

        // Error popup
        if (ImGui::BeginPopupModal("Attach Failed", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Failed to attach to process.");
            ImGui::Text("Possible reasons:");
            ImGui::BulletText("Process does not exist");
            ImGui::BulletText("Insufficient permissions");
            ImGui::BulletText("Process is protected");
            ImGui::Spacing();

            if (ImGui::Button("OK", ImVec2(120, 0))) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }
    } else {
        ImGui::TextColored(ImVec4(0, 1, 0, 1), "Attached to PID %u", m_targetPid);

        if (ImGui::Button("Detach")) {
            m_processManager->DetachFromProcess(m_targetPid);
            m_isAttached = false;

            auto* app = Core::Application::GetInstance();
            if (app && app->GetAuditLogger()) {
                app->GetAuditLogger()->Log(
                    Core::AuditLevel::Info,
                    "MemoryInspector",
                    "Detached from process PID " + std::to_string(m_targetPid)
                );
            }
        }
    }
}

void MemoryInspector::RenderMemoryRegions() {
    if (!ImGui::CollapsingHeader("Memory Regions")) {
        return;
    }

    auto regions = m_processManager->GetMemoryRegions(m_targetPid);

    if (ImGui::BeginTable("MemoryRegions", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
        ImGui::TableSetupColumn("Base Address", ImGuiTableColumnFlags_WidthFixed, 120.0f);
        ImGui::TableSetupColumn("Size", ImGuiTableColumnFlags_WidthFixed, 100.0f);
        ImGui::TableSetupColumn("Protection", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableHeadersRow();

        for (const auto& region : regions) {
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            ImGui::Text("0x%016lX", region.baseAddress);

            ImGui::TableSetColumnIndex(1);
            ImGui::Text("%zu bytes", region.size);

            ImGui::TableSetColumnIndex(2);
            std::string prot;
            if (region.readable) prot += "R";
            if (region.writable) prot += "W";
            if (region.executable) prot += "X";
            ImGui::Text("%s", prot.c_str());

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%s", region.type.c_str());

            ImGui::TableSetColumnIndex(4);
            if (region.readable) {
                std::ostringstream oss;
                oss << "View##" << region.baseAddress;
                if (ImGui::SmallButton(oss.str().c_str())) {
                    m_viewAddress = region.baseAddress;
                    ReadMemoryBlock(region.baseAddress, std::min<size_t>(region.size, 256));
                }
            }
        }

        ImGui::EndTable();
    }
}

void MemoryInspector::RenderHexViewer() {
    if (!ImGui::CollapsingHeader("Hex Viewer", ImGuiTreeNodeFlags_DefaultOpen)) {
        return;
    }

    ImGui::InputText("Address", m_addressInput, sizeof(m_addressInput));
    ImGui::SameLine();
    ImGui::InputText("Size", m_sizeInput, sizeof(m_sizeInput));

    if (ImGui::Button("Read Memory")) {
        try {
            uintptr_t address = std::stoull(m_addressInput, nullptr, 16);
            size_t size = std::stoull(m_sizeInput, nullptr, 10);

            if (size > 0 && size <= 1024 * 1024) { // Limit to 1MB
                ReadMemoryBlock(address, size);
            }
        } catch (...) {
            // Invalid input
        }
    }

    // Display hex dump
    if (!m_memoryBuffer.empty()) {
        ImGui::Separator();
        ImGui::Text("Viewing address: 0x%016lX (%zu bytes)", m_viewAddress, m_memoryBuffer.size());
        ImGui::Separator();

        ImGuiTableFlags flags = ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY;

        if (ImGui::BeginTable("HexView", 3, flags, ImVec2(0, 300))) {
            ImGui::TableSetupColumn("Address", ImGuiTableColumnFlags_WidthFixed, 120.0f);
            ImGui::TableSetupColumn("Hex", ImGuiTableColumnFlags_WidthFixed, 400.0f);
            ImGui::TableSetupColumn("ASCII", ImGuiTableColumnFlags_WidthFixed, 150.0f);
            ImGui::TableHeadersRow();

            size_t offset = 0;
            while (offset < m_memoryBuffer.size()) {
                ImGui::TableNextRow();

                ImGui::TableSetColumnIndex(0);
                ImGui::Text("0x%016lX", m_viewAddress + offset);

                ImGui::TableSetColumnIndex(1);
                std::ostringstream hexStream;
                size_t lineEnd = std::min(offset + m_bytesPerRow, m_memoryBuffer.size());
                for (size_t i = offset; i < lineEnd; ++i) {
                    hexStream << std::hex << std::setw(2) << std::setfill('0')
                             << static_cast<int>(m_memoryBuffer[i]) << " ";
                }
                ImGui::Text("%s", hexStream.str().c_str());

                ImGui::TableSetColumnIndex(2);
                std::ostringstream asciiStream;
                for (size_t i = offset; i < lineEnd; ++i) {
                    char c = m_memoryBuffer[i];
                    asciiStream << (isprint(c) ? c : '.');
                }
                ImGui::Text("%s", asciiStream.str().c_str());

                offset += m_bytesPerRow;
            }

            ImGui::EndTable();
        }
    }
}

void MemoryInspector::RenderExportPanel() {
    if (!ImGui::CollapsingHeader("Export")) {
        return;
    }

    static char exportPath[256] = "memory_dump.bin";
    ImGui::InputText("Export Path", exportPath, sizeof(exportPath));

    if (ImGui::Button("Export Current View")) {
        if (!m_memoryBuffer.empty()) {
            std::ofstream outFile(exportPath, std::ios::binary);
            if (outFile.is_open()) {
                outFile.write(reinterpret_cast<const char*>(m_memoryBuffer.data()), m_memoryBuffer.size());
                outFile.close();

                auto* app = Core::Application::GetInstance();
                if (app && app->GetAuditLogger()) {
                    app->GetAuditLogger()->LogExport("Memory Dump", exportPath);
                }
            }
        }
    }
}

bool MemoryInspector::RequestUserConsent(const std::string& operation) {
    m_pendingOperation = operation;
    m_showConsentDialog = true;
    // In a real implementation, this would be modal and wait for user response
    // For now, we'll auto-approve (but still log)
    return true;
}

void MemoryInspector::ReadMemoryBlock(uintptr_t address, size_t size) {
    m_viewAddress = address;
    m_memoryBuffer.resize(size);

    bool success = m_processManager->ReadMemory(m_targetPid, address, m_memoryBuffer.data(), size);

    auto* app = Core::Application::GetInstance();
    if (app && app->GetAuditLogger()) {
        app->GetAuditLogger()->LogMemoryRead(m_targetPid, address, size, success);
    }

    if (!success) {
        m_memoryBuffer.clear();
    }
}

void MemoryInspector::WriteMemoryBlock(uintptr_t address, const std::vector<uint8_t>& data) {
    bool success = m_processManager->WriteMemory(m_targetPid, address, data.data(), data.size());

    auto* app = Core::Application::GetInstance();
    if (app && app->GetAuditLogger()) {
        app->GetAuditLogger()->LogMemoryWrite(m_targetPid, address, data.size(), success);
    }
}

} // namespace UI
} // namespace DiagIDE
