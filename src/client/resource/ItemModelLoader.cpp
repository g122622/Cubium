/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include "ItemModelLoader.hpp"
#include "common/resource/IResourcePack.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <glm/gtc/matrix_transform.hpp>

namespace mc::client::resource {

// ============================================================================
// 辅助函数
// ============================================================================

ItemDisplayContext parseDisplayContext(std::string_view str)
{
    if (str == "thirdperson_righthand") return ItemDisplayContext::ThirdPersonRightHand;
    if (str == "thirdperson_lefthand") return ItemDisplayContext::ThirdPersonLeftHand;
    if (str == "firstperson_righthand") return ItemDisplayContext::FirstPersonRightHand;
    if (str == "firstperson_lefthand") return ItemDisplayContext::FirstPersonLeftHand;
    if (str == "head") return ItemDisplayContext::Head;
    if (str == "gui") return ItemDisplayContext::Gui;
    if (str == "ground") return ItemDisplayContext::Ground;
    if (str == "fixed") return ItemDisplayContext::Fixed;
    return ItemDisplayContext::Gui; // 默认
}

std::string displayContextToString(ItemDisplayContext ctx)
{
    switch (ctx) {
        case ItemDisplayContext::ThirdPersonRightHand:
            return "thirdperson_righthand";
        case ItemDisplayContext::ThirdPersonLeftHand:
            return "thirdperson_lefthand";
        case ItemDisplayContext::FirstPersonRightHand:
            return "firstperson_righthand";
        case ItemDisplayContext::FirstPersonLeftHand:
            return "firstperson_lefthand";
        case ItemDisplayContext::Head:
            return "head";
        case ItemDisplayContext::Gui:
            return "gui";
        case ItemDisplayContext::Ground:
            return "ground";
        case ItemDisplayContext::Fixed:
            return "fixed";
        default:
            return "gui";
    }
}

// ============================================================================
// ItemTransform
// ============================================================================

ItemTransform ItemTransform::identity()
{
    return ItemTransform{};
}

ItemTransform ItemTransform::fromJson(const nlohmann::json& json)
{
    ItemTransform t;

    if (json.contains("rotation") && json["rotation"].is_array() && json["rotation"].size() >= 3) {
        t.rotation =
            glm::vec3(json["rotation"][0].get<f32>(), json["rotation"][1].get<f32>(), json["rotation"][2].get<f32>());
    }

    if (json.contains("translation") && json["translation"].is_array() && json["translation"].size() >= 3) {
        // 物品模型 JSON 使用像素单位，转换为方块单位（除以 16）
        t.translation = glm::vec3(json["translation"][0].get<f32>() / 16.0f,
            json["translation"][1].get<f32>() / 16.0f,
            json["translation"][2].get<f32>() / 16.0f);
    }

    if (json.contains("scale") && json["scale"].is_array() && json["scale"].size() >= 3) {
        t.scale = glm::vec3(json["scale"][0].get<f32>(), json["scale"][1].get<f32>(), json["scale"][2].get<f32>());
    }

    return t;
}

glm::mat4 ItemTransform::toMatrix() const
{
    glm::mat4 mat = glm::mat4(1.0f);

    // 顺序：平移 -> 旋转 -> 缩放（应用顺序相反）
    mat = glm::translate(mat, translation);

    // 旋转顺序：Z -> Y -> X
    mat = glm::rotate(mat, glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
    mat = glm::rotate(mat, glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
    mat = glm::rotate(mat, glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));

    mat = glm::scale(mat, scale);

    return mat;
}

// ============================================================================
// ItemModelOverride
// ============================================================================

ItemModelOverride ItemModelOverride::fromJson(const nlohmann::json& json)
{
    ItemModelOverride override;

    if (json.contains("predicate") && json["predicate"].is_object()) {
        for (auto& [key, value] : json["predicate"].items()) {
            if (value.is_number()) {
                override.predicates[key] = value.get<f32>();
            }
        }
    }

    if (json.contains("model") && json["model"].is_string()) {
        override.model = ResourceLocation(json["model"].get<std::string>());
    }

    return override;
}

// ============================================================================
// BakedItemModel
// ============================================================================

const ItemTransform& BakedItemModel::getTransform(ItemDisplayContext ctx) const
{
    static const ItemTransform identity = ItemTransform::identity();

    auto it = display.find(ctx);
    if (it != display.end()) {
        return it->second;
    }
    return identity;
}

ResourceLocation BakedItemModel::resolveTexture(std::string_view textureRef) const
{
    std::string ref(textureRef);
    if (ref.empty() || ref[0] != '#') {
        // 直接路径
        return ResourceLocation(ref);
    }

    // 去掉 # 前缀
    std::string varName = ref.substr(1);

    // 查找纹理变量
    auto it = textures.find(varName);
    if (it != textures.end()) {
        return it->second;
    }

    // 未找到
    return ResourceLocation();
}

// ============================================================================
// ItemModelLoader
// ============================================================================

ItemModelLoader::ItemModelLoader(const std::vector<ResourcePackPtr>& resourcePacks)
    : m_resourcePacks(resourcePacks)
{
    _loadDefaultTransforms();
}

void ItemModelLoader::_loadDefaultTransforms()
{
    // item/generated 默认变换
    m_generatedDefaults[ItemDisplayContext::ThirdPersonRightHand] =
        ItemTransform{glm::vec3(0.0f, -90.0f, 55.0f), glm::vec3(0.0f, 1.5f, -1.5f), glm::vec3(0.55f, 0.55f, 0.55f)};
    m_generatedDefaults[ItemDisplayContext::ThirdPersonLeftHand] =
        ItemTransform{glm::vec3(0.0f, 90.0f, -55.0f), glm::vec3(0.0f, 1.5f, -1.5f), glm::vec3(0.55f, 0.55f, 0.55f)};
    m_generatedDefaults[ItemDisplayContext::FirstPersonRightHand] =
        ItemTransform{glm::vec3(0.0f, -90.0f, 25.0f), glm::vec3(1.13f, 3.2f, 1.13f), glm::vec3(0.68f, 0.68f, 0.68f)};
    m_generatedDefaults[ItemDisplayContext::FirstPersonLeftHand] =
        ItemTransform{glm::vec3(0.0f, 90.0f, -25.0f), glm::vec3(1.13f, 3.2f, 1.13f), glm::vec3(0.68f, 0.68f, 0.68f)};
    m_generatedDefaults[ItemDisplayContext::Head] = ItemTransform{
        glm::vec3(0.0f, 180.0f, 0.0f), glm::vec3(0.0f, 13.0f / 16.0f, 7.0f / 16.0f), glm::vec3(1.0f, 1.0f, 1.0f)};
    m_generatedDefaults[ItemDisplayContext::Gui] =
        ItemTransform{glm::vec3(30.0f, 225.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.625f, 0.625f, 0.625f)};
    m_generatedDefaults[ItemDisplayContext::Ground] =
        ItemTransform{glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 3.0f / 16.0f, 0.0f), glm::vec3(0.25f, 0.25f, 0.25f)};
    m_generatedDefaults[ItemDisplayContext::Fixed] =
        ItemTransform{glm::vec3(0.0f, 180.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.5f, 0.5f, 0.5f)};

    // item/handheld 默认变换（工具/武器）
    m_handheldDefaults[ItemDisplayContext::ThirdPersonRightHand] = ItemTransform{
        glm::vec3(0.0f, -90.0f, 55.0f), glm::vec3(0.0f, 4.0f / 16.0f, 0.5f / 16.0f), glm::vec3(0.85f, 0.85f, 0.85f)};
    m_handheldDefaults[ItemDisplayContext::ThirdPersonLeftHand] = ItemTransform{
        glm::vec3(0.0f, 90.0f, -55.0f), glm::vec3(0.0f, 4.0f / 16.0f, 0.5f / 16.0f), glm::vec3(0.85f, 0.85f, 0.85f)};
    m_handheldDefaults[ItemDisplayContext::FirstPersonRightHand] =
        ItemTransform{glm::vec3(0.0f, -90.0f, 25.0f), glm::vec3(1.13f, 3.2f, 1.13f), glm::vec3(0.68f, 0.68f, 0.68f)};
    m_handheldDefaults[ItemDisplayContext::FirstPersonLeftHand] =
        ItemTransform{glm::vec3(0.0f, 90.0f, -25.0f), glm::vec3(1.13f, 3.2f, 1.13f), glm::vec3(0.68f, 0.68f, 0.68f)};
    m_handheldDefaults[ItemDisplayContext::Head] = m_generatedDefaults[ItemDisplayContext::Head];
    m_handheldDefaults[ItemDisplayContext::Gui] = m_generatedDefaults[ItemDisplayContext::Gui];
    m_handheldDefaults[ItemDisplayContext::Ground] = m_generatedDefaults[ItemDisplayContext::Ground];
    m_handheldDefaults[ItemDisplayContext::Fixed] = m_generatedDefaults[ItemDisplayContext::Fixed];
}

Result<void> ItemModelLoader::loadAllModels()
{
    // TODO: 实现全量物品模型加载，当前采用延迟加载策略
    return Result<void>::ok();
}

Result<std::string> ItemModelLoader::_readModelFromResourcePacks(const std::string& filePath)
{
    // filePath 已是相对于 PackType 根目录的路径（如 "minecraft/models/item/stone.json"）
    // 无需再剥离 "assets/" 前缀
    const std::string& relativePath = filePath;

    for (const auto& pack : m_resourcePacks) {
        if (pack == nullptr) continue;

        auto result = pack->readTextResource(mc::resource::PackType::ClientResources, relativePath);
        if (result.success() && !result.value().empty()) {
            return result;
        }
    }

    return Error(ErrorCode::ResourceNotFound, "Item model not found: " + filePath);
}

Result<UnbakedItemModel> ItemModelLoader::loadModel(const ResourceLocation& location)
{
    // 检查缓存
    auto it = m_unbakedModels.find(location);
    if (it != m_unbakedModels.end()) {
        return it->second;
    }

    // 构建相对于 PackType 根目录的路径（不含 "assets/" 前缀）
    std::string filePath = location.namespace_() + "/models/" + location.path() + ".json";

    // 从资源包读取
    auto readResult = _readModelFromResourcePacks(filePath);
    if (!readResult.success()) {
        return readResult.error();
    }

    // 解析 JSON
    auto parseResult = _parseModel(location, readResult.value());
    if (!parseResult.success()) {
        return parseResult;
    }

    // 缓存
    auto& model = m_unbakedModels[location];
    model = parseResult.value();
    return model;
}

Result<UnbakedItemModel> ItemModelLoader::_parseModel(const ResourceLocation& location, std::string_view jsonContent)
{
    UnbakedItemModel model;
    model.location = location;
    model.name = location.path();

    try {
        auto json = nlohmann::json::parse(jsonContent);

        // 解析父模型
        if (json.contains("parent") && json["parent"].is_string()) {
            std::string parent = json["parent"].get<std::string>();
            model.parentLocation = ResourceLocation(parent);
        }

        // 解析纹理
        if (json.contains("textures") && json["textures"].is_object()) {
            _parseTextures(model, json["textures"]);
        }

        // 解析显示变换
        if (json.contains("display") && json["display"].is_object()) {
            _parseDisplay(model, json["display"]);
        }

        // 解析元素（可选，用于 3D 物品）
        if (json.contains("elements") && json["elements"].is_array()) {
            auto elemResult = _parseElements(model, json["elements"]);
            if (!elemResult.success()) {
                return elemResult.error();
            }
        }

        // 解析覆盖条件
        if (json.contains("overrides") && json["overrides"].is_array()) {
            _parseOverrides(model, json["overrides"]);
        }

        // 解析环境光遮蔽
        if (json.contains("ambientocclusion") && json["ambientocclusion"].is_boolean()) {
            model.ambientOcclusion = json["ambientocclusion"].get<bool>();
        }

        // 确定模型类型
        model.type = _determineModelType(model.parentLocation, !model.elements.empty());
    }
    catch (const nlohmann::json::exception& e) {
        return Error(
            ErrorCode::ResourceParseError, "JSON parse error in item model " + location.toString() + ": " + e.what());
    }

    return model;
}

void ItemModelLoader::_parseTextures(UnbakedItemModel& model, const nlohmann::json& textures)
{
    for (auto& [key, value] : textures.items()) {
        if (value.is_string()) {
            model.textures[key] = value.get<std::string>();
        }
    }
}

void ItemModelLoader::_parseDisplay(UnbakedItemModel& model, const nlohmann::json& display)
{
    for (auto& [key, value] : display.items()) {
        if (!value.is_object()) continue;

        ItemDisplayContext ctx = parseDisplayContext(key);
        if (ctx == ItemDisplayContext::Count) continue;

        model.display[ctx] = ItemTransform::fromJson(value);
    }
}

void ItemModelLoader::_parseOverrides(UnbakedItemModel& model, const nlohmann::json& overrides)
{
    for (const auto& override : overrides) {
        if (!override.is_object()) continue;
        model.overrides.push_back(ItemModelOverride::fromJson(override));
    }
}

Result<void> ItemModelLoader::_parseElements(UnbakedItemModel& model, const nlohmann::json& elements)
{
    // TODO: 提取公共解析方法，与 BlockModelLoader 复用元素解析逻辑

    for (const auto& elemJson : elements) {
        if (!elemJson.is_object()) continue;

        ModelElement elem;

        // from
        if (elemJson.contains("from") && elemJson["from"].is_array() && elemJson["from"].size() >= 3) {
            elem.from = glm::vec3(
                elemJson["from"][0].get<f32>(), elemJson["from"][1].get<f32>(), elemJson["from"][2].get<f32>());
        }

        // to
        if (elemJson.contains("to") && elemJson["to"].is_array() && elemJson["to"].size() >= 3) {
            elem.to =
                glm::vec3(elemJson["to"][0].get<f32>(), elemJson["to"][1].get<f32>(), elemJson["to"][2].get<f32>());
        }

        // shade
        if (elemJson.contains("shade") && elemJson["shade"].is_boolean()) {
            elem.shade = elemJson["shade"].get<bool>();
        }

        // rotation
        if (elemJson.contains("rotation") && elemJson["rotation"].is_object()) {
            auto& rotJson = elemJson["rotation"];
            if (rotJson.contains("origin") && rotJson["origin"].is_array() && rotJson["origin"].size() >= 3) {
                elem.rotation.origin = glm::vec3(
                    rotJson["origin"][0].get<f32>(), rotJson["origin"][1].get<f32>(), rotJson["origin"][2].get<f32>());
            }
            if (rotJson.contains("axis") && rotJson["axis"].is_string()) {
                elem.rotation.axis = rotJson["axis"].get<std::string>();
            }
            if (rotJson.contains("angle") && rotJson["angle"].is_number()) {
                elem.rotation.angle = rotJson["angle"].get<f32>();
            }
            if (rotJson.contains("rescale") && rotJson["rescale"].is_boolean()) {
                elem.rotation.rescale = rotJson["rescale"].get<bool>();
            }
        }

        // faces
        if (elemJson.contains("faces") && elemJson["faces"].is_object()) {
            auto& facesJson = elemJson["faces"];
            for (auto& [dirStr, faceJson] : facesJson.items()) {
                Direction dir = parseDirection(dirStr);
                if (dir == Direction::None) continue;

                ModelFace face;

                if (faceJson.contains("texture") && faceJson["texture"].is_string()) {
                    face.texture = faceJson["texture"].get<std::string>();
                }

                if (faceJson.contains("cullface") && faceJson["cullface"].is_string()) {
                    face.cullFace = parseDirection(faceJson["cullface"].get<std::string>());
                }

                if (faceJson.contains("tintindex") && faceJson["tintindex"].is_number()) {
                    face.tintIndex = faceJson["tintindex"].get<i32>();
                }

                // uv
                if (faceJson.contains("uv") && faceJson["uv"].is_array() && faceJson["uv"].size() >= 4) {
                    face.uv.u0 = faceJson["uv"][0].get<f32>();
                    face.uv.v0 = faceJson["uv"][1].get<f32>();
                    face.uv.u1 = faceJson["uv"][2].get<f32>();
                    face.uv.v1 = faceJson["uv"][3].get<f32>();
                }

                // rotation
                if (faceJson.contains("rotation") && faceJson["rotation"].is_number()) {
                    face.uv.rotation = faceJson["rotation"].get<i32>();
                }

                elem.faces[dir] = face;
            }
        }

        model.elements.push_back(elem);
    }

    return Result<void>::ok();
}

ItemModelType ItemModelLoader::_determineModelType(const ResourceLocation& parent, bool hasElements) const
{
    std::string parentPath = parent.path();

    // 检查父模型类型
    if (parentPath == "item/generated" || parentPath.find("generated") != std::string::npos) {
        return ItemModelType::Generated;
    }

    if (parentPath == "item/handheld" || parentPath == "item/handheld_rod" ||
        parentPath.find("handheld") != std::string::npos) {
        return ItemModelType::Handheld;
    }

    // 方块物品
    if (parentPath.find("block/") != std::string::npos) {
        return ItemModelType::Block;
    }

    // 有 elements 但不是以上类型，视为自定义 3D 模型
    if (hasElements) {
        return ItemModelType::Custom;
    }

    // 默认为平面图标
    return ItemModelType::Generated;
}

void ItemModelLoader::_mergeParent(UnbakedItemModel& child, const UnbakedItemModel& parent)
{
    // TODO: bakeModel 中存在重复的合并逻辑，应重构为调用此方法
    // 合并纹理（子模型覆盖父模型）
    for (const auto& [key, value] : parent.textures) {
        if (child.textures.find(key) == child.textures.end()) {
            child.textures[key] = value;
        }
    }

    // 合并元素（只有子模型没有元素时才继承）
    if (child.elements.empty() && !parent.elements.empty()) {
        child.elements = parent.elements;
    }

    // 合并显示变换（子模型覆盖父模型）
    for (const auto& [ctx, transform] : parent.display) {
        if (child.display.find(ctx) == child.display.end()) {
            child.display[ctx] = transform;
        }
    }

    // 合并 ambientOcclusion
    if (!child.ambientOcclusion && parent.ambientOcclusion) {
        child.ambientOcclusion = parent.ambientOcclusion;
    }
}

Result<BakedItemModel> ItemModelLoader::bakeModel(const ResourceLocation& location)
{
    // 检查缓存
    auto it = m_bakedModels.find(location);
    if (it != m_bakedModels.end()) {
        return it->second;
    }

    // 加载未烘焙模型
    auto loadResult = loadModel(location);
    if (!loadResult.success()) {
        return loadResult.error();
    }

    // 解析父模型链
    std::vector<UnbakedItemModel*> modelChain;
    ResourceLocation currentLoc = location;
    while (true) {
        auto modelIt = m_unbakedModels.find(currentLoc);
        if (modelIt == m_unbakedModels.end()) {
            // 尝试加载
            auto result = loadModel(currentLoc);
            if (!result.success()) {
                break;
            }
            modelIt = m_unbakedModels.find(currentLoc);
            if (modelIt == m_unbakedModels.end()) {
                break;
            }
        }
        modelChain.push_back(&modelIt->second);
        if (!modelIt->second.hasParent()) {
            break;
        }
        currentLoc = modelIt->second.parentLocation;
    }

    // 从根到叶合并
    BakedItemModel baked;
    baked.location = location;

    // 首先确定模型类型和默认变换
    bool hasHandheldParent = false;
    for (auto it = modelChain.rbegin(); it != modelChain.rend(); ++it) {
        auto& model = *it;
        if (model->parentLocation.path().find("handheld") != std::string::npos) {
            hasHandheldParent = true;
            break;
        }
    }

    // 设置默认变换
    if (hasHandheldParent) {
        baked.display = m_handheldDefaults;
    } else {
        baked.display = m_generatedDefaults;
    }

    // 合并属性
    for (auto it = modelChain.rbegin(); it != modelChain.rend(); ++it) {
        auto& model = *it;

        // 合并纹理
        for (const auto& [key, value] : model->textures) {
            baked.textures[key] = ResourceLocation(value);
        }

        // 合并元素
        if (baked.elements.empty() && !model->elements.empty()) {
            baked.elements = model->elements;
        }

        // 合并显示变换（子模型覆盖默认）
        for (const auto& [ctx, transform] : model->display) {
            baked.display[ctx] = transform;
        }

        // 继承模型类型
        baked.type = model->type;
    }

    // 解析纹理引用链
    _resolveTextureReferences(baked);

    // 提取纹理层（layer0, layer1, ...）
    for (u32 i = 0;; ++i) {
        std::string layerKey = "layer" + std::to_string(i);
        auto texIt = baked.textures.find(layerKey);
        if (texIt != baked.textures.end()) {
            baked.textureLayers.push_back(texIt->second);
        } else {
            break;
        }
    }

    // 复制 overrides
    if (!modelChain.empty()) {
        baked.overrides = modelChain.front()->overrides;
    }

    // 缓存并返回
    m_bakedModels[location] = baked;
    return baked;
}

void ItemModelLoader::_resolveTextureReferences(BakedItemModel& baked)
{
    // 递归解析 #variable 形式的纹理引用
    bool changed = true;
    i32 maxIterations = 10;
    while (changed && maxIterations-- > 0) {
        changed = false;
        for (auto& [name, texLoc] : baked.textures) {
            std::string path = texLoc.path();
            if (!path.empty() && path[0] == '#') {
                std::string varName = path.substr(1);
                auto varIt = baked.textures.find(varName);
                if (varIt != baked.textures.end()) {
                    texLoc = varIt->second;
                    changed = true;
                }
            }
        }
    }
}

const BakedItemModel* ItemModelLoader::getModel(const ResourceLocation& location) const
{
    auto it = m_bakedModels.find(location);
    if (it != m_bakedModels.end()) {
        return &it->second;
    }
    return nullptr;
}

bool ItemModelLoader::hasModel(const ResourceLocation& location) const
{
    return m_bakedModels.find(location) != m_bakedModels.end() ||
        m_unbakedModels.find(location) != m_unbakedModels.end();
}

void ItemModelLoader::clearCache()
{
    m_unbakedModels.clear();
    m_bakedModels.clear();
}

} // namespace mc::client::resource
