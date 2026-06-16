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
#include "common/resource/pack/IResourcePack.hpp"
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
            model.hasElements = true;
            auto elemResult = _parseElements(model, json["elements"]);
            if (!elemResult.success()) {
                return elemResult.error();
            }
        }

        // 解析覆盖条件
        if (json.contains("overrides") && json["overrides"].is_array()) {
            model.hasOverrides = true;
            _parseOverrides(model, json["overrides"]);
        }

        // 解析环境光遮蔽
        if (json.contains("ambientocclusion") && json["ambientocclusion"].is_boolean()) {
            model.ambientOcclusion = json["ambientocclusion"].get<bool>();
            model.hasAmbientOcclusion = true;
        }
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
    // 复用 BlockModelLoader::parseElement 共享的元素解析逻辑，
    // 包含默认 UV 计算等完整功能
    for (const auto& elemJson : elements) {
        if (!elemJson.is_object()) continue;

        auto result = BlockModelLoader::parseElement(elemJson);
        if (result.success()) {
            model.elements.push_back(result.value());
        }
    }

    return Result<void>::ok();
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

    // === 合并策略：与 MC Java 版一致 ===
    BakedItemModel baked;
    baked.location = location;

    // === 合并策略：与 MC Java 版一致 ===
    // MC Java 版使用 findTop* 模式：沿叶子到根方向查找第一个显式定义了该属性的模型。
    // 对于纹理：采用 merge 语义（所有层的纹理变量合并，子模型覆盖父模型同名变量）。
    // 对于元素、环境光遮蔽、覆盖条件：采用 leaf-wins 语义（从叶子到根查找，第一个显式定义的生效）。
    // 对于显示变换：按每个 ItemDisplayContext 独立地采用 leaf-wins。
    // 对于模型类型：leaf-wins（最靠近叶子的类型生效）。

    // 1. 纹理：从根到叶逐层合并，后处理的层覆盖先处理的层的同名键（等效于 merge 语义）
    for (auto it = modelChain.rbegin(); it != modelChain.rend(); ++it) {
        for (const auto& [key, value] : (*it)->textures) {
            baked.textures[key] = ResourceLocation(value);
        }
    }

    // 2. 元素：从叶子到根查找第一个显式定义了 elements 的模型（leaf-wins）
    for (auto it = modelChain.begin(); it != modelChain.end(); ++it) {
        if ((*it)->hasElements) {
            baked.elements = (*it)->elements;
            break;
        }
    }

    // 3. 环境光遮蔽：从叶子到根查找第一个显式设置了 ambientocclusion 的模型（leaf-wins）
    // 默认值为 true（与 MC Java 版 DEFAULT_AMBIENT_OCCLUSION 一致）
    for (auto it = modelChain.begin(); it != modelChain.end(); ++it) {
        if ((*it)->hasAmbientOcclusion) {
            baked.ambientOcclusion = (*it)->ambientOcclusion;
            break;
        }
    }

    // 4. 模型类型：基于合并结果确定
    // 检查整条链中是否有 handheld 父模型
    bool hasHandheldParent = false;
    for (auto it = modelChain.rbegin(); it != modelChain.rend(); ++it) {
        if ((*it)->hasParent() && ((*it)->parentLocation.path().find("handheld") != std::string::npos)) {
            hasHandheldParent = true;
            break;
        }
    }
    if (hasHandheldParent) {
        baked.type = ItemModelType::Handheld;
    } else if (!baked.elements.empty()) {
        baked.type = ItemModelType::Custom;
    } else {
        baked.type = ItemModelType::Generated;
    }

    // 5. 覆盖条件：从叶子到根查找第一个显式定义了 overrides 的模型（leaf-wins）
    for (auto it = modelChain.begin(); it != modelChain.end(); ++it) {
        if ((*it)->hasOverrides) {
            baked.overrides = (*it)->overrides;
            break;
        }
    }

    // 6. 显示变换：按每个 ItemDisplayContext 独立地采用 leaf-wins
    // 首先根据是否手持类模型设置默认变换
    if (hasHandheldParent) {
        baked.display = m_handheldDefaults;
    } else {
        baked.display = m_generatedDefaults;
    }
    // 然后从根到叶逐层覆盖显示变换（后处理的层覆盖先处理的层的同名上下文）
    for (auto it = modelChain.rbegin(); it != modelChain.rend(); ++it) {
        for (const auto& [ctx, transform] : (*it)->display) {
            baked.display[ctx] = transform;
        }
    }

    // 7. 环境光遮蔽：已计算但 BakedItemModel 当前不包含 AO 字段，供未来使用

    // 解析纹理引用链
    BlockModelLoader::resolveTextureReferences(baked.textures);

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

    // 缓存并返回
    m_bakedModels[location] = baked;
    return baked;
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
