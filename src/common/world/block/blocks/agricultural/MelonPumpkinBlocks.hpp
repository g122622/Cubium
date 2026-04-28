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
     */
    void trySpawnGolem(IWorld& world, const BlockPos& pos);
};

/**
 * @brief 南瓜灯方块
 *
 * 有 FACING 属性的发光南瓜。
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
};

} // namespace blocks
} // namespace mc
