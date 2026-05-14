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

#pragma once

#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace mc {

// Direction 枚举已在 util/Direction.hpp 中定义

/**
 * @brief 从字符串解析方向
 */
[[nodiscard]] Direction parseDirection(std::string_view str);

/**
 * @brief 方向转字符串
 */
[[nodiscard]] std::string directionToString(Direction dir);

/**
 * @brief 模型面UV数据
 */
struct ModelFaceUV {
    f32 u0 = 0.0f, v0 = 0.0f, u1 = 16.0f, v1 = 16.0f;
    i32 rotation = 0; // 0, 90, 180, 270

    [[nodiscard]] bool isDefault() const
    {
        return u0 == 0.0f && v0 == 0.0f && u1 == 16.0f && v1 == 16.0f && rotation == 0;
    }
};

/**
 * @brief 模型面数据
 */
struct ModelFace {
    std::string texture;                  // "#all" 或 "blocks/stone" 或纹理变量名
    Direction cullFace = Direction::None; // 剔除面方向
    i32 tintIndex = -1;                   // 着色索引，-1表示不着色
    ModelFaceUV uv;                       // UV坐标
};

/**
 * @brief 模型元素旋转
 */
struct ModelRotation {
    glm::vec3 origin{8.0f, 8.0f, 8.0f}; // 旋转中心
    std::string axis = "y";             // x, y, z
    f32 angle = 0.0f;                   // -45, -22.5, 0, 22.5, 45
    bool rescale = false;
};

/**
 * @brief 模型元素 (对应JSON中的elements数组元素)
 */
struct ModelElement {
    glm::vec3 from{0.0f, 0.0f, 0.0f};     // 起始坐标 (0-16)
    glm::vec3 to{16.0f, 16.0f, 16.0f};    // 结束坐标 (0-16)
    std::map<Direction, ModelFace> faces; // 各面数据
    ModelRotation rotation;               // 旋转
    bool shade = true;                    // 是否计算阴影
};

/**
 * @brief 未烘焙的方块模型
 */
struct UnbakedBlockModel {
    ResourceLocation parentLocation;             // 父模型位置
    std::vector<ModelElement> elements;          // 模型元素
    std::map<std::string, std::string> textures; // 纹理变量 -> 路径
    bool ambientOcclusion = true;                // 环境光遮蔽
    std::string name;                            // 模型名称(调试用)

    // 检查是否有父模型
    [[nodiscard]] bool hasParent() const { return !parentLocation.path().empty(); }
};

/**
 * @brief 已烘焙的方块模型 (所有纹理路径已解析)
 */
struct BakedBlockModel {
    std::vector<ModelElement> elements;
    std::map<std::string, ResourceLocation> textures; // 纹理变量 -> 资源位置
    bool ambientOcclusion = true;

    // 解析纹理变量引用
    // 例如: "#all" -> "minecraft:textures/blocks/stone"
    [[nodiscard]] ResourceLocation resolveTexture(std::string_view textureRef) const;
};

/**
 * @brief 方块状态变体
 */
struct BlockStateVariant {
    ResourceLocation model; // 模型位置
    i32 x = 0;              // X轴旋转角度 (0, 90, 180, 270)
    i32 y = 0;              // Y轴旋转角度 (0, 90, 180, 270)
    bool uvLock = false;    // 是否锁定UV
    i32 weight = 1;         // 权重

    [[nodiscard]] bool operator==(const BlockStateVariant& other) const;
};

/**
 * @brief 变体列表 (用于随机选择)
 */
struct VariantList {
    std::vector<BlockStateVariant> variants;

    // 根据权重随机选择一个变体
    [[nodiscard]] const BlockStateVariant& select() const;

    // 根据权重选择 (使用种子)
    [[nodiscard]] const BlockStateVariant& select(u64 seed) const;
};

/**
 * @brief 方块状态定义（解析 blockstates 目录下的 JSON）
 */
class BlockStateDefinition {
public:
    BlockStateDefinition() = default;

    // 从JSON解析
    [[nodiscard]] static Result<BlockStateDefinition> parse(std::string_view jsonContent);

    // 获取指定状态的变体
    // stateStr格式: "axis=y,facing=north" 或 "normal"
    [[nodiscard]] const VariantList* getVariants(std::string_view stateStr) const;

    // 获取所有变体映射
    [[nodiscard]] const std::map<std::string, VariantList>& getAllVariants() const { return m_variants; }

    // 是否有多部分数据
    [[nodiscard]] bool hasMultipart() const { return m_hasMultipart; }

private:
    // 规范化状态键，保证属性顺序一致（例如 a=1,b=2 与 b=2,a=1 等价）
    [[nodiscard]] static std::string normalizeStateKey(std::string_view stateKey);

    std::map<std::string, VariantList> m_variants;
    bool m_hasMultipart = false;
};

/**
 * @brief 模型加载器
 */
class BlockModelLoader {
public:
    BlockModelLoader() = default;

    // 从资源包加载模型（添加到资源包列表）
    [[nodiscard]] Result<void> loadFromResourcePack(class IResourcePack& resourcePack);

    // 设置资源包列表（用于模型加载时查找）
    void setResourcePackList(const std::vector<std::shared_ptr<class IResourcePack>>& resourcePacks);

    // 加载单个模型
    [[nodiscard]] Result<UnbakedBlockModel> loadModel(const ResourceLocation& location);

    // 烘焙模型 (解析所有父模型和纹理引用)
    [[nodiscard]] Result<BakedBlockModel> bakeModel(const ResourceLocation& location);

    // 检查模型是否已加载
    [[nodiscard]] bool hasModel(const ResourceLocation& location) const;

    // 获取未烘焙模型
    [[nodiscard]] const UnbakedBlockModel* getUnbakedModel(const ResourceLocation& location) const;

    // 清除缓存
    void clearCache();

private:
    std::map<ResourceLocation, UnbakedBlockModel> m_unbakedModels;
    IResourcePack* m_resourcePack = nullptr;        // 当前资源包（向后兼容）
    std::vector<IResourcePack*> m_resourcePackList; // 所有资源包列表（原始指针）

    // 从所有资源包中读取模型文件
    [[nodiscard]] Result<std::string> readModelFromResourcePacks(const std::string& filePath);

    // 解析模型JSON
    [[nodiscard]] Result<UnbakedBlockModel> parseModel(std::string_view jsonContent);

    // 解析元素
    [[nodiscard]] Result<ModelElement> parseElement(const nlohmann::json& json);

    // 解析面
    [[nodiscard]] Result<ModelFace> parseFace(const nlohmann::json& json, Direction dir);

    // 解析UV
    [[nodiscard]] ModelFaceUV parseUV(const nlohmann::json& json);

    // 解析旋转
    [[nodiscard]] ModelRotation parseRotation(const nlohmann::json& json);

    // 合并父子模型
    void mergeParent(UnbakedBlockModel& child, const UnbakedBlockModel& parent);
};

} // namespace mc
