# Building Messina VST3 for Windows

This document explains how to build the Messina VST3 plugin for Windows 64-bit.

## Quick Start (Recommended)

### Option 1: GitHub Actions (Cloud Build - Easiest)

A GitHub Actions workflow is configured to automatically build the Windows VST3:

1. **Push the code to GitHub**:

   ```bash
   cd /Users/etienneolivier/Documents/GitHub/messina
   git add .
   git commit -m "Add Windows build configuration"
   git push origin main
   ```

2. **Go to GitHub Actions** in your repository, then:
   - Wait for the build to complete (5-10 minutes)
   - Download the `Messina-Windows-VST3-Package` artifact from the workflow run

3. **Install the plugin**:
   - Unzip the downloaded artifact
   - Copy `Messina.vst3` to:
     - `C:\Program Files\VST3\` (system-wide installation)
     - OR `C:\Users\<YourUser>\AppData\Roaming\VST3\` (user-specific)

4. **Use the plugin**:
   - Restart your DAW
   - Look for "Messina" in your VST3 plugins list

### Option 2: Build on Windows Machine (Direct)

If you have access to a Windows PC or VM:

1. **Install prerequisites**:
   - Visual Studio 2022 (with "Desktop development with C++" workload)
   - Node.js 18+: https://nodejs.org/
   - Python 3: https://python.org/
   - CMake 3.22+: https://cmake.org/download/
   - Git: https://git-scm.com/

2. **Clone the repository**:

   ```cmd
   git clone https://github.com/malcolmaudio/messina.git
   cd messina
   ```

3. **Run the build script**:

   ```cmd
   build-windows-vst3.bat
   ```

4. **Find the VST3** at `build\Messina_artefacts\VST3\Messina.vst3`

### Option 3: Cross-Compile from macOS (Experimental)

Cross-compilation from macOS to Windows has been attempted with mingw-w64, but JUCE has compatibility issues with mingw on macOS that cause build errors in JUCE's own code. This approach is not recommended for production builds.

The toolchain file is available at `cmake/toolchains/mingw-w64-x64.cmake` for reference.

## Requirements for End Users

The Windows VST3 plugin requires:

- Windows 10 or later (64-bit)
- A VST3-compatible DAW (Ableton Live, FL Studio, Cubase, Studio One, etc.)
- Microsoft Edge WebView2 Runtime (pre-installed on Windows 10/11)

If the plugin UI doesn't load, install WebView2 from:
https://developer.microsoft.com/en-us/microsoft-edge/webview2/

## Troubleshooting

### "Plugin not appearing in DAW"

- Restart your DAW
- Re-scan plugin locations in your DAW's settings
- Check the VST3 folder location matches your DAW's expectation

### "UI not loading"

- Install WebView2 Runtime (see link above)
- Check that the plugin .vst3 file is in the correct folder
- Try running the DAW as administrator

### Build errors

- Ensure Visual Studio 2022 is installed with C++ desktop workload
- Update CMake to 3.22 or later
- Make sure Node.js and Python are in your PATH

## Build Configuration

The CMake configuration is located in `CMakeLists.txt`. Key options:

- `FORMATS VST3 Standalone` - Builds VST3 and standalone application
- `JUCE_VST3_CAN_REPLACE_VST2=0` - Disables VST2/VST3 bridge
- `JUCE_WEB_BROWSER=1` - Enables web-based UI (required for the React UI)
- `JUCE_USE_CURL=0` - Disables CURL dependency

## Support

For issues, open an issue on the GitHub repository.
