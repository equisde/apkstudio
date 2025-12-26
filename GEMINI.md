# APK Studio

APK Studio is an open-source, cross-platform IDE for reverse-engineering Android application packages (APKs). It is built with C++17 and the Qt6 framework.

## Project Overview

*   **Language:** C++17
*   **Framework:** Qt 6 (Core, Gui, Network, Widgets)
*   **Build System:** CMake (3.16+)
*   **Architecture:** Desktop Application (Windows, Linux, macOS)
*   **Key Features:**
    *   APK Decompile/Recompile/Sign/Install
    *   Code Editor (Smali, Java, XML, YAML) with syntax highlighting
    *   Hex Editor (`QHexView` integration)
    *   AI Assistant integration
    *   AntiSplit (merge split APKs)
    *   Image Viewer
*   **Dependencies:**
    *   **External Tools:** Java, Apktool, JADX, ADB, Uber APK Signer (can be automatically managed by the app).
    *   **Libraries:** `QHexView` (included as a submodule).

## Directory Structure

*   `sources/`: Contains all C++ source files (`.cpp` and `.h`).
*   `resources/`: Contains application assets (icons, themes, definitions).
*   `QHexView/`: Submodule for the hex editor component.
*   `.github/workflows/`: CI/CD configurations for GitHub Actions.
*   `CMakeLists.txt`: Main build configuration file.

## Building and Running

### Requirements
*   **CMake:** 3.16 or higher
*   **Qt6:** 6.10.1 or higher
*   **Compiler:** C++17 compatible (MSVC, GCC, Clang)
*   **Git:** Required for version information generation.

### Build Steps

1.  **Configure:**
    ```bash
    cmake -B build -S . -DCMAKE_BUILD_TYPE=Release
    ```
    *Note: On macOS, you may need to provide `CMAKE_PREFIX_PATH` to your Qt installation.*

2.  **Build:**
    ```bash
    cmake --build build --config Release
    ```

3.  **Run:**
    *   **Windows:** `build\bin\Release\ApkStudio.exe`
    *   **Linux/macOS:** `build/bin/ApkStudio`

### Deployment (Optional)
*   **Windows:** Use `windeployqt` to bundle dependencies.
*   **Linux:** Uses `linuxdeploy` and `linuxdeploy-plugin-qt`.
*   **macOS:** Uses `macdeployqt`.

## Development Conventions

*   **Coding Style:** Follows standard Qt coding conventions.
    *   Classes: `PascalCase` (e.g., `MainWindow`, `AISettingsWidget`)
    *   Methods/Variables: `camelCase` (e.g., `setWindowIcon`, `dark_theme`)
    *   Logging: Use `qDebug()`, `qInfo()`, `qWarning()`, etc. Custom message handler `myMessageOutput` in `main.cpp` handles formatting.
*   **Resources:** access resources using the Qt resource system (e.g., `:/images/icon.png`).
*   **Submodules:** The project uses Git submodules. Ensure they are initialized (`git submodule update --init --recursive`).
*   **Testing:** Currently, there are no automated unit tests. Validation is primarily manual.
