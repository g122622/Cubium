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
#include "common/resource/pack/IResourcePack.hpp"
#include "common/util/Direction.hpp"
#include <array>
#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

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
 *
 * 支持两种旋转格式（与 MC 1.21.11 BlockElementRotation 一致）：
 *
 * 1. 传统 axis+angle 格式（isEulerXYZ == false）：
 *    旋转由 axis（单轴 "x"/"y"/"z"）+ angle（度数）指定。
 *
 * 2. EulerXYZ 格式（isEulerXYZ == true，MC 1.21.11 新增）：
 *    旋转由 rotX/rotY/rotZ 三个欧拉角（度数）指定，
 *    应用顺序为内在 Z-Y-X（等价于外在 X-Y-Z）。
 *    参考 MC BlockElementRotation.EulerXYZRotation.transformation()。
 *
 * 当 JSON 旋转对象同时缺少 "axis" 和 "angle" 字段、
 * 但包含 "x"/"y"/"z" 中的至少一个时，使用 EulerXYZ 格式。
 */
struct ModelRotation {
    glm::vec3 origin{8.0f, 8.0f, 8.0f}; // 旋转中心

    // --- 传统 axis+angle 格式 ---
    Axis axis = Axis::Y; // x, y, z
    f32 angle = 0.0f;    // -45, -22.5, 0, 22.5, 45

    // --- EulerXYZ 格式（MC 1.21.11 新增）---
    bool isEulerXYZ = false; // 是否使用 EulerXYZ 旋转格式
    f32 rotX = 0.0f;         // 绕 X 轴旋转角度（度数）
    f32 rotY = 0.0f;         // 绕 Y 轴旋转角度（度数）
    f32 rotZ = 0.0f;         // 绕 Z 轴旋转角度（度数）

    bool rescale = false;

    /**
     * @brief 判断旋转是否为恒等变换（无旋转）
     *
     * 对于 axis+angle 格式，angle == 0 表示无旋转。
     * 对于 EulerXYZ 格式，rotX/rotY/rotZ 全为 0 表示无旋转。
     */
    [[nodiscard]] bool isIdentity() const
    {
        if (isEulerXYZ) {
            return rotX == 0.0f && rotY == 0.0f && rotZ == 0.0f;
        }
        return angle == 0.0f;
    }
};

/**
 * @brief 模型元素 (对应JSON中的elements数组元素)
 *
 * faces 以扁平数组存储，索引 = Directions::index(dir)（Down=0..East=5）。
 * 每个槽位为 std::optional<ModelFace>：nullopt 表示该方向无面，
 * 等价于原先 std::map<Direction, ModelFace> 的 count(dir)==0 语义，
 * 但避免了红黑树节点的逐面堆分配（6 个固定方向用树是浪费）。
 */
struct ModelElement {
    glm::vec3 from{0.0f, 0.0f, 0.0f};                // 起始坐标 (0-16)
    glm::vec3 to{16.0f, 16.0f, 16.0f};               // 结束坐标 (0-16)
    std::array<std::optional<ModelFace>, 6> faces{}; // 各面数据，索引 = Directions::index(dir)
    ModelRotation rotation;                          // 旋转
    bool shade = true;                               // 是否计算阴影

    /**
     * @brief 有效面数（替代原 map.size()）
     */
    [[nodiscard]] size_t faceCount() const
    {
        size_t n = 0;
        for (const auto& f : faces) {
            if (f.has_value()) ++n;
        }
        return n;
    }

    /**
     * @brief 是否存在任意面（替代原 map.empty() 的语义取反）
     */
    [[nodiscard]] bool hasAnyFace() const
    {
        for (const auto& f : faces) {
            if (f.has_value()) return true;
        }
        return false;
    }

    /**
     * @brief 按方向访问面（可写）
     * @note 返回 optional 引用；调用方写入时需 emplace 赋值，如 elem.at(dir) = face;
     */
    std::optional<ModelFace>& at(Direction d) { return faces[Directions::index(d)]; }

    /**
     * @brief 按方向访问面（只读）
     */
    [[nodiscard]] const std::optional<ModelFace>& at(Direction d) const { return faces[Directions::index(d)]; }
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

    // JSON 显式设置标记（用于 leaf-wins 合并语义）
    // MC Java 版中，沿子模型到根模型查找第一个显式设置了该属性的模型，其值生效
    // 若整个继承链都没有显式设置，则使用默认值
    bool hasElements = false;         // JSON 中是否包含 "elements" 字段
    bool hasAmbientOcclusion = false; // JSON 中是否包含 "ambientocclusion" 字段

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
    [[nodiscard]] static std::string _normalizeStateKey(std::string_view stateKey);

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
    [[nodiscard]] Result<void> loadFromResourcePack(IResourcePack& resourcePack);

    // 设置资源包列表（用于模型加载时查找）
    void setPackRepository(const std::vector<std::shared_ptr<IResourcePack>>& resourcePacks);

    // 加载单个模型
    [[nodiscard]] Result<UnbakedBlockModel> loadModel(const ResourceLocation& location);

    // 烘焙模型 (解析所有父模型和纹理引用)
    [[nodiscard]] Result<BakedBlockModel> bakeModel(const ResourceLocation& location);

    // 清除缓存
    void clearCache();

    // ---- 共享的模型元素解析工具方法（供 ItemModelLoader 等复用）----

    /**
     * @brief 从 JSON 对象解析单个模型元素
     *
     * 解析 from/to/rotation/shade/faces，并自动计算省略 UV 的面的默认坐标。
     * ItemModelLoader 等其他加载器也可使用此方法，避免重复实现元素解析逻辑。
     */
    [[nodiscard]] static Result<ModelElement> parseElement(const nlohmann::json& json);

    /**
     * @brief 从 JSON 对象解析模型面
     */
    [[nodiscard]] static Result<ModelFace> parseFace(const nlohmann::json& json, Direction dir);

    /**
     * @brief 从 JSON 数组解析 UV 坐标
     */
    [[nodiscard]] static ModelFaceUV parseUV(const nlohmann::json& json);

    /**
     * @brief 从 JSON 对象解析旋转信息
     */
    [[nodiscard]] static ModelRotation parseRotation(const nlohmann::json& json);

    /**
     * @brief 为省略 UV 的面根据元素几何计算默认 UV 坐标
     *
     * MC JSON 允许省略面的 UV 数据，此时需根据面的方向和元素的 from/to 坐标推导。
     */
    static void computeDefaultUVs(ModelElement& elem);

    /**
     * @brief 合并父子未烘焙模型的属性
     *
     * 在 root-to-leaf 累积遍历中使用。accumulated 是已累积的结果，
     * currentLayer 是当前正在处理的模型层（更靠近叶子）。
     *
     * 合并规则（与 MC Java 版一致）：
     * - 纹理：当前层覆盖累积结果中的同名键（merge 语义，子模型纹理覆盖父模型）
     * - 元素：当前层显式定义了元素时覆盖累积结果（leaf-wins 语义）
     * - 环境光遮蔽：当前层显式设置了 AO 时覆盖累积结果（leaf-wins 语义）
     *
     * @note bakeModel 已改用内联的 leaf-to-root 查找策略（与 MC Java 版
     *       ResolvedModel.findTop* 方法一致），此方法作为公共工具方法保留，
     *       可用于手动构建模型合并链或测试。
     */
    static void mergeParent(UnbakedBlockModel& accumulated, const UnbakedBlockModel& currentLayer);

    /**
     * @brief 解析纹理变量引用链
     *
     * 将 #variable 形式的引用递归替换为实际纹理路径，最多迭代 maxIterations 次。
     * 例如：down=#all, all=block/stone -> down=block/stone
     */
    static void resolveTextureReferences(std::map<std::string, ResourceLocation>& textures, i32 maxIterations = 10);

private:
    std::map<ResourceLocation, UnbakedBlockModel> m_unbakedModels;
    std::vector<ResourcePackPtr> m_resourcePacks;

    // 从所有资源包中读取模型文件
    [[nodiscard]] Result<std::string> _readModelFromResourcePacks(const std::string& filePath);

    // 解析模型JSON
    [[nodiscard]] Result<UnbakedBlockModel> _parseModel(std::string_view jsonContent);
};

} // namespace mc
