#include "logging.h"
#include <cstdio>
#include <cstdarg>
#include <ctime>

#ifdef HOST_BUILD
// Host-side implementation (for tests)
#include <iostream>
#include <chrono>
#else
// Hardware-side implementation
#include "daisy_patch_sm.h"
#endif

namespace daisysp_idm_grids
{
namespace logging
{

namespace
{
// Runtime log level filter (volatile to prevent optimization issues)
volatile Level currentLevel = static_cast<Level>(LOG_DEFAULT_LEVEL);
}

void Init(bool wait_for_pc)
{
#ifdef HOST_BUILD
    // Host-side: no-op (tests don't need hardware init)
    (void)wait_for_pc;
#else
    daisy::patch_sm::DaisyPatchSM::StartLog(wait_for_pc);
#endif
}

void SetLevel(Level lvl)
{
    currentLevel = lvl;
}

Level GetLevel()
{
    return currentLevel;
}

namespace
{
/**
 * Convert log level enum to human-readable string.
 */
const char* LevelName(Level lvl)
{
    switch(lvl)
    {
        case LOG_TRACE: return "TRACE";
        case LOG_DEBUG: return "DEBUG";
        case LOG_INFO: return "INFO";
        case LOG_WARN: return "WARN";
        case LOG_ERROR: return "ERROR";
        case LOG_OFF: return "OFF";
        default: return "UNKNOWN";
    }
}

/**
 * Extract just the filename from a full path.
 * Handles both Unix (/) and Windows (\) path separators.
 */
const char* ExtractFilename(const char* path)
{
    const char* filename = path;
    for(const char* p = path; *p != '\0'; ++p)
    {
        if(*p == '/' || *p == '\\')
        {
            filename = p + 1;
        }
    }
    return filename;
}
} // namespace

void Print(Level lvl, const char* file, int line, const char* fmt, ...)
{
    // Message buffer (192 chars minimum per spec, +64 for prefix = 256 total)
    char buffer[256];

#ifdef HOST_BUILD
    // Host-side: use system clock for timestamp
    auto now = std::chrono::system_clock::now();
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    uint32_t now_ms = static_cast<uint32_t>(ms);
#else
    // Hardware-side: use Daisy system clock
    uint32_t now_ms = daisy::System::GetNow();
#endif

    // Format: [timestamp_ms] [LEVEL] filename:line message
    const char* filename = ExtractFilename(file);
    int         prefix_len
        = snprintf(buffer, sizeof(buffer), "[%lu] [%s] %s:%d ",
                   static_cast<unsigned long>(now_ms), LevelName(lvl), filename, line);

    // Append user message (with variadic args)
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer + prefix_len, sizeof(buffer) - prefix_len, fmt, args);
    va_end(args);

#ifdef HOST_BUILD
    // Host-side: print to stderr (standard for logging)
    std::cerr << buffer << std::endl;
#else
    // Hardware-side: print via DaisyPatchSM logger (static method, no instance needed)
    daisy::patch_sm::DaisyPatchSM::PrintLine("%s", buffer);
#endif
}

} // namespace logging
} // namespace daisysp_idm_grids
