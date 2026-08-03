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

#include "common/advancement/MinMaxBounds.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/item/loot/StatePropertiesPredicate.hpp"
#include "common/resource/ResourceLocation.hpp"
#include <optional>
#include <string>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

// 前向声明
namespace mc {
class BlockState;
}

namespace mc::advancement {

/**
 * @brief 方块谓词
 *
 * 用于匹配方块的条件谓词，检查方块类型、标签、状态等。
 */
class BlockPredicate {
public:
    /**
     * @brief 默认构造（匹配任意方块）
     */
    BlockPredicate() = default;

    /**
     * @brief 构造方块谓词
     *
     * @param block 方块ID（可选）
     * @param tag 方块标签（可选）
     * @param state 状态属性谓词
     */
    BlockPredicate(
        std::optional<ResourceLocation> block, std::optional<ResourceLocation> tag, StatePropertiesPredicate state);

    /**
     * @brief 检查方块是否匹配
     * @param state 方块状态
     * @return 是否匹配
     */
    [[nodiscard]] bool test(const BlockState& state) const;

    /**
     * @brief 检查是否匹配任意方块
     */
    [[nodiscard]] bool isAny() const noexcept { return m_isAny; }

    /**
     * @brief 从JSON解析
     */
    static Result<BlockPredicate> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化为JSON
     */
    [[nodiscard]] nlohmann::json toJson() const;

    // ========== Getters ==========

    [[nodiscard]] const std::optional<ResourceLocation>& getBlock() const noexcept { return m_block; }
    [[nodiscard]] const std::optional<ResourceLocation>& getTag() const noexcept { return m_tag; }
    [[nodiscard]] const StatePropertiesPredicate& getState() const noexcept { return m_state; }

private:
    std::optional<ResourceLocation> m_block; ///< 方块ID
    std::optional<ResourceLocation> m_tag;   ///< 方块标签
    StatePropertiesPredicate m_state;        ///< 状态属性谓词
    bool m_isAny = true;                     ///< 是否匹配任意方块
};

/**
 * @brief 流体谓词
 *
 * 用于匹配流体的条件谓词。
 */
class FluidPredicate {
public:
    /**
     * @brief 默认构造（匹配任意流体）
     */
    FluidPredicate() = default;

    /**
     * @brief 检查流体是否匹配
     */
    [[nodiscard]] bool test(const BlockState& state) const;

    /**
     * @brief 检查是否匹配任意流体
     */
    [[nodiscard]] bool isAny() const noexcept { return m_isAny; }

    /**
     * @brief 从JSON解析
     */
    static Result<FluidPredicate> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化为JSON
     */
    [[nodiscard]] nlohmann::json toJson() const;

private:
    std::optional<ResourceLocation> m_fluid; ///< 流体ID
    StatePropertiesPredicate m_state;        ///< 状态属性谓词
    bool m_isAny = true;
};

} // namespace mc::advancement
