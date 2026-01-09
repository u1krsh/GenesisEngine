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
// Load - Uses same format as MapLoader::LoadSAU for compatibility
// ============================================================================

std::unique_ptr<Genesis::Map> Load(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "[SAU] Failed to open file: " << path << "\n";
        return nullptr;
    }
    
    auto map = std::make_unique<Genesis::Map>();
    std::string line;
    std::string currentSection;
    Genesis::Brush currentBrush;
    Genesis::MapEntity currentEntity;
    bool inBrush = false;
    bool inEntity = false;
    
    while (std::getline(file, line)) {
        line = Trim(line);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == '/') {
            continue;
        }
        
        // Section headers (with { on same line)
        if (line == "metadata {") {
            currentSection = "metadata";
            continue;
        } else if (line == "brush {") {
            currentSection = "brush";
            inBrush = true;
            currentBrush = Genesis::Brush();
            currentBrush.flags = Genesis::BrushFlags::CastShadow | Genesis::BrushFlags::ReceiveShadow;
            continue;
        } else if (line == "entity {") {
            currentSection = "entity";
            inEntity = true;
            currentEntity = Genesis::MapEntity();
            continue;
        } else if (line == "}") {
            if (inBrush) {
                map->AddBrush(currentBrush);
                inBrush = false;
            } else if (inEntity) {
                if (!currentEntity.classname.empty()) {
                    map->AddEntity(currentEntity);
                }
                inEntity = false;
            }
            currentSection = "";
            continue;
        }
        
        // Parse key = value pairs
        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos) continue;
        
        std::string key = Trim(line.substr(0, eqPos));
        std::string value = Trim(line.substr(eqPos + 1));
        
        // Remove quotes from value
        if (value.size() >= 2 && value.front() == '"' && value.back() == '"') {
            value = value.substr(1, value.size() - 2);
        }
        
        if (currentSection == "metadata") {
            if (key == "name") map->GetMetadata().name = value;
            else if (key == "author") map->GetMetadata().author = value;
            else if (key == "version") map->GetMetadata().version = value;
        } else if (inBrush) {
            if (key == "position") {
                std::istringstream ss(value);
                ss >> currentBrush.position.x >> currentBrush.position.y >> currentBrush.position.z;
            } else if (key == "size") {
                std::istringstream ss(value);
                ss >> currentBrush.size.x >> currentBrush.size.y >> currentBrush.size.z;
            } else if (key == "rotation") {
                std::istringstream ss(value);
                ss >> currentBrush.rotation.x >> currentBrush.rotation.y >> currentBrush.rotation.z;
            } else if (key == "material") {
                currentBrush.materialName = value;
            } else if (key == "shape") {
                currentBrush.shape = Genesis::StringToBrushShape(value);
            } else if (key == "flags") {
                if (value.find("nocollision") != std::string::npos)
                    currentBrush.flags = currentBrush.flags | Genesis::BrushFlags::NoCollision;
                if (value.find("stair") != std::string::npos)
                    currentBrush.flags = currentBrush.flags | Genesis::BrushFlags::Stair;
            }
        } else if (inEntity) {
            if (key == "classname") {
                currentEntity.classname = value;
            } else if (key == "position") {
                std::istringstream ss(value);
                ss >> currentEntity.position.x >> currentEntity.position.y >> currentEntity.position.z;
            } else if (key == "rotation") {
                std::istringstream ss(value);
                ss >> currentEntity.rotation.x >> currentEntity.rotation.y >> currentEntity.rotation.z;
            } else {
                // Store as custom property
                currentEntity.properties[key] = value;
            }
        }
    }
    
    std::cout << "[SAU] Loaded map: " << map->GetName() 
              << " (" << map->GetBrushCount() << " brushes, "
              << map->GetEntityCount() << " entities)\n";
    
    return map;
}

// ============================================================================
// Save - Uses same format as MapLoader::LoadSAU for compatibility
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
    
    // Metadata block
    file << "metadata {\n";
    file << "    name = \"" << meta.name << "\"\n";
    file << "    author = \"" << meta.author << "\"\n";
    file << "    version = \"" << meta.version << "\"\n";
    file << "}\n\n";
    
    // Brushes
    for (const auto& brush : map.GetBrushes()) {
        file << "brush {\n";
        file << "    position = " << brush.position.x << " " << brush.position.y << " " << brush.position.z << "\n";
        file << "    size = " << brush.size.x << " " << brush.size.y << " " << brush.size.z << "\n";
        if (brush.rotation != Genesis::Vec3(0.0f)) {
            file << "    rotation = " << brush.rotation.x << " " << brush.rotation.y << " " << brush.rotation.z << "\n";
        }
        file << "    shape = " << Genesis::BrushShapeToString(brush.shape) << "\n";
        file << "    material = " << brush.materialName << "\n";
        
        // Flags
        std::string flags;
        if (Genesis::HasFlag(brush.flags, Genesis::BrushFlags::NoCollision)) flags += "nocollision ";
        if (Genesis::HasFlag(brush.flags, Genesis::BrushFlags::Stair)) flags += "stair ";
        if (!flags.empty()) {
            file << "    flags = " << flags << "\n";
        }
        
        file << "}\n\n";
    }
    
    // Entities
    for (const auto& entity : map.GetEntities()) {
        file << "entity {\n";
        file << "    classname = " << entity.classname << "\n";
        file << "    position = " << entity.position.x << " " << entity.position.y << " " << entity.position.z << "\n";
        
        if (entity.rotation != Genesis::Vec3(0.0f)) {
            file << "    rotation = " << entity.rotation.x << " " << entity.rotation.y << " " << entity.rotation.z << "\n";
        }
        
        // Custom properties
        for (const auto& [key, value] : entity.properties) {
            if (key != "classname" && key != "position" && key != "rotation") {
                file << "    " << key << " = \"" << value << "\"\n";
            }
        }
        
        file << "}\n\n";
    }
    
    std::cout << "[SAU] Saved map: " << path << "\n";
    return true;
}

} // namespace SAU
