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

namespace clasp_gui {

#if CLASP_GUI_HAS_CHOC

struct WebView::Impl {
    std::unique_ptr<choc::ui::WebView> webview;
    void* parentWindow = nullptr;
    bool created = false;
};

WebView::WebView(const WebViewOptions& options)
    : impl_(std::make_unique<Impl>()), options_(options) {
}

WebView::~WebView() {
    destroy();
}

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
    opts.enableDebugInspector = options_.openDevToolsOnStart;

    impl_->webview = std::make_unique<choc::ui::WebView>(opts);
    if (!impl_->webview) return false;

    // Inject context menu disabling script if requested
    if (options_.disableContextMenu) {
        impl_->webview->addInitScript(
            "document.addEventListener('contextmenu', e => e.preventDefault());"
        );
    }

    // Inject user's init script if provided
    if (!options_.initScript.empty()) {
        impl_->webview->addInitScript(options_.initScript);
    }

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

#else // No CHOC

struct WebView::Impl {
    void* parentWindow = nullptr;
    bool created = false;
};

WebView::WebView(const WebViewOptions& options)
    : impl_(std::make_unique<Impl>()), options_(options) {}

WebView::~WebView() = default;

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

#endif // CLASP_GUI_HAS_CHOC

} // namespace clasp_gui
