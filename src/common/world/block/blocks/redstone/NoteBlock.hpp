#pragma once

#include "world/block/Block.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 音符盒方块
 *
 * 音符盒是一个可以发出不同音调的方块，通过红石或玩家右键触发。
 *
 * ## 特性
 * - 25个音阶（0-24）
 * - 音调取决于上方方块类型（乐器）
 * - 红石触发或玩家右键触发
 * - 发出声音并产生粒子效果
 *
 * 参考: net.minecraft.block.NoteBlock
 */
class NoteBlock : public Block {
public:
    /// 音阶数量
    static constexpr i32 NOTE_RANGE = 25;

    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit NoteBlock(const BlockProperties& properties);

    // ========== Block 接口实现 ==========

    void neighborChanged(IWorld& world, const BlockPos& pos, Block& neighborBlock,
                        const BlockPos& neighborPos, bool isMoving) override;

    [[nodiscard]] BlockState updatePostPlacement(
        const BlockState& state, Direction facing,
        const BlockState& facingState, IWorld& world,
        const BlockPos& currentPos, const BlockPos& facingPos) override;

    [[nodiscard]] bool canProvidePower(const BlockState& state) const override {
        MC_UNUSED(state);
        return false;
    }

    // ========== 音符盒特有方法 ==========

    /**
     * @brief 获取当前音调
     * @param state 方块状态
     * @return i32 音调值（0-24）
     */
    [[nodiscard]] static i32 getNote(const BlockState& state);

    /**
     * @brief 设置音调
     * @param state 方块状态
     * @param note 音调值（0-24）
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withNote(BlockState state, i32 note);

    /**
     * @brief 增加音调（循环）
     * @param state 方块状态
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState cycleNote(BlockState state);

    /**
     * @brief 触发播放音符
     *
     * 根据上方方块类型决定乐器，发出声音和粒子效果。
     *
     * @param world 世界引用
     * @param pos 音符盒位置
     * @param state 当前方块状态
     */
    void triggerNote(IWorld& world, const BlockPos& pos, const BlockState& state);

private:
    /**
     * @brief 根据上方方块类型获取乐器类型
     * @param world 世界引用
     * @param pos 音符盒位置
     * @return 乐器类型ID
     */
    [[nodiscard]] i32 getInstrumentType(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 播放音符声音
     * @param world 世界引用
     * @param pos 音符盒位置
     * @param instrument 乐器类型
     * @param note 音调
     */
    void playNote(IWorld& world, const BlockPos& pos, i32 instrument, i32 note);
};

} // namespace blocks
} // namespace mc
