#include "plugins/PluginManager.h"
#include <iostream>
#include <filesystem>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#else
#include <dlfcn.h>
#endif

namespace DiagIDE {
namespace Plugins {

PluginManager::PluginManager() = default;

PluginManager::~PluginManager() {
    // Unload all plugins
    for (auto& handle : m_plugins) {
        handle.plugin->Shutdown();
#ifdef PLATFORM_WINDOWS
        FreeLibrary(static_cast<HMODULE>(handle.libraryHandle));
#else
        dlclose(handle.libraryHandle);
#endif
    }
}

bool PluginManager::LoadPlugin(const std::string& pluginPath) {
    // Check if plugin already loaded
    for (const auto& handle : m_plugins) {
        if (handle.info.name == std::filesystem::path(pluginPath).stem().string()) {
            std::cerr << "Plugin already loaded: " << pluginPath << std::endl;
            return false;
        }
    }

#ifdef PLATFORM_WINDOWS
    HMODULE library = LoadLibraryA(pluginPath.c_str());
    if (!library) {
        std::cerr << "Failed to load plugin library: " << pluginPath << std::endl;
        return false;
    }

    auto createFunc = reinterpret_cast<CreatePluginFunc>(GetProcAddress(library, "CreatePlugin"));
    if (!createFunc) {
        std::cerr << "Failed to find CreatePlugin function in: " << pluginPath << std::endl;
        FreeLibrary(library);
        return false;
    }
#else
    void* library = dlopen(pluginPath.c_str(), RTLD_LAZY);
    if (!library) {
        std::cerr << "Failed to load plugin library: " << pluginPath << std::endl;
        std::cerr << "Error: " << dlerror() << std::endl;
        return false;
    }

    auto createFunc = reinterpret_cast<CreatePluginFunc>(dlsym(library, "CreatePlugin"));
    if (!createFunc) {
        std::cerr << "Failed to find CreatePlugin function in: " << pluginPath << std::endl;
        dlclose(library);
        return false;
    }
#endif

    // Create plugin instance
    IPlugin* plugin = createFunc();
    if (!plugin) {
        std::cerr << "Failed to create plugin instance from: " << pluginPath << std::endl;
#ifdef PLATFORM_WINDOWS
        FreeLibrary(library);
#else
        dlclose(library);
#endif
        return false;
    }

    PluginInfo info = plugin->GetInfo();

    // Check API version compatibility
    if (info.apiVersion != PLUGIN_API_VERSION) {
        std::cerr << "Plugin API version mismatch: " << info.name << std::endl;
        delete plugin;
#ifdef PLATFORM_WINDOWS
        FreeLibrary(library);
#else
        dlclose(library);
#endif
        return false;
    }

    // Initialize plugin
    if (!plugin->Initialize()) {
        std::cerr << "Failed to initialize plugin: " << info.name << std::endl;
        delete plugin;
#ifdef PLATFORM_WINDOWS
        FreeLibrary(library);
#else
        dlclose(library);
#endif
        return false;
    }

    // Store plugin handle
    PluginHandle handle;
    handle.libraryHandle = library;
    handle.plugin = std::unique_ptr<IPlugin>(plugin);
    handle.info = info;

    m_plugins.push_back(std::move(handle));

    std::cout << "Successfully loaded plugin: " << info.name << " v" << info.version << std::endl;
    return true;
}

bool PluginManager::UnloadPlugin(const std::string& pluginName) {
    for (auto it = m_plugins.begin(); it != m_plugins.end(); ++it) {
        if (it->info.name == pluginName) {
            it->plugin->Shutdown();

#ifdef PLATFORM_WINDOWS
            FreeLibrary(static_cast<HMODULE>(it->libraryHandle));
#else
            dlclose(it->libraryHandle);
#endif

            m_plugins.erase(it);
            std::cout << "Unloaded plugin: " << pluginName << std::endl;
            return true;
        }
    }

    std::cerr << "Plugin not found: " << pluginName << std::endl;
    return false;
}

void PluginManager::LoadPluginsFromDirectory(const std::string& directory) {
    if (!std::filesystem::exists(directory)) {
        std::cerr << "Plugin directory does not exist: " << directory << std::endl;
        return;
    }

    std::string extension;
#ifdef PLATFORM_WINDOWS
    extension = ".dll";
#else
    extension = ".so";
#endif

    for (const auto& entry : std::filesystem::directory_iterator(directory)) {
        if (entry.is_regular_file() && entry.path().extension() == extension) {
            LoadPlugin(entry.path().string());
        }
    }
}

std::vector<PluginInfo> PluginManager::GetLoadedPlugins() const {
    std::vector<PluginInfo> plugins;
    for (const auto& handle : m_plugins) {
        plugins.push_back(handle.info);
    }
    return plugins;
}

IPlugin* PluginManager::GetPlugin(const std::string& name) const {
    for (const auto& handle : m_plugins) {
        if (handle.info.name == name) {
            return handle.plugin.get();
        }
    }
    return nullptr;
}

void PluginManager::OnApplicationStart() {
    for (auto& handle : m_plugins) {
        // Plugins can override this to perform startup tasks
    }
}

void PluginManager::OnApplicationShutdown() {
    for (auto& handle : m_plugins) {
        handle.plugin->Shutdown();
    }
}

} // namespace Plugins
} // namespace DiagIDE
