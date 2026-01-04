#pragma once

/**
 * clasp.hpp - C++ helper for the clasp protocol
 *
 * Use with clasp.js on the JavaScript side to establish bidirectional
 * communication between your plugin C++ code and web UI.
 *
 * Usage:
 *   #include <clasp-gui/webview.h>
 *   #include <clasp-gui/clasp.hpp>
 *
 *   clasp_gui::WebView webview;
 *   clasp::Protocol proto(&webview);
 *
 *   proto.onCall("setParam", [](const std::string& args) {
 *       // Handle JS call
 *       return "ok";
 *   });
 *
 *   proto.queueParamChange(0, 0.5f);  // Thread-safe
 *   proto.processQueue();              // Call on UI thread
 */

#include "webview.h"

#include <array>
#include <chrono>
#include <functional>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

// Optional JSON parsing (for typed onCall handlers)
#if __has_include("choc/text/choc_JSON.h")
#include "choc/text/choc_JSON.h"
#define CLASP_HAS_JSON 1
#else
#define CLASP_HAS_JSON 0
#endif

namespace clasp {

/**
 * Protocol handler for clasp.js communication
 */
class Protocol {
public:
    using CallHandler = std::function<std::string(const std::string& argsJson)>;

    explicit Protocol(clasp_gui::WebView* webview)
        : webview_(webview) {
        lastParamUpdate_.fill(std::chrono::steady_clock::time_point{});
        setupBinding();
    }

    ~Protocol() = default;

    // Non-copyable
    Protocol(const Protocol&) = delete;
    Protocol& operator=(const Protocol&) = delete;

    /**
     * Register a function callable from JS via clasp.call()
     * Handler receives JSON array of arguments, returns JSON result
     */
    void onCall(const std::string& name, CallHandler handler) {
        std::lock_guard<std::mutex> lock(handlersMutex_);
        callHandlers_[name] = std::move(handler);
    }

    /**
     * Send a single parameter update to JS
     * Thread-safe - can be called from audio thread
     */
    void queueParamChange(int paramId, float value) {
        // Throttle updates per parameter
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

    /**
     * Queue a bulk parameter update (e.g., preset load)
     * Thread-safe
     */
    void queueBulkParamUpdate(const std::vector<std::pair<int, float>>& params) {
        std::lock_guard<std::mutex> lock(queueMutex_);
        pendingBulkParams_.insert(pendingBulkParams_.end(), params.begin(), params.end());
    }

    /**
     * Queue a MIDI note on event
     * Thread-safe
     */
    void queueNoteOn(int channel, int key, float velocity) {
        std::lock_guard<std::mutex> lock(queueMutex_);
        pendingNotes_.push_back({channel, key, velocity, true});
    }

    /**
     * Queue a MIDI note off event
     * Thread-safe
     */
    void queueNoteOff(int channel, int key) {
        std::lock_guard<std::mutex> lock(queueMutex_);
        pendingNotes_.push_back({channel, key, 0.0f, false});
    }

    /**
     * Queue a MIDI CC event
     * Thread-safe
     */
    void queueMidiCC(int channel, int cc, int value) {
        std::lock_guard<std::mutex> lock(queueMutex_);
        pendingCCs_.push_back({channel, cc, value});
    }

    /**
     * Process queued updates and send to JS
     * Must be called on UI/main thread
     */
    void processQueue() {
        if (!webview_) return;

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

        // Send individual param updates
        for (const auto& p : params) {
            sendToJs("param", "{\"id\":" + std::to_string(p.id) +
                              ",\"v\":" + std::to_string(p.value) + "}");
        }

        // Send bulk param updates
        if (!bulkParams.empty()) {
            std::ostringstream ss;
            ss << "{\"params\":[";
            for (size_t i = 0; i < bulkParams.size(); ++i) {
                if (i > 0) ss << ",";
                ss << "{\"id\":" << bulkParams[i].first
                   << ",\"v\":" << bulkParams[i].second << "}";
            }
            ss << "]}";
            sendToJs("params", ss.str());
        }

        // Send note events
        for (const auto& n : notes) {
            if (n.isNoteOn) {
                sendToJs("noteOn", "{\"ch\":" + std::to_string(n.channel) +
                                   ",\"k\":" + std::to_string(n.key) +
                                   ",\"vel\":" + std::to_string(n.velocity) + "}");
            } else {
                sendToJs("noteOff", "{\"ch\":" + std::to_string(n.channel) +
                                    ",\"k\":" + std::to_string(n.key) + "}");
            }
        }

        // Send CC events
        for (const auto& c : ccs) {
            sendToJs("midiCC", "{\"ch\":" + std::to_string(c.channel) +
                               ",\"cc\":" + std::to_string(c.cc) +
                               ",\"v\":" + std::to_string(c.value) + "}");
        }
    }

    /**
     * Send the ready signal to JS
     */
    void sendReady() {
        sendToJs("ready", "{}");
    }

    /**
     * Set update rate for parameter throttling
     * @param hz Updates per second (default: 60)
     */
    void setUpdateRateHz(int hz) {
        if (hz > 0 && hz <= 1000) {
            updateInterval_ = std::chrono::milliseconds(1000 / hz);
        }
    }

private:
    void setupBinding() {
        if (!webview_) return;

        // Register the __clasp binding for JS → C++ messages
        webview_->bind("__clasp", [this](const std::string& argsJson) -> std::string {
            return handleMessage(argsJson);
        });
    }

    std::string handleMessage(const std::string& argsJson) {
        // argsJson is a JSON array with one element (the message string)
        // e.g., ["{ \"t\": \"call\", \"fn\": \"foo\", \"args\": [], \"id\": 1 }"]

        std::string msgJson;

        // Extract the first element from the array
        // Simple parsing: find first string in array
        auto start = argsJson.find('"');
        if (start != std::string::npos) {
            start++;
            std::string result;
            for (size_t i = start; i < argsJson.size(); ++i) {
                char c = argsJson[i];
                if (c == '"') break;
                if (c == '\\' && i + 1 < argsJson.size()) {
                    char next = argsJson[i + 1];
                    if (next == '"' || next == '\\') {
                        result += next;
                        i++;
                        continue;
                    }
                }
                result += c;
            }
            msgJson = result;
        }

        if (msgJson.empty()) {
            return "{}";
        }

        // Parse the message type
        std::string msgType;
        auto tPos = msgJson.find("\"t\"");
        if (tPos != std::string::npos) {
            auto colonPos = msgJson.find(':', tPos);
            auto quoteStart = msgJson.find('"', colonPos);
            auto quoteEnd = msgJson.find('"', quoteStart + 1);
            if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                msgType = msgJson.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
            }
        }

        if (msgType == "call") {
            return handleCall(msgJson);
        } else if (msgType == "msg") {
            // Fire-and-forget message from JS
            // Could add a message callback here if needed
            return "{}";
        }

        return "{}";
    }

    std::string handleCall(const std::string& msgJson) {
        // Extract function name
        std::string fnName;
        auto fnPos = msgJson.find("\"fn\"");
        if (fnPos != std::string::npos) {
            auto colonPos = msgJson.find(':', fnPos);
            auto quoteStart = msgJson.find('"', colonPos);
            auto quoteEnd = msgJson.find('"', quoteStart + 1);
            if (quoteStart != std::string::npos && quoteEnd != std::string::npos) {
                fnName = msgJson.substr(quoteStart + 1, quoteEnd - quoteStart - 1);
            }
        }

        // Extract call ID
        int callId = 0;
        auto idPos = msgJson.find("\"id\"");
        if (idPos != std::string::npos) {
            auto colonPos = msgJson.find(':', idPos);
            if (colonPos != std::string::npos) {
                callId = std::atoi(msgJson.c_str() + colonPos + 1);
            }
        }

        // Extract args array
        std::string argsArray = "[]";
        auto argsPos = msgJson.find("\"args\"");
        if (argsPos != std::string::npos) {
            auto colonPos = msgJson.find(':', argsPos);
            auto bracketStart = msgJson.find('[', colonPos);
            if (bracketStart != std::string::npos) {
                int depth = 1;
                size_t bracketEnd = bracketStart + 1;
                while (bracketEnd < msgJson.size() && depth > 0) {
                    if (msgJson[bracketEnd] == '[') depth++;
                    else if (msgJson[bracketEnd] == ']') depth--;
                    bracketEnd++;
                }
                argsArray = msgJson.substr(bracketStart, bracketEnd - bracketStart);
            }
        }

        // Find and call handler
        std::string result;
        {
            std::lock_guard<std::mutex> lock(handlersMutex_);
            auto it = callHandlers_.find(fnName);
            if (it != callHandlers_.end()) {
                try {
                    result = it->second(argsArray);
                } catch (const std::exception& e) {
                    // Send error reply
                    sendReply(callId, "", e.what());
                    return "{}";
                }
            } else {
                sendReply(callId, "", "unknown function: " + fnName);
                return "{}";
            }
        }

        // Send reply
        sendReply(callId, result, "");
        return "{}";
    }

    void sendReply(int callId, const std::string& result, const std::string& error) {
        std::ostringstream ss;
        ss << "{\"t\":\"reply\",\"id\":" << callId;
        if (!error.empty()) {
            ss << ",\"error\":\"" << escapeJson(error) << "\"";
        } else {
            ss << ",\"result\":" << (result.empty() ? "null" : result);
        }
        ss << "}";

        std::string js = "__clasp_recv('" + escapeJs(ss.str()) + "');";
        webview_->evaluateScript(js);
    }

    void sendToJs(const std::string& type, const std::string& payload) {
        std::string msg = "{\"t\":\"" + type + "\"";
        // Merge payload object into message
        if (payload.size() > 2 && payload[0] == '{') {
            msg += "," + payload.substr(1, payload.size() - 2);
        }
        msg += "}";

        std::string js = "__clasp_recv('" + escapeJs(msg) + "');";
        webview_->evaluateScript(js);
    }

    static std::string escapeJs(const std::string& s) {
        std::string result;
        result.reserve(s.size());
        for (char c : s) {
            if (c == '\'') result += "\\'";
            else if (c == '\\') result += "\\\\";
            else if (c == '\n') result += "\\n";
            else if (c == '\r') result += "\\r";
            else result += c;
        }
        return result;
    }

    static std::string escapeJson(const std::string& s) {
        std::string result;
        result.reserve(s.size());
        for (char c : s) {
            if (c == '"') result += "\\\"";
            else if (c == '\\') result += "\\\\";
            else if (c == '\n') result += "\\n";
            else if (c == '\r') result += "\\r";
            else result += c;
        }
        return result;
    }

    // Queue structures
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

    clasp_gui::WebView* webview_;

    // Call handlers
    std::mutex handlersMutex_;
    std::unordered_map<std::string, CallHandler> callHandlers_;

    // Update queues
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

} // namespace clasp
