#pragma once

#include "RedstoneDiodeBlock.hpp"
#include "../../../../util/property/Properties.hpp"

namespace mc {
namespace blocks {

/**
 * @brief 红石比较器模式
 */
enum class ComparatorMode : u8 {
    Compare = 0,  ///< 比较模式
    Subtract = 1  ///< 减法模式
};

} // namespace blocks

// 特化 EnumProperty::Traits for ComparatorMode
template<>
struct EnumProperty<blocks::ComparatorMode>::Traits {
    static String toString(const blocks::ComparatorMode& value);
    static Optional<blocks::ComparatorMode> fromName(StringView name);
};

namespace blocks {

/**
 * @brief 红石比较器方块
 *
 * 红石比较器可以比较信号强度、检测容器内容物。
 *
 * ## 特性
 * - 比较模式：输出较强信号
 * - 减法模式：输出差值信号
 * - 容器检测：可以检测容器内容物
 * - 2 tick延迟
 * - 侧面锁定
 *
 * ## 容易踩的坑
 * - 比较器需要检测容器信号
 * - 减法模式计算复杂
 * - 方向性处理
 *
 * 参考: net.minecraft.block.ComparatorBlock
 */
class RedstoneComparatorBlock : public RedstoneDiodeBlock {
public:
    /**
     * @brief 构造函数
     * @param properties 方块属性
     */
    explicit RedstoneComparatorBlock(const BlockProperties& properties);

    // ========== 红石二极管接口实现 ==========

    [[nodiscard]] i32 getDelay(const BlockState& state) const override;

    [[nodiscard]] bool shouldBePowered(IWorld& world, const BlockPos& pos,
                                       const BlockState& state) const override;

    [[nodiscard]] i32 calculateOutputSignal(IWorld& world, const BlockPos& pos,
                                            const BlockState& state) const override;

    // ========== 比较器特有方法 ==========

    /**
     * @brief 获取比较器模式
     *
     * @param state 方块状态
     * @return ComparatorMode 当前模式
     */
    [[nodiscard]] static ComparatorMode getMode(const BlockState& state);

    /**
     * @brief 设置比较器模式
     *
     * @param state 方块状态
     * @param mode 模式
     * @return BlockState 更新后的状态
     */
    [[nodiscard]] static BlockState withMode(BlockState state, ComparatorMode mode);

    /**
     * @brief 检查是否为减法模式
     *
     * @param state 方块状态
     * @return true 如果是减法模式
     */
    [[nodiscard]] static bool isSubtractMode(const BlockState& state);

private:
    /// 比较器延迟（固定2 tick）
    static constexpr i32 COMPARATOR_DELAY = 2;

    /**
     * @brief 计算输入信号强度（包括侧面信号）
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     * @param mainInput 主输入信号（用于比较）
     * @param sideInput 侧面输入信号（用于减法模式）
     * @return i32 计算后的输出信号强度
     */
    [[nodiscard]] i32 calculateOutput(IWorld& world, const BlockPos& pos,
                                       const BlockState& state) const;
};

} // namespace blocks
} // namespace mc
