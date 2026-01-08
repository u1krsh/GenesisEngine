// ============================================================================
// OracularV2 SAU File Format - Implementation
// ============================================================================

#include "SAUFormat.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace SAU {

// ============================================================================
// Parser Helpers
// ============================================================================

static std::string Trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

static std::string RemoveQuotes(const std::string& str) {
    if (str.size() >= 2 && str.front() == '"' && str.back() == '"') {
        return str.substr(1, str.size() - 2);
    }
    return str;
}

static std::vector<std::string> SplitLine(const std::string& line) {
    std::vector<std::string> tokens;
    std::istringstream iss(line);
    std::string token;
    
    bool inQuotes = false;
    std::string current;
    
    for (char c : line) {
        if (c == '"') {
            inQuotes = !inQuotes;
            current += c;
        } else if ((c == ' ' || c == '\t') && !inQuotes) {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
        } else {
            current += c;
        }
    }
    
    if (!current.empty()) {
        tokens.push_back(current);
    }
    
    return tokens;
}

static Genesis::Vec3 ParseVec3(const std::vector<std::string>& tokens, size_t startIndex) {
    Genesis::Vec3 v(0.0f);
    if (startIndex + 2 < tokens.size()) {
        v.x = std::stof(tokens[startIndex]);
        v.y = std::stof(tokens[startIndex + 1]);
        v.z = std::stof(tokens[startIndex + 2]);
    }
    return v;
}

// ============================================================================
// Load
// ============================================================================

std::unique_ptr<Genesis::Map> Load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[SAU] Failed to open file: " << path << "\n";
        return nullptr;
    }
    
    auto map = std::make_unique<Genesis::Map>();
    std::string line;
    
    enum class ParseState { None, Map, Brush, Entity };
    ParseState state = ParseState::None;
    
    Genesis::Brush currentBrush;
    Genesis::MapEntity currentEntity;
    int braceDepth = 0;
    
    while (std::getline(file, line)) {
        line = Trim(line);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line.substr(0, 2) == "//") {
            continue;
        }
        
        auto tokens = SplitLine(line);
        if (tokens.empty()) continue;
        
        // Block start
        if (tokens[0] == "map") {
            state = ParseState::Map;
            continue;
        } else if (tokens[0] == "brush") {
            state = ParseState::Brush;
            currentBrush = Genesis::Brush();
            continue;
        } else if (tokens[0] == "entity" && tokens.size() > 1) {
            state = ParseState::Entity;
            currentEntity = Genesis::MapEntity();
            currentEntity.classname = RemoveQuotes(tokens[1]);
            continue;
        }
        
        // Block braces
        if (tokens[0] == "{") {
            braceDepth++;
            continue;
        } else if (tokens[0] == "}") {
            braceDepth--;
            if (braceDepth == 0) {
                if (state == ParseState::Brush) {
                    map->AddBrush(currentBrush);
                } else if (state == ParseState::Entity) {
                    map->AddEntity(currentEntity);
                }
                state = ParseState::None;
            }
            continue;
        }
        
        // Parse properties based on state
        if (state == ParseState::Map) {
            auto& meta = map->GetMetadata();
            if (tokens[0] == "name" && tokens.size() > 1) {
                meta.name = RemoveQuotes(tokens[1]);
            } else if (tokens[0] == "author" && tokens.size() > 1) {
                meta.author = RemoveQuotes(tokens[1]);
            } else if (tokens[0] == "spawn_position" && tokens.size() > 3) {
                meta.spawnPosition = ParseVec3(tokens, 1);
            }
        } else if (state == ParseState::Brush) {
            if (tokens[0] == "min" && tokens.size() > 3) {
                currentBrush.position = ParseVec3(tokens, 1);
            } else if (tokens[0] == "max" && tokens.size() > 3) {
                Genesis::Vec3 maxPos = ParseVec3(tokens, 1);
                currentBrush.size = maxPos - currentBrush.position;
            } else if (tokens[0] == "material" && tokens.size() > 1) {
                currentBrush.materialName = RemoveQuotes(tokens[1]);
            } else if (tokens[0] == "flags") {
                currentBrush.flags = Genesis::BrushFlags::None;
                for (size_t i = 1; i < tokens.size(); i++) {
                    if (tokens[i] == "noCollision") {
                        currentBrush.flags = currentBrush.flags | Genesis::BrushFlags::NoCollision;
                    } else if (tokens[i] == "castShadow") {
                        currentBrush.flags = currentBrush.flags | Genesis::BrushFlags::CastShadow;
                    } else if (tokens[i] == "receiveShadow") {
                        currentBrush.flags = currentBrush.flags | Genesis::BrushFlags::ReceiveShadow;
                    }
                }
            }
        } else if (state == ParseState::Entity) {
            if (tokens[0] == "position" && tokens.size() > 3) {
                currentEntity.position = ParseVec3(tokens, 1);
            } else if (tokens[0] == "rotation" && tokens.size() > 3) {
                currentEntity.rotation = ParseVec3(tokens, 1);
            } else if (tokens.size() > 1) {
                // Generic key-value property
                currentEntity.properties[tokens[0]] = RemoveQuotes(tokens[1]);
            }
        }
    }
    
    std::cout << "[SAU] Loaded map: " << map->GetName() 
              << " (" << map->GetBrushCount() << " brushes, "
              << map->GetEntityCount() << " entities)\n";
    
    return map;
}

// ============================================================================
// Save
// ============================================================================

bool Save(const Genesis::Map& map, const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) {
        std::cerr << "[SAU] Failed to create file: " << path << "\n";
        return false;
    }
    
    const auto& meta = map.GetMetadata();
    
    // Write header
    file << "# OracularV2 Map File\n";
    file << "# Format: SAU v1.0\n\n";
    
    // Map block
    file << "map\n{\n";
    file << "    name \"" << meta.name << "\"\n";
    file << "    author \"" << meta.author << "\"\n";
    file << "    spawn_position " << meta.spawnPosition.x << " " 
         << meta.spawnPosition.y << " " << meta.spawnPosition.z << "\n";
    file << "}\n\n";
    
    // Brushes
    for (const auto& brush : map.GetBrushes()) {
        Genesis::Vec3 maxPos = brush.position + brush.size;
        
        file << "brush\n{\n";
        file << "    min " << brush.position.x << " " << brush.position.y << " " << brush.position.z << "\n";
        file << "    max " << maxPos.x << " " << maxPos.y << " " << maxPos.z << "\n";
        file << "    material " << brush.materialName << "\n";
        
        // Flags
        std::string flags;
        if (Genesis::HasFlag(brush.flags, Genesis::BrushFlags::NoCollision)) flags += "noCollision ";
        if (Genesis::HasFlag(brush.flags, Genesis::BrushFlags::CastShadow)) flags += "castShadow ";
        if (Genesis::HasFlag(brush.flags, Genesis::BrushFlags::ReceiveShadow)) flags += "receiveShadow ";
        if (!flags.empty()) {
            file << "    flags " << flags << "\n";
        }
        
        file << "}\n\n";
    }
    
    // Entities
    for (const auto& entity : map.GetEntities()) {
        file << "entity " << entity.classname << "\n{\n";
        file << "    position " << entity.position.x << " " << entity.position.y << " " << entity.position.z << "\n";
        
        if (entity.rotation != Genesis::Vec3(0.0f)) {
            file << "    rotation " << entity.rotation.x << " " << entity.rotation.y << " " << entity.rotation.z << "\n";
        }
        
        // Custom properties
        for (const auto& [key, value] : entity.properties) {
            if (key != "position" && key != "rotation") {
                file << "    " << key << " \"" << value << "\"\n";
            }
        }
        
        file << "}\n\n";
    }
    
    std::cout << "[SAU] Saved map: " << path << "\n";
    return true;
}

} // namespace SAU
