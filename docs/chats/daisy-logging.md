Yeah — in this “compile → flash → reboot → USB serial” Daisy context, the idiomatic embedded approach is compile-time log gating via macros, often with an optional runtime level (a global variable) so you can turn verbosity up/down without rebuilding (if you have any control path, even a hardcoded “debug mode” button combo). Daisy/libDaisy gives you the transport (StartLog, PrintLine), but not a full level system out of the box.  ￼

Below is a pattern I’ve used for audio firmware: compile-time cap (zero cost for stripped logs) + runtime filter + “don’t log in audio callback” guidance.

⸻

Makefile: build-time level knobs

# 0=TRACE 1=DEBUG 2=INFO 3=WARN 4=ERROR 5=OFF
CFLAGS += -DLOG_COMPILETIME_LEVEL=1   # keep DEBUG+ in the binary (dev)
CFLAGS += -DLOG_DEFAULT_LEVEL=2       # default to INFO at runtime

# For release you might do:
# CFLAGS += -DLOG_COMPILETIME_LEVEL=3 -DLOG_DEFAULT_LEVEL=3


⸻

logging.h

#pragma once
#include <cstdarg>
#include <cstdint>

namespace logging {

enum Level : uint8_t {
    TRACE = 0,
    DEBUG = 1,
    INFO  = 2,
    WARN  = 3,
    ERROR = 4,
    OFF   = 5,
};

#ifndef LOG_COMPILETIME_LEVEL
#define LOG_COMPILETIME_LEVEL logging::INFO
#endif

#ifndef LOG_DEFAULT_LEVEL
#define LOG_DEFAULT_LEVEL logging::INFO
#endif

// Call once at boot (after hw.Init()).
void Init(bool wait_for_pc = true);

// Optional runtime control.
void SetLevel(Level lvl);
Level GetLevel();

// Internal printf-style sink.
void Print(Level lvl, const char* file, int line, const char* fmt, ...);

} // namespace logging

// ---------- Macros (compile-time + runtime gating) ----------
#define LOG_IMPL(LVL, ...)                                                     \
    do {                                                                       \
        if ((LOG_COMPILETIME_LEVEL) <= (LVL)) {                                \
            if (logging::GetLevel() <= (LVL)) {                                \
                logging::Print((LVL), __FILE__, __LINE__, __VA_ARGS__);        \
            }                                                                  \
        }                                                                      \
    } while (0)

#define LOGT(...) LOG_IMPL(logging::TRACE, __VA_ARGS__)
#define LOGD(...) LOG_IMPL(logging::DEBUG, __VA_ARGS__)
#define LOGI(...) LOG_IMPL(logging::INFO,  __VA_ARGS__)
#define LOGW(...) LOG_IMPL(logging::WARN,  __VA_ARGS__)
#define LOGE(...) LOG_IMPL(logging::ERROR, __VA_ARGS__)


⸻

logging.cpp (Patch.Init / Patch SM)

This uses libDaisy’s logger “wrappers” (StartLog, PrintLine) to send text over USB serial.  ￼

#include "logging.h"
#include "daisy_patch_sm.h"
#include <cstdio>

using namespace daisy;
using namespace daisy::patch_sm;

namespace logging {

static volatile Level g_level = static_cast<Level>(LOG_DEFAULT_LEVEL);

static const char* LevelName(Level l) {
    switch(l) {
        case TRACE: return "TRACE";
        case DEBUG: return "DEBUG";
        case INFO:  return "INFO";
        case WARN:  return "WARN";
        case ERROR: return "ERROR";
        default:    return "OFF";
    }
}

void Init(bool wait_for_pc) {
    // Wait-for-pc is useful so you don’t miss early boot messages.
    DaisyPatchSM::StartLog(wait_for_pc); // USB serial logger  [oai_citation:2‡electro-smith.github.io](https://electro-smith.github.io/libDaisy/classdaisy_1_1_logger.html?utm_source=chatgpt.com)
    DaisyPatchSM::PrintLine("log init (compile=%d default=%d)",
                            (int)LOG_COMPILETIME_LEVEL, (int)LOG_DEFAULT_LEVEL);
}

void SetLevel(Level lvl) { g_level = lvl; }
Level GetLevel() { return g_level; }

void Print(Level lvl, const char* file, int line, const char* fmt, ...) {
    // Keep it small to avoid heap/iostreams.
    char msg[192];

    va_list va;
    va_start(va, fmt);
    vsnprintf(msg, sizeof(msg), fmt, va);
    va_end(va);

    // Include file:line so you can grep logs later.
    DaisyPatchSM::PrintLine("[%s] %s:%d %s", LevelName(lvl), file, line, msg);
}

} // namespace logging


⸻

Usage pattern in your app

#include "daisy_patch_sm.h"
#include "logging.h"

using namespace daisy;
using namespace daisy::patch_sm;

DaisyPatchSM hw;

int main() {
    hw.Init();
    logging::Init(true);          // wait for host to open serial  [oai_citation:3‡daisy.audio](https://daisy.audio/tutorials/_a2_Getting-Started-Serial-Printing/?utm_source=chatgpt.com)

    LOGI("boot");
    LOGD("debug build marker");

    // IMPORTANT: don’t PrintLine from the audio callback. Log from main loop,
    // or buffer events and flush from main.
    while(true) {
        System::Delay(500);
        LOGI("heartbeat");
    }
}


⸻

What’s “best practice” for audio firmware here?
	•	Compile-time cap (your idea) is the standard embedded move: lets you ship with WARN/ERROR and keep dev builds chatty.
	•	Add runtime filter as a cheap extra reminder: even if DEBUG is compiled in, you can keep it quiet most of the time.
	•	Avoid logging directly in the audio callback; if you need that data, push lightweight events into a ring buffer and print them from the main loop.

If you want, paste one of your audio callback signatures and I’ll show the “ring-buffer event logging” variant that’s safe for real-time audio (no vsnprintf in the callback).