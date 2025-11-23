#include "core/Application.h"
#include <iostream>
#include <exception>

#ifdef _WIN32
#include <windows.h>
#endif

int AppMain() {
    try {
        DiagIDE::Core::Application app("Diagnostic IDE - Authorized Use Only", 1600, 900);

        if (!app.Initialize()) {
            #ifdef _WIN32
            MessageBoxA(NULL, "Failed to initialize application", "Error", MB_OK | MB_ICONERROR);
            #else
            std::cerr << "Failed to initialize application" << std::endl;
            #endif
            return 1;
        }

        app.Run();
        app.Shutdown();

        return 0;
    }
    catch (const std::exception& ex) {
        #ifdef _WIN32
        MessageBoxA(NULL, ex.what(), "Fatal Error", MB_OK | MB_ICONERROR);
        #else
        std::cerr << "Fatal error: " << ex.what() << std::endl;
        #endif
        return 1;
    }
}

#ifdef _WIN32
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    (void)hInstance;
    (void)hPrevInstance;
    (void)lpCmdLine;
    (void)nCmdShow;
    return AppMain();
}
#else
int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    return AppMain();
}
#endif
