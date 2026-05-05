#pragma once

#include "core/Types.hpp"

namespace mc {

// 前向声明
class IInventory;

namespace crafting {
    template<typename C>
    class IRecipe;
}

/**
 * @brief 配方持有者接口
 *
 * 实现此接口的背包可以追踪当前使用的配方。
 * 主要用于：
 * - 合成结果槽位追踪配方（用于解锁配方成就）
 * - 熔炉等方块实体追踪当前配方
 *
 * 参考: net.minecraft.inventory.IRecipeHolder
 *
 * 注意：onCrafting 和 canUseRecipe 的默认实现需要访问 ServerPlayer 和 World，
 * 这些在服务端模块中实现。此类只提供接口定义，具体逻辑在服务端处理。
 */
class IRecipeHolder {
public:
    virtual ~IRecipeHolder() = default;

    /**
     * @brief 设置当前使用的配方
     * @param recipe 配方指针，nullptr表示清除
     */
    virtual void setRecipeUsed(const crafting::IRecipe<IInventory>* recipe) = 0;

    /**
     * @brief 获取当前使用的配方
     * @return 配方指针，如果没有返回nullptr
     */
    [[nodiscard]] virtual const crafting::IRecipe<IInventory>* getRecipeUsed() const = 0;
};

} // namespace mc
