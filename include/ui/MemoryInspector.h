#pragma once

#include "ui/IPanel.h"
#include <cstdint>
#include <vector>
#include <memory>

namespace DiagIDE {
namespace Platform {
class ProcessManager;
}

namespace UI {

class MemoryInspector : public IPanel {
public:
    MemoryInspector();
    ~MemoryInspector() override;

    bool Initialize() override;
    void Shutdown() override;
    void Render() override;
    void Update(float deltaTime) override;
    const char* GetName() const override { return "Memory Inspector"; }

    void SetTargetProcess(uint32_t pid);

private:
    void RenderAttachmentPanel();
    void RenderMemoryRegions();
    void RenderHexViewer();
    void RenderMemoryEditor();
    void RenderExportPanel();

    bool RequestUserConsent(const std::string& operation);
    void ReadMemoryBlock(uintptr_t address, size_t size);
    void WriteMemoryBlock(uintptr_t address, const std::vector<uint8_t>& data);

private:
    std::shared_ptr<Platform::ProcessManager> m_processManager;
    uint32_t m_targetPid;
    bool m_isAttached;

    // Hex viewer state
    uintptr_t m_viewAddress;
    std::vector<uint8_t> m_memoryBuffer;
    size_t m_bytesPerRow;

    // UI state
    char m_addressInput[32];
    char m_sizeInput[32];
    bool m_showConsentDialog;
    std::string m_pendingOperation;
};

} // namespace UI
} // namespace DiagIDE
