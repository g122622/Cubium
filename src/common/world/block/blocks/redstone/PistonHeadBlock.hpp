#pragma once

#include "../../Block.hpp"
#include "../../../../util/property/Properties.hpp"
#include "../../../../util/property/EnumProperty.hpp"
#include "../../../../util/Direction.hpp"

namespace mc {

namespace util {
    template<typename T>
    class EnumProperty;
}

namespace blocks {

/**
 * @brief 活塞头方块
 *
 * 活塞头是活塞伸出时显示的方块部分。
 *
 * ## 特性
 * - 仅在活塞伸出时显示
 * - 与活塞主体关联
 * - 推动其他方块
 *
 * 参考: net.minecraft.block.PistonHeadBlock
 */
class PistonHeadBlock : public Block {
public:
    /**
     * @brief 活塞头类型
     */
    enum class Type : u8 {
        Normal = 0,  ///< 普通活塞头
        Sticky = 1   ///< 粘性活塞头
    };

    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit PistonHeadBlock(const BlockProperties& properties);

    // ========== Block 接口实现 ==========

    [[nodiscard]] BlockState updatePostPlacement(
        const BlockState& state, Direction facing,
        const BlockState& facingState, IWorld& world,
        const BlockPos& currentPos, const BlockPos& facingPos) override;

    [[nodiscard]] Material::PushReaction getPushReaction(const BlockState& state) const override {
        MC_UNUSED(state);
        return Material::PushReaction::Block;
    }

    // ========== 活塞头特有方法 ==========

    /**
     * @brief 获取活塞头朝向
     *
     * @param state 方块状态
     * @return Direction 朝向方向
     */
    [[nodiscard]] static Direction getFacing(const BlockState& state);

    /**
     * @brief 获取活塞头类型
     *
     * @param state 方块状态
     * @return Type 活塞头类型
     */
    [[nodiscard]] static Type getType(const BlockState& state);

    /**
     * @brief 设置活塞头类型
     *
     * @param state 方块状态
     * @param type 类型
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withType(BlockState state, Type type);

    /**
     * @brief 获取活塞头类型属性
     *
     * 用于 MovingPistonBlock 共享相同的属性。
     *
     * @return 类型属性的引用
     */
    [[nodiscard]] static const EnumProperty<Type>& getTypeProperty();
};

} // namespace blocks
} // namespace mc
