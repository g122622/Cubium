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

#include "client/resource/BlockModelLoader.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/resource/pack/IResourcePack.hpp"
#include <map>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

namespace mc::client::resource {

/**
 * @brief 物品显示上下文类型
 *
 * 定义物品在不同场景下的显示变换
 */
enum class ItemDisplayContext : u8 {
    ThirdPersonRightHand, // 第三人称右手
    ThirdPersonLeftHand,  // 第三人称左手
    FirstPersonRightHand, // 第一人称右手
    FirstPersonLeftHand,  // 第一人称左手
    Head,                 // 头部（头盔等）
    Gui,                  // GUI 界面
    Ground,               // 地面掉落物
    Fixed,                // 物品展示框
    Count                 // 数量
};

/**
 * @brief 从字符串解析显示上下文
 */
[[nodiscard]] ItemDisplayContext parseDisplayContext(std::string_view str);

/**
 * @brief 显示上下文转字符串
 */
[[nodiscard]] std::string displayContextToString(ItemDisplayContext ctx);

/**
 * @brief 物品变换数据
 *
 * 包含旋转、平移、缩放信息
 */
struct ItemTransform {
    glm::vec3 rotation{0.0f, 0.0f, 0.0f};    // 欧拉角旋转（度）
    glm::vec3 translation{0.0f, 0.0f, 0.0f}; // 平移（像素单位，需除以16转换为方块单位）
    glm::vec3 scale{1.0f, 1.0f, 1.0f};       // 缩放

    /**
     * @brief 获取单位变换
     */
    [[nodiscard]] static ItemTransform identity();

    /**
     * @brief 从 JSON 解析变换
     */
    [[nodiscard]] static ItemTransform fromJson(const nlohmann::json& json);

    /**
     * @brief 转换为 4x4 变换矩阵
     */
    [[nodiscard]] glm::mat4 toMatrix() const;
};

/**
 * @brief 物品模型覆盖条件
 *
 * 定义基于物品状态的模型替换规则
 */
struct ItemModelOverride {
    std::map<std::string, f32> predicates; // 条件谓词，如 {"damage": 0.25}
    ResourceLocation model;                // 替换模型

    /**
     * @brief 从 JSON 解析覆盖条件
     */
    [[nodiscard]] static ItemModelOverride fromJson(const nlohmann::json& json);
};

/**
 * @brief 物品模型类型
 */
enum class ItemModelType : u8 {
    Generated, // item/generated - 平面图标（大多数物品）
    Handheld,  // item/handheld - 手持工具
    Block,     // 方块物品（继承方块模型）
    Custom     // 自定义 3D 模型（有 elements 字段）
};

/**
 * @brief 未烘焙的物品模型
 */
struct UnbakedItemModel {
    ResourceLocation location;                           // 模型位置
    ResourceLocation parentLocation;                     // 父模型位置
    std::vector<ModelElement> elements;                  // 3D 元素（可选）
    std::map<std::string, std::string> textures;         // 纹理变量 -> 路径
    std::map<ItemDisplayContext, ItemTransform> display; // 显示变换
    std::vector<ItemModelOverride> overrides;            // 模型覆盖条件
    bool ambientOcclusion = true;
    std::string name; // 模型名称（调试用）

    // JSON 显式设置标记（用于 leaf-wins 合并语义）
    // MC Java 版中，沿子模型到根模型查找第一个显式设置了该属性的模型，其值生效
    // 若整个继承链都没有显式设置，则使用默认值
    bool hasElements = false;         // JSON 中是否包含 "elements" 字段
    bool hasAmbientOcclusion = false; // JSON 中是否包含 "ambientocclusion" 字段
    bool hasOverrides = false;        // JSON 中是否包含 "overrides" 字段

    /**
     * @brief 检查是否有父模型
     */
    [[nodiscard]] bool hasParent() const { return !parentLocation.path().empty(); }
};

/**
 * @brief 已烘焙的物品模型
 *
 * 所有纹理路径已解析，可直接用于渲染
 */
struct BakedItemModel {
    ResourceLocation location;

    // 模型类型
    ItemModelType type = ItemModelType::Generated;

    // 纹理层（Generated/Handheld 类型使用）
    // layer0, layer1, ... 按顺序存储
    std::vector<ResourceLocation> textureLayers;

    // 3D 元素（Block/Custom 类型使用）
    std::vector<ModelElement> elements;
    std::map<std::string, ResourceLocation> textures;

    // 环境光遮蔽
    bool ambientOcclusion = true;

    // 显示变换
    std::map<ItemDisplayContext, ItemTransform> display;

    // 模型覆盖
    std::vector<ItemModelOverride> overrides;

    /**
     * @brief 获取指定上下文的变换，如果不存在则返回单位变换
     */
    [[nodiscard]] const ItemTransform& getTransform(ItemDisplayContext ctx) const;

    /**
     * @brief 解析纹理变量引用
     * 例如: "#layer0" -> "minecraft:item/diamond_sword"
     */
    [[nodiscard]] ResourceLocation resolveTexture(std::string_view textureRef) const;
};

/**
 * @brief 物品模型加载器
 *
 * 从资源包加载 MC 1.16.5 格式的物品模型 JSON
 */
class ItemModelLoader {
public:
    /**
     * @brief 构造函数
     * @param resourcePacks 资源包列表（按优先级从高到低）
     */
    explicit ItemModelLoader(const std::vector<ResourcePackPtr>& resourcePacks);

    /**
     * @brief 全量预加载所有物品模型
     *
     * 枚举所有资源包中 `assets/<namespace>/models/item/` 目录下的所有 .json 文件并烘焙，
     * 将结果填充到内部缓存（m_unbakedModels / m_bakedModels），供后续 getModel /
     * getItemModel 直接命中。资源包优先级由 _readModelFromResourcePacks 内部处理，
     * 本方法仅负责触发烘焙。
     *
     * 单个模型烘焙失败不会中断整体流程，仅记录 spdlog::warn 警告。
     * 整体流程不返回错误（部分文件缺失属正常情况）。
     */
    [[nodiscard]] Result<void> loadAllModels();

    /**
     * @brief 加载单个模型
     */
    [[nodiscard]] Result<UnbakedItemModel> loadModel(const ResourceLocation& location);

    /**
     * @brief 烘焙模型（解析父模型链和纹理引用）
     */
    [[nodiscard]] Result<BakedItemModel> bakeModel(const ResourceLocation& location);

    /**
     * @brief 获取已烘焙的模型
     */
    [[nodiscard]] const BakedItemModel* getModel(const ResourceLocation& location) const;

    /**
     * @brief 检查模型是否已加载
     */
    [[nodiscard]] bool hasModel(const ResourceLocation& location) const;

    /**
     * @brief 清除缓存
     *
     * 同时清除未烘焙与已烘焙缓存（用于资源重载）。
     */
    void clearCache();

    /**
     * @brief 仅清除未烘焙模型缓存
     *
     * loadAllModels 预烘焙后，运行时 getModel/getItemModel 只查 m_bakedModels；
     * 延迟兜底路径（bakeModel）会重新调 loadModel 从资源包按需回填 m_unbakedModels，
     * 故预烘焙完成后可安全释放 m_unbakedModels 以降低运行时内存。
     * 注意：不能误用 clearCache()，那会连 m_bakedModels 一起清空，导致运行时模型丢失。
     */
    void clearUnbakedModels();

private:
    // 按值持有资源包列表（ResourcePackPtr 为 shared_ptr，拷贝增引用计数）。
    // 此前为引用，当 ResourceManager 析构、其 m_resourcePacks 释放后，
    // 全局单例 ItemModelCache 仍持有 loader，引用即成为悬空引用，
    // 导致后续 getItemModel 访问已释放内存（SEH 0xc0000005）。按值持有消除该 use-after-free。
    std::vector<ResourcePackPtr> m_resourcePacks;
    std::map<ResourceLocation, UnbakedItemModel> m_unbakedModels;
    std::map<ResourceLocation, BakedItemModel> m_bakedModels;

    /**
     * @brief 从资源包读取模型文件
     */
    [[nodiscard]] Result<std::string> _readModelFromResourcePacks(const std::string& filePath);

    /**
     * @brief 解析模型 JSON
     */
    [[nodiscard]] Result<UnbakedItemModel> _parseModel(const ResourceLocation& location, std::string_view jsonContent);

    /**
     * @brief 解析 display 节点
     */
    void _parseDisplay(UnbakedItemModel& model, const nlohmann::json& display);

    /**
     * @brief 解析 overrides 节点
     */
    void _parseOverrides(UnbakedItemModel& model, const nlohmann::json& overrides);

    /**
     * @brief 解析 elements 节点（复用 BlockModelLoader 的解析方法）
     */
    [[nodiscard]] Result<void> _parseElements(UnbakedItemModel& model, const nlohmann::json& elements);

    /**
     * @brief 解析纹理变量
     */
    void _parseTextures(UnbakedItemModel& model, const nlohmann::json& textures);

    /**
     * @brief 加载内置默认变换
     */
    void _loadDefaultTransforms();

    // 内置默认变换
    std::map<ItemDisplayContext, ItemTransform> m_generatedDefaults;
    std::map<ItemDisplayContext, ItemTransform> m_handheldDefaults;
};

} // namespace mc::client::resource
