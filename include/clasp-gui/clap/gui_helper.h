#pragma once

#include "../webview.h"
#include <clap/clap.h>

namespace clasp_gui {
namespace clap {

// Helper class that implements the CLAP GUI extension
// Derive from this or use it as a member
class GuiHelper {
public:
    GuiHelper();
    ~GuiHelper();

    // Set the webview instance to manage
    void setWebView(WebView* webview);
    WebView* getWebView() const { return webview_; }

    // CLAP gui extension implementation
    bool isApiSupported(const char* api, bool isFloating) const;
    bool getPreferredApi(const char** api, bool* isFloating) const;
    bool create(const char* api, bool isFloating);
    void destroy();
    bool setScale(double scale);
    bool getSize(uint32_t* width, uint32_t* height) const;
    bool canResize() const;
    bool getResizeHints(clap_gui_resize_hints_t* hints) const;
    bool adjustSize(uint32_t* width, uint32_t* height) const;
    bool setSize(uint32_t width, uint32_t height);
    bool setParent(const clap_window_t* window);
    bool setTransient(const clap_window_t* window);
    void suggestTitle(const char* title);
    bool show();
    bool hide();

    // Get the clap_plugin_gui_t struct for registration
    static const clap_plugin_gui_t* getClapGui();

    // Size management
    void setDefaultSize(uint32_t width, uint32_t height);
    void setMinSize(uint32_t width, uint32_t height);
    void setMaxSize(uint32_t width, uint32_t height);
    void setResizable(bool canResize);

private:
    WebView* webview_ = nullptr;
    uint32_t width_ = 800;
    uint32_t height_ = 600;
    uint32_t minWidth_ = 200;
    uint32_t minHeight_ = 150;
    uint32_t maxWidth_ = 4096;
    uint32_t maxHeight_ = 4096;
    double scale_ = 1.0;
    bool resizable_ = true;
    bool visible_ = false;
};

// Static callbacks for CLAP - you need to store a pointer to your GuiHelper
// in the plugin's user data or use a static map
namespace detail {
    bool gui_is_api_supported(const clap_plugin_t* plugin, const char* api, bool isFloating);
    bool gui_get_preferred_api(const clap_plugin_t* plugin, const char** api, bool* isFloating);
    bool gui_create(const clap_plugin_t* plugin, const char* api, bool isFloating);
    void gui_destroy(const clap_plugin_t* plugin);
    bool gui_set_scale(const clap_plugin_t* plugin, double scale);
    bool gui_get_size(const clap_plugin_t* plugin, uint32_t* width, uint32_t* height);
    bool gui_can_resize(const clap_plugin_t* plugin);
    bool gui_get_resize_hints(const clap_plugin_t* plugin, clap_gui_resize_hints_t* hints);
    bool gui_adjust_size(const clap_plugin_t* plugin, uint32_t* width, uint32_t* height);
    bool gui_set_size(const clap_plugin_t* plugin, uint32_t width, uint32_t height);
    bool gui_set_parent(const clap_plugin_t* plugin, const clap_window_t* window);
    bool gui_set_transient(const clap_plugin_t* plugin, const clap_window_t* window);
    void gui_suggest_title(const clap_plugin_t* plugin, const char* title);
    bool gui_show(const clap_plugin_t* plugin);
    bool gui_hide(const clap_plugin_t* plugin);
}

} // namespace clap
} // namespace clasp_gui
