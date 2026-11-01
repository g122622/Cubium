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
#include "item/crafting/RecipeBookCategories.hpp"
#include "resource/ResourceLocation.hpp"
#include <array>
#include <cstddef>
#include <functional>
#include <optional>
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

/**
 * @file RecipeBook.hpp
 * @brief 配方书（玩家已解锁的配方 + 各界面配方书的 GUI 状态）。
 *
 * 对齐 MC 1.21.11 的 `net.minecraft.stats.RecipeBook` / `RecipeBookSettings` /
 * `ServerRecipeBook`。
 *
 * ## 1.16.5 → 1.21.11 的两处语义变更
 *
 * **一、GUI 状态按「配方书类型」寻址，不是按「物品分类」。**
 * 1.16.5 的 `RecipeBookCategory`（crafting / furnace / blast_furnace / smoker）**名字**像
 * 类型，但它同时被当作「界面里那一排分类页签」使用，两者混为一谈。1.21.11 把概念拆成
 * 两层：
 *
 * - {@link RecipeBookType}：**哪个界面**（工作台 / 熔炉 / 高炉 / 烟熏炉）。只有它能打开
 *   或关闭配方书、能过滤为「只看可合成的」。`RecipeBookSettings` 就是这四个各一对布尔。
 * - {@link CraftingBookCategory} / {@link CookingBookCategory}：**配方在界面里归到哪个
 *   页签**（建筑 / 红石 / 装备 / 杂项，食物 / 方块 / 杂项）。它是配方数据的属性，写在
 *   JSON 的 `category` 字段里，与「配方书开没开」无关。
 *
 * 混用会造成一个具体的错：按分类存 GUI 状态时，「工作台配方书开着」这个状态挂在
 * `Crafting` 上，而 `Crafting` 同时还想表示「建筑类页签」——于是打开建筑页签会连带
 * 把整个工作台配方书算成打开。1.21.11 的四类型 × 两类别的正交结构没有这个问题。
 *
 * **二、解锁集合与「新配方高亮」是两个独立集合。**
 * 1.16.5 把「待显示」当解锁的附属状态（markNew / markSeen 一对方法）。1.21.11 的
 * `ServerRecipeBook` 直接持有 `known` 与 `highlight` 两个集合：解锁时同时插入两者，
 * 玩家看过（或重进游戏）后只从 `highlight` 移除，`known` 不受影响。
 * `Packed` 结构把两者与设置一起打包，用于重生 / 换维度时的整体转移。
 */

/**
 * @brief 配方书类型：**哪个界面**有配方书。
 *
 * 对齐 MC 1.21.11 的 `net.minecraft.world.inventory.RecipeBookType`。
 * 它是 {@link RecipeBookSettings} 的索引——GUI 打开状态与「只看可合成」按它分开存。
 */
enum class RecipeBookType : u8 {
    Crafting,     ///< 工作台（3×3 与玩家自带 2×2）
    Furnace,      ///< 熔炉
    BlastFurnace, ///< 高炉
    Smoker,       ///< 烟熏炉

    Count ///< 类型数量
};

/**
 * @brief 配方书类型的字符串表示（NBT / 命令里的写法）。
 * @return 小写下划线形式，例如 `blast_furnace`
 */
[[nodiscard]] const char* recipeBookTypeToString(RecipeBookType type) noexcept;

/**
 * @brief 从字符串解析配方书类型。
 * @param str 类型名，可带 `minecraft:` 前缀
 * @return 类型；无法识别时为空
 */
[[nodiscard]] std::optional<RecipeBookType> recipeBookTypeFromString(const std::string& str) noexcept;

/**
 * @brief 配方书界面设置：每类配方书一对「是否打开 / 是否过滤可合成」。
 *
 * 对齐 MC 1.21.11 的 `net.minecraft.stats.RecipeBookSettings`。
 *
 * 注意这里**没有**按物品分类寻址的接口：页签分类（{@link CraftingBookCategory} 等）
 * 不参与界面状态，混用会得出「打开建筑页签 = 打开工作台配方书」这类错误结论。
 */
class RecipeBookSettings {
public:
    /// 单个配方书类型的状态。
    struct TypeSettings {
        bool open = false;      ///< 配方书是否展开
        bool filtering = false; ///< 是否过滤为「只看现在能合成的」

        [[nodiscard]] bool operator==(const TypeSettings& other) const noexcept
        {
            return open == other.open && filtering == other.filtering;
        }
    };

    RecipeBookSettings() = default;

    /// 指定类型的打开状态。
    [[nodiscard]] bool isOpen(RecipeBookType type) const noexcept;

    /// 指定类型的过滤状态。
    [[nodiscard]] bool isFiltering(RecipeBookType type) const noexcept;

    /// 设置打开状态。
    void setOpen(RecipeBookType type, bool open) noexcept;

    /// 设置过滤状态。
    void setFiltering(RecipeBookType type, bool filtering) noexcept;

    /// 同时设置打开与过滤（对应原版 `RecipeBook#setBookSetting`）。
    void set(RecipeBookType type, bool open, bool filtering) noexcept;

    /// 读取某类型的整对状态。
    [[nodiscard]] TypeSettings get(RecipeBookType type) const noexcept;

    /// 原地替换全部四个类型的状态（原版 `replaceFrom`）。
    void replaceFrom(const RecipeBookSettings& other) noexcept;

    /// 复制一份（原版 `copy()`）。
    [[nodiscard]] RecipeBookSettings copy() const noexcept;

    [[nodiscard]] bool operator==(const RecipeBookSettings& other) const noexcept;

private:
    std::array<TypeSettings, static_cast<size_t>(RecipeBookType::Count)> m_settings{};
};

/**
 * @brief 配方书基类：持有界面设置，并提供按类型读写打开 / 过滤状态的方法。
 *
 * 对齐 MC 1.21.11 的 `net.minecraft.stats.RecipeBook`（该类只负责设置；解锁集合在
 * {@link ServerRecipeBook} 里）。
 */
class RecipeBook {
public:
    RecipeBook() = default;
    virtual ~RecipeBook() = default;

    [[nodiscard]] bool isOpen(RecipeBookType type) const noexcept
    {
        return m_settings.isOpen(type);
    }

    void setOpen(RecipeBookType type, bool open) noexcept
    {
        m_settings.setOpen(type, open);
    }

    [[nodiscard]] bool isFiltering(RecipeBookType type) const noexcept
    {
        return m_settings.isFiltering(type);
    }

    void setFiltering(RecipeBookType type, bool filtering) noexcept
    {
        m_settings.setFiltering(type, filtering);
    }

    /// 同时设置打开与过滤（原版 `RecipeBook#setBookSetting`）。
    void setBookSetting(RecipeBookType type, bool open, bool filtering) noexcept
    {
        m_settings.set(type, open, filtering);
    }

    [[nodiscard]] const RecipeBookSettings& getBookSettings() const noexcept
    {
        return m_settings;
    }

    /// 原地替换全部设置（原版 `RecipeBook#setBookSettings`）。
    void setBookSettings(const RecipeBookSettings& settings) noexcept
    {
        m_settings.replaceFrom(settings);
    }

protected:
    RecipeBookSettings m_settings; ///< 各个界面配方书的 GUI 状态
};

/**
 * @brief 服务端配方书：已解锁配方 + 新解锁高亮 + 界面设置。
 *
 * 对齐 MC 1.21.11 的 `net.minecraft.stats.ServerRecipeBook`。
 *
 * 两个集合的语义：
 *
 * - `known`：玩家已解锁的配方。`doLimitedCrafting` 开启时，不在其中的配方不能合成。
 * - `highlight`：**新解锁、尚未在界面上看过**的配方（原版字段名就叫 highlight）。
 *   解锁时与 `known` 一起插入；玩家看过或重新登录后移除，`known` 不受影响。
 *
 * 两者独立，是 1.21.11 相对 1.16.5 的改动。1.16.5 把「待显示」做成解锁的附属状态
 * （markNew / markSeen），分离后「移除高亮」不再有「顺手把配方也锁了」的风险。
 */
class ServerRecipeBook : public RecipeBook {
public:
    ServerRecipeBook() = default;

    // ========== 单条解锁 / 锁定 ==========

    /// 解锁一条配方（同时标记为新解锁，等待高亮）。
    void add(const ResourceLocation& recipeId);

    /// 该配方是否已解锁。
    [[nodiscard]] bool contains(const ResourceLocation& recipeId) const noexcept;

    /// 锁定一条配方：同时从 `known` 与 `highlight` 移除。
    void remove(const ResourceLocation& recipeId);

    /// 只移除高亮（玩家看过之后），配方保持解锁。
    void removeHighlight(const ResourceLocation& recipeId);

    /// 该配方是否仍处于「新解锁待高亮」状态。
    [[nodiscard]] bool isHighlighted(const ResourceLocation& recipeId) const noexcept;

    // ========== 批量解锁 / 锁定 ==========

    /**
     * @brief 检查配方是否为「特殊配方」（不进入配方书）。
     *
     * 原版 `ServerRecipeBook#addRecipes` 用 `!recipe.isSpecial()` 挡掉染色、修复、书本
     * 复制这类按物品外观动态匹配的配方：它们没有固定原料，列进配方书既无法展示也无法
     * 点击合成。
     */
    [[nodiscard]] static bool isSpecialRecipe(const ResourceLocation& recipeId);

    /**
     * @brief 批量解锁。
     *
     * 跳过特殊配方与已解锁配方；新解锁的同时插入高亮集合。
     *
     * @param begin 配方 ID 迭代器起始
     * @param end 配方 ID 迭代器结束
     * @param onUnlock 每条成功解锁的回调（用于触发进度）
     * @return 本次真正解锁的条数（= 新插入 `known` 的条数）
     */
    template <typename RecipeIter>
    size_t add(RecipeIter begin, RecipeIter end, std::function<void(const ResourceLocation&)> onUnlock = nullptr)
    {
        size_t added = 0;
        for (auto it = begin; it != end; ++it) {
            const ResourceLocation& recipeId = *it;
            if (isSpecialRecipe(recipeId)) {
                continue;
            }
            if (m_known.contains(recipeId)) {
                continue;
            }
            add(recipeId);
            ++added;
            if (onUnlock) {
                onUnlock(recipeId);
            }
        }
        return added;
    }

    /**
     * @brief 批量锁定。
     * @return 本次真正移除的条数
     */
    template <typename RecipeIter>
    size_t remove(RecipeIter begin, RecipeIter end)
    {
        size_t removed = 0;
        for (auto it = begin; it != end; ++it) {
            if (m_known.contains(*it)) {
                remove(*it);
                ++removed;
            }
        }
        return removed;
    }

    // ========== 集合访问 ==========

    [[nodiscard]] const std::unordered_set<ResourceLocation>& knownRecipes() const noexcept
    {
        return m_known;
    }

    [[nodiscard]] const std::unordered_set<ResourceLocation>& highlightedRecipes() const noexcept
    {
        return m_highlight;
    }

    [[nodiscard]] size_t knownCount() const noexcept
    {
        return m_known.size();
    }

    [[nodiscard]] size_t highlightCount() const noexcept
    {
        return m_highlight.size();
    }

    /// 清空解锁集合与高亮（不动界面设置）。
    void clearRecipes() noexcept;

    // ========== 打包转移（重生 / 换维度） ==========

    /// `known` / `highlight` / 设置的快照（原版 `ServerRecipeBook.Packed`）。
    struct Packed {
        RecipeBookSettings settings;
        std::vector<ResourceLocation> known;
        std::vector<ResourceLocation> highlight;
    };

    /// 打包当前状态。
    [[nodiscard]] Packed pack() const;

    /// 用另一份打包状态整体覆盖本实例（原版 `copyOverData`）。
    void apply(const Packed& packed);

    // ========== 持久化 ==========

    /**
     * @brief 写入 `player.dat` 的 `recipeBook` 复合标签。
     *
     * 布局与原版 `ServerRecipeBook.Packed.CODEC` 一致：
     * 顶层直接展开四个类型的设置（`isGuiOpen` / `isFurnaceGuiOpen` / …），
     * 再加 `recipes`（`known`）与 `toBeDisplayed`（`highlight`）两个字符串列表。
     */
    [[nodiscard]] nbt::tags::compound_tag write() const;

    /**
     * @brief 从 `recipeBook` 复合标签读入。
     *
     * @param tag 复合标签
     * @param known 查询「该配方存不存在」的谓词；原版会丢弃无法识别的配方并记日志，
     *              这里由调用方决定（探针 / 游戏各有自己的注册表）。
     */
    void read(const nbt::tags::compound_tag& tag, const std::function<bool(const ResourceLocation&)>& known);

private:
    std::unordered_set<ResourceLocation> m_known;     ///< 已解锁的配方
    std::unordered_set<ResourceLocation> m_highlight; ///< 新解锁、尚未看过的配方
};

} // namespace crafting

} // namespace mc
