#include "clasp-gui/webview.h"
#include "clasp-gui/platform.h"

// CHOC WebView - optional dependency
#if __has_include("choc/gui/choc_WebView.h")
#include "choc/gui/choc_WebView.h"
#define CLASP_GUI_HAS_CHOC 1
#else
#define CLASP_GUI_HAS_CHOC 0
#endif

#if __has_include("choc/containers/choc_Value.h")
#include "choc/containers/choc_Value.h"
#include "choc/text/choc_JSON.h"
#define CLASP_GUI_HAS_CHOC_VALUE 1
#else
#define CLASP_GUI_HAS_CHOC_VALUE 0
#endif

#include <sstream>

namespace clasp_gui {

// Default JS API injected into every webview
static const char* DEFAULT_INIT_SCRIPT = R"(
window.clasp = window.clasp || {};

// Message handling
window.clasp._bindings = {};
window.clasp.call = function(name, ...args) {
    if (window.clasp._bindings[name]) {
        return window.clasp._bindings[name](JSON.stringify(args));
    }
    console.warn('clasp: unknown binding:', name);
    return null;
};

// Receive messages from native
window.clasp.onMessage = function(type, payload) {
    // Override this in your UI code
};

// Parameter updates from native
window.clasp.onParamChange = function(id, value) {
    // Override this in your UI code
};

window.clasp.onParamsSync = function(params) {
    // Override this for bulk updates
    // params = [{id, value}, ...]
    params.forEach(function(p) {
        window.clasp.onParamChange(p.id, p.value);
    });
};

// MIDI events from native
window.clasp.onNoteOn = function(channel, key, velocity) {
    // Override this in your UI code
};

window.clasp.onNoteOff = function(channel, key) {
    // Override this in your UI code
};

window.clasp.onMidiCC = function(channel, cc, value) {
    // Override this in your UI code
};

// Ready notification
window.clasp._ready = false;
window.clasp._onReady = function() {
    window.clasp._ready = true;
    if (window.clasp.onReady) {
        window.clasp.onReady();
    }
};

// Signal ready after DOM loaded
if (document.readyState === 'complete') {
    setTimeout(window.clasp._onReady, 0);
} else {
    window.addEventListener('load', window.clasp._onReady);
}
)";

// Context menu fix
static const char* CONTEXT_MENU_FIX = R"(
document.addEventListener('contextmenu', function(e) {
    if (window.clasp.onContextMenu) {
        e.preventDefault();
        window.clasp.onContextMenu(e.clientX, e.clientY, e.target);
    } else {
        e.preventDefault(); // Just disable by default
    }
});
)";

// Pointer capture workaround (avoids the banner)
static const char* POINTER_CAPTURE_FIX = R"(
(function() {
    // Track drag state without using pointer capture
    window.clasp._dragState = {
        active: false,
        element: null,
        startX: 0,
        startY: 0,
        onMove: null,
        onEnd: null
    };

    window.clasp.startDrag = function(element, onMove, onEnd) {
        window.clasp._dragState = {
            active: true,
            element: element,
            onMove: onMove,
            onEnd: onEnd
        };
        document.body.style.cursor = 'grabbing';
        document.body.style.userSelect = 'none';
    };

    window.clasp.endDrag = function() {
        if (window.clasp._dragState.active) {
            if (window.clasp._dragState.onEnd) {
                window.clasp._dragState.onEnd();
            }
            window.clasp._dragState.active = false;
            window.clasp._dragState.element = null;
            window.clasp._dragState.onMove = null;
            window.clasp._dragState.onEnd = null;
            document.body.style.cursor = '';
            document.body.style.userSelect = '';
        }
    };

    document.addEventListener('mousemove', function(e) {
        if (window.clasp._dragState.active && window.clasp._dragState.onMove) {
            window.clasp._dragState.onMove(e.clientX, e.clientY, e.movementX, e.movementY);
        }
    });

    document.addEventListener('mouseup', function(e) {
        window.clasp.endDrag();
    });

    // Handle mouse leaving the window
    document.addEventListener('mouseleave', function(e) {
        // Don't end drag when leaving - allow dragging outside window
    });
})();
)";

#if CLASP_GUI_HAS_CHOC

struct WebView::Impl {
    std::unique_ptr<choc::ui::WebView> webview;
    void* parentWindow = nullptr;
    bool created = false;
};

WebView::WebView(const WebViewOptions& options)
    : impl_(std::make_unique<Impl>()), options_(options) {
    lastParamUpdate_.fill(std::chrono::steady_clock::time_point{});
}

WebView::~WebView() {
    destroy();
}

WebView::WebView(WebView&& other) noexcept = default;
WebView& WebView::operator=(WebView&& other) noexcept = default;

bool WebView::isAvailable() {
    return true;
}

bool WebView::isApiSupported(WindowApi api) {
#if defined(__APPLE__)
    return api == WindowApi::Cocoa;
#elif defined(_WIN32)
    return api == WindowApi::Win32;
#else
    return api == WindowApi::X11;
#endif
}

WindowApi WebView::getPreferredApi() {
#if defined(__APPLE__)
    return WindowApi::Cocoa;
#elif defined(_WIN32)
    return WindowApi::Win32;
#else
    return WindowApi::X11;
#endif
}

bool WebView::create() {
    if (impl_->created) return true;

    choc::ui::WebView::Options opts;
    opts.enableDebugMode = options_.enableDebugMode;

    impl_->webview = std::make_unique<choc::ui::WebView>(opts);
    if (!impl_->webview) return false;

    // Inject default API
    std::string initScript = DEFAULT_INIT_SCRIPT;

    if (options_.disableContextMenu) {
        initScript += CONTEXT_MENU_FIX;
    }

    if (options_.enablePointerCaptureFix) {
        initScript += POINTER_CAPTURE_FIX;
    }

    if (!options_.initScript.empty()) {
        initScript += options_.initScript;
    }

    impl_->webview->addInitScript(initScript);
    impl_->created = true;

    return true;
}

void WebView::destroy() {
    if (impl_->webview) {
        auto handle = impl_->webview->getViewHandle();
        platform::removeWebView(handle);
        impl_->webview.reset();
    }
    impl_->parentWindow = nullptr;
    impl_->created = false;
}

bool WebView::isCreated() const {
    return impl_->created;
}

bool WebView::setParent(const NativeWindow& parent) {
    if (!impl_->webview || !parent.handle) return false;

    impl_->parentWindow = parent.handle;
    return true;
}

bool WebView::setSize(uint32_t width, uint32_t height) {
    if (!impl_->webview) return false;

    auto handle = impl_->webview->getViewHandle();

    if (impl_->parentWindow) {
        platform::embedWebView(impl_->parentWindow, handle, width, height);
    } else {
        platform::resizeWebView(handle, width, height);
    }

    return true;
}

bool WebView::show() {
    return impl_->created;
}

bool WebView::hide() {
    return impl_->created;
}

void* WebView::getNativeHandle() const {
    if (!impl_->webview) return nullptr;
    return impl_->webview->getViewHandle();
}

void WebView::navigate(const std::string& url) {
    if (impl_->webview) {
        impl_->webview->navigate(url);
    }
}

void WebView::loadHtml(const std::string& html) {
    if (impl_->webview) {
        impl_->webview->setHTML(html);
    }
}

void WebView::evaluateScript(const std::string& js) {
    if (impl_->webview) {
        impl_->webview->evaluateJavascript(js);
    }
}

void WebView::bind(const std::string& name, BindingCallback callback) {
    if (!impl_->webview) return;

#if CLASP_GUI_HAS_CHOC_VALUE
    impl_->webview->bind(name,
        [callback](const choc::value::ValueView& args) -> choc::value::Value {
            std::string result = callback(choc::json::toString(args));
            if (result.empty()) {
                return {};
            }
            try {
                return choc::json::parse(result);
            } catch (...) {
                return choc::value::createString(result);
            }
        });
#endif
}

void WebView::setMessageCallback(MessageCallback callback) {
    messageCallback_ = std::move(callback);
}

void WebView::setReadyCallback(ReadyCallback callback) {
    readyCallback_ = std::move(callback);
}

void WebView::postMessage(const std::string& type, const std::string& payload) {
    if (!impl_->webview) return;

    std::string js = "if (window.clasp && window.clasp.onMessage) { "
                     "window.clasp.onMessage('" + type + "', " + payload + "); }";
    impl_->webview->evaluateJavascript(js);
}

void WebView::queueParamUpdate(int paramId, float value) {
    // Throttle updates
    if (paramId >= 0 && paramId < MAX_PARAMS) {
        auto now = std::chrono::steady_clock::now();
        if (now - lastParamUpdate_[paramId] < updateInterval_) {
            return;
        }
        lastParamUpdate_[paramId] = now;
    }

    std::lock_guard<std::mutex> lock(queueMutex_);
    pendingParams_.push_back({paramId, value});
}

void WebView::queueBulkParamUpdate(const std::vector<std::pair<int, float>>& params) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    pendingBulkParams_.insert(pendingBulkParams_.end(), params.begin(), params.end());
}

void WebView::queueNoteOn(int channel, int key, float velocity) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    pendingNotes_.push_back({channel, key, velocity, true});
}

void WebView::queueNoteOff(int channel, int key) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    pendingNotes_.push_back({channel, key, 0.0f, false});
}

void WebView::queueMidiCC(int channel, int cc, int value) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    pendingCCs_.push_back({channel, cc, value});
}

void WebView::processQueuedUpdates() {
    if (!impl_->webview) return;

    std::vector<ParamUpdate> params;
    std::vector<std::pair<int, float>> bulkParams;
    std::vector<NoteEvent> notes;
    std::vector<MidiCCEvent> ccs;

    {
        std::lock_guard<std::mutex> lock(queueMutex_);
        params = std::move(pendingParams_);
        bulkParams = std::move(pendingBulkParams_);
        notes = std::move(pendingNotes_);
        ccs = std::move(pendingCCs_);
        pendingParams_.clear();
        pendingBulkParams_.clear();
        pendingNotes_.clear();
        pendingCCs_.clear();
    }

    // Process individual param updates
    for (const auto& p : params) {
        std::ostringstream js;
        js << "if (window.clasp && window.clasp.onParamChange) { "
           << "window.clasp.onParamChange(" << p.id << ", " << p.value << "); }";
        impl_->webview->evaluateJavascript(js.str());
    }

    // Process bulk param updates
    if (!bulkParams.empty()) {
        std::ostringstream js;
        js << "if (window.clasp && window.clasp.onParamsSync) { "
           << "window.clasp.onParamsSync([";
        for (size_t i = 0; i < bulkParams.size(); ++i) {
            if (i > 0) js << ",";
            js << "{id:" << bulkParams[i].first << ",value:" << bulkParams[i].second << "}";
        }
        js << "]); }";
        impl_->webview->evaluateJavascript(js.str());
    }

    // Process MIDI notes
    for (const auto& n : notes) {
        std::ostringstream js;
        if (n.isNoteOn) {
            js << "if (window.clasp && window.clasp.onNoteOn) { "
               << "window.clasp.onNoteOn(" << n.channel << ", " << n.key << ", " << n.velocity << "); }";
        } else {
            js << "if (window.clasp && window.clasp.onNoteOff) { "
               << "window.clasp.onNoteOff(" << n.channel << ", " << n.key << "); }";
        }
        impl_->webview->evaluateJavascript(js.str());
    }

    // Process MIDI CCs
    for (const auto& c : ccs) {
        std::ostringstream js;
        js << "if (window.clasp && window.clasp.onMidiCC) { "
           << "window.clasp.onMidiCC(" << c.channel << ", " << c.cc << ", " << c.value << "); }";
        impl_->webview->evaluateJavascript(js.str());
    }
}

void WebView::setUpdateRateHz(int hz) {
    if (hz > 0 && hz <= 1000) {
        updateInterval_ = std::chrono::milliseconds(1000 / hz);
    }
}

#else // No CHOC

struct WebView::Impl {
    void* parentWindow = nullptr;
    bool created = false;
};

WebView::WebView(const WebViewOptions& options)
    : impl_(std::make_unique<Impl>()), options_(options) {}

WebView::~WebView() = default;
WebView::WebView(WebView&&) noexcept = default;
WebView& WebView::operator=(WebView&&) noexcept = default;

bool WebView::isAvailable() { return false; }
bool WebView::isApiSupported(WindowApi) { return false; }
WindowApi WebView::getPreferredApi() { return WindowApi::Unknown; }
bool WebView::create() { return false; }
void WebView::destroy() {}
bool WebView::isCreated() const { return false; }
bool WebView::setParent(const NativeWindow&) { return false; }
bool WebView::setSize(uint32_t, uint32_t) { return false; }
bool WebView::show() { return false; }
bool WebView::hide() { return false; }
void* WebView::getNativeHandle() const { return nullptr; }
void WebView::navigate(const std::string&) {}
void WebView::loadHtml(const std::string&) {}
void WebView::evaluateScript(const std::string&) {}
void WebView::bind(const std::string&, BindingCallback) {}
void WebView::setMessageCallback(MessageCallback) {}
void WebView::setReadyCallback(ReadyCallback) {}
void WebView::postMessage(const std::string&, const std::string&) {}
void WebView::queueParamUpdate(int, float) {}
void WebView::queueBulkParamUpdate(const std::vector<std::pair<int, float>>&) {}
void WebView::queueNoteOn(int, int, float) {}
void WebView::queueNoteOff(int, int) {}
void WebView::queueMidiCC(int, int, int) {}
void WebView::processQueuedUpdates() {}
void WebView::setUpdateRateHz(int) {}

#endif // CLASP_GUI_HAS_CHOC

} // namespace clasp_gui
