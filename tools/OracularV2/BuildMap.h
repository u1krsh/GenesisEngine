#pragma once

// ============================================================================
// OracularV2 Build Map Pipeline
// Compiles .sau → BSP + Lightmaps
// ============================================================================

#include <string>

namespace Build {

// Build a map from .sau file
// Returns true on success
bool BuildMap(const std::string& sauPath, const std::string& outputPath);

// Build options
struct Options {
    bool verbose = true;
    bool runLightBake = true;
    bool runGame = false;
    std::string gameExe;
};

bool BuildMap(const std::string& sauPath, const std::string& outputPath, const Options& options);

} // namespace Build
