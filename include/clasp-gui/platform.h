#pragma once

#include "webview.h"

namespace clasp_gui {
namespace platform {

// Platform detection
constexpr bool isMacOS() {
#if defined(__APPLE__)
    return true;
#else
    return false;
#endif
}

constexpr bool isWindows() {
#if defined(_WIN32)
    return true;
#else
    return false;
#endif
}

constexpr bool isLinux() {
#if !defined(__APPLE__) && !defined(_WIN32)
    return true;
#else
    return false;
#endif
}

// Platform-specific embedding functions
// These are implemented in platform/*.cpp files

bool embedWebView(void* parent, void* webview, int width, int height);
bool resizeWebView(void* webview, int width, int height);
bool removeWebView(void* webview);

// Platform-specific fixes
void initPlatformFixes(void* webview);

// Simulate keyboard shortcut to open dev tools (UNTESTED)
// macOS: Cmd+Option+I, Windows: Ctrl+Shift+I, Linux: Ctrl+Shift+I
void simulateDevToolsShortcut();

} // namespace platform
} // namespace clasp_gui
