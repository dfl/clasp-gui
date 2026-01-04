#pragma once

#include <array>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

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
    bool enableDebugMode = false;      // Enable dev tools
    bool disableContextMenu = true;    // Disable right-click menu
    bool enablePointerCaptureFix = true; // Fix pointer capture without banner
    std::string initScript;            // Additional JS to inject on load
};

// Message from JS to native
struct JsMessage {
    std::string type;
    std::string payload;  // JSON string
};

// Forward declaration
class WebView;

// Callback types
using MessageCallback = std::function<void(const JsMessage&)>;
using ReadyCallback = std::function<void()>;

// Main WebView class
class WebView {
public:
    explicit WebView(const WebViewOptions& options = {});
    ~WebView();

    // Non-copyable, non-movable (due to mutex)
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

    // Bind a C++ function callable from JS
    // JS calls: window.clasp.call('name', arg1, arg2, ...)
    using BindingCallback = std::function<std::string(const std::string& argsJson)>;
    void bind(const std::string& name, BindingCallback callback);

    // Message handling
    void setMessageCallback(MessageCallback callback);
    void setReadyCallback(ReadyCallback callback);

    // Send message to JS (calls window.clasp.onMessage(type, payload))
    void postMessage(const std::string& type, const std::string& payload);

    // Thread-safe parameter updates (can call from audio thread)
    void queueParamUpdate(int paramId, float value);
    void queueBulkParamUpdate(const std::vector<std::pair<int, float>>& params);

    // MIDI event notifications (can call from audio thread)
    void queueNoteOn(int channel, int key, float velocity);
    void queueNoteOff(int channel, int key);
    void queueMidiCC(int channel, int cc, int value);

    // Process queued updates (call from main/UI thread)
    void processQueuedUpdates();

    // Throttle settings
    void setUpdateRateHz(int hz);  // Default: 60

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    WebViewOptions options_;
    MessageCallback messageCallback_;
    ReadyCallback readyCallback_;

    // Thread-safe update queues
    struct ParamUpdate {
        int id;
        float value;
    };
    struct NoteEvent {
        int channel;
        int key;
        float velocity;
        bool isNoteOn;
    };
    struct MidiCCEvent {
        int channel;
        int cc;
        int value;
    };

    std::mutex queueMutex_;
    std::vector<ParamUpdate> pendingParams_;
    std::vector<std::pair<int, float>> pendingBulkParams_;
    std::vector<NoteEvent> pendingNotes_;
    std::vector<MidiCCEvent> pendingCCs_;

    // Throttling
    static constexpr int MAX_PARAMS = 256;
    std::array<std::chrono::steady_clock::time_point, MAX_PARAMS> lastParamUpdate_;
    std::chrono::milliseconds updateInterval_{16};  // ~60Hz
};

} // namespace clasp_gui
