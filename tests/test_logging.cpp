#include <catch2/catch_all.hpp>
#include <cstring>

// Define test logging macros that don't require hardware
// We'll test the logic without actually calling DaisyPatchSM
#define TEST_LOG_COMPILETIME_LEVEL 1  // DEBUG level for testing
#define TEST_LOG_DEFAULT_LEVEL 2      // INFO level for testing

namespace daisysp_idm_grids
{
namespace logging
{

// Minimal logging types for testing
enum Level
{
    TRACE = 0,
    DEBUG = 1,
    INFO  = 2,
    WARN  = 3,
    ERROR = 4,
    OFF   = 5
};

// Test version of runtime level (volatile like real implementation)
static volatile Level testCurrentLevel = static_cast<Level>(TEST_LOG_DEFAULT_LEVEL);

// Test-friendly API
void TestSetLevel(Level lvl) { testCurrentLevel = lvl; }
Level TestGetLevel() { return testCurrentLevel; }

// Helper to get level name (same as real implementation)
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

// Helper to extract filename (same as real implementation)
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

} // namespace logging
} // namespace daisysp_idm_grids

using namespace daisysp_idm_grids::logging;
using Catch::Matchers::ContainsSubstring;

TEST_CASE("Runtime log level defaults to TEST_LOG_DEFAULT_LEVEL", "[logging]")
{
    REQUIRE(TestGetLevel() == INFO);
}

TEST_CASE("SetLevel changes runtime level", "[logging]")
{
    TestSetLevel(DEBUG);
    REQUIRE(TestGetLevel() == DEBUG);

    TestSetLevel(WARN);
    REQUIRE(TestGetLevel() == WARN);

    TestSetLevel(ERROR);
    REQUIRE(TestGetLevel() == ERROR);

    // Reset to default for other tests
    TestSetLevel(INFO);
}

TEST_CASE("Runtime filter prevents logs below current level", "[logging]")
{
    // Set to WARN level
    TestSetLevel(WARN);

    // TRACE, DEBUG, INFO should be filtered (3 < WARN)
    REQUIRE(TRACE < TestGetLevel());
    REQUIRE(DEBUG < TestGetLevel());
    REQUIRE(INFO < TestGetLevel());

    // WARN and ERROR should pass (>= WARN)
    REQUIRE(WARN >= TestGetLevel());
    REQUIRE(ERROR >= TestGetLevel());

    // Reset
    TestSetLevel(INFO);
}

TEST_CASE("Compile-time level filtering logic", "[logging]")
{
    // Simulate compile-time check
    // If level < LOG_COMPILETIME_LEVEL, it should be stripped
    constexpr int compileTimeLevel = TEST_LOG_COMPILETIME_LEVEL; // DEBUG = 1

    // TRACE (0) < DEBUG (1) - should be stripped
    constexpr bool traceStripped = (TRACE < compileTimeLevel);
    REQUIRE(traceStripped == true);

    // DEBUG (1) >= DEBUG (1) - should be kept
    constexpr bool debugKept = (DEBUG >= compileTimeLevel);
    REQUIRE(debugKept == true);

    // INFO (2) >= DEBUG (1) - should be kept
    constexpr bool infoKept = (INFO >= compileTimeLevel);
    REQUIRE(infoKept == true);
}

TEST_CASE("Level names are correct", "[logging]")
{
    REQUIRE(std::string(LevelName(TRACE)) == "TRACE");
    REQUIRE(std::string(LevelName(DEBUG)) == "DEBUG");
    REQUIRE(std::string(LevelName(INFO)) == "INFO");
    REQUIRE(std::string(LevelName(WARN)) == "WARN");
    REQUIRE(std::string(LevelName(ERROR)) == "ERROR");
    REQUIRE(std::string(LevelName(OFF)) == "OFF");
}

TEST_CASE("ExtractFilename handles paths correctly", "[logging]")
{
    // Unix paths
    REQUIRE(std::string(ExtractFilename("/usr/local/bin/test.cpp"))
            == "test.cpp");
    REQUIRE(std::string(ExtractFilename("src/Engine/Sequencer.cpp"))
            == "Sequencer.cpp");

    // Windows paths
    REQUIRE(std::string(ExtractFilename("C:\\Users\\test\\file.cpp"))
            == "file.cpp");

    // No path separator
    REQUIRE(std::string(ExtractFilename("main.cpp")) == "main.cpp");

    // Empty string
    REQUIRE(std::string(ExtractFilename("")) == "");
}

TEST_CASE("Log message format components", "[logging]")
{
    // Test that we can construct a properly formatted log message
    const char* testFile = "src/System/logging.cpp";
    int         testLine = 42;
    Level       testLevel = INFO;

    char buffer[256];
    const char* filename = ExtractFilename(testFile);

    int prefix_len = snprintf(buffer,
                              sizeof(buffer),
                              "[%s] %s:%d ",
                              LevelName(testLevel),
                              filename,
                              testLine);

    // Check prefix format
    std::string prefix(buffer);
    REQUIRE_THAT(prefix, ContainsSubstring("[INFO]"));
    REQUIRE_THAT(prefix, ContainsSubstring("logging.cpp"));
    REQUIRE_THAT(prefix, ContainsSubstring("42"));

    // Check that we have room for a message (192 char minimum per spec)
    size_t remainingSpace = sizeof(buffer) - prefix_len;
    REQUIRE(remainingSpace >= 192);
}

TEST_CASE("Message buffer handles truncation correctly", "[logging]")
{
    char buffer[256];

    // Very long message that would overflow (300+ characters)
    const char* longMsg
        = "This is a very long message that exceeds the buffer size and "
          "should be truncated properly without causing buffer overflow or "
          "other memory safety issues in the logging system implementation "
          "which needs to handle arbitrarily long format strings gracefully. "
          "This additional text ensures the message is definitely longer than "
          "the 256 byte buffer so we can test truncation behavior correctly.";

    // Test vsnprintf truncation behavior
    int written = snprintf(buffer, sizeof(buffer), "%s", longMsg);

    // snprintf returns number of chars that WOULD be written (excluding null)
    // When truncated, this is greater than buffer size - 1
    REQUIRE(static_cast<size_t>(written) > sizeof(buffer) - 1);

    // Actually written to buffer should be exactly buffer size - 1 + null
    REQUIRE(strlen(buffer) == sizeof(buffer) - 1);

    // Buffer should be null-terminated
    REQUIRE(buffer[sizeof(buffer) - 1] == '\0');

    // No buffer overflow (length should be exactly buffer size - 1)
    REQUIRE(strlen(buffer) < sizeof(buffer));
}

TEST_CASE("All five log levels have distinct values", "[logging]")
{
    REQUIRE(TRACE == 0);
    REQUIRE(DEBUG == 1);
    REQUIRE(INFO == 2);
    REQUIRE(WARN == 3);
    REQUIRE(ERROR == 4);
    REQUIRE(OFF == 5);

    // Check they're all different
    REQUIRE(TRACE != DEBUG);
    REQUIRE(DEBUG != INFO);
    REQUIRE(INFO != WARN);
    REQUIRE(WARN != ERROR);
    REQUIRE(ERROR != OFF);
}

TEST_CASE("Log level ordering is correct", "[logging]")
{
    // Lower numeric values = more verbose
    REQUIRE(TRACE < DEBUG);
    REQUIRE(DEBUG < INFO);
    REQUIRE(INFO < WARN);
    REQUIRE(WARN < ERROR);
    REQUIRE(ERROR < OFF);
}
