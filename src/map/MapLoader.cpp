#include "MapLoader.h"
#include "core/Logger.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

namespace Genesis {

// ============================================================================
// Simple JSON Parser (minimal, no external dependencies)
// ============================================================================
namespace {

    // Trim whitespace from string
    std::string Trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(" \t\n\r");
        return str.substr(start, end - start + 1);
    }

    // Parse a float from string
    float ParseFloat(const std::string& str) {
        try {
            return std::stof(Trim(str));
        } catch (...) {
            return 0.0f;
        }
    }

    // Parse an int from string
    int ParseInt(const std::string& str) {
        try {
            return std::stoi(Trim(str));
        } catch (...) {
            return 0;
        }
    }

    // Extract string value from JSON (simple parser)
    std::string ExtractString(const std::string& json, const std::string& key) {
        std::string searchKey = "\"" + key + "\"";
        size_t keyPos = json.find(searchKey);
        if (keyPos == std::string::npos) return "";

        size_t colonPos = json.find(':', keyPos);
        if (colonPos == std::string::npos) return "";

        size_t startQuote = json.find('"', colonPos + 1);
        if (startQuote == std::string::npos) return "";

        size_t endQuote = json.find('"', startQuote + 1);
        if (endQuote == std::string::npos) return "";

        return json.substr(startQuote + 1, endQuote - startQuote - 1);
    }

    // Extract number value from JSON
    float ExtractNumber(const std::string& json, const std::string& key, float defaultValue = 0.0f) {
        std::string searchKey = "\"" + key + "\"";
        size_t keyPos = json.find(searchKey);
        if (keyPos == std::string::npos) return defaultValue;

        size_t colonPos = json.find(':', keyPos);
        if (colonPos == std::string::npos) return defaultValue;

        // Skip whitespace
        size_t valueStart = colonPos + 1;
        while (valueStart < json.size() && std::isspace(json[valueStart])) valueStart++;

        // Find end of number
        size_t valueEnd = valueStart;
        while (valueEnd < json.size() && (std::isdigit(json[valueEnd]) || json[valueEnd] == '.' || json[valueEnd] == '-' || json[valueEnd] == '+')) {
            valueEnd++;
        }

        if (valueEnd == valueStart) return defaultValue;

        return ParseFloat(json.substr(valueStart, valueEnd - valueStart));
    }

    // Extract boolean value from JSON
    bool ExtractBool(const std::string& json, const std::string& key, bool defaultValue = false) {
        std::string searchKey = "\"" + key + "\"";
        size_t keyPos = json.find(searchKey);
        if (keyPos == std::string::npos) return defaultValue;

        size_t colonPos = json.find(':', keyPos);
        if (colonPos == std::string::npos) return defaultValue;

        size_t truePos = json.find("true", colonPos);
        size_t falsePos = json.find("false", colonPos);

        if (truePos != std::string::npos && (falsePos == std::string::npos || truePos < falsePos)) {
            return true;
        }
        return false;
    }

    // Extract Vec3 from JSON array
    Vec3 ExtractVec3(const std::string& json, const std::string& key, const Vec3& defaultValue = Vec3(0.0f)) {
        std::string searchKey = "\"" + key + "\"";
        size_t keyPos = json.find(searchKey);
        if (keyPos == std::string::npos) return defaultValue;

        size_t arrayStart = json.find('[', keyPos);
        if (arrayStart == std::string::npos) return defaultValue;

        size_t arrayEnd = json.find(']', arrayStart);
        if (arrayEnd == std::string::npos) return defaultValue;

        std::string arrayContent = json.substr(arrayStart + 1, arrayEnd - arrayStart - 1);

        // Parse comma-separated values
        Vec3 result;
        std::stringstream ss(arrayContent);
        std::string token;
        int i = 0;
        while (std::getline(ss, token, ',') && i < 3) {
            result[i++] = ParseFloat(token);
        }

        return result;
    }

    // Find matching brace
    size_t FindMatchingBrace(const std::string& json, size_t start) {
        if (start >= json.size() || json[start] != '{') return std::string::npos;

        int depth = 1;
        for (size_t i = start + 1; i < json.size(); i++) {
            if (json[i] == '{') depth++;
            else if (json[i] == '}') {
                depth--;
                if (depth == 0) return i;
            }
        }
        return std::string::npos;
    }

    // Extract array of objects
    std::vector<std::string> ExtractObjectArray(const std::string& json, const std::string& key) {
        std::vector<std::string> result;

        std::string searchKey = "\"" + key + "\"";
        size_t keyPos = json.find(searchKey);
        if (keyPos == std::string::npos) return result;

        size_t arrayStart = json.find('[', keyPos);
        if (arrayStart == std::string::npos) return result;

        size_t pos = arrayStart + 1;
        while (pos < json.size()) {
            // Skip whitespace
            while (pos < json.size() && std::isspace(json[pos])) pos++;

            if (pos >= json.size() || json[pos] == ']') break;

            if (json[pos] == '{') {
                size_t objEnd = FindMatchingBrace(json, pos);
                if (objEnd != std::string::npos) {
                    result.push_back(json.substr(pos, objEnd - pos + 1));
                    pos = objEnd + 1;
                } else {
                    break;
                }
            } else if (json[pos] == ',') {
                pos++;
            } else {
                pos++;
            }
        }

        return result;
    }

} // anonymous namespace

// ============================================================================
// MapLoader Implementation
// ============================================================================

MapLoader::MapLoader() {
    // Register default materials if needed
}

MapPtr MapLoader::Load(const std::string& filepath) {
    ClearError();

    std::string fullPath = m_basePath + filepath;

    // Detect format by extension
    size_t dotPos = filepath.rfind('.');
    std::string ext = (dotPos != std::string::npos) ? filepath.substr(dotPos) : "";
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == ".json") {
        return LoadJSON(fullPath);
    } else if (ext == ".map" || ext == ".txt") {
        return LoadSimple(fullPath);
    } else {
        // Default to JSON
        return LoadJSON(fullPath);
    }
}

MapPtr MapLoader::LoadJSON(const std::string& filepath) {
    ClearError();

    std::ifstream file(filepath);
    if (!file.is_open()) {
        SetError("Failed to open file: " + filepath);
        return nullptr;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string content = buffer.str();
    file.close();

    return LoadFromString(content);
}

MapPtr MapLoader::LoadFromString(const std::string& jsonString) {
    ClearError();

    auto map = std::make_shared<Map>();

    // Parse metadata
    MapMetadata& meta = map->GetMetadata();
    meta.name = ExtractString(jsonString, "name");
    if (meta.name.empty()) meta.name = "Untitled";
    meta.author = ExtractString(jsonString, "author");
    meta.version = ExtractString(jsonString, "version");
    meta.description = ExtractString(jsonString, "description");

    // Parse spawn
    meta.spawnPosition = ExtractVec3(jsonString, "spawn_position", Vec3(0, 1, 0));
    meta.spawnRotation = ExtractVec3(jsonString, "spawn_rotation", Vec3(0));

    // Parse environment
    meta.sunDirection = ExtractVec3(jsonString, "sun_direction", Vec3(0.5f, 1.0f, 0.3f));
    meta.sunColor = ExtractVec3(jsonString, "sun_color", Vec3(1.0f, 0.98f, 0.95f));
    meta.sunIntensity = ExtractNumber(jsonString, "sun_intensity", 1.0f);
    meta.ambientColor = ExtractVec3(jsonString, "ambient_color", Vec3(0.15f, 0.15f, 0.2f));

    // Parse brushes
    auto brushObjects = ExtractObjectArray(jsonString, "brushes");
    for (const auto& brushJson : brushObjects) {
        Brush brush;
        if (ParseJSONBrush(brushJson, brush)) {
            map->AddBrush(std::move(brush));
        }
    }

    // Parse entities
    auto entityObjects = ExtractObjectArray(jsonString, "entities");
    for (const auto& entityJson : entityObjects) {
        MapEntity entity;
        entity.classname = ExtractString(entityJson, "classname");
        entity.targetname = ExtractString(entityJson, "targetname");
        entity.position = ExtractVec3(entityJson, "position");
        entity.rotation = ExtractVec3(entityJson, "rotation");

        if (!entity.classname.empty()) {
            map->AddEntity(entity);
        }
    }

    LOG_INFO("MapLoader", "Loaded map '" + meta.name + "' with " +
             std::to_string(map->GetBrushCount()) + " brushes, " +
             std::to_string(map->GetEntityCount()) + " entities");

    // Build the map (resolve meshes, materials, colliders)
    BuildMap(*map);

    return map;
}

MapPtr MapLoader::LoadSimple(const std::string& filepath) {
    ClearError();

    std::ifstream file(filepath);
    if (!file.is_open()) {
        SetError("Failed to open file: " + filepath);
        return nullptr;
    }

    auto map = std::make_shared<Map>();
    std::string line;
    int lineNum = 0;

    while (std::getline(file, line)) {
        lineNum++;
        line = Trim(line);

        // Skip empty lines and comments
        if (line.empty() || line[0] == '#' || line[0] == '/') {
            continue;
        }

        // Parse metadata directives
        if (line[0] == '@') {
            // @name "Map Name"
            // @author "Author Name"
            // @spawn 0 1 0
            size_t spacePos = line.find(' ');
            if (spacePos != std::string::npos) {
                std::string directive = line.substr(1, spacePos - 1);
                std::string value = Trim(line.substr(spacePos + 1));

                // Remove quotes if present
                if (!value.empty() && value.front() == '"' && value.back() == '"') {
                    value = value.substr(1, value.size() - 2);
                }

                if (directive == "name") {
                    map->GetMetadata().name = value;
                } else if (directive == "author") {
                    map->GetMetadata().author = value;
                } else if (directive == "spawn") {
                    std::stringstream ss(value);
                    Vec3 spawn;
                    ss >> spawn.x >> spawn.y >> spawn.z;
                    map->GetMetadata().spawnPosition = spawn;
                }
            }
            continue;
        }

        // Parse brush line: shape posX posY posZ sizeX sizeY sizeZ material [flags]
        Brush brush;
        if (ParseSimpleLine(line, brush)) {
            map->AddBrush(std::move(brush));
        } else {
            LOG_WARNING("MapLoader", "Failed to parse line " + std::to_string(lineNum) + ": " + line);
        }
    }

    file.close();

    LOG_INFO("MapLoader", "Loaded simple map with " + std::to_string(map->GetBrushCount()) + " brushes");

    // Build the map
    BuildMap(*map);

    return map;
}

bool MapLoader::ParseJSONBrush(const std::string& json, Brush& brush) {
    // Shape
    std::string shapeStr = ExtractString(json, "shape");
    brush.shape = StringToBrushShape(shapeStr.empty() ? "cube" : shapeStr);

    // Position and size
    brush.position = ExtractVec3(json, "position", Vec3(0.0f));
    brush.size = ExtractVec3(json, "size", Vec3(1.0f));
    brush.rotation = ExtractVec3(json, "rotation", Vec3(0.0f));

    // Material
    brush.materialName = ExtractString(json, "material");
    if (brush.materialName.empty()) {
        brush.materialName = m_defaultMaterial;
    }

    // Name and layer
    brush.name = ExtractString(json, "name");
    brush.layer = ExtractString(json, "layer");
    if (brush.layer.empty()) brush.layer = "default";

    // Flags
    brush.flags = BrushFlags::CastShadow | BrushFlags::ReceiveShadow;
    if (ExtractBool(json, "no_collision")) {
        brush.flags = brush.flags | BrushFlags::NoCollision;
    }
    if (ExtractBool(json, "stair")) {
        brush.flags = brush.flags | BrushFlags::Stair;
    }
    if (ExtractBool(json, "trigger")) {
        brush.flags = brush.flags | BrushFlags::Trigger;
    }
    if (ExtractBool(json, "no_render")) {
        brush.flags = brush.flags | BrushFlags::NoRender;
    }
    if (ExtractBool(json, "detail")) {
        brush.flags = brush.flags | BrushFlags::Detail;
    }

    return true;
}

bool MapLoader::ParseSimpleLine(const std::string& line, Brush& brush) {
    std::stringstream ss(line);
    std::string token;

    // Parse: shape posX posY posZ sizeX sizeY sizeZ material [flags]
    std::vector<std::string> tokens;
    while (ss >> token) {
        tokens.push_back(token);
    }

    if (tokens.size() < 8) {
        return false;
    }

    // Shape
    brush.shape = StringToBrushShape(tokens[0]);

    // Position
    brush.position.x = ParseFloat(tokens[1]);
    brush.position.y = ParseFloat(tokens[2]);
    brush.position.z = ParseFloat(tokens[3]);

    // Size
    brush.size.x = ParseFloat(tokens[4]);
    brush.size.y = ParseFloat(tokens[5]);
    brush.size.z = ParseFloat(tokens[6]);

    // Material
    brush.materialName = tokens[7];

    // Parse optional flags
    brush.flags = BrushFlags::CastShadow | BrushFlags::ReceiveShadow;
    for (size_t i = 8; i < tokens.size(); i++) {
        std::string flag = tokens[i];
        std::transform(flag.begin(), flag.end(), flag.begin(), ::tolower);

        if (flag == "stair") {
            brush.flags = brush.flags | BrushFlags::Stair;
        } else if (flag == "nocollision" || flag == "nocol") {
            brush.flags = brush.flags | BrushFlags::NoCollision;
        } else if (flag == "trigger") {
            brush.flags = brush.flags | BrushFlags::Trigger;
        } else if (flag == "norender" || flag == "invisible") {
            brush.flags = brush.flags | BrushFlags::NoRender;
        } else if (flag == "detail") {
            brush.flags = brush.flags | BrushFlags::Detail;
        }
    }

    return true;
}

void MapLoader::BuildMap(Map& map) {
    for (auto& brush : map.GetBrushes()) {
        BuildBrush(brush);
    }

    // Register layers
    for (const auto& brush : map.GetBrushes()) {
        map.AddLayer(brush.layer);
    }
}

void MapLoader::BuildBrush(Brush& brush) {
    // Build transform matrix
    brush.BuildTransform();

    // Get mesh from library
    auto& meshLib = MeshLibrary::Instance();
    brush.mesh = meshLib.GetForShape(brush.shape);

    // Get material from library
    auto& matLib = MaterialLibrary::Instance();
    brush.material = matLib.Get(brush.materialName);

    // If material not found, create a default one
    if (!brush.material) {
        // Create a simple colored material based on material name
        Vec3 color(0.5f, 0.5f, 0.5f); // Default gray

        // Some common material name to color mappings
        std::string matName = brush.materialName;
        std::transform(matName.begin(), matName.end(), matName.begin(), ::tolower);

        if (matName == "floor" || matName == "ground") {
            color = Vec3(0.3f, 0.3f, 0.35f);
        } else if (matName == "wall") {
            color = Vec3(0.6f, 0.55f, 0.5f);
        } else if (matName == "ceiling") {
            color = Vec3(0.7f, 0.7f, 0.75f);
        } else if (matName == "brick") {
            color = Vec3(0.6f, 0.3f, 0.2f);
        } else if (matName == "concrete" || matName == "cement") {
            color = Vec3(0.5f, 0.5f, 0.5f);
        } else if (matName == "wood") {
            color = Vec3(0.5f, 0.35f, 0.2f);
        } else if (matName == "metal") {
            color = Vec3(0.6f, 0.6f, 0.65f);
        } else if (matName == "grass") {
            color = Vec3(0.2f, 0.5f, 0.2f);
        } else if (matName == "water") {
            color = Vec3(0.2f, 0.4f, 0.7f);
        } else if (matName == "red") {
            color = Vec3(0.7f, 0.2f, 0.2f);
        } else if (matName == "green") {
            color = Vec3(0.2f, 0.7f, 0.2f);
        } else if (matName == "blue") {
            color = Vec3(0.2f, 0.2f, 0.7f);
        } else if (matName == "white") {
            color = Vec3(0.9f, 0.9f, 0.9f);
        } else if (matName == "black") {
            color = Vec3(0.1f, 0.1f, 0.1f);
        }

        brush.material = matLib.CreateSolidColor(brush.materialName, color);
        LOG_DEBUG("MapLoader", "Created material '" + brush.materialName + "'");
    }

    // Create collider if brush has collision
    if (brush.HasCollision()) {
        brush.collider = CreateCollider(brush);

        // Set stair flag on collider
        if (brush.collider && brush.IsStair()) {
            brush.collider->SetStair(true);
        }

        // Set trigger flag
        if (brush.collider && brush.IsTrigger()) {
            brush.collider->SetTrigger(true);
        }
    }
}

ColliderPtr MapLoader::CreateCollider(const Brush& brush) {
    switch (brush.shape) {
        case BrushShape::Cube:
        case BrushShape::Wedge: // Use box for wedge for now
            return BoxCollider::FromSize(brush.size);

        case BrushShape::Sphere:
            // Use largest dimension as radius
            return std::make_shared<SphereCollider>(
                std::max({brush.size.x, brush.size.y, brush.size.z}) * 0.5f
            );

        case BrushShape::Cylinder:
        case BrushShape::Cone:
            // Approximate with box for now (TODO: proper cylinder/cone collider)
            return BoxCollider::FromSize(brush.size);

        default:
            return BoxCollider::FromSize(brush.size);
    }
}

bool MapLoader::SaveJSON(const Map& map, const std::string& filepath) {
    ClearError();

    std::string fullPath = m_basePath + filepath;
    std::ofstream file(fullPath);
    if (!file.is_open()) {
        SetError("Failed to open file for writing: " + fullPath);
        return false;
    }

    const auto& meta = map.GetMetadata();

    file << "{\n";
    file << "  \"name\": \"" << meta.name << "\",\n";
    file << "  \"author\": \"" << meta.author << "\",\n";
    file << "  \"version\": \"" << meta.version << "\",\n";
    file << "  \"description\": \"" << meta.description << "\",\n";
    file << "  \n";
    file << "  \"spawn_position\": [" << meta.spawnPosition.x << ", " << meta.spawnPosition.y << ", " << meta.spawnPosition.z << "],\n";
    file << "  \"spawn_rotation\": [" << meta.spawnRotation.x << ", " << meta.spawnRotation.y << ", " << meta.spawnRotation.z << "],\n";
    file << "  \n";
    file << "  \"sun_direction\": [" << meta.sunDirection.x << ", " << meta.sunDirection.y << ", " << meta.sunDirection.z << "],\n";
    file << "  \"sun_color\": [" << meta.sunColor.x << ", " << meta.sunColor.y << ", " << meta.sunColor.z << "],\n";
    file << "  \"sun_intensity\": " << meta.sunIntensity << ",\n";
    file << "  \"ambient_color\": [" << meta.ambientColor.x << ", " << meta.ambientColor.y << ", " << meta.ambientColor.z << "],\n";
    file << "  \n";
    file << "  \"brushes\": [\n";

    const auto& brushes = map.GetBrushes();
    for (size_t i = 0; i < brushes.size(); i++) {
        const auto& brush = brushes[i];
        file << "    {\n";
        file << "      \"shape\": \"" << BrushShapeToString(brush.shape) << "\",\n";
        file << "      \"position\": [" << brush.position.x << ", " << brush.position.y << ", " << brush.position.z << "],\n";
        file << "      \"size\": [" << brush.size.x << ", " << brush.size.y << ", " << brush.size.z << "],\n";
        if (brush.rotation != Vec3(0.0f)) {
            file << "      \"rotation\": [" << brush.rotation.x << ", " << brush.rotation.y << ", " << brush.rotation.z << "],\n";
        }
        file << "      \"material\": \"" << brush.materialName << "\"";

        // Write flags
        if (HasFlag(brush.flags, BrushFlags::Stair)) file << ",\n      \"stair\": true";
        if (HasFlag(brush.flags, BrushFlags::NoCollision)) file << ",\n      \"no_collision\": true";
        if (HasFlag(brush.flags, BrushFlags::Trigger)) file << ",\n      \"trigger\": true";
        if (HasFlag(brush.flags, BrushFlags::NoRender)) file << ",\n      \"no_render\": true";
        if (HasFlag(brush.flags, BrushFlags::Detail)) file << ",\n      \"detail\": true";
        if (!brush.layer.empty() && brush.layer != "default") {
            file << ",\n      \"layer\": \"" << brush.layer << "\"";
        }
        if (!brush.name.empty()) {
            file << ",\n      \"name\": \"" << brush.name << "\"";
        }

        file << "\n    }";
        if (i < brushes.size() - 1) file << ",";
        file << "\n";
    }

    file << "  ],\n";
    file << "  \n";
    file << "  \"entities\": [\n";

    const auto& entities = map.GetEntities();
    for (size_t i = 0; i < entities.size(); i++) {
        const auto& entity = entities[i];
        file << "    {\n";
        file << "      \"classname\": \"" << entity.classname << "\",\n";
        if (!entity.targetname.empty()) {
            file << "      \"targetname\": \"" << entity.targetname << "\",\n";
        }
        file << "      \"position\": [" << entity.position.x << ", " << entity.position.y << ", " << entity.position.z << "],\n";
        file << "      \"rotation\": [" << entity.rotation.x << ", " << entity.rotation.y << ", " << entity.rotation.z << "]\n";
        file << "    }";
        if (i < entities.size() - 1) file << ",";
        file << "\n";
    }

    file << "  ]\n";
    file << "}\n";

    file.close();

    LOG_INFO("MapLoader", "Saved map to " + fullPath);
    return true;
}

bool MapLoader::SaveSimple(const Map& map, const std::string& filepath) {
    ClearError();

    std::string fullPath = m_basePath + filepath;
    std::ofstream file(fullPath);
    if (!file.is_open()) {
        SetError("Failed to open file for writing: " + fullPath);
        return false;
    }

    const auto& meta = map.GetMetadata();

    // Write metadata
    file << "# Genesis Engine Map File\n";
    file << "# Format: shape posX posY posZ sizeX sizeY sizeZ material [flags]\n";
    file << "#\n";
    file << "@name \"" << meta.name << "\"\n";
    file << "@author \"" << meta.author << "\"\n";
    file << "@spawn " << meta.spawnPosition.x << " " << meta.spawnPosition.y << " " << meta.spawnPosition.z << "\n";
    file << "\n";

    // Write brushes
    for (const auto& brush : map.GetBrushes()) {
        file << BrushShapeToString(brush.shape) << " ";
        file << brush.position.x << " " << brush.position.y << " " << brush.position.z << " ";
        file << brush.size.x << " " << brush.size.y << " " << brush.size.z << " ";
        file << brush.materialName;

        // Write flags
        if (HasFlag(brush.flags, BrushFlags::Stair)) file << " stair";
        if (HasFlag(brush.flags, BrushFlags::NoCollision)) file << " nocollision";
        if (HasFlag(brush.flags, BrushFlags::Trigger)) file << " trigger";
        if (HasFlag(brush.flags, BrushFlags::NoRender)) file << " norender";
        if (HasFlag(brush.flags, BrushFlags::Detail)) file << " detail";

        file << "\n";
    }

    file.close();

    LOG_INFO("MapLoader", "Saved simple map to " + fullPath);
    return true;
}

void MapLoader::SetError(const std::string& error) {
    m_lastError = error;
    LOG_ERROR("MapLoader", error);
    if (m_errorCallback) {
        m_errorCallback(error);
    }
}

} // namespace Genesis

