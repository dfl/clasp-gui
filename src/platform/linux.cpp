// Platform-specific WebView embedding for Linux (X11)
#if !defined(__APPLE__) && !defined(_WIN32)

#include <cstdint>
#include <X11/Xlib.h>
#include <X11/keysym.h>
#include <X11/extensions/XTest.h>

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

void simulateDevToolsShortcut() {
    // Simulate Ctrl+Shift+I to open dev tools (UNTESTED)
    Display* display = XOpenDisplay(NULL);
    if (!display) return;

    KeyCode keyI = XKeysymToKeycode(display, XK_i);
    KeyCode keyCtrl = XKeysymToKeycode(display, XK_Control_L);
    KeyCode keyShift = XKeysymToKeycode(display, XK_Shift_L);

    // Press modifiers
    XTestFakeKeyEvent(display, keyCtrl, True, 0);
    XTestFakeKeyEvent(display, keyShift, True, 0);

    // Press and release I
    XTestFakeKeyEvent(display, keyI, True, 0);
    XTestFakeKeyEvent(display, keyI, False, 0);

    // Release modifiers
    XTestFakeKeyEvent(display, keyShift, False, 0);
    XTestFakeKeyEvent(display, keyCtrl, False, 0);

    XFlush(display);
    XCloseDisplay(display);
}

} // namespace platform
} // namespace clasp_gui

#endif // Linux
