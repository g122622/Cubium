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
#include "world/gen/feature/Feature.hpp"
#include <memory>
#include <optional>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

/**
 * @brief 位置规则测试类型枚举
 *
 * 仅 PosRuleTest 用到（RuleTest 基类已并入 mc::RuleTest 引用风格）。
 * 保留用于序列化和测试的类型识别。
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

// ============================================================================
// 方块规则测试
//
// 结构处理器的方块谓词统一复用 mc::RuleTest（引用风格 test(const BlockState&,
// Random&)），不再在此重复定义。下列别名供 template_ 命名空间内代码直接引用，
// 与 mc:: 同名（mc::AlwaysTrueRuleTest / mc::BlockMatchRuleTest 等）。
// ============================================================================

using ::mc::AlwaysTrueRuleTest;
using ::mc::BlockMatchRuleTest;
using ::mc::BlockStateMatchRuleTest;
using ::mc::RandomBlockMatchRuleTest;
using ::mc::RandomBlockStateMatchRuleTest;
using ::mc::RuleTest;
using ::mc::TagMatchRuleTest;

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
 * 定义一个完整的替换规则：条件 + 输出。
 * 方块谓词使用 mc::RuleTest（引用风格），位置谓词使用本命名空间的 PosRuleTest。
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
     *
     * 外部仍按指针传入（与 RuleStructureProcessor 调用点一致），内部对空指针做守卫：
     * 方块状态为空时该谓词视为不匹配。生产路径（结构处理器）方块状态均来自已注册
     * 方块，不会为空；指针签名仅为兼容既有调用点而保留。
     *
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
