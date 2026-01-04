#include "clasp-gui/clap/gui_helper.h"
#include "clasp-gui/platform.h"
#include <cstring>

namespace clasp_gui {
namespace clap {

GuiHelper::GuiHelper() = default;
GuiHelper::~GuiHelper() = default;

void GuiHelper::setWebView(WebView* webview) {
    webview_ = webview;
}

bool GuiHelper::isApiSupported(const char* api, bool isFloating) const {
    // We don't support floating windows
    if (isFloating) return false;

    if (api == nullptr) return true;  // Just checking if GUI exists

#if defined(__APPLE__)
    return strcmp(api, CLAP_WINDOW_API_COCOA) == 0;
#elif defined(_WIN32)
    return strcmp(api, CLAP_WINDOW_API_WIN32) == 0;
#else
    return strcmp(api, CLAP_WINDOW_API_X11) == 0;
#endif
}

bool GuiHelper::getPreferredApi(const char** api, bool* isFloating) const {
#if defined(__APPLE__)
    *api = CLAP_WINDOW_API_COCOA;
#elif defined(_WIN32)
    *api = CLAP_WINDOW_API_WIN32;
#else
    *api = CLAP_WINDOW_API_X11;
#endif
    *isFloating = false;
    return true;
}

bool GuiHelper::create(const char* api, bool isFloating) {
    if (!isApiSupported(api, isFloating)) return false;
    if (!webview_) return false;

    return webview_->create();
}

void GuiHelper::destroy() {
    if (webview_) {
        webview_->destroy();
    }
    visible_ = false;
}

bool GuiHelper::setScale(double scale) {
    scale_ = scale;
    return true;
}

bool GuiHelper::getSize(uint32_t* width, uint32_t* height) const {
    *width = static_cast<uint32_t>(width_ * scale_);
    *height = static_cast<uint32_t>(height_ * scale_);
    return true;
}

bool GuiHelper::canResize() const {
    return resizable_;
}

bool GuiHelper::getResizeHints(clap_gui_resize_hints_t* hints) const {
    hints->can_resize_horizontally = resizable_;
    hints->can_resize_vertically = resizable_;
    hints->preserve_aspect_ratio = false;
    hints->aspect_ratio_width = 1;
    hints->aspect_ratio_height = 1;
    return true;
}

bool GuiHelper::adjustSize(uint32_t* width, uint32_t* height) const {
    // Clamp to min/max
    if (*width < minWidth_) *width = minWidth_;
    if (*height < minHeight_) *height = minHeight_;
    if (*width > maxWidth_) *width = maxWidth_;
    if (*height > maxHeight_) *height = maxHeight_;
    return true;
}

bool GuiHelper::setSize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;

    if (webview_) {
        webview_->setSize(width, height);
    }

    return true;
}

bool GuiHelper::setParent(const clap_window_t* window) {
    if (!webview_ || !window) return false;

    NativeWindow native;

#if defined(__APPLE__)
    if (window->api && strcmp(window->api, CLAP_WINDOW_API_COCOA) == 0) {
        native.api = WindowApi::Cocoa;
        native.handle = window->cocoa;
    }
#elif defined(_WIN32)
    if (window->api && strcmp(window->api, CLAP_WINDOW_API_WIN32) == 0) {
        native.api = WindowApi::Win32;
        native.handle = window->win32;
    }
#else
    if (window->api && strcmp(window->api, CLAP_WINDOW_API_X11) == 0) {
        native.api = WindowApi::X11;
        native.handle = reinterpret_cast<void*>(window->x11);
    }
#endif

    if (!native.handle) return false;

    webview_->setParent(native);
    webview_->setSize(width_, height_);

    return true;
}

bool GuiHelper::setTransient(const clap_window_t* window) {
    // Not supported for embedded views
    return false;
}

void GuiHelper::suggestTitle(const char* title) {
    // Not applicable for embedded views
}

bool GuiHelper::show() {
    visible_ = true;
    if (webview_) {
        return webview_->show();
    }
    return false;
}

bool GuiHelper::hide() {
    visible_ = false;
    if (webview_) {
        return webview_->hide();
    }
    return false;
}

void GuiHelper::setDefaultSize(uint32_t width, uint32_t height) {
    width_ = width;
    height_ = height;
}

void GuiHelper::setMinSize(uint32_t width, uint32_t height) {
    minWidth_ = width;
    minHeight_ = height;
}

void GuiHelper::setMaxSize(uint32_t width, uint32_t height) {
    maxWidth_ = width;
    maxHeight_ = height;
}

void GuiHelper::setResizable(bool canResize) {
    resizable_ = canResize;
}

// Static CLAP GUI extension struct
static const clap_plugin_gui_t s_clapGui = {
    detail::gui_is_api_supported,
    detail::gui_get_preferred_api,
    detail::gui_create,
    detail::gui_destroy,
    detail::gui_set_scale,
    detail::gui_get_size,
    detail::gui_can_resize,
    detail::gui_get_resize_hints,
    detail::gui_adjust_size,
    detail::gui_set_size,
    detail::gui_set_parent,
    detail::gui_set_transient,
    detail::gui_suggest_title,
    detail::gui_show,
    detail::gui_hide
};

const clap_plugin_gui_t* GuiHelper::getClapGui() {
    return &s_clapGui;
}

// Note: The detail:: callbacks need to be implemented by the user
// since they require access to the plugin instance to get the GuiHelper.
// Example implementation:
//
// GuiHelper* getGuiHelper(const clap_plugin_t* plugin) {
//     return static_cast<MyPlugin*>(plugin->plugin_data)->guiHelper();
// }
//
// bool detail::gui_is_api_supported(const clap_plugin_t* p, const char* api, bool f) {
//     return getGuiHelper(p)->isApiSupported(api, f);
// }
// ... etc

} // namespace clap
} // namespace clasp_gui
