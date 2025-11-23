#include "core/Application.h"
#include <iostream>
#include <exception>

int main(int argc, char** argv) {
    try {
        DiagIDE::Core::Application app("Diagnostic IDE - Authorized Use Only", 1600, 900);

        if (!app.Initialize()) {
            std::cerr << "Failed to initialize application" << std::endl;
            return 1;
        }

        app.Run();
        app.Shutdown();

        return 0;
    }
    catch (const std::exception& ex) {
        std::cerr << "Fatal error: " << ex.what() << std::endl;
        return 1;
    }
}
