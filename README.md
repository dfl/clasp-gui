# clasp-gui

A lightweight, cross-platform WebView library for audio plugin GUIs.

## Features

- **Cross-platform**: macOS (Cocoa/WKWebView), Windows (WebView2), Linux (WebKitGTK)
- **Host-agnostic**: Plugins work in any CLAP host with webview support
- **Thread-safe**: Queue parameter updates from audio thread, process on UI thread
- **Decoupled**: `clasp.js` + `clasp.hpp` protocol layer is separate from raw WebView
- **CLAP integration**: Helper class implements `clap_plugin_gui_t` extension

## Architecture

```
Plugin HTML includes clasp.js
         ↓
Plugin C++ uses clasp::Protocol (clasp.hpp)
         ↓
Raw WebView (webview.h) - host-agnostic
```

The `WebView` class is a thin wrapper around platform webviews. The clasp protocol (`clasp.js` + `clasp.hpp`) provides the communication layer between your plugin's C++ code and web UI.

## Usage

### C++ Side

```cpp
#include <clasp-gui/webview.h>
#include <clasp-gui/clasp.hpp>

// Create a raw webview
clasp_gui::WebViewOptions options;
options.enableDebugMode = true;

clasp_gui::WebView webview(options);
webview.create();

// Wrap with clasp protocol
clasp::Protocol proto(&webview);

// Register functions callable from JS
proto.onCall("setParam", [this](const std::string& argsJson) {
    // argsJson is a JSON array, e.g., "[0, 0.5]"
    // Parse and handle...
    return "\"ok\"";  // Return JSON
});

// From audio thread - queue updates (thread-safe)
proto.queueParamChange(0, 0.5f);
proto.queueNoteOn(0, 60, 0.8f);

// On UI thread - send queued updates to JS
proto.processQueue();

// Set parent window and navigate
clasp_gui::NativeWindow parent;
parent.api = clasp_gui::WindowApi::Cocoa;
parent.handle = parentNSView;
webview.setParent(parent);
webview.setSize(800, 600);
webview.navigate("file:///path/to/ui/index.html");
```

### JavaScript Side

Include `clasp.js` in your plugin's HTML:

```html
<!DOCTYPE html>
<html>
<head>
    <script src="clasp.js"></script>
</head>
<body>
    <script>
        // Subscribe to events from C++
        clasp.on('paramChange', (id, value) => {
            console.log(`Param ${id} = ${value}`);
            updateKnob(id, value);
        });

        clasp.on('noteOn', (channel, key, velocity) => {
            highlightKey(key);
        });

        clasp.on('ready', () => {
            console.log('Clasp protocol ready');
        });

        // Call C++ functions (returns Promise)
        clasp.call('setParam', 0, 0.75).then(result => {
            console.log('Result:', result);
        });

        // Fire-and-forget message
        clasp.send('myEvent', { data: 'hello' });

        // Drag handling (no pointer capture banner)
        knob.addEventListener('mousedown', () => {
            clasp.startDrag(
                (x, y, dx, dy) => rotateKnob(dy),
                () => commitValue()
            );
        });

        // Optional: disable context menu
        clasp.disableContextMenu();
    </script>
</body>
</html>
```

## JavaScript API

### Events (C++ → JS)

| Event | Arguments | Description |
|-------|-----------|-------------|
| `paramChange` | `(id, value)` | Single parameter update |
| `paramsSync` | `(params[])` | Bulk sync `[{id, v}, ...]` |
| `noteOn` | `(channel, key, velocity)` | MIDI note on |
| `noteOff` | `(channel, key)` | MIDI note off |
| `midiCC` | `(channel, cc, value)` | MIDI CC |
| `ready` | `()` | Protocol initialized |

### Methods

| Method | Description |
|--------|-------------|
| `clasp.on(event, handler)` | Subscribe to an event |
| `clasp.off(event, handler)` | Unsubscribe from an event |
| `clasp.call(name, ...args)` | Call C++ function, returns Promise |
| `clasp.send(type, payload)` | Send message to C++ (fire-and-forget) |
| `clasp.startDrag(onMove, onEnd)` | Start drag operation |
| `clasp.endDrag()` | End drag operation |
| `clasp.disableContextMenu()` | Disable browser context menu |

## TypeScript

TypeScript definitions are provided in `js/clasp.d.ts`.

## CLAP Integration

```cpp
#include <clasp-gui/clap/gui_helper.h>

class MyPlugin {
    clasp_gui::WebView webview_;
    clasp::Protocol proto_{&webview_};
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

## Files

| File | Description |
|------|-------------|
| `include/clasp-gui/webview.h` | Raw WebView wrapper |
| `include/clasp-gui/clasp.hpp` | C++ protocol helper (header-only) |
| `js/clasp.js` | JavaScript protocol library |
| `js/clasp.d.ts` | TypeScript definitions |

## Credits

- [CHOC](https://github.com/Tracktion/choc) - Cross-platform WebView
- [webview-gui](https://github.com/geraintluff/webview-gui) - Platform abstraction patterns
- [imagiro_webview](https://github.com/augustpemberton/imagiro_webview) - Windows keypress workaround

## License

MIT
