/**
 * Weight Configuration Loader for pattern_viz
 *
 * Loads algorithm weight configuration from JSON files at runtime.
 * This allows rapid iteration without recompiling firmware.
 *
 * Note: This is HOST-ONLY code. The firmware uses generated headers
 * with constexpr values for zero runtime overhead.
 */

#pragma once

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cstdlib>

namespace daisysp_idm_grids {

/**
 * Weight configuration values loaded from JSON
 */
struct LoadedWeightConfig {
    // Metadata
    std::string version;
    std::string name;

    // Euclidean curve
    float euclideanFadeStart = 0.30f;
    float euclideanFadeEnd = 0.70f;

    // Per-channel k ranges
    int anchorKMin = 4;
    int anchorKMax = 12;
    int shimmerKMin = 6;
    int shimmerKMax = 16;
    int auxKMin = 2;
    int auxKMax = 8;

    // Syncopation curve
    float syncopationCenter = 0.50f;
    float syncopationWidth = 0.30f;

    // Random curve
    float randomFadeStart = 0.50f;
    float randomFadeEnd = 0.90f;

    bool isLoaded = false;
};

/**
 * Simple JSON value extraction helpers
 * (Using basic string parsing to avoid external dependencies)
 */
inline float extractFloat(const std::string& json, const std::string& key, float defaultVal) {
    size_t pos = json.find("\"" + key + "\"");
    if (pos == std::string::npos) return defaultVal;

    pos = json.find(":", pos);
    if (pos == std::string::npos) return defaultVal;

    pos = json.find_first_not_of(" \t\n\r", pos + 1);
    if (pos == std::string::npos) return defaultVal;

    // Use atof which doesn't throw exceptions
    const char* str = json.c_str() + pos;
    char* end;
    float result = std::strtof(str, &end);
    if (end == str) return defaultVal;
    return result;
}

inline int extractInt(const std::string& json, const std::string& key, int defaultVal) {
    size_t pos = json.find("\"" + key + "\"");
    if (pos == std::string::npos) return defaultVal;

    pos = json.find(":", pos);
    if (pos == std::string::npos) return defaultVal;

    pos = json.find_first_not_of(" \t\n\r", pos + 1);
    if (pos == std::string::npos) return defaultVal;

    // Use strtol which doesn't throw exceptions
    const char* str = json.c_str() + pos;
    char* end;
    long result = std::strtol(str, &end, 10);
    if (end == str) return defaultVal;
    return static_cast<int>(result);
}

inline std::string extractString(const std::string& json, const std::string& key, const std::string& defaultVal) {
    size_t pos = json.find("\"" + key + "\"");
    if (pos == std::string::npos) return defaultVal;

    pos = json.find(":", pos);
    if (pos == std::string::npos) return defaultVal;

    pos = json.find("\"", pos + 1);
    if (pos == std::string::npos) return defaultVal;

    size_t end = json.find("\"", pos + 1);
    if (end == std::string::npos) return defaultVal;

    return json.substr(pos + 1, end - pos - 1);
}

/**
 * Load weight configuration from JSON file
 *
 * @param filepath Path to JSON configuration file
 * @return Loaded configuration (isLoaded=false if failed)
 */
inline LoadedWeightConfig LoadWeightConfigFromJSON(const std::string& filepath) {
    LoadedWeightConfig config;

    std::ifstream file(filepath);
    if (!file.is_open()) {
        std::cerr << "Error: Cannot open config file: " << filepath << "\n";
        return config;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string json = buffer.str();

    // Extract top-level values
    config.version = extractString(json, "version", "unknown");
    config.name = extractString(json, "name", "unknown");

    // Extract euclidean section
    size_t eucStart = json.find("\"euclidean\"");
    if (eucStart != std::string::npos) {
        size_t eucEnd = json.find("}", eucStart);
        std::string eucSection = json.substr(eucStart, eucEnd - eucStart);

        config.euclideanFadeStart = extractFloat(eucSection, "fadeStart", 0.30f);
        config.euclideanFadeEnd = extractFloat(eucSection, "fadeEnd", 0.70f);

        // Extract anchor subsection
        size_t anchorStart = eucSection.find("\"anchor\"");
        if (anchorStart != std::string::npos) {
            size_t anchorEnd = eucSection.find("}", anchorStart);
            std::string anchorSection = eucSection.substr(anchorStart, anchorEnd - anchorStart);
            config.anchorKMin = extractInt(anchorSection, "kMin", 4);
            config.anchorKMax = extractInt(anchorSection, "kMax", 12);
        }

        // Extract shimmer subsection
        size_t shimmerStart = eucSection.find("\"shimmer\"");
        if (shimmerStart != std::string::npos) {
            size_t shimmerEnd = eucSection.find("}", shimmerStart);
            std::string shimmerSection = eucSection.substr(shimmerStart, shimmerEnd - shimmerStart);
            config.shimmerKMin = extractInt(shimmerSection, "kMin", 6);
            config.shimmerKMax = extractInt(shimmerSection, "kMax", 16);
        }

        // Extract aux subsection
        size_t auxStart = eucSection.find("\"aux\"");
        if (auxStart != std::string::npos) {
            size_t auxEnd = eucSection.find("}", auxStart);
            std::string auxSection = eucSection.substr(auxStart, auxEnd - auxStart);
            config.auxKMin = extractInt(auxSection, "kMin", 2);
            config.auxKMax = extractInt(auxSection, "kMax", 8);
        }
    }

    // Extract syncopation section
    size_t syncStart = json.find("\"syncopation\"");
    if (syncStart != std::string::npos) {
        size_t syncEnd = json.find("}", syncStart);
        std::string syncSection = json.substr(syncStart, syncEnd - syncStart);

        config.syncopationCenter = extractFloat(syncSection, "center", 0.50f);
        config.syncopationWidth = extractFloat(syncSection, "width", 0.30f);
    }

    // Extract random section
    size_t randStart = json.find("\"random\"");
    if (randStart != std::string::npos) {
        size_t randEnd = json.find("}", randStart);
        std::string randSection = json.substr(randStart, randEnd - randStart);

        config.randomFadeStart = extractFloat(randSection, "fadeStart", 0.50f);
        config.randomFadeEnd = extractFloat(randSection, "fadeEnd", 0.90f);
    }

    config.isLoaded = true;
    return config;
}

/**
 * Print loaded configuration for debugging
 */
inline void PrintLoadedConfig(const LoadedWeightConfig& config) {
    std::cout << "\n=== Loaded Weight Configuration ===\n";
    std::cout << "Name: " << config.name << " v" << config.version << "\n\n";

    std::cout << "Euclidean:\n";
    std::cout << "  fadeStart: " << config.euclideanFadeStart << "\n";
    std::cout << "  fadeEnd: " << config.euclideanFadeEnd << "\n";
    std::cout << "  anchor k: [" << config.anchorKMin << ", " << config.anchorKMax << "]\n";
    std::cout << "  shimmer k: [" << config.shimmerKMin << ", " << config.shimmerKMax << "]\n";
    std::cout << "  aux k: [" << config.auxKMin << ", " << config.auxKMax << "]\n\n";

    std::cout << "Syncopation:\n";
    std::cout << "  center: " << config.syncopationCenter << "\n";
    std::cout << "  width: " << config.syncopationWidth << "\n\n";

    std::cout << "Random:\n";
    std::cout << "  fadeStart: " << config.randomFadeStart << "\n";
    std::cout << "  fadeEnd: " << config.randomFadeEnd << "\n";
}

} // namespace daisysp_idm_grids
