# Diagnostic IDE - Authorized Use Only

## ⚠️ Security Notice

This tool is designed for **authorized diagnostic and debugging purposes only**. All operations are logged and require explicit user consent. Unauthorized use or malicious deployment of this software may violate local, state, and federal laws.

### Legal & Ethical Requirements

- **NO unauthorized kernel-level memory access**
- **NO rootkit/driver creation**
- **NO privilege escalation**
- All memory/process inspection uses documented OS APIs
- Requires explicit user/admin consent for privileged operations
- All operations are logged in an audit trail

## Overview

Diagnostic IDE is a cross-platform ImGui-based desktop application that provides a compact, secure interface for developers and system administrators to:

- Inspect running processes
- Monitor system health and performance
- Perform authorized memory inspection on permitted targets
- Export diagnostic reports
- Maintain a complete audit trail of all operations

### Target Platforms

- **Windows** (Primary) - Windows 10/11
- **Linux** (Secondary) - Ubuntu 20.04+, Fedora, etc.

## Features

### Core Features (MVP)

✅ **Process Explorer**
- List all running processes with details (name, PID, path, user, CPU%, memory%)
- Searchable and sortable process table
- Real-time process monitoring

✅ **Authorized Process Inspection**
- Attach to processes using documented OS debug APIs (OpenProcess/ptrace)
- View thread information
- Enumerate loaded modules
- Display memory regions with permissions

✅ **Safe Memory Operations**
- Read memory from attached processes (with explicit consent)
- Write memory to attached processes (with explicit consent and logging)
- All operations require user confirmation dialogs

✅ **Diagnostics Dashboard**
- Real-time CPU usage graphs
- Memory usage monitoring
- Disk I/O statistics
- Network I/O statistics
- Configurable time windows

✅ **Hex Viewer & Export**
- View memory regions in hex format
- ASCII representation
- Export memory dumps to binary files
- All exports are logged

✅ **Module & Offset Information**
- List loaded modules for attached processes
- Display base addresses and sizes
- Export module information

✅ **Audit Trail & Logging**
- Complete audit log of all privileged operations
- Timestamp, user, operation type, and result
- Export audit logs to JSON or HTML
- Real-time audit log viewer

✅ **Plugin System**
- Simple plugin API for extensibility
- Load plugins from directory
- API version checking for compatibility

✅ **ImGui Interface**
- Dockable panels
- Persistent layout save/load
- Dark theme
- Multi-viewport support

## Building the Project

### Prerequisites

#### Linux
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential cmake git libgl1-mesa-dev libx11-dev libxrandr-dev libxinerama-dev libxcursor-dev libxi-dev

# Fedora
sudo dnf install -y gcc-c++ cmake git mesa-libGL-devel libX11-devel libXrandr-devel libXinerama-devel libXcursor-devel libXi-devel
```

#### Windows
- Visual Studio 2019 or later (with C++ development tools)
- CMake 3.15 or later
- Git

### Build Instructions

1. **Clone the repository**
   ```bash
   git clone <repository-url>
   cd FullProject
   ```

2. **Configure with CMake**
   ```bash
   cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
   ```

3. **Build the project**
   ```bash
   cmake --build build --config Release --parallel
   ```

4. **Run the application**
   ```bash
   # Linux
   ./build/src/DiagnosticIDE

   # Windows
   .\build\src\Release\DiagnosticIDE.exe
   ```

### VS Code Development

The project includes full VS Code integration:

1. **Open the project in VS Code**
   ```bash
   code .
   ```

2. **Install recommended extensions** (when prompted)
   - C/C++ (ms-vscode.cpptools)
   - CMake Tools (ms-vscode.cmake-tools)

3. **Build with VS Code**
   - Press `Ctrl+Shift+B` (or `Cmd+Shift+B` on macOS) to build
   - Or use the command palette: `Tasks: Run Build Task`

4. **Debug with VS Code**
   - Press `F5` to start debugging
   - Or use the command palette: `Debug: Start Debugging`

### Build Options

Configure build options with CMake:

```bash
# Disable tests
cmake -B build -DBUILD_TESTS=OFF

# Disable audit logging (not recommended)
cmake -B build -DENABLE_AUDIT_LOG=OFF

# Debug build
cmake -B build -DCMAKE_BUILD_TYPE=Debug
```

## Usage

### Starting the Application

```bash
./DiagnosticIDE
```

On first run, the application will:
- Create an audit log file: `diagnostic_ide.audit.log`
- Load default layout configuration
- Initialize all diagnostic panels

### Security Model

#### Permission Requirements

**Linux:**
- Attaching to processes owned by the same user: No special permissions
- Attaching to other users' processes: Requires `sudo` or `CAP_SYS_PTRACE` capability
- Reading/writing memory: Requires successful attachment

**Windows:**
- Attaching to processes in the same session: No special permissions
- Attaching to protected processes: Requires administrator privileges
- Some system processes cannot be attached even with admin rights

#### Consent Flow

1. All privileged operations display a consent dialog
2. User must explicitly approve the operation
3. Operation is logged in the audit trail with:
   - Timestamp
   - Username
   - Operation type
   - Target process
   - Success/failure status

### Example Workflows

#### Inspecting a Process

1. Open the **Process Explorer** panel
2. Search or scroll to find your target process
3. Right-click and select "Inspect Process"
4. In the **Memory Inspector** panel:
   - Click "Attach to Process"
   - Approve the consent dialog
   - View memory regions
   - Select a region to view in hex

#### Monitoring System Performance

1. Open the **Diagnostics Dashboard** panel
2. Adjust the time window and update interval as needed
3. Monitor real-time graphs for:
   - CPU usage
   - Memory usage
   - Disk I/O
   - Network I/O

#### Reviewing Audit Logs

1. Open the **Audit Log** panel
2. Use filters to narrow down by:
   - Severity level (Info, Warning, Critical, Security)
   - Search text
3. Export audit logs:
   - Click "Export to JSON" for machine-readable format
   - Click "Export to HTML" for human-readable report

## Architecture

### Project Structure

```
FullProject/
├── CMakeLists.txt              # Root CMake configuration
├── README.md                   # This file
├── docs/                       # Documentation
├── external/                   # External dependencies (ImGui, ImPlot, etc.)
│   └── CMakeLists.txt
├── include/                    # Public headers
│   ├── core/                   # Core application logic
│   ├── ui/                     # UI panels
│   ├── platform/               # Platform-specific abstractions
│   ├── plugins/                # Plugin system
│   └── utils/                  # Utility functions
├── src/                        # Implementation files
│   ├── core/
│   ├── ui/
│   ├── platform/
│   │   ├── linux/             # Linux-specific implementations
│   │   └── windows/           # Windows-specific implementations
│   ├── plugins/
│   ├── main.cpp
│   └── CMakeLists.txt
├── tests/                      # Unit tests
├── .vscode/                    # VS Code configuration
│   ├── launch.json
│   ├── tasks.json
│   └── settings.json
└── layout.ini                  # Saved UI layout (created at runtime)
```

### Key Components

#### Application Core
- `Application`: Main application class, manages lifecycle
- `AuditLogger`: Centralized audit logging system
- `ProcessInfo`: Data structures for process information
- `SystemMetrics`: Platform-specific system metrics collection

#### Platform Abstraction
- `ProcessManager`: Cross-platform process operations interface
  - `ProcessManager_Linux`: Linux implementation using procfs and ptrace
  - `ProcessManager_Windows`: Windows implementation using Win32 APIs

#### UI Panels
- `ProcessExplorer`: Process listing and management
- `MemoryInspector`: Memory viewing and editing with consent
- `DiagnosticsDashboard`: Real-time system metrics
- `AuditLogViewer`: Audit trail visualization

#### Plugin System
- `PluginManager`: Dynamic plugin loading and management
- `IPlugin`: Plugin interface for extensions

## Limitations (MVP)

### What This Tool Does NOT Do

❌ **No Kernel Mode Access**
- This MVP does not include kernel-mode drivers
- All operations use user-mode documented APIs
- If kernel-mode access is required, it must be:
  - Part of a separate, contractually approved project
  - Properly signed by Microsoft/vendor
  - Audited for security compliance

❌ **No Covert Operations**
- All operations are logged
- User consent is required
- No attempt to hide or disguise activities

❌ **No Unsigned Driver Loading**
- Does not bypass Windows driver signing requirements
- Does not load unsigned kernel modules

❌ **Limited Process Protection Bypass**
- Cannot attach to protected processes without proper privileges
- Respects OS security boundaries

## Testing

### Running Tests

```bash
# Build with tests enabled (default)
cmake -B build -DBUILD_TESTS=ON

# Build the project
cmake --build build

# Run all tests
ctest --test-dir build --output-on-failure
```

### Test Coverage

- Unit tests for core components
- Platform abstraction tests
- UI component tests
- Plugin system tests

## Contributing

### Code Style

- C++17 standard
- 4 spaces for indentation
- Class names: PascalCase
- Function names: PascalCase
- Variables: camelCase
- Constants: UPPER_SNAKE_CASE

### Adding New Features

1. Create feature branch
2. Implement with appropriate error handling
3. Add to audit logging if privileged operation
4. Write unit tests
5. Update documentation
6. Submit pull request

## Troubleshooting

### Build Issues

**Problem:** CMake cannot find OpenGL
```bash
# Linux: Install mesa development libraries
sudo apt-get install libgl1-mesa-dev

# Windows: Update graphics drivers
```

**Problem:** ImGui headers not found
```bash
# Clean build and reconfigure
rm -rf build
cmake -B build
```

### Runtime Issues

**Problem:** Cannot attach to process (Permission Denied)
- **Linux:** Run with `sudo` or set `CAP_SYS_PTRACE` capability
- **Windows:** Run as Administrator

**Problem:** Segmentation fault when reading memory
- Ensure the target process is still running
- Check that the memory region is readable
- Verify the address is valid

**Problem:** Plugin fails to load
- Check that plugin API version matches
- Verify plugin has correct entry points (`CreatePlugin`, `DestroyPlugin`)
- Check plugin dependencies

## Security Considerations

### Audit Trail

All security-relevant operations are logged to `diagnostic_ide.audit.log`:

```
[2025-11-23 10:30:45.123] [SECURITY] [Process] [user] Attempt to attach to process 'example' (PID: 1234) - SUCCESS
[2025-11-23 10:30:52.456] [SECURITY] [Memory] [user] Memory read from PID 1234 at 0x7f1234567890 size 256 bytes - SUCCESS
[2025-11-23 10:31:01.789] [INFO] [Export] [user] Exported Memory Dump to file: memory_dump.bin
```

### Best Practices

1. **Only attach to processes you own or have authorization to inspect**
2. **Review audit logs regularly**
3. **Export audit logs for compliance purposes**
4. **Use minimal necessary privileges**
5. **Understand your organization's security policies**

## License

[Insert appropriate license]

## Support

For issues, questions, or feature requests:
- Create an issue in the project repository
- Consult the documentation in `/docs`
- Review the audit logs for operational issues

## Acknowledgments

- **ImGui** - Immediate mode graphical user interface library
- **ImPlot** - Advanced plotting library for ImGui
- **GLFW** - Cross-platform window and input handling
- **nlohmann/json** - JSON for Modern C++

---

**Remember:** This tool is for authorized use only. All operations are logged and monitored. Use responsibly and ethically.
