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
#include "util/Direction.hpp"
#include "util/math/random/Random.hpp"
#include "util/nbt/Nbt.hpp"
#include "world/block/BlockPos.hpp"
#include <memory>
#include <optional>

namespace mc {

class BlockState;

namespace world {
namespace gen {
namespace feature {
namespace template_ {

/**
 * @brief 规则测试类型枚举
 *
 * 用于序列化和类型识别
 */
enum class RuleTestType : u32 {
    AlwaysTrue = 0,
    BlockMatch = 1,
    BlockStateMatch = 2,
    RandomBlockMatch = 3,
    RandomBlockStateMatch = 4,
    TagMatch = 5,
    // PosRuleTest 类型
    AlwaysTruePos = 0,
    LinearPos = 1,
    AxisAlignedLinearPos = 2
};

/**
 * @brief 方块规则测试基类
 *
 * 用于测试方块是否匹配特定条件
 */
class RuleTest {
public:
    virtual ~RuleTest() = default;

    /**
     * @brief 测试方块状态是否匹配
     * @param state 方块状态
     * @param rng 随机数生成器
     * @return 是否匹配
     */
    [[nodiscard]] virtual bool test(const BlockState* state, math::Random& rng) const = 0;

    /**
     * @brief 获取测试类型ID
     */
    [[nodiscard]] virtual u32 getTypeId() const = 0;

    /**
     * @brief 克隆测试
     */
    [[nodiscard]] virtual std::unique_ptr<RuleTest> clone() const = 0;
};

/**
 * @brief 总是返回 true 的规则测试
 */
class AlwaysTrueRuleTest : public RuleTest {
public:
    AlwaysTrueRuleTest() = default;

    [[nodiscard]] bool test(const BlockState* /*state*/, math::Random& /*rng*/) const override { return true; }

    [[nodiscard]] u32 getTypeId() const override { return static_cast<u32>(RuleTestType::AlwaysTrue); }
    [[nodiscard]] std::unique_ptr<RuleTest> clone() const override { return std::make_unique<AlwaysTrueRuleTest>(); }
};

/**
 * @brief 匹配特定方块的规则测试
 */
class BlockMatchRuleTest : public RuleTest {
public:
    explicit BlockMatchRuleTest(u32 blockId);

    [[nodiscard]] bool test(const BlockState* state, math::Random& /*rng*/) const override;

    [[nodiscard]] u32 getTypeId() const override { return static_cast<u32>(RuleTestType::BlockMatch); }
    [[nodiscard]] std::unique_ptr<RuleTest> clone() const override
    {
        return std::make_unique<BlockMatchRuleTest>(m_blockId);
    }

    [[nodiscard]] u32 blockId() const { return m_blockId; }

private:
    u32 m_blockId;
};

/**
 * @brief 匹配特定方块状态的规则测试
 */
class BlockStateMatchRuleTest : public RuleTest {
public:
    explicit BlockStateMatchRuleTest(u32 stateId);

    [[nodiscard]] bool test(const BlockState* state, math::Random& /*rng*/) const override;

    [[nodiscard]] u32 getTypeId() const override { return static_cast<u32>(RuleTestType::BlockStateMatch); }
    [[nodiscard]] std::unique_ptr<RuleTest> clone() const override
    {
        return std::make_unique<BlockStateMatchRuleTest>(m_stateId);
    }

    [[nodiscard]] u32 stateId() const { return m_stateId; }

private:
    u32 m_stateId;
};

/**
 * @brief 随机匹配特定方块的规则测试
 */
class RandomBlockMatchRuleTest : public RuleTest {
public:
    RandomBlockMatchRuleTest(u32 blockId, f32 probability);

    [[nodiscard]] bool test(const BlockState* state, math::Random& rng) const override;

    [[nodiscard]] u32 getTypeId() const override { return static_cast<u32>(RuleTestType::RandomBlockMatch); }
    [[nodiscard]] std::unique_ptr<RuleTest> clone() const override
    {
        return std::make_unique<RandomBlockMatchRuleTest>(m_blockId, m_probability);
    }

    [[nodiscard]] u32 blockId() const { return m_blockId; }
    [[nodiscard]] f32 probability() const { return m_probability; }

private:
    u32 m_blockId;
    f32 m_probability;
};

/**
 * @brief 随机匹配特定方块状态的规则测试
 */
class RandomBlockStateMatchRuleTest : public RuleTest {
public:
    RandomBlockStateMatchRuleTest(u32 stateId, f32 probability);

    [[nodiscard]] bool test(const BlockState* state, math::Random& rng) const override;

    [[nodiscard]] u32 getTypeId() const override { return static_cast<u32>(RuleTestType::RandomBlockStateMatch); }
    [[nodiscard]] std::unique_ptr<RuleTest> clone() const override
    {
        return std::make_unique<RandomBlockStateMatchRuleTest>(m_stateId, m_probability);
    }

    [[nodiscard]] u32 stateId() const { return m_stateId; }
    [[nodiscard]] f32 probability() const { return m_probability; }

private:
    u32 m_stateId;
    f32 m_probability;
};

/**
 * @brief 方块标签匹配规则测试
 *
 * 检查方块是否属于指定标签
 */
class TagMatchRuleTest : public RuleTest {
public:
    /**
     * @brief 构造标签匹配测试
     * @param tagId 标签资源位置（如 "minecraft:logs"）
     */
    explicit TagMatchRuleTest(const ResourceLocation& tagId);

    [[nodiscard]] bool test(const BlockState* state, math::Random& /*rng*/) const override;

    [[nodiscard]] u32 getTypeId() const override { return static_cast<u32>(RuleTestType::TagMatch); }
    [[nodiscard]] std::unique_ptr<RuleTest> clone() const override
    {
        return std::make_unique<TagMatchRuleTest>(m_tagId);
    }

    [[nodiscard]] const ResourceLocation& tagId() const { return m_tagId; }

private:
    ResourceLocation m_tagId;
};

/**
 * @brief 位置规则测试基类
 */
class PosRuleTest {
public:
    virtual ~PosRuleTest() = default;

    /**
     * @brief 测试位置是否匹配
     * @param originalPos 原始位置（模板内）
     * @param worldPos 世界位置
     * @param seedPos 种子位置（结构起点）
     * @param rng 随机数生成器
     * @return 是否匹配
     */
    [[nodiscard]] virtual bool test(
        const BlockPos& originalPos, const BlockPos& worldPos, const BlockPos& seedPos, math::Random& rng) const = 0;

    /**
     * @brief 获取测试类型ID
     */
    [[nodiscard]] virtual u32 getTypeId() const = 0;

    /**
     * @brief 克隆测试
     */
    [[nodiscard]] virtual std::unique_ptr<PosRuleTest> clone() const = 0;
};

/**
 * @brief 总是返回 true 的位置规则测试
 */
class AlwaysTruePosRuleTest : public PosRuleTest {
public:
    AlwaysTruePosRuleTest() = default;

    [[nodiscard]] bool test(const BlockPos& /*originalPos*/,
        const BlockPos& /*worldPos*/,
        const BlockPos& /*seedPos*/,
        math::Random& /*rng*/) const override
    {
        return true;
    }

    [[nodiscard]] u32 getTypeId() const override { return static_cast<u32>(RuleTestType::AlwaysTruePos); }
    [[nodiscard]] std::unique_ptr<PosRuleTest> clone() const override
    {
        return std::make_unique<AlwaysTruePosRuleTest>();
    }
};

/**
 * @brief 线性距离位置规则测试
 *
 * 根据 worldPos 到 seedPos 的曼哈顿距离线性插值概率
 */
class LinearPosRuleTest : public PosRuleTest {
public:
    /**
     * @brief 构造线性位置测试
     * @param minDistance 最小距离
     * @param maxDistance 最大距离
     * @param minProbability 最小概率
     * @param maxProbability 最大概率
     */
    LinearPosRuleTest(i32 minDistance, i32 maxDistance, f32 minProbability, f32 maxProbability);

    [[nodiscard]] bool test(const BlockPos& originalPos,
        const BlockPos& worldPos,
        const BlockPos& seedPos,
        math::Random& rng) const override;

    [[nodiscard]] u32 getTypeId() const override { return static_cast<u32>(RuleTestType::LinearPos); }
    [[nodiscard]] std::unique_ptr<PosRuleTest> clone() const override
    {
        return std::make_unique<LinearPosRuleTest>(m_minDistance, m_maxDistance, m_minProbability, m_maxProbability);
    }

private:
    i32 m_minDistance;
    i32 m_maxDistance;
    f32 m_minProbability;
    f32 m_maxProbability;
};

/**
 * @brief 轴对齐线性位置规则测试
 *
 * 根据指定轴方向上的距离线性插值概率
 */
class AxisAlignedLinearPosTest : public PosRuleTest {
public:
    /**
     * @brief 构造轴对齐线性位置测试
     * @param minProbability 最小概率
     * @param maxProbability 最大概率
     * @param minDistance 最小距离
     * @param maxDistance 最大距离
     * @param axis 轴向（X, Y, 或 Z）
     */
    AxisAlignedLinearPosTest(f32 minProbability, f32 maxProbability, i32 minDistance, i32 maxDistance, Axis axis);

    [[nodiscard]] bool test(const BlockPos& originalPos,
        const BlockPos& worldPos,
        const BlockPos& seedPos,
        math::Random& rng) const override;

    [[nodiscard]] u32 getTypeId() const override { return static_cast<u32>(RuleTestType::AxisAlignedLinearPos); }
    [[nodiscard]] std::unique_ptr<PosRuleTest> clone() const override
    {
        return std::make_unique<AxisAlignedLinearPosTest>(
            m_minProbability, m_maxProbability, m_minDistance, m_maxDistance, m_axis);
    }

private:
    f32 m_minProbability;
    f32 m_maxProbability;
    i32 m_minDistance;
    i32 m_maxDistance;
    Axis m_axis;
};

/**
 * @brief 规则条目
 *
 * 定义一个完整的替换规则：条件 + 输出
 */
class RuleEntry {
public:
    /**
     * @brief 构造规则条目
     * @param inputPredicate 输入方块测试（测试模板中的方块）
     * @param locationPredicate 位置方块测试（测试世界中的方块）
     * @param posPredicate 位置测试（可选，默认 AlwaysTrue）
     * @param outputStateId 输出方块状态ID
     * @param outputNbt 输出NBT数据（可选，用于方块实体）
     */
    RuleEntry(std::unique_ptr<RuleTest> inputPredicate,
        std::unique_ptr<RuleTest> locationPredicate,
        std::unique_ptr<PosRuleTest> posPredicate,
        u32 outputStateId,
        std::optional<nbt::tags::compound_tag> outputNbt = std::nullopt);

    RuleEntry(std::unique_ptr<RuleTest> inputPredicate, std::unique_ptr<RuleTest> locationPredicate, u32 outputStateId);

    /**
     * @brief 测试是否匹配规则
     * @param inputState 输入方块状态（模板中的）
     * @param locationState 位置方块状态（世界中的）
     * @param originalPos 原始位置
     * @param worldPos 世界位置
     * @param seedPos 种子位置
     * @param rng 随机数生成器
     */
    [[nodiscard]] bool matches(const BlockState* inputState,
        const BlockState* locationState,
        const BlockPos& originalPos,
        const BlockPos& worldPos,
        const BlockPos& seedPos,
        math::Random& rng) const;

    [[nodiscard]] u32 outputStateId() const { return m_outputStateId; }
    [[nodiscard]] const RuleTest* inputPredicate() const { return m_inputPredicate.get(); }
    [[nodiscard]] const RuleTest* locationPredicate() const { return m_locationPredicate.get(); }
    [[nodiscard]] const PosRuleTest* posPredicate() const { return m_posPredicate.get(); }
    [[nodiscard]] const std::optional<nbt::tags::compound_tag>& outputNbt() const { return m_outputNbt; }

    /**
     * @brief 深拷贝规则条目
     */
    [[nodiscard]] std::unique_ptr<RuleEntry> clone() const;

private:
    std::unique_ptr<RuleTest> m_inputPredicate;
    std::unique_ptr<RuleTest> m_locationPredicate;
    std::unique_ptr<PosRuleTest> m_posPredicate;
    u32 m_outputStateId;
    std::optional<nbt::tags::compound_tag> m_outputNbt;
};

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
