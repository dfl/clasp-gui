#pragma once

#include <functional>
#include <memory>
#include <string>

namespace clasp_gui {

// Platform window API types
enum class WindowApi {
    Unknown,
    Cocoa,   // macOS NSView
    Win32,   // Windows HWND
    X11,     // Linux X11 Window
    Wayland  // Linux Wayland (future)
};

// Native window handle
struct NativeWindow {
    WindowApi api = WindowApi::Unknown;
    void* handle = nullptr;
};

// WebView options
struct WebViewOptions {
    bool enableDebugMode = false;      // Enable dev tools (right-click inspect, etc.)
    bool openDevToolsOnStart = false;  // Open dev tools inspector window on creation
    bool disableContextMenu = false;   // Disable right-click context menu
    std::string initScript;            // Additional JS to inject on load
};

// Forward declaration
class WebView;

// Main WebView class - a thin wrapper around the platform webview
class WebView {
public:
    explicit WebView(const WebViewOptions& options = {});
    ~WebView();

    // Non-copyable, non-movable
    WebView(const WebView&) = delete;
    WebView& operator=(const WebView&) = delete;
    WebView(WebView&&) = delete;
    WebView& operator=(WebView&&) = delete;

    // Check if webview is available on this platform
    static bool isAvailable();

    // Platform support
    static bool isApiSupported(WindowApi api);
    static WindowApi getPreferredApi();

    // Lifecycle
    bool create();
    void destroy();
    bool isCreated() const;

    // Window embedding
    bool setParent(const NativeWindow& parent);
    bool setSize(uint32_t width, uint32_t height);
    bool show();
    bool hide();

    // Get native handle (for platform-specific operations)
    void* getNativeHandle() const;

    // Navigation
    void navigate(const std::string& url);
    void loadHtml(const std::string& html);

    // JavaScript execution (fire-and-forget)
    void evaluateScript(const std::string& js);

    // Open developer tools (requires enableDebugMode = true)
    // Uses platform-specific keyboard simulation (UNTESTED)
    void openDevTools();

    // Bind a C++ function callable from JS
    // The function is exposed globally as window.<name>
    using BindingCallback = std::function<std::string(const std::string& argsJson)>;
    void bind(const std::string& name, BindingCallback callback);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    WebViewOptions options_;
};

} // namespace clasp_gui
