#pragma once
// Windows keypress workaround
// Based on imagiro_webview by August Pemberton
//
// Problem: In some hosts, keyboard events don't properly reach the WebView
// when it's embedded as a plugin GUI. This workaround intercepts key events
// and re-sends them via Windows SendInput API.

#if defined(_WIN32)

#include <windows.h>
#include <functional>

namespace clasp_gui {
namespace fixes {

class WindowsKeypressWorkaround {
public:
    using KeyCallback = std::function<void(int keyCode, bool isDown)>;

    WindowsKeypressWorkaround() = default;
    ~WindowsKeypressWorkaround();

    // Initialize with the webview HWND and optionally a parent component HWND
    void initialize(HWND webviewHwnd, HWND parentHwnd = nullptr);

    // Called when webview receives a key event
    // Returns true if the event was handled (re-routed)
    bool onKeyDown(int keyCode);
    bool onKeyUp(int keyCode);

    // Enable/disable the workaround
    void setEnabled(bool enabled) { enabled_ = enabled; }
    bool isEnabled() const { return enabled_; }

private:
    void sendKeyInput(int keyCode, bool isKeyUp);

    HWND webviewHwnd_ = nullptr;
    HWND parentHwnd_ = nullptr;
    bool enabled_ = true;
};

// Implementation
inline WindowsKeypressWorkaround::~WindowsKeypressWorkaround() {
    // Cleanup if needed
}

inline void WindowsKeypressWorkaround::initialize(HWND webviewHwnd, HWND parentHwnd) {
    webviewHwnd_ = webviewHwnd;
    parentHwnd_ = parentHwnd;
}

inline bool WindowsKeypressWorkaround::onKeyDown(int keyCode) {
    if (!enabled_ || !parentHwnd_) return false;

    // Save current focus
    HWND previousFocus = GetFocus();

    // Set focus to parent component
    SetFocus(parentHwnd_);

    // Send the key input
    sendKeyInput(keyCode, false);

    // Restore focus after a short delay (async would be better)
    SetFocus(previousFocus);

    return true;
}

inline bool WindowsKeypressWorkaround::onKeyUp(int keyCode) {
    if (!enabled_ || !parentHwnd_) return false;

    HWND previousFocus = GetFocus();
    SetFocus(parentHwnd_);
    sendKeyInput(keyCode, true);
    SetFocus(previousFocus);

    return true;
}

inline void WindowsKeypressWorkaround::sendKeyInput(int keyCode, bool isKeyUp) {
    // Convert to virtual key code if needed
    SHORT vkResult = VkKeyScan(static_cast<WCHAR>(keyCode));
    UINT vk = LOBYTE(vkResult);

    // Get scan code
    UINT scanCode = MapVirtualKey(vk, MAPVK_VK_TO_VSC);

    // Build INPUT structure
    INPUT input = {};
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = 0;  // Use scan code instead
    input.ki.wScan = static_cast<WORD>(scanCode);
    input.ki.dwFlags = KEYEVENTF_SCANCODE;

    if (isKeyUp) {
        input.ki.dwFlags |= KEYEVENTF_KEYUP;
    }

    SendInput(1, &input, sizeof(INPUT));
}

} // namespace fixes
} // namespace clasp_gui

#endif // _WIN32
