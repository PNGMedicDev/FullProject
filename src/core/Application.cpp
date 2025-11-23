#include "core/Application.h"
#include "core/AuditLogger.h"
#include "ui/IPanel.h"
#include "ui/ProcessExplorer.h"
#include "ui/MemoryInspector.h"
#include "ui/DiagnosticsDashboard.h"
#include "ui/AuditLogViewer.h"
#include "ui/ImGuiTheme.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <chrono>
#include <algorithm>

namespace DiagIDE {
namespace Core {

Application* Application::s_instance = nullptr;

static void glfw_error_callback(int error, const char* description) {
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

Application::Application(const std::string& title, int width, int height)
    : m_title(title)
    , m_width(width)
    , m_height(height)
    , m_window(nullptr)
    , m_running(false)
{
    s_instance = this;
}

Application::~Application() {
    s_instance = nullptr;
}

bool Application::Initialize() {
    // Initialize GLFW
    glfwSetErrorCallback(glfw_error_callback);
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    // GL 3.3 + GLSL 330
    const char* glsl_version = "#version 330";
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    // Create window
    m_window = glfwCreateWindow(m_width, m_height, m_title.c_str(), nullptr, nullptr);
    if (!m_window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // Enable vsync

    // Setup Dear ImGui
    SetupImGui();

    // Initialize audit logger
    m_auditLogger = std::make_unique<AuditLogger>("diagnostic_ide.audit.log");
    m_auditLogger->Log(AuditLevel::Info, "Application", "Diagnostic IDE started");

    // Register default panels
    RegisterPanel(std::make_shared<UI::ProcessExplorer>());
    RegisterPanel(std::make_shared<UI::MemoryInspector>());
    RegisterPanel(std::make_shared<UI::DiagnosticsDashboard>());
    RegisterPanel(std::make_shared<UI::AuditLogViewer>());

    // Initialize all panels
    for (auto& panel : m_panels) {
        if (!panel->Initialize()) {
            std::cerr << "Failed to initialize panel: " << panel->GetName() << std::endl;
        }
    }

    // Try to load saved layout
    LoadLayout("layout.ini");

    m_running = true;
    return true;
}

void Application::Run() {
    auto lastTime = std::chrono::high_resolution_clock::now();

    while (m_running && !glfwWindowShouldClose(m_window)) {
        auto currentTime = std::chrono::high_resolution_clock::now();
        float deltaTime = std::chrono::duration<float>(currentTime - lastTime).count();
        lastTime = currentTime;

        glfwPollEvents();

        // Update panels
        for (auto& panel : m_panels) {
            if (panel->IsVisible()) {
                panel->Update(deltaTime);
            }
        }

        // Render
        RenderFrame();

        glfwSwapBuffers(m_window);
    }
}

void Application::Shutdown() {
    m_auditLogger->Log(AuditLevel::Info, "Application", "Diagnostic IDE shutting down");

    // Save layout
    SaveLayout("layout.ini");

    // Shutdown panels
    for (auto& panel : m_panels) {
        panel->Shutdown();
    }
    m_panels.clear();

    ShutdownImGui();

    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }

    glfwTerminate();
}

void Application::RegisterPanel(std::shared_ptr<UI::IPanel> panel) {
    m_panels.push_back(panel);
}

void Application::RemovePanel(const std::string& panelName) {
    m_panels.erase(
        std::remove_if(m_panels.begin(), m_panels.end(),
            [&panelName](const std::shared_ptr<UI::IPanel>& p) {
                return std::string(p->GetName()) == panelName;
            }),
        m_panels.end()
    );
}

bool Application::SaveLayout(const std::string& filepath) {
    ImGui::SaveIniSettingsToDisk(filepath.c_str());
    return true;
}

bool Application::LoadLayout(const std::string& filepath) {
    ImGui::LoadIniSettingsFromDisk(filepath.c_str());
    return true;
}

void Application::SetupImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Enable docking if available (requires ImGui docking branch)
    #ifdef IMGUI_HAS_DOCK
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    #endif

    // Enable viewports if available
    #ifdef IMGUI_HAS_VIEWPORT
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    #endif

    // Setup custom theme (light mode by default, use false for light, true for dark)
    UI::SetupImGuiTheme(false, 1.0f);  // Change first parameter to true for dark mode

    // When viewports are enabled we tweak WindowRounding/WindowBg
    #ifdef IMGUI_HAS_VIEWPORT
    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }
    #endif

    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");
}

void Application::ShutdownImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Application::RenderFrame() {
    // Start ImGui frame
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    // Main menu bar and dockspace (if available)
    #ifdef IMGUI_HAS_DOCK
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);

    ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
    window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse;
    window_flags |= ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
    window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGui::Begin("DockSpace", nullptr, window_flags);
    ImGui::PopStyleVar(3);

    // DockSpace
    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

    // Main menu bar
    RenderMainMenuBar();

    ImGui::End();
    #else
    // Fallback: just render menu bar without dockspace
    RenderMainMenuBar();
    #endif

    // Render all panels
    RenderPanels();

    // Rendering
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(m_window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    // Update and Render additional Platform Windows (if viewports enabled)
    #ifdef IMGUI_HAS_VIEWPORT
    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup_current_context = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup_current_context);
    }
    #endif
}

void Application::RenderMainMenuBar() {
    if (ImGui::BeginMenuBar()) {
        if (ImGui::BeginMenu("File")) {
            if (ImGui::MenuItem("Save Layout", "Ctrl+S")) {
                SaveLayout("layout.ini");
            }
            if (ImGui::MenuItem("Load Layout", "Ctrl+O")) {
                LoadLayout("layout.ini");
            }
            ImGui::Separator();
            if (ImGui::MenuItem("Exit", "Alt+F4")) {
                m_running = false;
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Panels")) {
            ImGui::TextDisabled("Toggle Panel Visibility:");
            ImGui::Separator();

            for (auto& panel : m_panels) {
                bool visible = panel->IsVisible();
                if (ImGui::MenuItem(panel->GetName(), nullptr, &visible)) {
                    panel->SetVisible(visible);
                }
            }

            ImGui::Separator();
            if (ImGui::MenuItem("Show All")) {
                for (auto& panel : m_panels) {
                    panel->SetVisible(true);
                }
            }
            if (ImGui::MenuItem("Hide All")) {
                for (auto& panel : m_panels) {
                    panel->SetVisible(false);
                }
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Help")) {
            if (ImGui::MenuItem("About")) {
                // TODO: Show about dialog
            }
            ImGui::Separator();
            ImGui::TextDisabled("Diagnostic IDE v0.1.0");
            ImGui::TextDisabled("Authorized Use Only");
            ImGui::EndMenu();
        }

        ImGui::EndMenuBar();
    }
}

void Application::RenderPanels() {
    for (auto& panel : m_panels) {
        if (panel->IsVisible()) {
            panel->Render();
        }
    }
}

} // namespace Core
} // namespace DiagIDE
