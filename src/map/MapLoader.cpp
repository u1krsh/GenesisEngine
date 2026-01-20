#include "MapLoader.h"
#include "renderer/material/MaterialLibrary.h"
#include "renderer/texture/TextureLibrary.h"
#include "map/MeshLibrary.h"
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

MapPtr MapLoader::Load(const std::string& filepath, bool skipBuild) {
    ClearError();

    // Don't prepend base path if filepath is already absolute
    std::string fullPath;
    if (!filepath.empty() && (filepath[0] == '/' || (filepath.size() > 1 && filepath[1] == ':'))) {
        fullPath = filepath;  // Already absolute (Unix or Windows)
    } else {
        fullPath = m_basePath + filepath;
    }

    // Detect format by extension
    size_t dotPos = filepath.rfind('.');
    std::string ext = (dotPos != std::string::npos) ? filepath.substr(dotPos) : "";
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    std::cout << "Loaded map: " << filepath << std::endl;
    if (ext == ".json") {
        return LoadJSON(fullPath, skipBuild);
    } else if (ext == ".sau") {
        return LoadSAU(fullPath, skipBuild);
    } else if (ext == ".map" || ext == ".txt") {
        return LoadSimple(fullPath, skipBuild);
    } else {
        // Default to JSON
        return LoadJSON(fullPath, skipBuild);
    }
}

MapPtr MapLoader::LoadJSON(const std::string& filepath, bool skipBuild) {
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

    auto map = LoadFromString(content);
    if (map && !skipBuild) {
        BuildMap(*map);
    }
    return map;
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

    return map;
}

MapPtr MapLoader::LoadSimple(const std::string& filepath, bool skipBuild) {
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
    if (!skipBuild) {
        BuildMap(*map);
    }

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
    
    // Shader
    std::string shaderStr = ExtractString(json, "shader");
    if (!shaderStr.empty()) {
        brush.shaderType = StringToShaderType(shaderStr);
        
        // Load shader specific properties
        brush.transparency = ExtractNumber(json, "transparency", 0.3f);
        brush.fresnelPower = ExtractNumber(json, "fresnel", 2.0f);
        brush.roughness = ExtractNumber(json, "roughness", 0.1f);
        
        brush.normalMapPath = ExtractString(json, "normal_map");
        brush.maskMapPath = ExtractString(json, "mask_map");
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
    
    // Create unique cache key including shader type to prevent cache collisions
    // e.g., "glass/glasswindow001a.png__glass" vs "glass/glasswindow001a.png__standard"
    std::string materialKey = brush.materialName;
    if (brush.shaderType == ShaderType::Glass) {
        materialKey += "__glass";
    } else if (brush.shaderType == ShaderType::GlassReal) {
        materialKey += "__glass_real";
    }
    
    // Get material from library or create if missing
    brush.material = matLib.Get(materialKey);

    // If material not found, try to create from texture or fallback to color
    if (!brush.material) {
        // Try to load as texture
        auto& texLib = TextureLibrary::Instance();
        // Check if it's a file path (has extension)
        bool isFilePath = brush.materialName.find('.') != std::string::npos;
        
        if (isFilePath) {
            auto texture = texLib.Load(brush.materialName);
            if (texture) {
                // Texture found! Create material based on shader type
                if (brush.shaderType == ShaderType::Glass) {
                     brush.material = matLib.CreateGlass(materialKey, brush.tintColor, 1.0f - brush.transparency);
                } else if (brush.shaderType == ShaderType::GlassReal) {
                     // Create material for realistic glass - stores all properties
                     brush.material = matLib.CreateFromTemplate(materialKey, "LitOpaque");
                     if (brush.material) {
                         brush.material->SetVec3("u_GlassTint", brush.tintColor);
                         brush.material->SetFloat("u_IOR", brush.ior);
                         brush.material->SetFloat("u_Thickness", brush.thickness);
                         brush.material->SetFloat("u_FresnelPower", brush.fresnelPower);
                         brush.material->SetFloat("u_Absorption", brush.absorption);
                         brush.material->SetFloat("u_Roughness", brush.roughness);
                         brush.material->SetFloat("u_Alpha", 1.0f - brush.transparency);
                         brush.material->SetBlendMode(BlendMode::Transparent);
                         brush.material->SetRenderQueue(RenderQueue::Transparent);
                     }
                } else {
                     brush.material = matLib.CreateFromTemplate(materialKey, "LitOpaque");
                }
                
                if (brush.material) {
                    // Set texture wrap mode based on tileTexture property
                    if (brush.tileTexture) {
                        texture->SetWrap(TextureWrap::Repeat);  // Texture tiles/repeats
                    } else {
                        texture->SetWrap(TextureWrap::Clamp);   // Texture stretches to fit
                    }
                    
                    brush.material->SetTexture("u_BaseTexture", texture, 0);
                    brush.material->SetInt("u_HasBaseTexture", 1);
                    
                    // Auto-detect normal map using naming convention: name_normal.png
                    if (brush.normalMapPath.empty()) {
                        // Build normal map path from base texture path
                        std::string basePath = brush.materialName;
                        size_t dotPos = basePath.rfind('.');
                        if (dotPos != std::string::npos) {
                            std::string normalPath = basePath.substr(0, dotPos) + "_normal" + basePath.substr(dotPos);
                            std::cout << "[NormalMap] Trying to load: " << normalPath << std::endl;
                            auto normalMap = texLib.Load(normalPath);
                            if (normalMap) {
                                brush.material->SetTexture("u_NormalTexture", normalMap, 2);
                                brush.material->SetInt("u_HasNormalMap", 1);
                                std::cout << "[NormalMap] SUCCESS: Loaded normal map for " << brush.materialName << std::endl;
                                LOG_INFO("MapLoader", "Auto-detected normal map: " + normalPath);
                            } else {
                                std::cout << "[NormalMap] FAILED: No normal map found at " << normalPath << std::endl;
                            }
                        }
                    }
                    
                    LOG_DEBUG("MapLoader", "Created material '" + materialKey + "' with texture");
                }
            } else {
                LOG_WARNING("MapLoader", "Failed to load texture: " + brush.materialName);
            }
        }
        
        // If still no material (not a texture or load failed), create solid color
        if (!brush.material) {
            // For GlassReal, create material even without texture
            if (brush.shaderType == ShaderType::GlassReal) {
                brush.material = matLib.CreateFromTemplate(materialKey, "LitOpaque");
                if (brush.material) {
                    brush.material->SetVec3("u_GlassTint", brush.tintColor);
                    brush.material->SetFloat("u_IOR", brush.ior);
                    brush.material->SetFloat("u_Thickness", brush.thickness);
                    brush.material->SetFloat("u_FresnelPower", brush.fresnelPower);
                    brush.material->SetFloat("u_Absorption", brush.absorption);
                    brush.material->SetFloat("u_Roughness", brush.roughness);
                    brush.material->SetFloat("u_Alpha", 1.0f - brush.transparency);
                    brush.material->SetBlendMode(BlendMode::Transparent);
                    brush.material->SetRenderQueue(RenderQueue::Transparent);
                    LOG_DEBUG("MapLoader", "Created glass_real material '" + materialKey + "'");
                }
            } else {
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
        
                brush.material = matLib.CreateSolidColor(materialKey, color);
                LOG_DEBUG("MapLoader", "Created solid color material '" + materialKey + "'");
            }
        }
    }
    
    // Validate material exists now
    if (brush.material) {
        // Set shader type and ACTUALLY switch shader program based on type
        brush.material->SetShaderType(brush.shaderType);
        
        // Switch shader program based on shader type
        auto& shaderLib = ShaderLibrary::Instance();
        switch (brush.shaderType) {
            case ShaderType::Glass: {
                auto glassShader = shaderLib.Get("glass");
                if (glassShader) {
                    brush.material->SetShader(glassShader);
                }
                // Set glass-specific properties
                brush.material->SetFloat("u_Transparency", brush.transparency);
                brush.material->SetFloat("u_FresnelPower", brush.fresnelPower);
                brush.material->SetFloat("u_Roughness", brush.roughness);
                brush.material->SetVec3("u_TintColor", brush.tintColor);
                
                // Ensure render state is correct for glass
                brush.material->SetBlendMode(BlendMode::Transparent);
                brush.material->SetRenderQueue(RenderQueue::Transparent);
                break;
            }
            case ShaderType::Metal: {
                auto meshShader = shaderLib.Get("mesh");
                if (meshShader) {
                    brush.material->SetShader(meshShader);
                }
                brush.material->SetFloat("u_Roughness", brush.roughness);
                brush.material->SetFloat("u_Metallic", brush.metallic);
                break;
            }
            case ShaderType::Unlit: {
                auto unlitShader = shaderLib.Get("basic");
                if (unlitShader) {
                    brush.material->SetShader(unlitShader);
                }
                break;
            }
            case ShaderType::Standard:
            default: {
                auto meshShader = shaderLib.Get("mesh");
                if (meshShader) {
                    brush.material->SetShader(meshShader);
                }
                break;
            }
        }
        
        // Load additional maps (normal/mask)
        if (!brush.normalMapPath.empty()) {
            auto normalMap = TextureLibrary::Instance().Load(brush.normalMapPath);
            if (normalMap) {
                brush.material->SetTexture("u_NormalTexture", normalMap, 1);
                brush.material->SetInt("u_HasNormalMap", 1);
            }
        }
        
        if (!brush.maskMapPath.empty()) {
            auto maskMap = TextureLibrary::Instance().Load(brush.maskMapPath);
            if (maskMap) {
                brush.material->SetTexture("u_MaskTexture", maskMap, 2);
                brush.material->SetInt("u_HasMaskTexture", 1);
            }
        }
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
        
        // Write shader type if not standard
        if (brush.shaderType != ShaderType::Standard) {
             file << ",\n      \"shader\": \"" << ShaderTypeToString(brush.shaderType) << "\"";
             
             // Shader specific properties
             if (brush.shaderType == ShaderType::Glass) {
                 file << ",\n      \"transparency\": " << brush.transparency;
                 file << ",\n      \"fresnel\": " << brush.fresnelPower;
                 file << ",\n      \"roughness\": " << brush.roughness;
             }
             
             if (!brush.normalMapPath.empty()) {
                 file << ",\n      \"normal_map\": \"" << brush.normalMapPath << "\"";
             }
             if (!brush.maskMapPath.empty()) {
                 file << ",\n      \"mask_map\": \"" << brush.maskMapPath << "\"";
             }
        }

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

MapPtr MapLoader::LoadSAU(const std::string& filepath, bool skipBuild) {
    ClearError();

    std::ifstream file(filepath);
    if (!file.is_open()) {
        SetError("Failed to open SAU file: " + filepath);
        return nullptr;
    }

    auto map = std::make_shared<Map>();
    std::string line;
    std::string currentSection;
    Brush currentBrush;
    MapEntity currentEntity;
    bool inBrush = false;
    bool inEntity = false;

    while (std::getline(file, line)) {
        line = Trim(line);
        
        // Skip empty lines and comments
        if (line.empty() || line[0] == '/' || line[0] == '#') {
            continue;
        }

        // Section headers
        if (line == "metadata {") {
            currentSection = "metadata";
            continue;
        } else if (line == "brush {") {
            currentSection = "brush";
            inBrush = true;
            currentBrush = Brush();
            currentBrush.flags = BrushFlags::CastShadow | BrushFlags::ReceiveShadow;
            continue;
        } else if (line == "entity {") {
            currentSection = "entity";
            inEntity = true;
            currentEntity = MapEntity();
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
                std::stringstream ss(value);
                ss >> currentBrush.position.x >> currentBrush.position.y >> currentBrush.position.z;
            } else if (key == "size") {
                std::stringstream ss(value);
                ss >> currentBrush.size.x >> currentBrush.size.y >> currentBrush.size.z;
            } else if (key == "rotation") {
                std::stringstream ss(value);
                ss >> currentBrush.rotation.x >> currentBrush.rotation.y >> currentBrush.rotation.z;
            } else if (key == "material") {
                currentBrush.materialName = value;
            } else if (key == "shape") {
                currentBrush.shape = StringToBrushShape(value);
            } else if (key == "shader_type") {
                currentBrush.shaderType = StringToShaderType(value);
            } else if (key == "normal_map") {
                currentBrush.normalMapPath = value;
            } else if (key == "mask_map") {
                currentBrush.maskMapPath = value;
            } else if (key == "transparency") {
                try { currentBrush.transparency = std::stof(value); } catch(...) {}
            } else if (key == "fresnel_power") {
                try { currentBrush.fresnelPower = std::stof(value); } catch(...) {}
            } else if (key == "roughness") {
                try { currentBrush.roughness = std::stof(value); } catch(...) {}
            } else if (key == "metallic") {
                try { currentBrush.metallic = std::stof(value); } catch(...) {}
            } else if (key == "tint_color") {
                std::stringstream ss(value);
                ss >> currentBrush.tintColor.r >> currentBrush.tintColor.g >> currentBrush.tintColor.b;
            } else if (key == "tile_texture") {
                currentBrush.tileTexture = (value == "true" || value == "1");
            } else if (key == "ior") {
                try { currentBrush.ior = std::stof(value); } catch(...) {}
            } else if (key == "thickness") {
                try { currentBrush.thickness = std::stof(value); } catch(...) {}
            } else if (key == "absorption") {
                try { currentBrush.absorption = std::stof(value); } catch(...) {}
            } else if (key == "flags") {
                if (value.find("nocollision") != std::string::npos)
                    currentBrush.flags = currentBrush.flags | BrushFlags::NoCollision;
                if (value.find("stair") != std::string::npos)
                    currentBrush.flags = currentBrush.flags | BrushFlags::Stair;
                if (value.find("detail") != std::string::npos)
                    currentBrush.flags = currentBrush.flags | BrushFlags::Detail;
            }
        } else if (inEntity) {
            if (key == "classname") {
                currentEntity.classname = value;
            } else if (key == "position") {
                std::stringstream ss(value);
                ss >> currentEntity.position.x >> currentEntity.position.y >> currentEntity.position.z;
            } else if (key == "rotation") {
                std::stringstream ss(value);
                ss >> currentEntity.rotation.x >> currentEntity.rotation.y >> currentEntity.rotation.z;
            } else {
                // Store as custom property
                currentEntity.properties[key] = value;
            }
        }
    }

    file.close();

    LOG_INFO("MapLoader", "Loaded SAU map '" + map->GetMetadata().name + "' with " +
             std::to_string(map->GetBrushCount()) + " brushes, " +
             std::to_string(map->GetEntityCount()) + " entities");

    // Build the map
    if (!skipBuild) {
        BuildMap(*map);
    }

    return map;
}

void MapLoader::SetError(const std::string& error) {
    m_lastError = error;
    LOG_ERROR("MapLoader", error);
    if (m_errorCallback) {
        m_errorCallback(error);
    }
}

} // namespace Genesis

