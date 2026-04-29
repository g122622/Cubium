#pragma once

#include "IInventory.hpp"
#include "util/Direction.hpp"

namespace mc {

/**
 * @brief 侧面访问受限的背包接口
 *
 * 某些容器（如熔炉、酿造台、潜影盒）根据访问方向有不同的槽位访问权限。
 * 此接口允许漏斗等自动化设备正确地与这些容器交互。
 *
 * 例如，熔炉的槽位访问规则：
 * - 上方：只能访问输入槽（槽位 0）
 * - 下方：可以访问输出槽（槽位 2）和燃料槽（槽位 1）
 * - 侧面：只能访问燃料槽（槽位 1）
 *
 * 参考: net.minecraft.inventory.ISidedInventory
 */
class ISidedInventory : public IInventory {
public:
    ~ISidedInventory() override = default;

    /**
     * @brief 获取指定面可以访问的槽位
     *
     * 返回一个槽位索引数组，表示从指定方向可以访问哪些槽位。
     *
     * @param side 访问方向
     * @return 可访问的槽位索引数组
     *
     * 示例（熔炉）：
     * - Direction::Up -> {0} (输入槽)
     * - Direction::Down -> {2, 1} (输出槽、燃料槽)
     * - Direction::North/South/East/West -> {1} (燃料槽)
     */
    [[nodiscard]] virtual std::vector<i32> getSlotsForFace(Direction side) const = 0;

    /**
     * @brief 检查是否可以从指定方向向指定槽位插入物品
     *
     * @param slot 槽位索引
     * @param stack 要插入的物品
     * @param direction 插入方向（可为 Direction::None）
     * @return 如果可以插入返回 true
     */
    [[nodiscard]] virtual bool canInsertItem(i32 slot, const ItemStack& stack, Direction direction) const = 0;

    /**
     * @brief 检查是否可以从指定方向从指定槽位提取物品
     *
     * @param slot 槽位索引
     * @param stack 要提取的物品
     * @param direction 提取方向
     * @return 如果可以提取返回 true
     */
    [[nodiscard]] virtual bool canExtractItem(i32 slot, const ItemStack& stack, Direction direction) const = 0;
};

} // namespace mc
