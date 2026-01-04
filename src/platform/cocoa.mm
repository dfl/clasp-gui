// Platform-specific WebView embedding for macOS
#if defined(__APPLE__)

#import <Cocoa/Cocoa.h>
#import <WebKit/WebKit.h>
#import <Carbon/Carbon.h> // For kVK_ANSI_I

namespace clasp_gui {
namespace platform {

bool embedWebView(void* parent, void* webview, int width, int height) {
    if (!parent || !webview) return false;

    NSView* parentView = (__bridge NSView*)parent;
    NSView* webviewView = (__bridge NSView*)webview;

    // Set frame to fill parent
    [webviewView setFrame:NSMakeRect(0, 0, width, height)];

    // Auto-resize with parent
    [webviewView setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];

    // Add as subview
    [parentView addSubview:webviewView];

    return true;
}

bool resizeWebView(void* webview, int width, int height) {
    if (!webview) return false;

    NSView* webviewView = (__bridge NSView*)webview;
    [webviewView setFrame:NSMakeRect(0, 0, width, height)];

    return true;
}

bool removeWebView(void* webview) {
    if (!webview) return false;

    NSView* webviewView = (__bridge NSView*)webview;
    [webviewView removeFromSuperview];

    return true;
}

void initPlatformFixes(void* webview) {
    if (!webview) return;

    // macOS-specific fixes could go here
    // e.g., handling Escape key crash, first responder issues
}

void simulateDevToolsShortcut() {
    // Simulate Cmd+Option+I to open dev tools (UNTESTED)
    // Creates a keyboard event for 'I' with Command+Option modifiers
    CGEventRef keyDown = CGEventCreateKeyboardEvent(NULL, kVK_ANSI_I, true);
    CGEventRef keyUp = CGEventCreateKeyboardEvent(NULL, kVK_ANSI_I, false);

    if (keyDown && keyUp) {
        CGEventFlags flags = kCGEventFlagMaskCommand | kCGEventFlagMaskAlternate;
        CGEventSetFlags(keyDown, flags);
        CGEventSetFlags(keyUp, flags);

        CGEventPost(kCGHIDEventTap, keyDown);
        CGEventPost(kCGHIDEventTap, keyUp);
    }

    if (keyDown) CFRelease(keyDown);
    if (keyUp) CFRelease(keyUp);
}

} // namespace platform
} // namespace clasp_gui

#endif // __APPLE__
