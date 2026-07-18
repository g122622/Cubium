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

#include "BlockModelLoader.hpp"
#include "common/profiler/TraceEvents.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include "common/util/math/random/Random.hpp"
#include <algorithm>
#include <cctype>
#include <nlohmann/json.hpp>

using namespace mc::trace;

namespace {

std::string trimWhitespace(std::string_view input)
{
    size_t begin = 0;
    size_t end = input.size();

    while (begin < end && std::isspace(static_cast<unsigned char>(input[begin]))) {
        ++begin;
    }
    while (end > begin && std::isspace(static_cast<unsigned char>(input[end - 1]))) {
        --end;
    }

    return std::string(input.substr(begin, end - begin));
}

} // namespace

namespace mc {

Direction parseDirection(std::string_view str)
{
    auto result = Directions::fromName(str);
    return result.has_value() ? result.value() : Direction::None;
}

std::string directionToString(Direction dir)
{
    if (dir == Direction::None) return "";
    return Directions::toString(dir);
}

ResourceLocation BakedBlockModel::resolveTexture(std::string_view textureRef) const
{
    std::string ref(textureRef);

    // 移除前导 #
    if (!ref.empty() && ref[0] == '#') {
        ref = ref.substr(1);
    }

    // 查找纹理变量
    auto it = textures.find(ref);
    if (it != textures.end()) {
        return it->second;
    }

    // 直接作为路径使用
    return ResourceLocation(ref);
}

const BlockStateVariant& VariantList::select() const
{
    return select(0);
}

const BlockStateVariant& VariantList::select(u64 seed) const
{
    if (variants.empty()) {
        static BlockStateVariant empty;
        return empty;
    }

    if (variants.size() == 1) {
        return variants[0];
    }

    // 计算总权重
    i32 totalWeight = 0;
    for (const auto& v : variants) {
        totalWeight += v.weight;
    }

    // 使用种子随机选择
    math::Random rng(seed);
    i32 value = rng.nextInt(totalWeight);

    // 选择变体
    i32 cumulative = 0;
    for (const auto& v : variants) {
        cumulative += v.weight;
        if (value < cumulative) {
            return v;
        }
    }

    return variants.back();
}

bool BlockStateVariant::operator==(const BlockStateVariant& other) const
{
    return model == other.model && x == other.x && y == other.y && uvLock == other.uvLock && weight == other.weight;
}

Result<BlockStateDefinition> BlockStateDefinition::parse(std::string_view jsonContent)
{
    try {
        auto json = nlohmann::json::parse(jsonContent);

        BlockStateDefinition def;

        // 解析 variants
        if (json.contains("variants")) {
            const auto& variants = json["variants"];

            for (auto it = variants.begin(); it != variants.end(); ++it) {
                std::string stateKey = _normalizeStateKey(it.key());
                VariantList list;

                if (it.value().is_array()) {
                    // 数组形式
                    for (const auto& v : it.value()) {
                        BlockStateVariant variant;

                        if (v.contains("model")) {
                            variant.model = ResourceLocation(v["model"].get<std::string>());
                        }

                        if (v.contains("x")) {
                            variant.x = v["x"].get<i32>();
                        }

                        if (v.contains("y")) {
                            variant.y = v["y"].get<i32>();
                        }

                        if (v.contains("uvlock")) {
                            variant.uvLock = v["uvlock"].get<bool>();
                        }

                        if (v.contains("weight")) {
                            variant.weight = v["weight"].get<i32>();
                        }

                        list.variants.push_back(variant);
                    }
                } else {
                    // 单个对象形式
                    BlockStateVariant variant;

                    if (it.value().contains("model")) {
                        variant.model = ResourceLocation(it.value()["model"].get<std::string>());
                    }

                    if (it.value().contains("x")) {
                        variant.x = it.value()["x"].get<i32>();
                    }

                    if (it.value().contains("y")) {
                        variant.y = it.value()["y"].get<i32>();
                    }

                    if (it.value().contains("uvlock")) {
                        variant.uvLock = it.value()["uvlock"].get<bool>();
                    }

                    if (it.value().contains("weight")) {
                        variant.weight = it.value()["weight"].get<i32>();
                    }

                    list.variants.push_back(variant);
                }

                def.m_variants[stateKey] = std::move(list);
            }
        }

        // 解析 multipart
        if (json.contains("multipart")) {
            def.m_hasMultipart = true;

            VariantList multipartList;
            const auto& multipart = json["multipart"];

            if (multipart.is_array()) {
                auto appendVariantFromJson = [&multipartList](const nlohmann::json& applyJson) {
                    if (!applyJson.is_object()) {
                        return;
                    }

                    BlockStateVariant variant;

                    if (applyJson.contains("model")) {
                        variant.model = ResourceLocation(applyJson["model"].get<std::string>());
                    }

                    if (applyJson.contains("x")) {
                        variant.x = applyJson["x"].get<i32>();
                    }

                    if (applyJson.contains("y")) {
                        variant.y = applyJson["y"].get<i32>();
                    }

                    if (applyJson.contains("uvlock")) {
                        variant.uvLock = applyJson["uvlock"].get<bool>();
                    }

                    if (applyJson.contains("weight")) {
                        variant.weight = applyJson["weight"].get<i32>();
                    }

                    if (!variant.model.path().empty()) {
                        multipartList.variants.push_back(std::move(variant));
                    }
                };

                for (const auto& part : multipart) {
                    if (!part.is_object() || !part.contains("apply")) {
                        continue;
                    }

                    const auto& apply = part["apply"];
                    if (apply.is_array()) {
                        for (const auto& applyEntry : apply) {
                            appendVariantFromJson(applyEntry);
                        }
                    } else {
                        appendVariantFromJson(apply);
                    }
                }
            }

            if (!multipartList.variants.empty()) {
                auto normalIt = def.m_variants.find("normal");
                if (normalIt == def.m_variants.end()) {
                    def.m_variants["normal"] = std::move(multipartList);
                } else {
                    auto& target = normalIt->second.variants;
                    target.insert(target.end(), multipartList.variants.begin(), multipartList.variants.end());
                }
            }
        }

        return def;
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::ResourceParseError, std::string("Failed to parse block state: ") + e.what());
    }
}

const VariantList* BlockStateDefinition::getVariants(std::string_view stateStr) const
{
    std::string key(stateStr);
    auto it = m_variants.find(key);
    if (it != m_variants.end()) {
        return &it->second;
    }

    std::string normalizedKey = _normalizeStateKey(stateStr);
    if (normalizedKey != key) {
        it = m_variants.find(normalizedKey);
        if (it != m_variants.end()) {
            return &it->second;
        }
    }

    // 尝试空键 (normal状态)
    if (stateStr == "normal" || stateStr == "") {
        it = m_variants.find("normal");
        if (it != m_variants.end()) {
            return &it->second;
        }
    }

    return nullptr;
}

std::string BlockStateDefinition::_normalizeStateKey(std::string_view stateKey)
{
    std::string trimmed = trimWhitespace(stateKey);
    if (trimmed.empty() || trimmed == "normal") {
        return "normal";
    }

    std::vector<std::pair<std::string, std::string>> props;

    size_t start = 0;
    while (start < trimmed.size()) {
        size_t end = trimmed.find(',', start);
        if (end == std::string::npos) {
            end = trimmed.size();
        }

        std::string token = trimWhitespace(std::string_view(trimmed.data() + start, end - start));
        if (!token.empty()) {
            size_t eq = token.find('=');
            if (eq != std::string::npos) {
                std::string key = trimWhitespace(std::string_view(token.data(), eq));
                std::string value = trimWhitespace(std::string_view(token.data() + eq + 1, token.size() - eq - 1));
                if (!key.empty()) {
                    props.emplace_back(std::move(key), std::move(value));
                }
            } else {
                props.emplace_back(std::move(token), std::string());
            }
        }

        start = end + 1;
    }

    if (props.empty()) {
        return "normal";
    }

    std::sort(props.begin(), props.end(), [](const auto& a, const auto& b) { return a.first < b.first; });

    std::string normalized;
    for (size_t i = 0; i < props.size(); ++i) {
        if (i > 0) {
            normalized += ",";
        }

        normalized += props[i].first;
        if (!props[i].second.empty()) {
            normalized += "=";
            normalized += props[i].second;
        }
    }

    return normalized;
}

Result<void> BlockModelLoader::loadFromResourcePack(IResourcePack& resourcePack)
{
    MC_TRACE_SCOPED_EVENT(TraceEvents.Client.Resource, "BlockModelLoader::loadFromResourcePack");

    auto result = resourcePack.listResources(resource::PackType::ClientResources, "minecraft/models/block", "json");

    if (result.failed()) {
        // 目录可能不存在，不算错误
        return Result<void>::ok();
    }

    // 不预加载所有模型，按需加载
    return Result<void>::ok();
}

Result<UnbakedBlockModel> BlockModelLoader::loadModel(const ResourceLocation& location)
{
    // 检查缓存
    auto it = m_unbakedModels.find(location);
    if (it != m_unbakedModels.end()) {
        return it->second;
    }

    // 构建相对于 assets/ 根目录的路径
    std::string filePath;
    std::string path = location.path();

    if (path.find("models/block") != std::string::npos || path.find("models\\block") != std::string::npos) {
        filePath = location.namespace_() + "/" + path + ".json";
    } else if (path.find("block/") == 0 || path.find("block\\") == 0) {
        filePath = location.namespace_() + "/models/" + path + ".json";
    } else {
        filePath = location.namespace_() + "/models/block/" + path + ".json";
    }

    // 从资源包列表中读取模型文件
    auto readResult = _readModelFromResourcePacks(filePath);
    if (readResult.failed()) {
        return readResult.error();
    }

    // 解析JSON
    auto parseResult = _parseModel(readResult.value());
    if (parseResult.failed()) {
        return parseResult.error();
    }

    // 缓存模型
    auto& model = m_unbakedModels[location];
    model = parseResult.value();
    model.name = location.toString();

    return model;
}

Result<std::string> BlockModelLoader::_readModelFromResourcePacks(const std::string& filePath)
{
    // filePath 已是相对于 PackType 根目录的路径（如 "minecraft/models/block/stone.json"）
    // 无需再剥离 "assets/" 前缀
    const std::string& relativePath = filePath;

    for (size_t i = m_resourcePacks.size(); i > 0; --i) {
        const auto& pack = m_resourcePacks[i - 1];
        if (pack && pack->hasResource(resource::PackType::ClientResources, relativePath)) {
            auto result = pack->readTextResource(resource::PackType::ClientResources, relativePath);
            if (result.success()) {
                return result.value();
            }
        }
    }

    return Error(ErrorCode::ResourceNotFound, "Model not found in any resource pack: " + filePath);
}

void BlockModelLoader::setPackRepository(const std::vector<std::shared_ptr<IResourcePack>>& resourcePacks)
{
    m_resourcePacks = resourcePacks;
}

Result<BakedBlockModel> BlockModelLoader::bakeModel(const ResourceLocation& location)
{
    BakedBlockModel baked;

    // 加载模型链 (子 -> 父 -> 祖父...)
    // modelChain[0] = 目标模型（叶子）, modelChain.back() = 最远祖先（根）
    std::vector<UnbakedBlockModel*> modelChain;

    ResourceLocation currentLoc = location;
    while (true) {
        auto result = loadModel(currentLoc);
        if (result.failed()) {
            return result.error();
        }

        auto& model = m_unbakedModels[currentLoc];
        modelChain.push_back(&model);

        if (!model.hasParent()) {
            break;
        }

        // 转换父模型位置
        std::string parentPath = model.parentLocation.path();
        if (parentPath.find("block/") == std::string::npos && parentPath.find("block\\") == std::string::npos) {
            // 父模型路径可能是相对路径
            parentPath = "block/" + parentPath;
            model.parentLocation = ResourceLocation(model.parentLocation.namespace_(), parentPath);
        }

        currentLoc = model.parentLocation;
    }

    if (!modelChain.empty()) {
        baked.textures.clear();
        baked.elements.clear();
        baked.ambientOcclusion = true;

        // === 合并策略：与 MC Java 版一致 ===
        // MC Java 版使用 findTop* 模式：沿叶子到根方向查找第一个显式定义了该属性的模型。
        // 对于纹理：采用 merge 语义（所有层的纹理变量合并，子模型覆盖父模型同名变量）。
        // 对于元素和环境光遮蔽：采用 leaf-wins 语义（从叶子到根查找，第一个显式定义的生效）。

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
    }

    // 解析纹理变量引用 (递归解析 #variable)
    resolveTextureReferences(baked.textures);

    return baked;
}

void BlockModelLoader::clearCache()
{
    m_unbakedModels.clear();
}

void BlockModelLoader::computeDefaultUVs(ModelElement& elem)
{
    for (auto& [dir, face] : elem.faces) {
        if (face.uv.isDefault()) {
            // 根据面的方向计算默认UV
            switch (dir) {
                case Direction::Down:
                    face.uv.u0 = elem.from.x;
                    face.uv.v0 = 16.0f - elem.to.z;
                    face.uv.u1 = elem.to.x;
                    face.uv.v1 = 16.0f - elem.from.z;
                    break;
                case Direction::Up:
                    face.uv.u0 = elem.from.x;
                    face.uv.v0 = elem.from.z;
                    face.uv.u1 = elem.to.x;
                    face.uv.v1 = elem.to.z;
                    break;
                case Direction::North:
                    face.uv.u0 = 16.0f - elem.to.x;
                    face.uv.v0 = 16.0f - elem.to.y;
                    face.uv.u1 = 16.0f - elem.from.x;
                    face.uv.v1 = 16.0f - elem.from.y;
                    break;
                case Direction::South:
                    face.uv.u0 = elem.from.x;
                    face.uv.v0 = 16.0f - elem.to.y;
                    face.uv.u1 = elem.to.x;
                    face.uv.v1 = 16.0f - elem.from.y;
                    break;
                case Direction::West:
                    face.uv.u0 = elem.from.z;
                    face.uv.v0 = 16.0f - elem.to.y;
                    face.uv.u1 = elem.to.z;
                    face.uv.v1 = 16.0f - elem.from.y;
                    break;
                case Direction::East:
                    face.uv.u0 = 16.0f - elem.to.z;
                    face.uv.v0 = 16.0f - elem.to.y;
                    face.uv.u1 = 16.0f - elem.from.z;
                    face.uv.v1 = 16.0f - elem.from.y;
                    break;
                default:
                    break;
            }
        }
    }
}

void BlockModelLoader::mergeParent(UnbakedBlockModel& accumulated, const UnbakedBlockModel& currentLayer)
{
    // 合并纹理：当前层的纹理覆盖累积结果中的同名键（merge 语义）
    // MC Java 版 TextureSlots 采用合并语义：子模型纹理变量覆盖父模型同名变量，
    // 但保留父模型中子模型未定义的变量。在 root-to-leaf 逐层合并中，
    // 后处理的层（更靠近叶子）覆盖先处理的层（更靠近根），结果与 MC Java 一致。
    for (const auto& [key, value] : currentLayer.textures) {
        accumulated.textures[key] = value;
    }

    // 合并元素：leaf-wins 语义
    // MC Java 版语义：沿子模型到根模型查找，第一个定义了 elements 的模型生效。
    // 在 root-to-leaf 累积遍历中，如果当前层显式定义了 elements（hasElements=true），
    // 则用当前层的元素覆盖累积结果，这样后处理的（更靠近叶子的）层会覆盖先处理的层，
    // 最终效果与 leaf-to-root 查找一致——最靠近叶子的定义了 elements 的模型生效。
    if (currentLayer.hasElements) {
        accumulated.elements = currentLayer.elements;
        accumulated.hasElements = true;
    }

    // 合并环境光遮蔽：leaf-wins 语义
    // MC Java 版语义：沿子模型到根模型查找，第一个显式声明了 ambientocclusion 的模型，其值生效。
    // 在 root-to-leaf 累积遍历中，如果当前层显式设置了 AO（hasAmbientOcclusion=true），
    // 则用当前层的 AO 值覆盖累积结果，这样后处理的层会覆盖先处理的层，
    // 最终效果与 leaf-to-root 查找一致。
    if (currentLayer.hasAmbientOcclusion) {
        accumulated.ambientOcclusion = currentLayer.ambientOcclusion;
        accumulated.hasAmbientOcclusion = true;
    }
}

void BlockModelLoader::resolveTextureReferences(std::map<std::string, ResourceLocation>& textures, i32 maxIterations)
{
    bool changed = true;
    while (changed && maxIterations-- > 0) {
        changed = false;
        for (auto& [name, texLoc] : textures) {
            std::string path = texLoc.path();
            if (!path.empty() && path[0] == '#') {
                // 这是一个纹理变量引用
                std::string varName = path.substr(1);
                auto varIt = textures.find(varName);
                if (varIt != textures.end()) {
                    std::string varPath = varIt->second.path();
                    // 只有当变量值不是另一个变量引用时才解析
                    if (!varPath.empty() && varPath[0] != '#') {
                        texLoc = varIt->second;
                        changed = true;
                    }
                }
            }
        }
    }
}

Result<UnbakedBlockModel> BlockModelLoader::_parseModel(std::string_view jsonContent)
{
    try {
        auto json = nlohmann::json::parse(jsonContent);

        UnbakedBlockModel model;

        // 解析父模型
        if (json.contains("parent")) {
            model.parentLocation = ResourceLocation(json["parent"].get<std::string>());
        }

        // 解析环境光遮蔽
        if (json.contains("ambientocclusion")) {
            model.ambientOcclusion = json["ambientocclusion"].get<bool>();
            model.hasAmbientOcclusion = true;
        }

        // 解析纹理
        if (json.contains("textures")) {
            const auto& textures = json["textures"];
            for (auto it = textures.begin(); it != textures.end(); ++it) {
                std::string name = it.key();
                std::string path = it.value().get<std::string>();

                // 移除前导 # (如果有)
                if (!path.empty() && path[0] == '#') {
                    // 纹理变量引用，保持原样
                }

                model.textures[name] = path;
            }
        }

        // 解析元素
        if (json.contains("elements")) {
            model.hasElements = true;
            for (const auto& elemJson : json["elements"]) {
                auto result = parseElement(elemJson);
                if (result.success()) {
                    model.elements.push_back(result.value());
                }
            }
        }

        return model;
    }
    catch (const std::exception& e) {
        return Error(ErrorCode::ResourceParseError, std::string("Failed to parse model: ") + e.what());
    }
}

Result<ModelElement> BlockModelLoader::parseElement(const nlohmann::json& json)
{
    ModelElement elem;

    // 解析 from/to
    if (json.contains("from")) {
        const auto& from = json["from"];
        if (from.is_array() && from.size() >= 3) {
            elem.from.x = from[0].get<f32>();
            elem.from.y = from[1].get<f32>();
            elem.from.z = from[2].get<f32>();
        }
    }

    if (json.contains("to")) {
        const auto& to = json["to"];
        if (to.is_array() && to.size() >= 3) {
            elem.to.x = to[0].get<f32>();
            elem.to.y = to[1].get<f32>();
            elem.to.z = to[2].get<f32>();
        }
    }

    // 解析旋转
    if (json.contains("rotation")) {
        elem.rotation = parseRotation(json["rotation"]);
    }

    // 解析阴影
    if (json.contains("shade")) {
        elem.shade = json["shade"].get<bool>();
    }

    // 解析面
    if (json.contains("faces")) {
        const auto& faces = json["faces"];
        for (auto it = faces.begin(); it != faces.end(); ++it) {
            Direction dir = parseDirection(it.key());
            if (dir != Direction::None) {
                auto result = parseFace(it.value(), dir);
                if (result.success()) {
                    elem.faces[dir] = result.value();
                }
            }
        }
    }

    // 计算默认UV (如果没有指定)
    computeDefaultUVs(elem);

    return elem;
}

Result<ModelFace> BlockModelLoader::parseFace(const nlohmann::json& json, Direction /* dir */)
{
    ModelFace face;

    // 解析纹理
    if (json.contains("texture")) {
        face.texture = json["texture"].get<std::string>();
    }

    // 解析剔除面
    if (json.contains("cullface")) {
        face.cullFace = parseDirection(json["cullface"].get<std::string>());
    }

    // 解析着色索引
    if (json.contains("tintindex")) {
        face.tintIndex = json["tintindex"].get<i32>();
    }

    // 解析UV
    if (json.contains("uv")) {
        face.uv = parseUV(json["uv"]);
    }

    // 解析旋转
    if (json.contains("rotation")) {
        face.uv.rotation = json["rotation"].get<i32>();
    }

    return face;
}

ModelFaceUV BlockModelLoader::parseUV(const nlohmann::json& json)
{
    ModelFaceUV uv;

    if (json.is_array() && json.size() >= 4) {
        uv.u0 = json[0].get<f32>();
        uv.v0 = json[1].get<f32>();
        uv.u1 = json[2].get<f32>();
        uv.v1 = json[3].get<f32>();
    }

    return uv;
}

ModelRotation BlockModelLoader::parseRotation(const nlohmann::json& json)
{
    ModelRotation rot;

    // 解析旋转中心（两种格式共用）
    if (json.contains("origin")) {
        const auto& origin = json["origin"];
        if (origin.is_array() && origin.size() >= 3) {
            rot.origin.x = origin[0].get<f32>();
            rot.origin.y = origin[1].get<f32>();
            rot.origin.z = origin[2].get<f32>();
        }
    }

    // 判断旋转格式：如果 JSON 中同时缺少 "axis" 和 "angle"，
    // 则尝试使用 EulerXYZ 格式（x/y/z 三轴欧拉角）。
    // 参考 MC BlockElement.java Deserializer.getRotation() 的格式分发逻辑。
    const bool hasAxis = json.contains("axis");
    const bool hasAngle = json.contains("angle");

    if (!hasAxis && !hasAngle) {
        // EulerXYZ 格式（MC 1.21.11 新增）
        const bool hasX = json.contains("x");
        const bool hasY = json.contains("y");
        const bool hasZ = json.contains("z");

        if (!hasX && !hasY && !hasZ) {
            // 既没有 axis/angle 也没有 x/y/z，返回默认旋转（无旋转）
            if (json.contains("rescale")) {
                rot.rescale = json["rescale"].get<bool>();
            }
            return rot;
        }

        rot.isEulerXYZ = true;
        rot.rotX = hasX ? json["x"].get<f32>() : 0.0f;
        rot.rotY = hasY ? json["y"].get<f32>() : 0.0f;
        rot.rotZ = hasZ ? json["z"].get<f32>() : 0.0f;
        // 重置 axis+angle 的默认值，保持语义清晰
        rot.axis = "y";
        rot.angle = 0.0f;
    } else {
        // 传统 axis+angle 格式
        if (hasAxis) {
            rot.axis = json["axis"].get<std::string>();
        }
        if (hasAngle) {
            rot.angle = json["angle"].get<f32>();
        }
        // 确保 EulerXYZ 字段保持默认值
        rot.isEulerXYZ = false;
        rot.rotX = 0.0f;
        rot.rotY = 0.0f;
        rot.rotZ = 0.0f;
    }

    // rescale 字段两种格式共用，默认为 false
    if (json.contains("rescale")) {
        rot.rescale = json["rescale"].get<bool>();
    }

    return rot;
}

} // namespace mc
