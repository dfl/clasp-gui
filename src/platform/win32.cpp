// Platform-specific WebView embedding for Windows
#if defined(_WIN32)

#include <windows.h>

namespace clasp_gui {
namespace platform {

bool embedWebView(void* parent, void* webview, int width, int height) {
    if (!parent || !webview) return false;

    HWND parentHwnd = (HWND)parent;
    HWND webviewHwnd = (HWND)webview;

    // Set parent and style
    SetParent(webviewHwnd, parentHwnd);
    SetWindowLong(webviewHwnd, GWL_STYLE, WS_CHILD | WS_VISIBLE);

    // Resize to fill parent
    MoveWindow(webviewHwnd, 0, 0, width, height, TRUE);

    return true;
}

bool resizeWebView(void* webview, int width, int height) {
    if (!webview) return false;

    HWND webviewHwnd = (HWND)webview;
    MoveWindow(webviewHwnd, 0, 0, width, height, TRUE);

    return true;
}

bool removeWebView(void* webview) {
    if (!webview) return false;

    HWND webviewHwnd = (HWND)webview;
    SetParent(webviewHwnd, NULL);

    return true;
}

void initPlatformFixes(void* webview) {
    // Windows keypress workaround will be initialized here
    // See fixes/keypress_win.cpp
}

void simulateDevToolsShortcut() {
    // Simulate Ctrl+Shift+I to open dev tools (UNTESTED)
    INPUT inputs[6] = {};

    // Press Ctrl
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_CONTROL;

    // Press Shift
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = VK_SHIFT;

    // Press I
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = 'I';

    // Release I
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = 'I';
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;

    // Release Shift
    inputs[4].type = INPUT_KEYBOARD;
    inputs[4].ki.wVk = VK_SHIFT;
    inputs[4].ki.dwFlags = KEYEVENTF_KEYUP;

    // Release Ctrl
    inputs[5].type = INPUT_KEYBOARD;
    inputs[5].ki.wVk = VK_CONTROL;
    inputs[5].ki.dwFlags = KEYEVENTF_KEYUP;

    SendInput(6, inputs, sizeof(INPUT));
}

} // namespace platform
} // namespace clasp_gui

#endif // _WIN32
