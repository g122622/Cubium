#include "common/util/math/random/BlockPositionRandom.hpp"
#include <array>
#include <limits>
#include <gtest/gtest.h>

namespace mc {
namespace {

TEST(BlockPositionRandomTest, PackedPositionSequenceMatchesJavaIntWrapping)
{
    struct Case {
        i32 seed;
        std::array<i32, 8> expected;
    };
    // 由独立 Java int LCG 程序生成；覆盖正常值、符号位和乘加溢出。
    const std::array<Case, 4> cases = {
        {{0, {253476055, -59837601, 73963252, 475365814, -467910151, -76512571, 23938342, 325291084}},
            {-1, {253476055, -59837604, 73963246, 475365793, -467910211, -76512754, 23937796, 325289443}},
            {std::numeric_limits<i32>::min(),
                {-283394857, 477033311, -462907660, -61505098, 68960761, 460358341, -512932570, -211579828}},
            {std::numeric_limits<i32>::max(),
                {-283394857, 477033308, -462907666, -61505119, 68960701, 460358158, -512933116, -211581469}}}};
    for (const auto& sample : cases) {
        math::BlockPositionRandom random(static_cast<u32>(sample.seed));
        for (const auto expected : sample.expected) {
            EXPECT_EQ(random.nextPositionBits(), expected);
        }
    }
}

} // namespace
} // namespace mc
