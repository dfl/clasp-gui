// Platform-specific WebView embedding for Linux (X11)
#if !defined(__APPLE__) && !defined(_WIN32)

#include <X11/Xlib.h>

namespace clasp_gui {
namespace platform {

bool embedWebView(void* parent, void* webview, int width, int height) {
    if (!parent || !webview) return false;

    Display* display = XOpenDisplay(NULL);
    if (!display) return false;

    Window parentWindow = (Window)(uintptr_t)parent;
    Window webviewWindow = (Window)(uintptr_t)webview;

    XReparentWindow(display, webviewWindow, parentWindow, 0, 0);
    XMapWindow(display, webviewWindow);
    XMoveResizeWindow(display, webviewWindow, 0, 0, width, height);
    XFlush(display);
    XCloseDisplay(display);

    return true;
}

bool resizeWebView(void* webview, int width, int height) {
    if (!webview) return false;

    Display* display = XOpenDisplay(NULL);
    if (!display) return false;

    Window webviewWindow = (Window)(uintptr_t)webview;
    XMoveResizeWindow(display, webviewWindow, 0, 0, width, height);
    XFlush(display);
    XCloseDisplay(display);

    return true;
}

bool removeWebView(void* webview) {
    if (!webview) return false;

    Display* display = XOpenDisplay(NULL);
    if (!display) return false;

    Window webviewWindow = (Window)(uintptr_t)webview;
    XUnmapWindow(display, webviewWindow);
    XFlush(display);
    XCloseDisplay(display);

    return true;
}

void initPlatformFixes(void* webview) {
    // Linux-specific fixes could go here
}

} // namespace platform
} // namespace clasp_gui

#endif // Linux
