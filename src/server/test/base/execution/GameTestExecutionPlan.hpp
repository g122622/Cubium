#pragma once

#include "common/core/Types.hpp"
#include "common/util/assert/AssertAll.hpp"
#include <algorithm>
#include <charconv>
#include <chrono>
#include <numeric>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace mc::test {

/** @brief 显式指定的零起始分片索引和分片总数。 */
struct GameTestShard {
    u32 index;
    u32 count;
};

/** @brief 已登记测试的稳定标识和声明执行成本。 */
struct GameTestWorkItem {
    std::string id;
    u64 ticks;
};

/** @brief 按声明成本均衡分片，并为长时测试提供有限的批次时间预算。 */
class GameTestExecutionPlan {
public:
    /** @brief 解析 index/count；空配置由调用方保持为未启用分片。 */
    [[nodiscard]] static std::optional<GameTestShard> parseShard(std::string_view specification)
    {
        const auto separator = specification.find('/');
        if (separator == std::string_view::npos) return std::nullopt;
        u32 index;
        u32 count;
        const auto first = std::from_chars(specification.data(), specification.data() + separator, index);
        const auto second =
            std::from_chars(specification.data() + separator + 1, specification.data() + specification.size(), count);
        if (first.ec != std::errc{} || first.ptr != specification.data() + separator || second.ec != std::errc{} ||
            second.ptr != specification.data() + specification.size() || count == 0 || count > 64 || index >= count) {
            return std::nullopt;
        }
        return GameTestShard{index, count};
    }

    /** @brief 每个测试恰分配一次；长时测试优先，标识排序保证登记顺序变化不影响分片。 */
    [[nodiscard]] static std::vector<usize> select(std::span<const GameTestWorkItem> items, GameTestShard shard)
    {
        MC_ASSERT_RELEASE(shard.count > 0 && shard.count <= 64 && shard.index < shard.count);
        std::vector<usize> order(items.size());
        std::iota(order.begin(), order.end(), 0);
        std::sort(order.begin(), order.end(), [&](usize left, usize right) {
            if (items[left].ticks != items[right].ticks) return items[left].ticks > items[right].ticks;
            return items[left].id < items[right].id;
        });
        std::vector<u64> costs(shard.count, 0);
        std::vector<usize> counts(shard.count, 0);
        std::vector<usize> selected;
        for (const auto item : order) {
            u32 destination = 0;
            for (u32 index = 1; index < shard.count; ++index) {
                if (costs[index] < costs[destination] ||
                    (costs[index] == costs[destination] && counts[index] < counts[destination])) {
                    destination = index;
                }
            }
            // 以单进程总期限截断估计成本，并计入每个隔离重跑的启动成本。
            costs[destination] += std::min<u64>(items[item].ticks, 17400) + 100;
            ++counts[destination];
            if (destination == shard.index) selected.push_back(item);
        }
        std::sort(selected.begin(), selected.end(), [&](usize left, usize right) {
            return items[left].id < items[right].id;
        });
        return selected;
    }

    /** @brief 按最低 10 TPS 加 60 秒准备预算计算；仍受整轮 30 分钟和单 tick 2 秒期限约束。 */
    [[nodiscard]] static std::chrono::milliseconds batchBudget(u64 declaredTicks)
    {
        // 先限幅再乘法，防止来自测试元数据的大数使 chrono 时长溢出。
        return std::chrono::milliseconds(60000 + std::min<u64>(declaredTicks, 17400) * 100);
    }
};

} // namespace mc::test
