#pragma once

#include "../../../../../core/Types.hpp"
#include "../../../../../util/math/random/Random.hpp"
#include <memory>

namespace mc {

class BlockState;

namespace world {
namespace gen {
namespace feature {
namespace template_ {

/**
 * @brief 方块规则测试基类
 *
 * 参考 MC 1.16.5 RuleTest
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
 *
 * 参考 MC 1.16.5 AlwaysTrueTest
 */
class AlwaysTrueRuleTest : public RuleTest {
public:
    AlwaysTrueRuleTest() = default;

    [[nodiscard]] bool test(const BlockState* /*state*/, math::Random& /*rng*/) const override {
        return true;
    }

    [[nodiscard]] u32 getTypeId() const override { return 0; }
    [[nodiscard]] std::unique_ptr<RuleTest> clone() const override {
        return std::make_unique<AlwaysTrueRuleTest>();
    }
};

/**
 * @brief 匹配特定方块的规则测试
 *
 * 参考 MC 1.16.5 BlockMatchRuleTest
 */
class BlockMatchRuleTest : public RuleTest {
public:
    explicit BlockMatchRuleTest(u32 blockId);

    [[nodiscard]] bool test(const BlockState* state, math::Random& /*rng*/) const override;

    [[nodiscard]] u32 getTypeId() const override { return 1; }
    [[nodiscard]] std::unique_ptr<RuleTest> clone() const override {
        return std::make_unique<BlockMatchRuleTest>(m_blockId);
    }

    [[nodiscard]] u32 blockId() const { return m_blockId; }

private:
    u32 m_blockId;
};

/**
 * @brief 匹配特定方块状态的规则测试
 *
 * 参考 MC 1.16.5 BlockStateMatchRuleTest
 */
class BlockStateMatchRuleTest : public RuleTest {
public:
    explicit BlockStateMatchRuleTest(u32 stateId);

    [[nodiscard]] bool test(const BlockState* state, math::Random& /*rng*/) const override;

    [[nodiscard]] u32 getTypeId() const override { return 2; }
    [[nodiscard]] std::unique_ptr<RuleTest> clone() const override {
        return std::make_unique<BlockStateMatchRuleTest>(m_stateId);
    }

    [[nodiscard]] u32 stateId() const { return m_stateId; }

private:
    u32 m_stateId;
};

/**
 * @brief 随机匹配特定方块的规则测试
 *
 * 参考 MC 1.16.5 RandomBlockMatchRuleTest
 */
class RandomBlockMatchRuleTest : public RuleTest {
public:
    RandomBlockMatchRuleTest(u32 blockId, f32 probability);

    [[nodiscard]] bool test(const BlockState* state, math::Random& rng) const override;

    [[nodiscard]] u32 getTypeId() const override { return 3; }
    [[nodiscard]] std::unique_ptr<RuleTest> clone() const override {
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
 *
 * 参考 MC 1.16.5 RandomBlockStateMatchRuleTest
 */
class RandomBlockStateMatchRuleTest : public RuleTest {
public:
    RandomBlockStateMatchRuleTest(u32 stateId, f32 probability);

    [[nodiscard]] bool test(const BlockState* state, math::Random& rng) const override;

    [[nodiscard]] u32 getTypeId() const override { return 4; }
    [[nodiscard]] std::unique_ptr<RuleTest> clone() const override {
        return std::make_unique<RandomBlockStateMatchRuleTest>(m_stateId, m_probability);
    }

    [[nodiscard]] u32 stateId() const { return m_stateId; }
    [[nodiscard]] f32 probability() const { return m_probability; }

private:
    u32 m_stateId;
    f32 m_probability;
};

/**
 * @brief 位置规则测试基类
 *
 * 参考 MC 1.16.5 PosRuleTest
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
        const BlockPos& originalPos,
        const BlockPos& worldPos,
        const BlockPos& seedPos,
        math::Random& rng) const = 0;

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
 *
 * 参考 MC 1.16.5 AlwaysTruePosRuleTest
 */
class AlwaysTruePosRuleTest : public PosRuleTest {
public:
    AlwaysTruePosRuleTest() = default;

    [[nodiscard]] bool test(
        const BlockPos& /*originalPos*/,
        const BlockPos& /*worldPos*/,
        const BlockPos& /*seedPos*/,
        math::Random& /*rng*/) const override {
        return true;
    }

    [[nodiscard]] u32 getTypeId() const override { return 0; }
    [[nodiscard]] std::unique_ptr<PosRuleTest> clone() const override {
        return std::make_unique<AlwaysTruePosRuleTest>();
    }
};

/**
 * @brief 线性高度位置规则测试
 *
 * 参考 MC 1.16.5 LinearPosRuleTest
 * 根据Y坐标相对于最小/最大高度的线性插值决定概率
 */
class LinearPosRuleTest : public PosRuleTest {
public:
    LinearPosRuleTest(i32 minHeight, i32 maxHeight, f32 minProbability, f32 maxProbability);

    [[nodiscard]] bool test(
        const BlockPos& originalPos,
        const BlockPos& worldPos,
        const BlockPos& seedPos,
        math::Random& rng) const override;

    [[nodiscard]] u32 getTypeId() const override { return 1; }
    [[nodiscard]] std::unique_ptr<PosRuleTest> clone() const override {
        return std::make_unique<LinearPosRuleTest>(m_minHeight, m_maxHeight, m_minProbability, m_maxProbability);
    }

private:
    i32 m_minHeight;
    i32 m_maxHeight;
    f32 m_minProbability;
    f32 m_maxProbability;
};

/**
 * @brief 方块引用（用于规则输出）
 *
 * 可以表示一个方块状态或从输入方块转换
 */
struct RuleOutput {
    enum class Type : u8 {
        Fixed,      // 固定方块状态
        Input,      // 使用输入方块状态
        InputNbt    // 使用输入方块状态和NBT
    };

    Type type = Type::Fixed;
    u32 stateId = 0;  // 当 type == Fixed 时使用

    static RuleOutput fixed(u32 stateId) {
        return { Type::Fixed, stateId };
    }

    static RuleOutput input() {
        return { Type::Input, 0 };
    }

    static RuleOutput inputWithNbt() {
        return { Type::InputNbt, 0 };
    }
};

/**
 * @brief 规则条目
 *
 * 参考 MC 1.16.5 RuleEntry
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
     */
    RuleEntry(
        std::unique_ptr<RuleTest> inputPredicate,
        std::unique_ptr<RuleTest> locationPredicate,
        std::unique_ptr<PosRuleTest> posPredicate,
        u32 outputStateId);

    RuleEntry(
        std::unique_ptr<RuleTest> inputPredicate,
        std::unique_ptr<RuleTest> locationPredicate,
        u32 outputStateId);

    /**
     * @brief 测试是否匹配规则
     * @param inputState 输入方块状态（模板中的）
     * @param locationState 位置方块状态（世界中的）
     * @param originalPos 原始位置
     * @param worldPos 世界位置
     * @param seedPos 种子位置
     * @param rng 随机数生成器
     */
    [[nodiscard]] bool matches(
        const BlockState* inputState,
        const BlockState* locationState,
        const BlockPos& originalPos,
        const BlockPos& worldPos,
        const BlockPos& seedPos,
        math::Random& rng) const;

    [[nodiscard]] u32 outputStateId() const { return m_outputStateId; }
    [[nodiscard]] const RuleTest* inputPredicate() const { return m_inputPredicate.get(); }
    [[nodiscard]] const RuleTest* locationPredicate() const { return m_locationPredicate.get(); }
    [[nodiscard]] const PosRuleTest* posPredicate() const { return m_posPredicate.get(); }

private:
    std::unique_ptr<RuleTest> m_inputPredicate;
    std::unique_ptr<RuleTest> m_locationPredicate;
    std::unique_ptr<PosRuleTest> m_posPredicate;
    u32 m_outputStateId;
};

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
