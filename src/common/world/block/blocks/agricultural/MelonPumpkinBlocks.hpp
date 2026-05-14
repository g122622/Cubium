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
 *
 * 参考: net.minecraft.block.MelonBlock
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

    /**
     * @brief 设置茎方块指针
     * @param stem 茎方块指针
     */
    void setStem(const Block* stem) { m_stem = stem; }

    /**
     * @brief 设置连接茎方块指针
     * @param attachedStem 连接茎方块指针
     */
    void setAttachedStem(const Block* attachedStem) { m_attachedStem = attachedStem; }

private:
    const Block* m_stem;
    const Block* m_attachedStem;
};

/**
 * @brief 南瓜方块
 *
 * 由南瓜茎生成的果实方块。可以用剪刀刻成刻过的南瓜。
 *
 * 参考: net.minecraft.block.PumpkinBlock
 */
class PumpkinBlock : public StemGrownBlock {
public:
    /**
     * @brief 构造函数
     * @param stem 对应的茎方块
     * @param attachedStem 对应的连接茎方块
     * @param carvedPumpkin 刻过的南瓜方块
     * @param properties 方块属性
     */
    PumpkinBlock(
        const Block* stem, const Block* attachedStem, const Block* carvedPumpkin, const BlockProperties& properties);

    ~PumpkinBlock() override = default;

    // ========== StemGrownBlock 接口 ==========

    [[nodiscard]] const Block* getStem() const override { return m_stem; }
    [[nodiscard]] const Block* getAttachedStem() const override { return m_attachedStem; }

    // ========== 茎指针设置（用于解决循环依赖） ==========

    /**
     * @brief 设置茎方块指针
     * @param stem 茎方块指针
     */
    void setStem(const Block* stem) { m_stem = stem; }

    /**
     * @brief 设置连接茎方块指针
     * @param attachedStem 连接茎方块指针
     */
    void setAttachedStem(const Block* attachedStem) { m_attachedStem = attachedStem; }

    // ========== 交互接口 ==========

    /**
     * @brief 玩家右键点击方块
     *
     * 当玩家使用剪刀右键点击南瓜时，将南瓜雕刻成刻过的南瓜。
     *
     * @param state 方块状态
     * @param world 世界
     * @param pos 方块位置
     * @param player 玩家
     * @param hand 手
     * @param hit 射线检测结果
     * @return 交互结果类型
     */
    [[nodiscard]] ActionResultType onBlockActivated(const BlockState& state,
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
 *
 * 参考: net.minecraft.block.CarvedPumpkinBlock
 */
class CarvedPumpkinBlock : public HorizontalBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit CarvedPumpkinBlock(const BlockProperties& properties);

    ~CarvedPumpkinBlock() override = default;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 方块添加 ==========

    /**
     * @brief 方块添加到世界时调用
     *
     * 检测是否可以生成雪傀儡或铁傀儡。
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

private:
    /**
     * @brief 尝试生成傀儡
     *
     * 检测雪傀儡模式（两个雪块+南瓜）和铁傀儡模式（T形铁块+南瓜）。
     * 如果匹配，移除方块并生成对应实体。
     *
     * @param world 世界引用
     * @param pos 南瓜位置
     * @return 是否成功生成傀儡
     */
    bool trySpawnGolem(IWorld& world, const BlockPos& pos);

    /**
     * @brief 检测雪傀儡模式
     *
     * 模式：从上到下依次为南瓜、雪块、雪块（垂直线形）
     *
     * @param world 世界引用
     * @param pos 南瓜位置
     * @return 是否匹配雪傀儡模式
     */
    [[nodiscard]] bool checkSnowGolemPattern(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 检测铁傀儡模式
     *
     * 模式：T形铁块结构
     * 顶层：空气、南瓜、空气
     * 中层：铁块、铁块、铁块（手臂）
     * 底层：空气、铁块、空气（身体）
     *
     * @param world 世界引用
     * @param pos 南瓜位置
     * @param outBodyPos 输出铁傀儡身体位置（模式匹配时填充）
     * @return 是否匹配铁傀儡模式
     */
    [[nodiscard]] bool checkIronGolemPattern(IWorld& world, const BlockPos& pos, BlockPos& outBodyPos) const;

    /**
     * @brief 检查方块是否为南瓜类型（雕刻南瓜或南瓜灯）
     *
     * @param state 方块状态
     * @return 是否为南瓜类型
     */
    [[nodiscard]] static bool isPumpkin(const BlockState* state);

    /**
     * @brief 检查方块是否为空气
     *
     * @param state 方块状态
     * @return 是否为空气
     */
    [[nodiscard]] static bool isAir(const BlockState* state);
};

/**
 * @brief 南瓜灯方块
 *
 * 有 FACING 属性的发光南瓜。也可以用于制作雪傀儡和铁傀儡。
 *
 * 参考: net.minecraft.block.JackOLanternBlock
 */
class JackOLanternBlock : public HorizontalBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit JackOLanternBlock(const BlockProperties& properties);

    ~JackOLanternBlock() override = default;

    // ========== 放置逻辑 ==========

    [[nodiscard]] BlockState getStateForPlacement(BlockItemUseContext& context) override;

    // ========== 方块添加 ==========

    /**
     * @brief 方块添加到世界时调用
     *
     * 检测是否可以生成雪傀儡或铁傀儡。
     * 与 CarvedPumpkinBlock 相同，南瓜灯也可以触发傀儡生成。
     */
    void onBlockAdded(IWorld& world, const BlockPos& pos, const BlockState& state) override;

private:
    /**
     * @brief 尝试生成傀儡
     *
     * 使用 CarvedPumpkinBlock 的静态辅助方法检测模式。
     *
     * @param world 世界引用
     * @param pos 南瓜灯位置
     * @return 是否成功生成傀儡
     */
    bool trySpawnGolem(IWorld& world, const BlockPos& pos);
};

// ============================================================================
// 茎类方块具体实现
// ============================================================================

/**
 * @brief 西瓜茎方块
 *
 * 西瓜的茎类作物，生长成熟后在相邻位置生成西瓜方块。
 *
 * 参考: net.minecraft.block.StemBlock (西瓜茎变体)
 */
class MelonStemBlock : public StemBlock {
public:
    /**
     * @brief 构造函数
     * @param crop 对应的果实方块（西瓜）
     * @param properties 方块属性
     */
    MelonStemBlock(const StemGrownBlock* crop, const BlockProperties& properties);

    ~MelonStemBlock() override = default;

    /**
     * @brief 获取种子物品ID
     * @return 西瓜种子物品ID
     */
    [[nodiscard]] u32 getSeedItem() const override;
};

/**
 * @brief 南瓜茎方块
 *
 * 南瓜的茎类作物，生长成熟后在相邻位置生成南瓜方块。
 *
 * 参考: net.minecraft.block.StemBlock (南瓜茎变体)
 */
class PumpkinStemBlock : public StemBlock {
public:
    /**
     * @brief 构造函数
     * @param crop 对应的果实方块（南瓜）
     * @param properties 方块属性
     */
    PumpkinStemBlock(const StemGrownBlock* crop, const BlockProperties& properties);

    ~PumpkinStemBlock() override = default;

    /**
     * @brief 获取种子物品ID
     * @return 南瓜种子物品ID
     */
    [[nodiscard]] u32 getSeedItem() const override;
};

/**
 * @brief 连接西瓜茎方块
 *
 * 西瓜生成后茎变成的方块，朝向西瓜方向。
 *
 * 参考: net.minecraft.block.AttachedStemBlock (西瓜变体)
 */
class MelonAttachedStemBlock : public AttachedStemBlock {
public:
    /**
     * @brief 构造函数
     * @param crop 对应的果实方块（西瓜）
     * @param properties 方块属性
     */
    MelonAttachedStemBlock(const StemGrownBlock* crop, const BlockProperties& properties);

    ~MelonAttachedStemBlock() override = default;

    /**
     * @brief 获取种子物品ID
     * @return 西瓜种子物品ID
     */
    [[nodiscard]] u32 getSeedItem() const override;
};

/**
 * @brief 连接南瓜茎方块
 *
 * 南瓜生成后茎变成的方块，朝向南瓜方向。
 *
 * 参考: net.minecraft.block.AttachedStemBlock (南瓜变体)
 */
class PumpkinAttachedStemBlock : public AttachedStemBlock {
public:
    /**
     * @brief 构造函数
     * @param crop 对应的果实方块（南瓜）
     * @param properties 方块属性
     */
    PumpkinAttachedStemBlock(const StemGrownBlock* crop, const BlockProperties& properties);

    ~PumpkinAttachedStemBlock() override = default;

    /**
     * @brief 获取种子物品ID
     * @return 南瓜种子物品ID
     */
    [[nodiscard]] u32 getSeedItem() const override;
};

} // namespace blocks
} // namespace mc
