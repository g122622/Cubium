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

#include "../HorizontalBlock.hpp"
#include "StemBlock.hpp"

namespace mc {

class IWorld;
class BlockItemUseContext;

namespace blocks {

/**
 * @brief 西瓜方块
 *
 * 由西瓜茎生成的果实方块。无状态属性。
 */
class MelonBlock : public StemGrownBlock {
public:
    /**
     * @brief 构造函数
     * @param stem 对应的茎方块
     * @param attachedStem 对应的连接茎方块
     * @param properties 方块属性
     */
    MelonBlock(const Block* stem, const Block* attachedStem, const BlockProperties& properties);

    ~MelonBlock() override = default;

    // ========== StemGrownBlock 接口 ==========

    [[nodiscard]] const Block* getStem() const override { return m_stem; }
    [[nodiscard]] const Block* getAttachedStem() const override { return m_attachedStem; }

    // ========== 茎指针设置（用于解决循环依赖） ==========

    void setStem(const Block* stem) { m_stem = stem; }
    void setAttachedStem(const Block* attachedStem) { m_attachedStem = attachedStem; }

private:
    const Block* m_stem;
    const Block* m_attachedStem;
};

/**
 * @brief 南瓜方块
 *
 * 由南瓜茎生成的果实方块。可以用剪刀刻成刻过的南瓜。
 */
class PumpkinBlock : public StemGrownBlock {
public:
    PumpkinBlock(
        const Block* stem, const Block* attachedStem, const Block* carvedPumpkin, const BlockProperties& properties);

    ~PumpkinBlock() override = default;

    // ========== StemGrownBlock 接口 ==========

    [[nodiscard]] const Block* getStem() const override { return m_stem; }
    [[nodiscard]] const Block* getAttachedStem() const override { return m_attachedStem; }

    // ========== 茎指针设置（用于解决循环依赖） ==========

    void setStem(const Block* stem) { m_stem = stem; }
    void setAttachedStem(const Block* attachedStem) { m_attachedStem = attachedStem; }

    // ========== 交互接口 ==========

    /**
     * @brief 玩家右键点击方块
     *
     * 当玩家使用剪刀右键点击南瓜时，将南瓜雕刻成刻过的南瓜。
     */
    [[nodiscard]] BlockActionResult onBlockActivated(const BlockState& state,
        IWorld& world,
        const BlockPos& pos,
        Player& player,
        Hand hand,
        const BlockRaycastResult& hit) override;

private:
    const Block* m_stem;
    const Block* m_attachedStem;
    const Block* m_carvedPumpkin;
};

/**
 * @brief 刻过的南瓜方块
 *
 * 有 FACING 属性的南瓜，可以用于制作雪傀儡和铁傀儡。
 * 傀儡生成的核心逻辑在此类中实现，JackOLanternBlock 复用此类的静态方法。
 */
class CarvedPumpkinBlock : public HorizontalBlock {
public:
    explicit CarvedPumpkinBlock(const BlockProperties& properties);

    ~CarvedPumpkinBlock() override = default;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 方块添加 ==========

    /**
     * @brief 方块添加到世界时调用
     *
     * 检测是否可以生成雪傀儡或铁傀儡。
     * 仅在方块类型实际改变时触发（防止 FACING 属性变化时重复触发）。
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

    // ========== 傀儡生成静态方法 ==========

    /**
     * @brief 尝试在指定位置生成傀儡
     *
     * 检测雪傀儡模式和铁傀儡模式。如果匹配，移除方块并生成对应实体。
     * 此方法为静态方法，供 CarvedPumpkinBlock 和 JackOLanternBlock 共用。
     *
     * @param world 世界引用
     * @param pos 南瓜/南瓜灯位置
     * @return 是否成功生成傀儡
     */
    static bool trySpawnGolem(IWorld& world, const BlockPos& pos);

    /**
     * @brief 检查方块状态是否为南瓜类型（可作为傀儡头部）
     *
     * 刻过的南瓜和南瓜灯都可以作为傀儡的头部方块。
     * 对应 MC 原版的 PUMPKINS_PREDICATE。
     * 当 onBlockAdded 被调用时，触发方块本身已知是南瓜头部，无需额外检查；
     * 此方法预留供未来 canSpawnGolem API 使用（仅检测身体部分是否满足傀儡模式）。
     *
     * TODO: 实现 canSpawnGolem 公共 API 时，此方法将作为头部检测的核心判断，
     * 允许外部查询某位置是否可生成傀儡（仅检查身体部分，头部由调用方提供）。
     *
     * @param state 方块状态
     * @return 是否为南瓜类型
     */
    [[nodiscard]] static bool isPumpkinHead(const BlockState* state);

private:
    /**
     * @brief 检测雪傀儡模式
     *
     * 模式：从上到下依次为南瓜头部、雪块、雪块（垂直线形）
     */
    [[nodiscard]] static bool checkSnowGolemPattern(IWorld& world, const BlockPos& pos);

    /**
     * @brief 检测铁傀儡模式
     *
     * 模式：T形铁块结构
     * 顶层：空气、南瓜头部、空气
     * 中层：铁块、铁块、铁块（手臂）
     * 底层：空气、铁块、空气（身体）
     *
     * @param world 世界引用
     * @param pos 南瓜头部位置
     * @param outBodyPos 输出铁傀儡身体位置（模式匹配时填充）
     * @param outIsEastWest 输出手臂是否为东西方向（模式匹配时填充）
     * @return 是否匹配铁傀儡模式
     */
    [[nodiscard]] static bool checkIronGolemPattern(
        IWorld& world, const BlockPos& pos, BlockPos& outBodyPos, bool& outIsEastWest);

    /**
     * @brief 执行雪傀儡生成：移除方块并生成实体
     */
    static void spawnSnowGolem(IWorld& world, const BlockPos& headPos);

    /**
     * @brief 执行铁傀儡生成：移除方块并生成实体
     *
     * @param world 世界引用
     * @param headPos 南瓜头部位置
     * @param armCenterPos 手臂中央位置（中层铁块）
     * @param isEastWest 手臂是否为东西方向
     */
    static void spawnIronGolem(IWorld& world, const BlockPos& headPos, const BlockPos& armCenterPos, bool isEastWest);

    /**
     * @brief 检查方块是否为空气
     */
    [[nodiscard]] static bool isAir(const BlockState* state);
};

/**
 * @brief 南瓜灯方块
 *
 * 有 FACING 属性的发光南瓜。也可以用于制作雪傀儡和铁傀儡。
 * 傀儡生成逻辑复用 CarvedPumpkinBlock 的静态方法。
 */
class JackOLanternBlock : public HorizontalBlock {
public:
    explicit JackOLanternBlock(const BlockProperties& properties);

    ~JackOLanternBlock() override = default;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 方块添加 ==========

    /**
     * @brief 方块添加到世界时调用
     *
     * 检测是否可以生成雪傀儡或铁傀儡。
     * 仅在方块类型实际改变时触发（防止 FACING 属性变化时重复触发）。
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;
};

// ============================================================================
// 茎类方块具体实现
// ============================================================================

/**
 * @brief 西瓜茎方块
 *
 * 西瓜的茎类作物，生长成熟后在相邻位置生成西瓜方块。
 */
class MelonStemBlock : public StemBlock {
public:
    MelonStemBlock(const StemGrownBlock* crop, const BlockProperties& properties);

    ~MelonStemBlock() override = default;

    [[nodiscard]] u32 getSeedItem() const override;
};

/**
 * @brief 南瓜茎方块
 *
 * 南瓜的茎类作物，生长成熟后在相邻位置生成南瓜方块。
 */
class PumpkinStemBlock : public StemBlock {
public:
    PumpkinStemBlock(const StemGrownBlock* crop, const BlockProperties& properties);

    ~PumpkinStemBlock() override = default;

    [[nodiscard]] u32 getSeedItem() const override;
};

/**
 * @brief 连接西瓜茎方块
 *
 * 西瓜生成后茎变成的方块，朝向西瓜方向。
 */
class MelonAttachedStemBlock : public AttachedStemBlock {
public:
    MelonAttachedStemBlock(const StemGrownBlock* crop, const BlockProperties& properties);

    ~MelonAttachedStemBlock() override = default;

    [[nodiscard]] u32 getSeedItem() const override;
};

/**
 * @brief 连接南瓜茎方块
 *
 * 南瓜生成后茎变成的方块，朝向南瓜方向。
 */
class PumpkinAttachedStemBlock : public AttachedStemBlock {
public:
    PumpkinAttachedStemBlock(const StemGrownBlock* crop, const BlockProperties& properties);

    ~PumpkinAttachedStemBlock() override = default;

    [[nodiscard]] u32 getSeedItem() const override;
};

} // namespace blocks
} // namespace mc
