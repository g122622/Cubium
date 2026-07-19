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
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR THE DEALINGS IN THE
 * SOFTWARE.
 *
 */

#pragma once

#include "../../chunk/IChunkGenerator.hpp"
#include "../../feature/template/Template.hpp"
#include "../../feature/template/TemplateManager.hpp"
#include "../Structure.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <memory>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace structure {

/**
 * @brief 废弃传送门垂直放置位置
 */
enum class RuinedPortalLocation : u8 {
    OnLandSurface, ///< 在地表
    PartlyBuried,  ///< 部分掩埋
    OnOceanFloor,  ///< 在海底
    InMountain,    ///< 在山中
    Underground,   ///< 地下
    InNether       ///< 下界
};

/**
 * @brief 废弃传送门属性配置
 */
struct RuinedPortalProperties {
    bool cold = false;                  ///< 是否为寒冷生物群系
    f32 mossiness = 0.2f;               ///< 苔藓程度 (0.0-1.0)
    bool airPocket = false;             ///< 是否有空气口袋
    bool overgrown = false;             ///< 是否过度生长（丛林）
    bool vines = false;                 ///< 是否有藤蔓
    bool replaceWithBlackstone = false; ///< 是否替换为黑石（下界）
};

/**
 * @brief 废弃传送门结构片段
 *
 * 使用模板系统生成废弃传送门。
 */
class RuinedPortalPiece : public StructurePiece {
public:
    /**
     * @brief 构造函数
     * @param templateName 模板名称（资源位置）
     * @param position 放置位置
     * @param rotation 旋转角度
     * @param mirror 镜像
     * @param location 垂直放置位置
     * @param properties 属性配置
     */
    RuinedPortalPiece(const std::string& templateName,
        const BlockPos& position,
        Rotation rotation,
        Mirror mirror,
        RuinedPortalLocation location,
        const RuinedPortalProperties& properties);

    ~RuinedPortalPiece() noexcept override = default;

    void generate(IWorldWriter& world,
        math::Random& rng,
        i32 chunkX,
        i32 chunkZ,
        const StructureBoundingBox& chunkBounds,
        ChunkPrimer* chunk = nullptr,
        IChunkGenerator* generator = nullptr) override;

    /**
     * @brief 设置模板管理器
     *
     * 必须在 generate 之前调用
     */
    void setTemplateManager(feature::template_::TemplateManager* manager) { m_templateManager = manager; }

    [[nodiscard]] const std::string& templateName() const { return m_templateName; }
    [[nodiscard]] RuinedPortalLocation location() const { return m_location; }
    [[nodiscard]] const RuinedPortalProperties& properties() const { return m_properties; }

private:
    void _loadTemplate();
    void _updateBoundingBox();

    std::string m_templateName;
    Rotation m_rotation;
    Mirror m_mirror;
    RuinedPortalLocation m_location;
    RuinedPortalProperties m_properties;

    feature::template_::TemplateManager* m_templateManager = nullptr;
    const feature::template_::Template* m_template = nullptr;
    BlockPos m_size{1, 1, 1};
    BlockPos m_centerOffset{0, 0, 0};
};

/**
 * @brief 废弃传送门结构类型
 *
 * 根据生物群系决定传送门的变体类型
 */
enum class RuinedPortalType : u8 {
    Standard, ///< 标准类型
    Desert,   ///< 沙漠类型
    Jungle,   ///< 丛林类型
    Swamp,    ///< 沼泽类型
    Mountain, ///< 山地类型
    Ocean,    ///< 海洋类型
    Nether    ///< 下界类型
};

/**
 * @brief 废弃传送门结构
 *
 * 使用模板系统生成废弃传送门，支持多种变体。
 */
class RuinedPortalStructure : public Structure {
public:
    explicit RuinedPortalStructure(ResourceLocation id);

    [[nodiscard]] const std::string& name() const override { return s_name; }

    /**
     * @brief 获取结构关联的生物群系标签
     *
     * 返回 minecraft:has_structure/ruined_portal_standard 标签，用于 O(1) 生物群系查找。
     * 实际变体区分在 canGenerate() 中根据生物群系/维度判断。
     */
    [[nodiscard]] const biome::BiomeTag* defaultBiomeTag() const override;

    [[nodiscard]] bool canGenerate(
        IWorld& world, IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) override;

    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IChunkGenerator& generator, math::Random& rng, i32 chunkX, i32 chunkZ) const override;

    /**
     * @brief 设置模板管理器
     */
    void setTemplateManager(feature::template_::TemplateManager* manager) { m_templateManager = manager; }

    /**
     * @brief 根据生物群系确定传送门类型
     * @param biome 生物群系ID
     * @return 传送门类型
     */
    [[nodiscard]] static RuinedPortalType getPortalType(BiomeId biome);

    /**
     * @brief 获取普通传送门模板列表
     */
    [[nodiscard]] static const std::vector<std::string>& getNormalTemplates() { return s_normalTemplates; }

    /**
     * @brief 获取巨型传送门模板列表
     */
    [[nodiscard]] static const std::vector<std::string>& getGiantTemplates() { return s_giantTemplates; }

    // 模板名称常量
    static const std::vector<std::string> s_normalTemplates;
    static const std::vector<std::string> s_giantTemplates;

private:
    static const std::string s_name;
    feature::template_::TemplateManager* m_templateManager = nullptr;

    /**
     * @brief 配置传送门属性
     * @param type 传送门类型
     * @param rng 随机数生成器
     * @param biome 生物群系
     * @return 属性配置
     */
    [[nodiscard]] RuinedPortalProperties configureProperties(
        RuinedPortalType type, math::Random& rng, BiomeId biome) const;

    /**
     * @brief 确定垂直放置位置
     * @param type 传送门类型
     * @param rng 随机数生成器
     * @return 垂直放置位置
     */
    [[nodiscard]] RuinedPortalLocation determineLocation(RuinedPortalType type, math::Random& rng) const;
};

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
