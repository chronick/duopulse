#pragma once

#include <cstdarg>

namespace daisysp_idm_grids
{
namespace logging
{

/**
 * Log levels for runtime logging system.
 * Lower numeric values = more verbose.
 *
 * Note: Prefixed with LOG_ to avoid conflicts with common macros (e.g., DEBUG)
 */
enum Level
{
    LOG_TRACE = 0, // Verbose debugging (per-step dumps, loop internals)
    LOG_DEBUG = 1, // Development info (bar generation, archetype selection)
    LOG_INFO  = 2, // Normal operation (boot, mode changes, config updates)
    LOG_WARN  = 3, // Warnings (constraint violations, soft repairs)
    LOG_ERROR = 4, // Critical issues (hardware init failures, invalid state)
    LOG_OFF   = 5  // Disable all logging
};

/**
 * Initialize the logging system.
 * Must be called after hardware initialization (hw.Init()).
 *
 * @param wait_for_pc If true, wait for host to connect before proceeding.
 *                    Prevents missing early boot messages. Default: true.
 */
void Init(bool wait_for_pc = true);

/**
 * Set the runtime log level filter.
 * Only logs at or above this level will be printed.
 *
 * @param lvl The minimum log level to display
 */
void SetLevel(Level lvl);

/**
 * Get the current runtime log level filter.
 *
 * @return The current minimum log level
 */
Level GetLevel();

/**
 * Print a formatted log message (internal, use macros instead).
 *
 * @param lvl Log level for this message
 * @param file Source file name (usually __FILE__)
 * @param line Source line number (usually __LINE__)
 * @param fmt Printf-style format string
 * @param ... Variable arguments for format string
 */
void Print(Level lvl, const char* file, int line, const char* fmt, ...);

} // namespace logging
} // namespace daisysp_idm_grids

//
// Compile-Time Configuration
//
// These defines control what gets compiled into the binary:
//
// LOG_COMPILETIME_LEVEL: Minimum level to compile in (0=TRACE, 5=OFF)
//   - Logs below this level are stripped at compile time (zero cost)
//   - Set in Makefile with -DLOG_COMPILETIME_LEVEL=N
//   - Default: DEBUG (1) if not specified
//
// LOG_DEFAULT_LEVEL: Initial runtime filter level
//   - Sets the default runtime level on boot
//   - Can be changed at runtime with logging::SetLevel()
//   - Default: INFO (2) if not specified
//

#ifndef LOG_COMPILETIME_LEVEL
#define LOG_COMPILETIME_LEVEL 1 // DEBUG: keep DEBUG+ logs by default
#endif

#ifndef LOG_DEFAULT_LEVEL
#define LOG_DEFAULT_LEVEL 2 // INFO: default runtime filter
#endif

//
// Logging Macros
//
// These macros provide compile-time and runtime gating:
// 1. Compile-time: If level < LOG_COMPILETIME_LEVEL, code is stripped
// 2. Runtime: Check current runtime level before calling Print()
//
// Usage:
//   LOGI("Boot complete");
//   LOGD("Selected archetype [%d,%d]", x, y);
//   LOGW("Guard rail triggered: %s", reason);
//

// Internal implementation macro (do not use directly)
#define LOG_IMPL(level, ...)                                                   \
    do                                                                         \
    {                                                                          \
        if(level >= LOG_COMPILETIME_LEVEL)                                    \
        {                                                                      \
            if(level >= ::daisysp_idm_grids::logging::GetLevel())             \
            {                                                                  \
                ::daisysp_idm_grids::logging::Print(                          \
                    static_cast<::daisysp_idm_grids::logging::Level>(level),  \
                    __FILE__,                                                  \
                    __LINE__,                                                  \
                    __VA_ARGS__);                                              \
            }                                                                  \
        }                                                                      \
    } while(0)

// Convenience macros for each log level
#define LOGT(...) LOG_IMPL(::daisysp_idm_grids::logging::LOG_TRACE, __VA_ARGS__)
#define LOGD(...) LOG_IMPL(::daisysp_idm_grids::logging::LOG_DEBUG, __VA_ARGS__)
#define LOGI(...) LOG_IMPL(::daisysp_idm_grids::logging::LOG_INFO, __VA_ARGS__)
#define LOGW(...) LOG_IMPL(::daisysp_idm_grids::logging::LOG_WARN, __VA_ARGS__)
#define LOGE(...) LOG_IMPL(::daisysp_idm_grids::logging::LOG_ERROR, __VA_ARGS__)
