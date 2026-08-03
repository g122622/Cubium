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

#include "core/Types.hpp"
#include "resource/ResourceLocation.hpp"
#include <cstddef>
#include <functional>
#include <optional>
#include <set>
#include <string>
#include <unordered_set>
#include <vector>

namespace mc {

// 前向声明
namespace nbt {
namespace tags {
struct compound_tag;
}
} // namespace nbt

namespace crafting {

// 前向声明
class IRecipeBase;

/**
 * @brief 配方书分类枚举
 *
 * 参考 MC 1.16.5: net.minecraft.item.crafting.RecipeBookCategory
 * 用于区分不同类型的合成界面的配方书状态。
 */
enum class RecipeBookCategory : u8 {
    Crafting,     ///< 工作台合成
    Furnace,      ///< 熔炉
    BlastFurnace, ///< 高炉
    Smoker,       ///< 烟熏炉

    Count ///< 分类数量
};

/**
 * @brief 获取 RecipeBookCategory 的字符串表示
 * @param category 配方书分类
 * @return 分类的字符串名称
 */
[[nodiscard]] const char* recipeBookCategoryToString(RecipeBookCategory category) noexcept;

/**
 * @brief 从字符串解析 RecipeBookCategory
 * @param str 分类的字符串名称
 * @return 配方书分类，如果无法识别返回 std::nullopt
 */
[[nodiscard]] std::optional<RecipeBookCategory> recipeBookCategoryFromString(const std::string& str) noexcept;

/**
 * @brief 配方书GUI状态
 *
 * 存储每个配方书分类的GUI状态（是否打开、是否过滤可合成）。
 * 参考 MC 1.16.5: net.minecraft.item.crafting.RecipeBookStatus
 */
class RecipeBookStatus {
public:
    /**
     * @brief 单个分类的状态
     */
    struct CategoryStatus {
        bool guiOpen = false;            ///< GUI是否打开
        bool filteringCraftable = false; ///< 是否过滤为可合成物品

        CategoryStatus() = default;
        CategoryStatus(bool open, bool filtering)
            : guiOpen(open)
            , filteringCraftable(filtering)
        {}

        [[nodiscard]] bool operator==(const CategoryStatus& other) const noexcept
        {
            return guiOpen == other.guiOpen && filteringCraftable == other.filteringCraftable;
        }

        [[nodiscard]] CategoryStatus copy() const noexcept { return *this; }
    };

    RecipeBookStatus();

    /**
     * @brief 获取指定分类的GUI打开状态
     * @param category 配方书分类
     * @return GUI是否打开
     */
    [[nodiscard]] bool isGuiOpen(RecipeBookCategory category) const noexcept;

    /**
     * @brief 设置指定分类的GUI打开状态
     * @param category 配方书分类
     * @param open 是否打开
     */
    void setGuiOpen(RecipeBookCategory category, bool open) noexcept;

    /**
     * @brief 获取指定分类的过滤状态
     * @param category 配方书分类
     * @return 是否过滤为可合成物品
     */
    [[nodiscard]] bool isFilteringCraftable(RecipeBookCategory category) const noexcept;

    /**
     * @brief 设置指定分类的过滤状态
     * @param category 配方书分类
     * @param filtering 是否过滤
     */
    void setFilteringCraftable(RecipeBookCategory category, bool filtering) noexcept;

    /**
     * @brief 设置指定分类的完整状态
     * @param category 配方书分类
     * @param open GUI是否打开
     * @param filtering 是否过滤
     */
    void setCategoryStatus(RecipeBookCategory category, bool open, bool filtering) noexcept;

    /**
     * @brief 从另一个状态复制
     * @param other 源状态
     */
    void copyFrom(const RecipeBookStatus& other) noexcept;

    /**
     * @brief 创建副本
     * @return 当前状态的副本
     */
    [[nodiscard]] RecipeBookStatus copy() const noexcept;

    /**
     * @brief 相等比较
     */
    [[nodiscard]] bool operator==(const RecipeBookStatus& other) const noexcept;

private:
    CategoryStatus m_status[static_cast<size_t>(RecipeBookCategory::Count)];
};

/**
 * @brief 配方书基类
 *
 * 存储玩家已解锁的配方列表和新解锁的配方列表。
 * 参考 MC 1.16.5: net.minecraft.item.crafting.RecipeBook
 */
class RecipeBook {
public:
    RecipeBook() = default;
    virtual ~RecipeBook() = default;

    // ========== 配方解锁/锁定 ==========

    /**
     * @brief 解锁配方
     *
     * 如果配方不是动态配方，将其添加到已解锁列表。
     *
     * @param recipeId 配方资源位置
     */
    void unlock(const ResourceLocation& recipeId);

    /**
     * @brief 锁定配方
     *
     * 从已解锁列表和新配方列表中移除配方。
     *
     * @param recipeId 配方资源位置
     */
    void lock(const ResourceLocation& recipeId);

    /**
     * @brief 检查配方是否已解锁
     * @param recipeId 配方资源位置
     * @return 是否已解锁
     */
    [[nodiscard]] bool isUnlocked(const ResourceLocation& recipeId) const noexcept;

    /**
     * @brief 检查配方是否在配方书中显示
     *
     * 动态配方（如染色、修复等特殊配方）虽然可以合成，
     * 但不会出现在配方书中。此方法检查配方是否已解锁且非动态。
     * 参考 MC 1.21.1: ServerRecipeBook 中的 isSpecial 过滤逻辑。
     *
     * @param recipeId 配方资源位置
     * @return 是否在配方书中（已解锁且非动态配方）
     */
    [[nodiscard]] bool isBookRecipe(const ResourceLocation& recipeId) const noexcept;

    // ========== 新配方管理 ==========

    /**
     * @brief 检查配方是否为新解锁（未查看）
     * @param recipeId 配方资源位置
     * @return 是否为新配方
     */
    [[nodiscard]] bool isNew(const ResourceLocation& recipeId) const noexcept;

    /**
     * @brief 标记配方为新解锁
     *
     * 新解锁的配方会在配方书UI中高亮显示。
     *
     * @param recipeId 配方资源位置
     */
    void markNew(const ResourceLocation& recipeId);

    /**
     * @brief 标记配方为已查看
     *
     * 玩家在配方书中查看配方后调用，将配方从新配方列表移除。
     *
     * @param recipeId 配方资源位置
     */
    void markSeen(const ResourceLocation& recipeId);

    // ========== 配方列表访问 ==========

    /**
     * @brief 获取所有已解锁的配方ID
     * @return 已解锁配方ID集合
     */
    [[nodiscard]] const std::unordered_set<ResourceLocation>& getUnlockedRecipes() const noexcept { return m_recipes; }

    /**
     * @brief 获取所有新解锁的配方ID
     * @return 新解锁配方ID集合
     */
    [[nodiscard]] const std::unordered_set<ResourceLocation>& getNewRecipes() const noexcept { return m_newRecipes; }

    // ========== 状态管理 ==========

    /**
     * @brief 获取配方书GUI状态
     * @return GUI状态引用
     */
    [[nodiscard]] RecipeBookStatus& getStatus() noexcept { return m_status; }
    [[nodiscard]] const RecipeBookStatus& getStatus() const noexcept { return m_status; }

    /**
     * @brief 设置配方书GUI状态
     * @param status 新状态
     */
    void setStatus(const RecipeBookStatus& status) noexcept { m_status.copyFrom(status); }

    /**
     * @brief 从另一个配方书复制状态
     * @param other 源配方书
     */
    void copyFrom(const RecipeBook& other);

    /**
     * @brief 清空所有配方
     */
    void clear() noexcept;

    // ========== 数量查询 ==========

    /**
     * @brief 获取已解锁配方数量
     * @return 已解锁配方数量
     */
    [[nodiscard]] size_t getUnlockedCount() const noexcept { return m_recipes.size(); }

    /**
     * @brief 获取新配方数量
     * @return 新配方数量
     */
    [[nodiscard]] size_t getNewCount() const noexcept { return m_newRecipes.size(); }

protected:
    std::unordered_set<ResourceLocation> m_recipes;    ///< 已解锁的配方
    std::unordered_set<ResourceLocation> m_newRecipes; ///< 新解锁的配方（待显示）
    RecipeBookStatus m_status;                         ///< GUI状态
};

/**
 * @brief 服务端配方书
 *
 * 扩展 RecipeBook，添加服务端特有的功能：
 * - 批量添加/移除配方
 * - 成就触发
 * - 网络同步
 * - NBT序列化
 *
 * 参考 MC 1.16.5: net.minecraft.item.crafting.ServerRecipeBook
 */
class ServerRecipeBook : public RecipeBook {
public:
    ServerRecipeBook() = default;

    // ========== 批量操作 ==========

    /**
     * @brief 检查配方是否为动态配方
     *
     * 动态配方（染色、修复、书本复制等特殊配方）不会出现在配方书中。
     * 参考 MC 1.21.1: ServerRecipeBook.addRecipes() 中 !recipe.isSpecial() 过滤逻辑。
     *
     * @param recipeId 配方资源位置
     * @return 如果是动态配方返回 true，否则返回 false
     */
    [[nodiscard]] static bool isDynamicRecipe(const ResourceLocation& recipeId);

    /**
     * @brief 批量添加（解锁）配方
     *
     * 解锁配方、标记为新配方、触发成就、发送网络包。
     * 动态配方（isDynamic() == true）会被自动过滤，不会添加到配方书中。
     * 参考 MC 1.21.1: ServerRecipeBook.addRecipes() 中的 !recipe.isSpecial() 过滤。
     *
     * @tparam RecipeIter 配方迭代器类型
     * @param begin 配方ID迭代器起始
     * @param end 配方ID迭代器结束
     * @param onUnlock 配方解锁回调（参数：配方ID）
     * @return 成功解锁的配方数量
     */
    template <typename RecipeIter>
    size_t add(RecipeIter begin, RecipeIter end, std::function<void(const ResourceLocation&)> onUnlock = nullptr)
    {
        size_t added = 0;
        for (auto it = begin; it != end; ++it) {
            const ResourceLocation& recipeId = *it;

            // 动态配方不添加到配方书
            if (isDynamicRecipe(recipeId)) {
                continue;
            }

            // 检查是否已解锁
            if (m_recipes.contains(recipeId)) {
                continue;
            }

            // 解锁配方
            unlock(recipeId);
            markNew(recipeId);
            added++;

            // 触发回调（用于触发成就）
            if (onUnlock) {
                onUnlock(recipeId);
            }
        }
        return added;
    }

    /**
     * @brief 批量移除（锁定）配方
     *
     * 锁定配方、发送网络包。
     *
     * @tparam RecipeIter 配方迭代器类型
     * @param begin 配方ID迭代器起始
     * @param end 配方ID迭代器结束
     * @return 成功锁定的配方数量
     */
    template <typename RecipeIter>
    size_t remove(RecipeIter begin, RecipeIter end)
    {
        size_t removed = 0;
        for (auto it = begin; it != end; ++it) {
            const ResourceLocation& recipeId = *it;
            if (m_recipes.contains(recipeId)) {
                lock(recipeId);
                removed++;
            }
        }
        return removed;
    }

    // ========== NBT序列化 ==========

    /**
     * @brief 将配方书状态写入NBT
     * @return NBT复合标签
     */
    [[nodiscard]] nbt::tags::compound_tag write() const;

    /**
     * @brief 从NBT读取配方书状态
     * @param tag NBT复合标签
     */
    void read(const nbt::tags::compound_tag& tag);

    // ========== 网络同步数据 ==========

    /**
     * @brief 获取需要同步到客户端的新解锁配方列表
     *
     * 调用后清空新配方列表。
     *
     * @return 新解锁的配方ID列表
     */
    [[nodiscard]] std::vector<ResourceLocation> consumeNewRecipes();

    /**
     * @brief 获取所有已解锁配方列表（用于初始化同步）
     * @return 已解锁配方ID列表
     */
    [[nodiscard]] std::vector<ResourceLocation> getAllUnlockedRecipes() const;
};

} // namespace crafting

} // namespace mc
