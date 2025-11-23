#pragma once

#include <string>
#include <memory>
#include <vector>

struct GLFWwindow;

namespace DiagIDE {

namespace UI {
class IPanel;
}

namespace Core {

class AuditLogger;

class Application {
public:
    Application(const std::string& title, int width, int height);
    ~Application();

    // No copy
    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    bool Initialize();
    void Run();
    void Shutdown();

    // Panel management
    void RegisterPanel(std::shared_ptr<UI::IPanel> panel);
    void RemovePanel(const std::string& panelName);

    // Layout persistence
    bool SaveLayout(const std::string& filepath);
    bool LoadLayout(const std::string& filepath);

    // Audit logger access
    AuditLogger* GetAuditLogger() const { return m_auditLogger.get(); }

    static Application* GetInstance() { return s_instance; }

private:
    void SetupImGui();
    void ShutdownImGui();
    void RenderFrame();
    void RenderMainMenuBar();
    void RenderPanels();

private:
    static Application* s_instance;

    std::string m_title;
    int m_width;
    int m_height;
    GLFWwindow* m_window;
    bool m_running;

    std::vector<std::shared_ptr<UI::IPanel>> m_panels;
    std::unique_ptr<AuditLogger> m_auditLogger;
};

} // namespace Core
} // namespace DiagIDE
