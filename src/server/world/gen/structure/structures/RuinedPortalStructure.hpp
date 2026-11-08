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

#include "common/core/Types.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/util/Direction.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/world/biome/BiomeTag.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/feature/template/Template.hpp"
#include "server/world/gen/feature/template/TemplateManager.hpp"
#include "server/world/gen/structure/Structure.hpp"
#include "server/world/gen/structure/StructureDefinitionLoader.hpp"
#include <memory>
#include <string>
#include <vector>

namespace mc {
namespace world {
namespace gen {
namespace structure {

/**
 * @brief 废弃传送门垂直放置位置
 *
 * 直接复用数据包的 `placement` 枚举，避免两套枚举之间的映射维护成本。
 */
using RuinedPortalLocation = RuinedPortalSetupDefinition::Placement;

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
        const RuinedPortalProperties& properties,
        const BlockPos& candidatePos);

    ~RuinedPortalPiece() noexcept override = default;

    void generate(IWorldWriter& world,
        math::IRandom& rng,
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
    void setTemplateManager(feature::template_::TemplateManager* manager)
    {
        m_templateManager = manager;
    }

    [[nodiscard]] const std::string& templateName() const
    {
        return m_templateName;
    }

    /**
     * @brief 模板是否已成功加载
     *
     * 模板缺失时包围盒退化到单点、generate 不写任何方块，但结构起点仍会被登记。
     * 调用方应在登记起点前用它断言，把"缺模板"暴露成错误而不是静默的空结构。
     */
    [[nodiscard]] bool hasLoadedTemplate() const
    {
        return m_template != nullptr;
    }

    /** @brief 立即加载模板并据此算出真实包围盒（幂等）。登记起点前必须调用。 */
    void loadTemplateNow()
    {
        if (m_template == nullptr) {
            _loadTemplate();
        }
    }

    [[nodiscard]] RuinedPortalLocation location() const
    {
        return m_location;
    }

    [[nodiscard]] const RuinedPortalProperties& properties() const
    {
        return m_properties;
    }

    /// 候选生成点（模板包围盒中心的地面高度处），用于装配后的生物群系校验
    [[nodiscard]] const BlockPos& candidatePos() const noexcept
    {
        return m_candidatePos;
    }

private:
    void _loadTemplate();
    void _updateBoundingBox();

    // ===== 放置后的地狱岩/藤蔓点缀（原版 RuinedPortalPiece#postProcess 的尾段） =====

    /// 世界高度图顶端方块 Y（原版 `getSurfaceY`：Heightmap 高度 - 1）
    [[nodiscard]] i32 _surfaceY(const IWorld& world, i32 x, i32 z) const;

    /// 该位置能否被地狱岩/岩浆块替换（原版 `canBlockBeReplacedByNetherrackOrMagma`）
    [[nodiscard]] bool _canBeReplacedByNetherrackOrMagma(const IWorld& world, const BlockPos& pos) const;

    /// 按 cold 属性以 7% 概率写岩浆块，否则写地狱岩（原版 `placeNetherrackOrMagma`）
    void _placeNetherrackOrMagma(IWorld& world, math::IRandom& rng, const BlockPos& pos);

    /// 从 pos 起向下概率性垂滴最多 8 格（原版 `addNetherrackDripColumn`）
    void _addNetherrackDripColumn(IWorld& world, math::IRandom& rng, const BlockPos& pos);

    /// 传送门底面每一格地狱岩下方补垂滴列（原版 `addNetherrackDripColumnsBelowPortal`）
    void _addNetherrackDripColumnsBelowPortal(IWorld& world, math::IRandom& rng);

    /// 以包围盒中心为原点按曼哈顿距离衰减概率铺开地狱岩/岩浆块（原版 `spreadNetherrack`）
    void _spreadNetherrack(IWorld& world, math::IRandom& rng);

    /// 实心方块的某个空侧面挂一格藤蔓（原版 `maybeAddVines`）
    void _maybeAddVines(IWorld& world, math::IRandom& rng, const BlockPos& pos);

    /// 地狱岩上方以 50% 概率长一格丛林树叶（原版 `maybeAddLeavesAbove`）
    void _maybeAddLeavesAbove(IWorld& world, math::IRandom& rng, const BlockPos& pos);

    std::string m_templateName;
    // 旋转与镜像必须用基类 StructurePiece::m_rotation / m_mirror：子类另起同名成员会把
    // 基类那份遮蔽掉，而 getRotation()/getMirror() 读的是基类成员，差分里表现为
    // "抽到了旋转却始终输出 NONE"。
    RuinedPortalLocation m_location;
    RuinedPortalProperties m_properties;

    feature::template_::TemplateManager* m_templateManager = nullptr;
    const feature::template_::Template* m_template = nullptr;
    BlockPos m_candidatePos{0, 0, 0};
    BlockPos m_size{1, 1, 1};
    BlockPos m_centerOffset{0, 0, 0};
};

/**
 * @brief 废弃传送门结构
 *
 * 数据包为每个 `minecraft:ruined_portal` 结构定义提供一张 `setups` 表，每项描述一种
 * 垂直放置方式及苔藓/藤蔓/空气口袋等属性，并按 weight 加权抽取。变体（平原/沙漠/丛林/
 * 沼泽/山地/海洋/下界）的差异**完全**由各自的 setups 表表达，代码里不再有按生物群系
 * 分派的硬编码配置。
 */
class RuinedPortalStructure : public Structure {
public:
    /**
     * @brief 构造函数
     * @param id 结构资源位置
     * @param setups 数据包的 setups 表（不可为空；原版 Codec 用 nonEmptyList 校验）
     */
    RuinedPortalStructure(ResourceLocation id, std::vector<RuinedPortalSetupDefinition> setups);

    [[nodiscard]] const std::string& name() const override
    {
        return s_name;
    }

    /**
     * @brief 获取结构关联的生物群系标签
     *
     * 返回 minecraft:has_structure/ruined_portal_standard 标签。实际生效的标签由
     * StructureTypeRegistry 依据数据包的 `biomes` 字段覆盖，这里只是子类默认值。
     */
    [[nodiscard]] const biome::BiomeTag* defaultBiomeTag() const override;

    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IChunkGenerator& generator, math::IRandom& rng, i32 chunkX, i32 chunkZ) const override;

    /**
     * @brief 带生成区域重载：落点扫描从生成区域取整列地形
     *
     * 原版 `getBaseColumn` / `getBaseHeight` 在结构落点扫描阶段固定使用
     * `DensityFunctions.BeardifierMarker`（不含任何结构贡献），因此扫描看到的一直是
     * **原始地形**，与区域内是否已有其它结构无关。region 只是地形列的来源。
     */
    [[nodiscard]] std::unique_ptr<StructureStart> generate(
        IChunkGenerator& generator, WorldGenRegion* region, math::IRandom& rng, i32 chunkX, i32 chunkZ) const override;

    /**
     * @brief 设置模板管理器
     */
    void setTemplateManager(feature::template_::TemplateManager* manager)
    {
        m_templateManager = manager;
    }

    /**
     * @brief 获取普通传送门模板列表
     */
    [[nodiscard]] static const std::vector<std::string>& getNormalTemplates()
    {
        return s_normalTemplates;
    }

    /**
     * @brief 获取巨型传送门模板列表
     */
    [[nodiscard]] static const std::vector<std::string>& getGiantTemplates()
    {
        return s_giantTemplates;
    }

    [[nodiscard]] const std::vector<RuinedPortalSetupDefinition>& setups() const noexcept
    {
        return m_setups;
    }

    // 模板名称常量
    static const std::vector<std::string> s_normalTemplates;
    static const std::vector<std::string> s_giantTemplates;

private:
    static const std::string s_name;

    std::vector<RuinedPortalSetupDefinition> m_setups;
    feature::template_::TemplateManager* m_templateManager = nullptr;
};

} // namespace structure
} // namespace gen
} // namespace world
} // namespace mc
