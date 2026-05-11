#pragma once

#include "StemBlock.hpp"
#include "../HorizontalBlock.hpp"

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
    PumpkinBlock(const Block* stem, const Block* attachedStem, const Block* carvedPumpkin, const BlockProperties& properties);

    ~PumpkinBlock() override = default;

    // ========== StemGrownBlock 接口 ==========

    [[nodiscard]] const Block* getStem() const override { return m_stem; }
    [[nodiscard]] const Block* getAttachedStem() const override { return m_attachedStem; }

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
    [[nodiscard]] bool checkIronGolemPattern(
        IWorld& world,
        const BlockPos& pos,
        BlockPos& outBodyPos) const;

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

} // namespace blocks
} // namespace mc
