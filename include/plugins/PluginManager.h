#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>

namespace DiagIDE {
namespace UI {
class IPanel;
}

namespace Plugins {

// Plugin API version for compatibility checking
constexpr int PLUGIN_API_VERSION = 1;

struct PluginInfo {
    std::string name;
    std::string version;
    std::string author;
    std::string description;
    int apiVersion;
};

// Base plugin interface
class IPlugin {
public:
    virtual ~IPlugin() = default;

    virtual bool Initialize() = 0;
    virtual void Shutdown() = 0;

    virtual PluginInfo GetInfo() const = 0;

    // Optional: plugins can provide custom panels
    virtual std::vector<std::shared_ptr<UI::IPanel>> GetPanels() { return {}; }

    // Optional: plugins can register custom commands
    virtual void RegisterCommands() {}
};

// Plugin loading/management
class PluginManager {
public:
    PluginManager();
    ~PluginManager();

    // Plugin discovery and loading
    bool LoadPlugin(const std::string& pluginPath);
    bool UnloadPlugin(const std::string& pluginName);
    void LoadPluginsFromDirectory(const std::string& directory);

    // Plugin query
    std::vector<PluginInfo> GetLoadedPlugins() const;
    IPlugin* GetPlugin(const std::string& name) const;

    // Event dispatch to plugins
    void OnApplicationStart();
    void OnApplicationShutdown();

private:
    struct PluginHandle {
        void* libraryHandle;
        std::unique_ptr<IPlugin> plugin;
        PluginInfo info;
    };

    std::vector<PluginHandle> m_plugins;
};

} // namespace Plugins
} // namespace DiagIDE

// Plugin entry point (to be implemented by plugins)
extern "C" {
    typedef DiagIDE::Plugins::IPlugin* (*CreatePluginFunc)();
    typedef void (*DestroyPluginFunc)(DiagIDE::Plugins::IPlugin*);
}
