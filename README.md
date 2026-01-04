# clasp-gui

A lightweight, thread-safe WebView library for audio plugin GUIs.

## Features

- **Cross-platform**: macOS (Cocoa/WKWebView), Windows (WebView2), Linux (WebKitGTK)
- **Thread-safe**: Queue parameter updates from audio thread, process on UI thread
- **Throttled updates**: Configurable rate limiting (default 60Hz) prevents UI flooding
- **CLAP integration**: Helper class implements `clap_plugin_gui_t` extension
- **Platform fixes**: Windows keypress workaround, pointer capture without banners, context menu handling
- **Minimal dependencies**: CHOC optional (provides WebView on Windows/Linux)

## Usage

```cpp
#include <clasp-gui/webview.h>

// Create webview with options
clasp_gui::WebViewOptions options;
options.enableDebugMode = true;  // Enable dev tools
options.disableContextMenu = true;

clasp_gui::WebView webview(options);
webview.create();

// Set parent window and size
clasp_gui::NativeWindow parent;
parent.api = clasp_gui::WindowApi::Cocoa;
parent.handle = parentNSView;
webview.setParent(parent);
webview.setSize(800, 600);

// Navigate to UI
webview.navigate("file:///path/to/ui/index.html");

// Queue updates from audio thread (thread-safe)
webview.queueParamUpdate(0, 0.5f);
webview.queueNoteOn(0, 60, 0.8f);

// Process on UI thread
webview.processQueuedUpdates();
```

## JavaScript API

clasp-gui injects a `window.clasp` object:

```javascript
// Receive parameter updates
window.clasp.onParamChange = function(id, value) {
    updateKnob(id, value);
};

// Receive bulk updates (e.g., preset load)
window.clasp.onParamsSync = function(params) {
    params.forEach(p => updateKnob(p.id, p.value));
};

// MIDI events
window.clasp.onNoteOn = function(channel, key, velocity) { };
window.clasp.onNoteOff = function(channel, key) { };
window.clasp.onMidiCC = function(channel, cc, value) { };

// Drag handling (avoids pointer capture banner)
element.addEventListener('mousedown', (e) => {
    window.clasp.startDrag(element,
        (x, y, dx, dy) => { /* move */ },
        () => { /* end */ }
    );
});
```

## CLAP Integration

```cpp
#include <clasp-gui/clap/gui_helper.h>

class MyPlugin {
    clasp_gui::WebView webview_;
    clasp_gui::clap::GuiHelper guiHelper_;

public:
    MyPlugin() {
        guiHelper_.setWebView(&webview_);
        guiHelper_.setDefaultSize(800, 600);
    }

    const void* getExtension(const char* id) {
        if (strcmp(id, CLAP_EXT_GUI) == 0)
            return clasp_gui::clap::GuiHelper::getClapGui();
        return nullptr;
    }
};
```

## Building

```bash
mkdir build && cd build
cmake .. -DCHOC_DIR=/path/to/choc
make
```

### Dependencies

- **CHOC** (optional): Provides WebView on Windows/Linux. On macOS, WKWebView is used directly.
- **CLAP**: Required for CLAP helper (header-only)

## Credits

- [CHOC](https://github.com/Tracktion/choc) - Cross-platform WebView
- [webview-gui](https://github.com/geraintluff/webview-gui) - Platform abstraction patterns
- [imagiro_webview](https://github.com/augustpemberton/imagiro_webview) - Windows keypress workaround

## License

MIT
