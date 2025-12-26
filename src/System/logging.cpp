#include "logging.h"
#include "daisy_patch_sm.h"
#include <cstdio>

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
    daisy::DaisyPatchSM::StartLog(wait_for_pc);
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
        case TRACE: return "TRACE";
        case DEBUG: return "DEBUG";
        case INFO: return "INFO";
        case WARN: return "WARN";
        case ERROR: return "ERROR";
        case OFF: return "OFF";
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

    // Format: [LEVEL] filename:line message
    const char* filename = ExtractFilename(file);
    int         prefix_len
        = snprintf(buffer, sizeof(buffer), "[%s] %s:%d ", LevelName(lvl), filename, line);

    // Append user message (with variadic args)
    va_list args;
    va_start(args, fmt);
    vsnprintf(buffer + prefix_len, sizeof(buffer) - prefix_len, fmt, args);
    va_end(args);

    // Print via DaisyPatchSM logger (static method, no instance needed)
    daisy::DaisyPatchSM::PrintLine("%s", buffer);
}

} // namespace logging
} // namespace daisysp_idm_grids
