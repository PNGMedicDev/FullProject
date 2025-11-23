# Source files
set(CORE_SOURCES
    core/Application.cpp
    core/AuditLogger.cpp
)

set(UI_SOURCES
    ui/ProcessExplorer.cpp
    ui/MemoryInspector.cpp
    ui/DiagnosticsDashboard.cpp
    ui/AuditLogViewer.cpp
)

set(PLATFORM_SOURCES
    platform/ProcessManager_Factory.cpp
)

set(PLUGIN_SOURCES
    plugins/PluginManager.cpp
)

# Platform-specific sources
if(PLATFORM_WINDOWS)
    list(APPEND PLATFORM_SOURCES
        platform/windows/ProcessManager_Windows.cpp
        platform/windows/SystemMetrics_Windows.cpp
    )
elseif(PLATFORM_LINUX)
    list(APPEND PLATFORM_SOURCES
        platform/linux/ProcessManager_Linux.cpp
        platform/linux/SystemMetrics_Linux.cpp
    )
endif()

# Main executable
add_executable(DiagnosticIDE
    main.cpp
    ${CORE_SOURCES}
    ${UI_SOURCES}
    ${PLATFORM_SOURCES}
    ${PLUGIN_SOURCES}
)

target_include_directories(DiagnosticIDE PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(DiagnosticIDE PRIVATE
    imgui
    implot
    glfw
    OpenGL::GL
    Threads::Threads
    nlohmann_json::nlohmann_json
)

# Platform-specific libraries
if(PLATFORM_WINDOWS)
    target_link_libraries(DiagnosticIDE PRIVATE
        psapi
        pdh
    )
elseif(PLATFORM_LINUX)
    target_link_libraries(DiagnosticIDE PRIVATE
        ${CMAKE_DL_LIBS}
    )
endif()

# Compiler warnings
if(MSVC)
    target_compile_options(DiagnosticIDE PRIVATE /W4)
else()
    target_compile_options(DiagnosticIDE PRIVATE -Wall -Wextra -Wpedantic)
endif()
