#pragma once

#include "../common/core/Types.hpp"
#include "../common/core/Result.hpp"
#include "../common/resource/ResourceLocation.hpp"
#include "../common/util/Direction.hpp"
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <map>
#include <memory>

namespace mc {

// Direction 枚举已在 util/Direction.hpp 中定义

/**
 * @brief 从字符串解析方向
 */
[[nodiscard]] Direction parseDirection(StringView str);

/**
 * @brief 方向转字符串
 */
[[nodiscard]] String directionToString(Direction dir);

/**
 * @brief 模型面UV数据
 */
struct ModelFaceUV {
    f32 u0 = 0.0f, v0 = 0.0f, u1 = 16.0f, v1 = 16.0f;
    i32 rotation = 0; // 0, 90, 180, 270

    [[nodiscard]] bool isDefault() const {
        return u0 == 0.0f && v0 == 0.0f && u1 == 16.0f && v1 == 16.0f && rotation == 0;
    }
};

/**
 * @brief 模型面数据
 */
struct ModelFace {
    String texture;           // "#all" 或 "blocks/stone" 或纹理变量名
    Direction cullFace = Direction::None;  // 剔除面方向
    i32 tintIndex = -1;       // 着色索引，-1表示不着色
    ModelFaceUV uv;           // UV坐标
};

/**
 * @brief 模型元素旋转
 */
struct ModelRotation {
    glm::vec3 origin{8.0f, 8.0f, 8.0f};  // 旋转中心
    String axis = "y";                    // x, y, z
    f32 angle = 0.0f;                     // -45, -22.5, 0, 22.5, 45
    bool rescale = false;
};

/**
 * @brief 模型元素 (对应JSON中的elements数组元素)
 */
struct ModelElement {
    glm::vec3 from{0.0f, 0.0f, 0.0f};  // 起始坐标 (0-16)
    glm::vec3 to{16.0f, 16.0f, 16.0f}; // 结束坐标 (0-16)
    std::map<Direction, ModelFace> faces; // 各面数据
    ModelRotation rotation;              // 旋转
    bool shade = true;                   // 是否计算阴影
};

/**
 * @brief 未烘焙的方块模型
 */
struct UnbakedBlockModel {
    ResourceLocation parentLocation;              // 父模型位置
    std::vector<ModelElement> elements;           // 模型元素
    std::map<String, String> textures;            // 纹理变量 -> 路径
    bool ambientOcclusion = true;                 // 环境光遮蔽
    String name;                                  // 模型名称(调试用)

    // 检查是否有父模型
    [[nodiscard]] bool hasParent() const {
        return !parentLocation.path().empty();
    }
};

/**
 * @brief 已烘焙的方块模型 (所有纹理路径已解析)
 */
struct BakedBlockModel {
    std::vector<ModelElement> elements;
    std::map<String, ResourceLocation> textures; // 纹理变量 -> 资源位置
    bool ambientOcclusion = true;

    // 解析纹理变量引用
    // 例如: "#all" -> "minecraft:textures/blocks/stone"
    [[nodiscard]] ResourceLocation resolveTexture(StringView textureRef) const;
};

/**
 * @brief 方块状态变体
 */
struct BlockStateVariant {
    ResourceLocation model;  // 模型位置
    i32 x = 0;               // X轴旋转角度 (0, 90, 180, 270)
    i32 y = 0;               // Y轴旋转角度 (0, 90, 180, 270)
    bool uvLock = false;     // 是否锁定UV
    i32 weight = 1;          // 权重

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
    [[nodiscard]] static Result<BlockStateDefinition> parse(StringView jsonContent);

    // 获取指定状态的变体
    // stateStr格式: "axis=y,facing=north" 或 "normal"
    [[nodiscard]] const VariantList* getVariants(StringView stateStr) const;

    // 获取所有变体映射
    [[nodiscard]] const std::map<String, VariantList>& getAllVariants() const { return m_variants; }

    // 是否有多部分数据
    [[nodiscard]] bool hasMultipart() const { return m_hasMultipart; }

private:
    // 规范化状态键，保证属性顺序一致（例如 a=1,b=2 与 b=2,a=1 等价）
    [[nodiscard]] static String normalizeStateKey(StringView stateKey);

    std::map<String, VariantList> m_variants;
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
    IResourcePack* m_resourcePack = nullptr;  // 当前资源包（向后兼容）
    std::vector<IResourcePack*> m_resourcePackList;  // 所有资源包列表（原始指针）

    // 从所有资源包中读取模型文件
    [[nodiscard]] Result<String> readModelFromResourcePacks(const String& filePath);

    // 解析模型JSON
    [[nodiscard]] Result<UnbakedBlockModel> parseModel(StringView jsonContent);

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
