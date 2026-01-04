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

} // namespace platform
} // namespace clasp_gui

#endif // _WIN32
