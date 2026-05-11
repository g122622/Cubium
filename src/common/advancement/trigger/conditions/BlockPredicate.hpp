#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "common/resource/ResourceLocation.hpp"
#include "../../MinMaxBounds.hpp"
#include <nlohmann/json.hpp>
#include <optional>
#include <string>

// 前向声明
namespace mc {
    struct BlockState;
    class Block;
}

namespace mc::advancement {

/**
 * @brief 方块状态谓词
 *
 * 用于匹配方块状态属性的条件谓词。
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.StatePropertiesPredicate
 */
class StatePropertiesPredicate {
public:
    /**
     * @brief 检查方块状态是否匹配
     */
    [[nodiscard]] bool test(const BlockState& state) const;

    /**
     * @brief 检查是否匹配任意状态
     */
    [[nodiscard]] bool isAny() const noexcept { return m_isAny; }

    /**
     * @brief 从JSON解析
     */
    static Result<StatePropertiesPredicate> fromJson(const nlohmann::json& json);

    /**
     * @brief 序列化为JSON
     */
    [[nodiscard]] nlohmann::json toJson() const;

private:
    // TODO: 属性匹配（如 facing=north, powered=true 等）
    bool m_isAny = true;
};

/**
 * @brief 方块谓词
 *
 * 用于匹配方块的条件谓词，检查方块类型、状态等。
 * 参考 MC 1.16.5: net.minecraft.advancements.criterion.BlockPredicate
 */
class BlockPredicate {
public:
    /**
     * @brief 默认构造（匹配任意方块）
     */
    BlockPredicate() = default;

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

private:
    std::optional<ResourceLocation> m_block;           ///< 方块ID
    std::optional<ResourceLocation> m_tag;             ///< 方块标签
    StatePropertiesPredicate m_state;                  ///< 状态谓词
    // TODO: NBT匹配
    bool m_isAny = true;
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
    // TODO: 状态匹配
    bool m_isAny = true;
};

} // namespace mc::advancement
