// ============================================================================
// OracularV2 Build Map Pipeline - Implementation
// ============================================================================

#include "BuildMap.h"
#include "SAUFormat.h"
#include "bsp/BSPCompiler.h"
#include "bsp/LightBaker.h"

#include <iostream>
#include <chrono>

namespace Build {

bool BuildMap(const std::string& sauPath, const std::string& outputPath) {
    return BuildMap(sauPath, outputPath, Options{});
}

bool BuildMap(const std::string& sauPath, const std::string& outputPath, const Options& options) {
    auto startTime = std::chrono::high_resolution_clock::now();
    
    if (options.verbose) {
        std::cout << "========================================\n";
        std::cout << "  OracularV2 Map Compiler\n";
        std::cout << "========================================\n";
        std::cout << "Input:  " << sauPath << "\n";
        std::cout << "Output: " << outputPath << "\n\n";
    }
    
    // Step 1: Load .sau file
    if (options.verbose) {
        std::cout << "[1/3] Loading map file...\n";
    }
    
    auto map = SAU::Load(sauPath);
    if (!map) {
        std::cerr << "ERROR: Failed to load map file\n";
        return false;
    }
    
    if (options.verbose) {
        std::cout << "  Loaded: " << map->GetName() << "\n";
        std::cout << "  Brushes: " << map->GetBrushCount() << "\n";
        std::cout << "  Entities: " << map->GetEntityCount() << "\n\n";
    }
    
    // Step 2: Compile BSP
    if (options.verbose) {
        std::cout << "[2/3] Compiling BSP tree...\n";
    }
    
    Genesis::BSPCompiler compiler;
    Genesis::BSPCompiler::Options bspOptions;
    bspOptions.verbose = options.verbose;
    bspOptions.buildPVS = true;
    
    auto bsp = compiler.Compile(*map, bspOptions);
    if (!bsp) {
        std::cerr << "ERROR: BSP compilation failed\n";
        std::cerr << compiler.GetLastError() << "\n";
        return false;
    }
    
    if (options.verbose) {
        std::cout << "  BSP compilation complete\n";
        std::cout << "  Nodes: " << bsp->GetNodes().size() << "\n";
        std::cout << "  Leaves: " << bsp->GetLeafs().size() << "\n";
        std::cout << "  Faces: " << bsp->GetFaces().size() << "\n\n";
    }
    
    // Step 3: Bake lighting
    if (options.runLightBake) {
        if (options.verbose) {
            std::cout << "[3/3] Baking lightmaps...\n";
        }
        
        Genesis::LightBaker baker;
        Genesis::LightBaker::Options lightOptions;
        lightOptions.verbose = options.verbose;
        
        baker.BakeWithSceneLights(*bsp, lightOptions);
        
        auto stats = baker.GetLastStats();
        if (options.verbose) {
            std::cout << "  Lightmap faces: " << stats.numFaces << "\n";
            std::cout << "  Texels: " << stats.numTexels << "\n";
            std::cout << "  Shadow rays: " << stats.numShadowRays << "\n\n";
        }
    } else if (options.verbose) {
        std::cout << "[3/3] Skipping light bake\n\n";
    }
    
    // Step 4: Save output
    // TODO: Implement BSP file I/O
    // bsp->SaveToFile(outputPath);
    
    auto endTime = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(endTime - startTime);
    
    if (options.verbose) {
        std::cout << "========================================\n";
        std::cout << "  Build Complete!\n";
        std::cout << "  Time: " << duration.count() << " ms\n";
        std::cout << "========================================\n";
    }
    
    // Launch game if requested
    if (options.runGame && !options.gameExe.empty()) {
        std::string cmd = options.gameExe + " " + outputPath;
        if (options.verbose) {
            std::cout << "Launching game: " << cmd << "\n";
        }
        system(cmd.c_str());
    }
    
    return true;
}

} // namespace Build
