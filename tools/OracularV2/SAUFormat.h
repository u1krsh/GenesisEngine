#pragma once

// ============================================================================
// OracularV2 SAU File Format
// Text-based map format (similar to Hammer's VMF)
// ============================================================================

#include "map/Map.h"
#include <string>
#include <memory>

namespace SAU {

// ============================================================================
// File I/O
// ============================================================================

// Load a .sau file into a Map
std::unique_ptr<Genesis::Map> Load(const std::string& path);

// Save a Map to a .sau file
bool Save(const Genesis::Map& map, const std::string& path);

// ============================================================================
// Format Specification
// ============================================================================
/*
    SAU File Format v1.0
    ====================
    
    Text-based, human-readable format inspired by Valve Map Format (VMF).
    
    Example:
    --------
    
    map
    {
        name "My Level"
        author "Developer"
        spawn_position 0 8 0
    }
    
    brush
    {
        min 0 0 0
        max 128 16 128
        material floor_concrete
        flags castShadow receiveShadow
    }
    
    brush
    {
        min 0 16 0
        max 8 128 128
        material wall_brick
    }
    
    entity light
    {
        position 64 64 64
        color 255 200 150
        intensity 600
        radius 200
    }
    
    entity info_player_start
    {
        position 64 8 64
        angle 0
    }
*/

} // namespace SAU
