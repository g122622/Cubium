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
 */

#include "ChunkPyramid.hpp"
#include "ChunkStatus.hpp"
#include <algorithm>
#include <array>

namespace mc {

namespace {

/**
 * @brief 构建累积依赖
 *
 * 将前序步骤的累积依赖与当前步骤的直接依赖合并，
 * 对每个半径取两者中较高的状态。
 *
 * @param directDeps 当前步骤的直接依赖
 * @param parentAccumulatedDeps 父步骤的累积依赖（nullptr 表示无父步骤）
 * @param parentTargetStatus 父步骤的目标状态
 * @return 累积依赖列表
 */
std::vector<const ChunkStatus*> buildAccumulatedDependencies(const std::vector<const ChunkStatus*>& directDeps,
    const ChunkDependencies* parentAccumulatedDeps,
    const ChunkStatus* parentTargetStatus)
{
    if (parentAccumulatedDeps == nullptr) {
        // 第一步（EMPTY）的累积依赖 = 直接依赖
        return directDeps;
    }

    // 计算父状态在直接依赖中的半径
    // 计算父状态在直接依赖中的半径
    i32 radiusOfParent = 0;
    for (i32 i = static_cast<i32>(directDeps.size()) - 1; i >= 0; --i) {
        if (directDeps[static_cast<size_t>(i)] != nullptr &&
            directDeps[static_cast<size_t>(i)]->isAtLeast(*parentTargetStatus)) {
            radiusOfParent = i;
            break;
        }
    }

    const auto& parentAccList = parentAccumulatedDeps->asList();
    const i32 parentAccSize = static_cast<i32>(parentAccList.size());
    const i32 resultSize = std::max(radiusOfParent + parentAccSize, static_cast<i32>(directDeps.size()));

    std::vector<const ChunkStatus*> result(static_cast<size_t>(resultSize), nullptr);

    for (i32 j = 0; j < resultSize; ++j) {
        const i32 k = j - radiusOfParent; // parentAcc 中的偏移索引
        if (k < 0 || k >= parentAccSize) {
            // 超出父累积依赖范围，使用直接依赖
            result[static_cast<size_t>(j)] =
                (j < static_cast<i32>(directDeps.size())) ? directDeps[static_cast<size_t>(j)] : nullptr;
        } else if (j >= static_cast<i32>(directDeps.size())) {
            // 超出直接依赖范围，使用父累积依赖
            result[static_cast<size_t>(j)] = parentAccList[static_cast<size_t>(k)];
        } else {
            // 两者都有值，取较高的状态
            const ChunkStatus* direct = directDeps[static_cast<size_t>(j)];
            const ChunkStatus* parentAcc = parentAccList[static_cast<size_t>(k)];
            if (direct == nullptr) {
                result[static_cast<size_t>(j)] = parentAcc;
            } else if (parentAcc == nullptr) {
                result[static_cast<size_t>(j)] = direct;
            } else {
                result[static_cast<size_t>(j)] = (direct->isAtLeast(*parentAcc)) ? direct : parentAcc;
            }
        }
    }

    return result;
}

/**
 * @brief 向直接依赖列表添加需求
 *
 * 扩展列表到 radius+1 长度，并将 [0..radius] 范围内每个位置
 * 设置为 max(existing, status)。
 *
 * @param deps 当前直接依赖列表
 * @param status 要求的 ChunkStatus
 * @param radius 要求的半径
 */
void addRequirement(std::vector<const ChunkStatus*>& deps, const ChunkStatus* status, i32 radius)
{
    if (status == nullptr || radius < 0) {
        return;
    }

    const i32 requiredSize = radius + 1;
    if (requiredSize > static_cast<i32>(deps.size())) {
        deps.resize(static_cast<size_t>(requiredSize), status);
    }

    for (i32 j = 0; j < std::min(requiredSize, static_cast<i32>(deps.size())); ++j) {
        if (deps[static_cast<size_t>(j)] == nullptr) {
            deps[static_cast<size_t>(j)] = status;
        } else if (status->isAtLeast(*deps[static_cast<size_t>(j)])) {
            deps[static_cast<size_t>(j)] = status;
        }
    }
}

} // anonymous namespace

const ChunkPyramid& ChunkPyramid::generationPyramid()
{
    static const ChunkPyramid pyramid = []() {
        std::vector<ChunkStep> steps;
        steps.reserve(12);

        // === EMPTY ===
        // 无直接依赖，无累积依赖
        {
            std::vector<const ChunkStatus*> directDeps;
            auto accumulated = buildAccumulatedDependencies(directDeps, nullptr, nullptr);
            steps.emplace_back(&ChunkStatuses::EMPTY,
                ChunkDependencies(std::vector<const ChunkStatus*>(directDeps)),
                ChunkDependencies(std::move(accumulated)),
                -1); // blockStateWriteRadius = -1
        }

        // === STRUCTURE_STARTS ===
        // 直接依赖 = [EMPTY]（继承自父步骤）
        {
            const ChunkStep& parent = steps.back();
            std::vector<const ChunkStatus*> directDeps = {parent.targetStatus()};
            auto accumulated =
                buildAccumulatedDependencies(directDeps, &parent.accumulatedDependencies(), parent.targetStatus());
            steps.emplace_back(&ChunkStatuses::STRUCTURE_STARTS,
                ChunkDependencies(std::vector<const ChunkStatus*>(directDeps)),
                ChunkDependencies(std::move(accumulated)),
                -1);
        }

        // === STRUCTURE_REFERENCES ===
        // directDeps = [STRUCTURE_STARTS] * 9 (radius 0-8)
        {
            const ChunkStep& parent = steps.back();
            std::vector<const ChunkStatus*> directDeps = {parent.targetStatus()};
            addRequirement(directDeps, &ChunkStatuses::STRUCTURE_STARTS, 8);
            auto accumulated =
                buildAccumulatedDependencies(directDeps, &parent.accumulatedDependencies(), parent.targetStatus());
            steps.emplace_back(&ChunkStatuses::STRUCTURE_REFERENCES,
                ChunkDependencies(std::vector<const ChunkStatus*>(directDeps)),
                ChunkDependencies(std::move(accumulated)),
                -1);
        }

        // === BIOMES ===
        // directDeps = [STRUCTURE_STARTS] * 9
        {
            const ChunkStep& parent = steps.back();
            std::vector<const ChunkStatus*> directDeps = {parent.targetStatus()};
            addRequirement(directDeps, &ChunkStatuses::STRUCTURE_STARTS, 8);
            auto accumulated =
                buildAccumulatedDependencies(directDeps, &parent.accumulatedDependencies(), parent.targetStatus());
            steps.emplace_back(&ChunkStatuses::BIOMES,
                ChunkDependencies(std::vector<const ChunkStatus*>(directDeps)),
                ChunkDependencies(std::move(accumulated)),
                -1);
        }

        // === NOISE ===
        // directDeps = [BIOMES, STRUCTURE_STARTS, STRUCTURE_STARTS, ..., STRUCTURE_STARTS]
        //   index 0: BIOMES, index 1-8: max(BIOMES, STRUCTURE_STARTS) = STRUCTURE_STARTS
        {
            const ChunkStep& parent = steps.back();
            std::vector<const ChunkStatus*> directDeps = {parent.targetStatus()};
            addRequirement(directDeps, &ChunkStatuses::STRUCTURE_STARTS, 8);
            addRequirement(directDeps, &ChunkStatuses::BIOMES, 1);
            auto accumulated =
                buildAccumulatedDependencies(directDeps, &parent.accumulatedDependencies(), parent.targetStatus());
            steps.emplace_back(&ChunkStatuses::NOISE,
                ChunkDependencies(std::vector<const ChunkStatus*>(directDeps)),
                ChunkDependencies(std::move(accumulated)),
                0);
        }

        // === SURFACE ===
        // 同 NOISE
        {
            const ChunkStep& parent = steps.back();
            std::vector<const ChunkStatus*> directDeps = {parent.targetStatus()};
            addRequirement(directDeps, &ChunkStatuses::STRUCTURE_STARTS, 8);
            addRequirement(directDeps, &ChunkStatuses::BIOMES, 1);
            auto accumulated =
                buildAccumulatedDependencies(directDeps, &parent.accumulatedDependencies(), parent.targetStatus());
            steps.emplace_back(&ChunkStatuses::SURFACE,
                ChunkDependencies(std::vector<const ChunkStatus*>(directDeps)),
                ChunkDependencies(std::move(accumulated)),
                0);
        }

        // === CARVERS ===
        // directDeps = [SURFACE, STRUCTURE_STARTS, STRUCTURE_STARTS, ..., STRUCTURE_STARTS]
        {
            const ChunkStep& parent = steps.back();
            std::vector<const ChunkStatus*> directDeps = {parent.targetStatus()};
            addRequirement(directDeps, &ChunkStatuses::STRUCTURE_STARTS, 8);
            auto accumulated =
                buildAccumulatedDependencies(directDeps, &parent.accumulatedDependencies(), parent.targetStatus());
            steps.emplace_back(&ChunkStatuses::CARVERS,
                ChunkDependencies(std::vector<const ChunkStatus*>(directDeps)),
                ChunkDependencies(std::move(accumulated)),
                0);
        }

        // === FEATURES ===
        // directDeps = [CARVERS, STRUCTURE_STARTS, STRUCTURE_STARTS, ..., STRUCTURE_STARTS]
        {
            const ChunkStep& parent = steps.back();
            std::vector<const ChunkStatus*> directDeps = {parent.targetStatus()};
            addRequirement(directDeps, &ChunkStatuses::STRUCTURE_STARTS, 8);
            addRequirement(directDeps, &ChunkStatuses::CARVERS, 1);
            auto accumulated =
                buildAccumulatedDependencies(directDeps, &parent.accumulatedDependencies(), parent.targetStatus());
            steps.emplace_back(&ChunkStatuses::FEATURES,
                ChunkDependencies(std::vector<const ChunkStatus*>(directDeps)),
                ChunkDependencies(std::move(accumulated)),
                1);
        }

        // === INITIALIZE_LIGHT ===
        // 仅前一步
        {
            const ChunkStep& parent = steps.back();
            std::vector<const ChunkStatus*> directDeps = {parent.targetStatus()};
            auto accumulated =
                buildAccumulatedDependencies(directDeps, &parent.accumulatedDependencies(), parent.targetStatus());
            steps.emplace_back(&ChunkStatuses::INITIALIZE_LIGHT,
                ChunkDependencies(std::vector<const ChunkStatus*>(directDeps)),
                ChunkDependencies(std::move(accumulated)),
                -1);
        }

        // === LIGHT ===
        // directDeps = [INITIALIZE_LIGHT, INITIALIZE_LIGHT]
        {
            const ChunkStep& parent = steps.back();
            std::vector<const ChunkStatus*> directDeps = {parent.targetStatus()};
            addRequirement(directDeps, &ChunkStatuses::INITIALIZE_LIGHT, 1);
            auto accumulated =
                buildAccumulatedDependencies(directDeps, &parent.accumulatedDependencies(), parent.targetStatus());
            steps.emplace_back(&ChunkStatuses::LIGHT,
                ChunkDependencies(std::vector<const ChunkStatus*>(directDeps)),
                ChunkDependencies(std::move(accumulated)),
                -1);
        }

        // === SPAWN ===
        // directDeps = [LIGHT, BIOMES]
        {
            const ChunkStep& parent = steps.back();
            std::vector<const ChunkStatus*> directDeps = {parent.targetStatus()};
            addRequirement(directDeps, &ChunkStatuses::BIOMES, 1);
            auto accumulated =
                buildAccumulatedDependencies(directDeps, &parent.accumulatedDependencies(), parent.targetStatus());
            steps.emplace_back(&ChunkStatuses::SPAWN,
                ChunkDependencies(std::vector<const ChunkStatus*>(directDeps)),
                ChunkDependencies(std::move(accumulated)),
                -1);
        }

        // === FULL ===
        // 仅前一步
        {
            const ChunkStep& parent = steps.back();
            std::vector<const ChunkStatus*> directDeps = {parent.targetStatus()};
            auto accumulated =
                buildAccumulatedDependencies(directDeps, &parent.accumulatedDependencies(), parent.targetStatus());
            steps.emplace_back(&ChunkStatuses::FULL,
                ChunkDependencies(std::vector<const ChunkStatus*>(directDeps)),
                ChunkDependencies(std::move(accumulated)),
                -1);
        }

        return ChunkPyramid(std::move(steps));
    }();

    return pyramid;
}

const ChunkPyramid& ChunkPyramid::loadingPyramid()
{
    static const ChunkPyramid pyramid = []() {
        std::vector<ChunkStep> steps;
        steps.reserve(12);

        // === EMPTY ===
        // 加载路径：从存档加载数据，无直接依赖
        {
            std::vector<const ChunkStatus*> directDeps;
            auto accumulated = buildAccumulatedDependencies(directDeps, nullptr, nullptr);
            steps.emplace_back(&ChunkStatuses::EMPTY,
                ChunkDependencies(std::vector<const ChunkStatus*>(directDeps)),
                ChunkDependencies(std::move(accumulated)),
                -1);
        }

        // === STRUCTURE_STARTS ===
        // 加载路径：从存档恢复结构起点，仅依赖前一步
        {
            const ChunkStep& parent = steps.back();
            std::vector<const ChunkStatus*> directDeps = {parent.targetStatus()};
            auto accumulated =
                buildAccumulatedDependencies(directDeps, &parent.accumulatedDependencies(), parent.targetStatus());
            steps.emplace_back(&ChunkStatuses::STRUCTURE_STARTS,
                ChunkDependencies(std::vector<const ChunkStatus*>(directDeps)),
                ChunkDependencies(std::move(accumulated)),
                -1);
        }

        // === STRUCTURE_REFERENCES ===
        // 加载路径：空操作，仅依赖前一步
        {
            const ChunkStep& parent = steps.back();
            std::vector<const ChunkStatus*> directDeps = {parent.targetStatus()};
            auto accumulated =
                buildAccumulatedDependencies(directDeps, &parent.accumulatedDependencies(), parent.targetStatus());
            steps.emplace_back(&ChunkStatuses::STRUCTURE_REFERENCES,
                ChunkDependencies(std::vector<const ChunkStatus*>(directDeps)),
                ChunkDependencies(std::move(accumulated)),
                -1);
        }

        // === BIOMES ===
        // 加载路径：空操作，仅依赖前一步
        {
            const ChunkStep& parent = steps.back();
            std::vector<const ChunkStatus*> directDeps = {parent.targetStatus()};
            auto accumulated =
                buildAccumulatedDependencies(directDeps, &parent.accumulatedDependencies(), parent.targetStatus());
            steps.emplace_back(&ChunkStatuses::BIOMES,
                ChunkDependencies(std::vector<const ChunkStatus*>(directDeps)),
                ChunkDependencies(std::move(accumulated)),
                -1);
        }

        // === NOISE ===
        // 加载路径：空操作，仅依赖前一步
        {
            const ChunkStep& parent = steps.back();
            std::vector<const ChunkStatus*> directDeps = {parent.targetStatus()};
            auto accumulated =
                buildAccumulatedDependencies(directDeps, &parent.accumulatedDependencies(), parent.targetStatus());
            steps.emplace_back(&ChunkStatuses::NOISE,
                ChunkDependencies(std::vector<const ChunkStatus*>(directDeps)),
                ChunkDependencies(std::move(accumulated)),
                -1);
        }

        // === SURFACE ===
        // 加载路径：空操作，仅依赖前一步
        {
            const ChunkStep& parent = steps.back();
            std::vector<const ChunkStatus*> directDeps = {parent.targetStatus()};
            auto accumulated =
                buildAccumulatedDependencies(directDeps, &parent.accumulatedDependencies(), parent.targetStatus());
            steps.emplace_back(&ChunkStatuses::SURFACE,
                ChunkDependencies(std::vector<const ChunkStatus*>(directDeps)),
                ChunkDependencies(std::move(accumulated)),
                -1);
        }

        // === CARVERS ===
        // 加载路径：空操作，仅依赖前一步
        {
            const ChunkStep& parent = steps.back();
            std::vector<const ChunkStatus*> directDeps = {parent.targetStatus()};
            auto accumulated =
                buildAccumulatedDependencies(directDeps, &parent.accumulatedDependencies(), parent.targetStatus());
            steps.emplace_back(&ChunkStatuses::CARVERS,
                ChunkDependencies(std::vector<const ChunkStatus*>(directDeps)),
                ChunkDependencies(std::move(accumulated)),
                -1);
        }

        // === FEATURES ===
        // 加载路径：空操作，仅依赖前一步
        {
            const ChunkStep& parent = steps.back();
            std::vector<const ChunkStatus*> directDeps = {parent.targetStatus()};
            auto accumulated =
                buildAccumulatedDependencies(directDeps, &parent.accumulatedDependencies(), parent.targetStatus());
            steps.emplace_back(&ChunkStatuses::FEATURES,
                ChunkDependencies(std::vector<const ChunkStatus*>(directDeps)),
                ChunkDependencies(std::move(accumulated)),
                -1);
        }

        // === INITIALIZE_LIGHT ===
        // 加载路径：初始化光照，仅依赖前一步
        {
            const ChunkStep& parent = steps.back();
            std::vector<const ChunkStatus*> directDeps = {parent.targetStatus()};
            auto accumulated =
                buildAccumulatedDependencies(directDeps, &parent.accumulatedDependencies(), parent.targetStatus());
            steps.emplace_back(&ChunkStatuses::INITIALIZE_LIGHT,
                ChunkDependencies(std::vector<const ChunkStatus*>(directDeps)),
                ChunkDependencies(std::move(accumulated)),
                -1);
        }

        // === LIGHT ===
        // 加载路径：光照传播，依赖 INITIALIZE_LIGHT(1)
        // directDeps = [INITIALIZE_LIGHT, INITIALIZE_LIGHT]
        {
            const ChunkStep& parent = steps.back();
            std::vector<const ChunkStatus*> directDeps = {parent.targetStatus()};
            addRequirement(directDeps, &ChunkStatuses::INITIALIZE_LIGHT, 1);
            auto accumulated =
                buildAccumulatedDependencies(directDeps, &parent.accumulatedDependencies(), parent.targetStatus());
            steps.emplace_back(&ChunkStatuses::LIGHT,
                ChunkDependencies(std::vector<const ChunkStatus*>(directDeps)),
                ChunkDependencies(std::move(accumulated)),
                -1);
        }

        // === SPAWN ===
        // 加载路径：空操作，仅依赖前一步
        {
            const ChunkStep& parent = steps.back();
            std::vector<const ChunkStatus*> directDeps = {parent.targetStatus()};
            auto accumulated =
                buildAccumulatedDependencies(directDeps, &parent.accumulatedDependencies(), parent.targetStatus());
            steps.emplace_back(&ChunkStatuses::SPAWN,
                ChunkDependencies(std::vector<const ChunkStatus*>(directDeps)),
                ChunkDependencies(std::move(accumulated)),
                -1);
        }

        // === FULL ===
        // 加载路径：ProtoChunk→LevelChunk 转换，仅依赖前一步
        {
            const ChunkStep& parent = steps.back();
            std::vector<const ChunkStatus*> directDeps = {parent.targetStatus()};
            auto accumulated =
                buildAccumulatedDependencies(directDeps, &parent.accumulatedDependencies(), parent.targetStatus());
            steps.emplace_back(&ChunkStatuses::FULL,
                ChunkDependencies(std::vector<const ChunkStatus*>(directDeps)),
                ChunkDependencies(std::move(accumulated)),
                -1);
        }

        return ChunkPyramid(std::move(steps));
    }();

    return pyramid;
}

} // namespace mc
