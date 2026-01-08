#pragma once

// ============================================================================
// OracularV2 Editor Entity
// Wrapper for engine MapEntity with editor-specific state
// ============================================================================

#include "map/Map.h"
#include <string>

// ============================================================================
// Entity Types for visual representation
// ============================================================================
enum class EditorEntityType {
    Generic,        // Default icon
    Light,          // Light source with radius sphere
    PlayerStart,    // Player spawn point
    Trigger,        // Trigger volume
    InfoTarget      // Target for logic
};

// ============================================================================
// EditorEntity - Editor wrapper for engine entities
// ============================================================================
struct EditorEntity {
    // The actual engine entity data
    Genesis::MapEntity entity;
    
    // Editor-only state
    bool isSelected = false;
    bool isHovered = false;
    bool isVisible = true;
    
    // Editor ID
    uint32_t editorId = 0;
    
    // Entity type for visual representation
    EditorEntityType visualType = EditorEntityType::Generic;
    
    // ========================================================================
    // Convenience accessors
    // ========================================================================
    
    Genesis::Vec3& Position() { return entity.position; }
    const Genesis::Vec3& Position() const { return entity.position; }
    
    const std::string& Classname() const { return entity.classname; }
    void SetClassname(const std::string& name) { 
        entity.classname = name;
        UpdateVisualType();
    }
    
    // Get/Set property
    std::string GetProperty(const std::string& key, const std::string& defaultVal = "") const {
        auto it = entity.properties.find(key);
        return (it != entity.properties.end()) ? it->second : defaultVal;
    }
    
    void SetProperty(const std::string& key, const std::string& value) {
        entity.properties[key] = value;
    }
    
    // ========================================================================
    // Light-specific helpers
    // ========================================================================
    
    Genesis::Vec3 GetLightColor() const {
        float r = std::stof(GetProperty("color_r", "255"));
        float g = std::stof(GetProperty("color_g", "255"));
        float b = std::stof(GetProperty("color_b", "255"));
        return Genesis::Vec3(r / 255.0f, g / 255.0f, b / 255.0f);
    }
    
    float GetLightRadius() const {
        return std::stof(GetProperty("radius", "200"));
    }
    
    float GetLightIntensity() const {
        return std::stof(GetProperty("intensity", "500"));
    }
    
    // ========================================================================
    // Creation helpers
    // ========================================================================
    
    static EditorEntity CreateLight(const Genesis::Vec3& pos, 
                                     const Genesis::Vec3& color = Genesis::Vec3(1, 1, 1),
                                     float radius = 200.0f, 
                                     float intensity = 500.0f) {
        EditorEntity e;
        e.entity.classname = "light";
        e.entity.position = pos;
        e.entity.properties["color_r"] = std::to_string((int)(color.r * 255));
        e.entity.properties["color_g"] = std::to_string((int)(color.g * 255));
        e.entity.properties["color_b"] = std::to_string((int)(color.b * 255));
        e.entity.properties["radius"] = std::to_string(radius);
        e.entity.properties["intensity"] = std::to_string(intensity);
        e.visualType = EditorEntityType::Light;
        return e;
    }
    
    static EditorEntity CreatePlayerStart(const Genesis::Vec3& pos, float angle = 0.0f) {
        EditorEntity e;
        e.entity.classname = "info_player_start";
        e.entity.position = pos;
        e.entity.rotation.y = angle;
        e.visualType = EditorEntityType::PlayerStart;
        return e;
    }
    
    static EditorEntity CreateTrigger(const Genesis::Vec3& pos, const Genesis::Vec3& size) {
        EditorEntity e;
        e.entity.classname = "trigger_once";
        e.entity.position = pos;
        e.entity.properties["mins"] = std::to_string(size.x) + " " + std::to_string(size.y) + " " + std::to_string(size.z);
        e.visualType = EditorEntityType::Trigger;
        return e;
    }

private:
    void UpdateVisualType() {
        if (entity.classname == "light") 
            visualType = EditorEntityType::Light;
        else if (entity.classname == "info_player_start")
            visualType = EditorEntityType::PlayerStart;
        else if (entity.classname.find("trigger") != std::string::npos)
            visualType = EditorEntityType::Trigger;
        else if (entity.classname.find("info_") != std::string::npos)
            visualType = EditorEntityType::InfoTarget;
        else
            visualType = EditorEntityType::Generic;
    }
};
