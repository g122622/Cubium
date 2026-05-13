#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "common/entity/loot/StatePropertiesPredicate.hpp"
#include "../../MinMaxBounds.hpp"
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

// 前向声明
namespace mc {
    class BlockState;
}

namespace mc::advancement {

/**
 * @brief 方块谓词
 *
 * 用于匹配方块的条件谓词，检查方块类型、标签、状态等。
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.BlockPredicate
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
        std::optional<ResourceLocation> block,
        std::optional<ResourceLocation> tag,
        StatePropertiesPredicate state
    );

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
    std::optional<ResourceLocation> m_block;           ///< 方块ID
    std::optional<ResourceLocation> m_tag;             ///< 方块标签
    StatePropertiesPredicate m_state;                  ///< 状态属性谓词
    bool m_isAny = true;                               ///< 是否匹配任意方块
};

/**
 * @brief 流体谓词
 *
 * 用于匹配流体的条件谓词。
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.FluidPredicate
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
    std::optional<ResourceLocation> m_fluid;  ///< 流体ID
    StatePropertiesPredicate m_state;         ///< 状态属性谓词
    bool m_isAny = true;
};

} // namespace mc::advancement
