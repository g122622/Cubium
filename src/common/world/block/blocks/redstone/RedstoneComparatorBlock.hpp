#pragma once

#include "RedstoneDiodeBlock.hpp"
#include "../../../../util/property/Properties.hpp"
#include <memory>

namespace mc {

class BlockEntity;

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
    static std::optional<blocks::ComparatorMode> fromName(StringView name);
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
 * - 前端信号保持（需要 BlockEntity 存储）
 *
 * ## 容易踩的坑
 * - 比较器需要 BlockEntity 存储输出信号强度
 * - 减法模式计算复杂
 * - 方向性处理
 * - 输出信号在 tick 时存储到 BlockEntity，而不是实时计算
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

    // ========== Block 接口实现 ==========

    /**
     * @brief 是否有方块实体
     * @return true 比较器需要 BlockEntity 存储输出信号
     */
    [[nodiscard]] bool hasBlockEntity() const override { return true; }

    /**
     * @brief 创建方块实体
     * @param pos 方块位置
     * @return 新创建的比较器方块实体
     */
    [[nodiscard]] std::unique_ptr<BlockEntity> createBlockEntity(const BlockPos& pos) override;

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

    /**
     * @brief 获取存储的输出信号强度
     *
     * 从 BlockEntity 读取输出信号。如果 BlockEntity 不存在，返回 0。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @return i32 存储的输出信号强度 0-15
     */
    [[nodiscard]] i32 getStoredOutputSignal(IWorld& world, const BlockPos& pos) const;

    /**
     * @brief 存储输出信号强度
     *
     * 将输出信号存储到 BlockEntity 中。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param signal 信号强度 0-15
     */
    void storeOutputSignal(IWorld& world, const BlockPos& pos, i32 signal) const;

protected:
    /**
     * @brief 状态更新时触发
     *
     * 重写以在状态变化时更新 BlockEntity 中的输出信号。
     */
    void onStateChanged(IWorld& world, const BlockPos& pos, const BlockState& oldState,
                        const BlockState& newState);

private:
    /// 比较器延迟（固定2 tick）
    static constexpr i32 COMPARATOR_DELAY = 2;

    /**
     * @brief 计算输入信号强度（包括容器信号检测）
     *
     * MC Java: calculateInputStrength
     * 不仅检测红石信号，还检测容器信号和物品展示框信号。
     *
     * @param world 世界引用
     * @param pos 方块位置
     * @param state 当前方块状态
     * @return i32 输入信号强度 0-15
     */
    [[nodiscard]] i32 calculateInputStrength(IWorld& world, const BlockPos& pos,
                                              const BlockState& state) const;

    /**
     * @brief 计算输出信号强度
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
