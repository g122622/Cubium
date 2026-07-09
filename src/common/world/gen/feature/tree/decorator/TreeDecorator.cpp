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

#include "TreeDecorator.hpp"
#include "AttachToLogsDecorator.hpp"
#include "TrunkVineDecorator.hpp"
#include "common/util/Direction.hpp"
#include "common/world/block/registry/VanillaBlocks.hpp"

#include <algorithm>
#include <nlohmann/json.hpp>

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace tree {
namespace decorator {

namespace {

/// MC Vec3i::getY 比较器：logs/leaves/roots 按 Y 坐标排序。
i32 getY(const BlockPos& pos) noexcept
{
    return pos.y;
}

} // namespace

TreeDecoratorContext::TreeDecoratorContext(WorldGenRegion& region,
    DecorationSetter setter,
    math::Random& random,
    std::vector<BlockPos> logs,
    std::vector<BlockPos> leaves,
    std::vector<BlockPos> roots)
    : m_region(region)
    , m_setter(std::move(setter))
    , m_random(random)
    , m_logs(std::move(logs))
    , m_leaves(std::move(leaves))
    , m_roots(std::move(roots))
{
    auto byY = [](const BlockPos& a, const BlockPos& b) { return getY(a) < getY(b); };
    std::sort(m_logs.begin(), m_logs.end(), byY);
    std::sort(m_leaves.begin(), m_leaves.end(), byY);
    std::sort(m_roots.begin(), m_roots.end(), byY);
}

void TreeDecoratorContext::placeVine(const BlockPos& pos, const BooleanProperty& face) const
{
    // MC: setBlock(pos, Blocks.VINE.defaultBlockState().setValue(face, true))
    if (VanillaBlocks::VINE == nullptr) {
        return;
    }
    const BlockState* vineDefault = &VanillaBlocks::VINE->defaultState();
    if (vineDefault == nullptr) {
        return;
    }
    const BlockState* placed = &vineDefault->with(face, true);
    m_setter(pos, placed);
}

bool TreeDecoratorContext::isAir(const BlockPos& pos) const
{
    // MC: level.isStateAtPosition(pos, BlockStateBase::isAir)。项目 nullptr 视为空气。
    const BlockState* state = m_region.getBlockState(pos);
    return state == nullptr || state->isAir();
}

Result<std::unique_ptr<TreeDecorator>> parseDecorator(const nlohmann::json& decoratorJson)
{
    if (!decoratorJson.is_object() || !decoratorJson.contains("type")) {
        return Error(ErrorCode::InvalidData, "tree decorator missing 'type'");
    }
    std::string typeStr = decoratorJson["type"].get<std::string>();
    // 剥离 "minecraft:" 前缀。
    const std::string::size_type colon = typeStr.find(':');
    if (colon != std::string::npos) {
        typeStr = typeStr.substr(colon + 1);
    }

    if (typeStr == "trunk_vine") {
        // MC TrunkVineDecorator：MapCodec.unit(INSTANCE)，无配置字段。
        std::unique_ptr<TreeDecorator> decorator = std::make_unique<TrunkVineDecorator>();
        return decorator;
    }

    if (typeStr == "attached_to_logs") {
        // probability[0.0,1.0] / block_provider / directions(非空)
        if (!decoratorJson.contains("probability") || !decoratorJson["probability"].is_number()) {
            return Error(ErrorCode::InvalidData, "attached_to_logs missing 'probability'");
        }
        const f32 probability = decoratorJson["probability"].get<f32>();
        if (probability < 0.0f || probability > 1.0f) {
            return Error(ErrorCode::InvalidData, "attached_to_logs probability out of range [0.0,1.0]");
        }

        if (!decoratorJson.contains("block_provider")) {
            return Error(ErrorCode::InvalidData, "attached_to_logs missing 'block_provider'");
        }
        auto providerResult = parser::BlockStateProviderParser::parse(decoratorJson["block_provider"]);
        if (!providerResult.success()) {
            return providerResult.error();
        }

        if (!decoratorJson.contains("directions") || !decoratorJson["directions"].is_array() ||
            decoratorJson["directions"].empty()) {
            return Error(ErrorCode::InvalidData, "attached_to_logs 'directions' must be a non-empty array");
        }
        std::vector<Direction> directions;
        directions.reserve(decoratorJson["directions"].size());
        for (const auto& dirJson : decoratorJson["directions"]) {
            if (!dirJson.is_string()) {
                return Error(ErrorCode::InvalidData, "attached_to_logs direction entry must be a string");
            }
            auto dir = Directions::fromName(dirJson.get<std::string>());
            if (!dir.has_value()) {
                return Error(
                    ErrorCode::InvalidData, "attached_to_logs unknown direction: " + dirJson.get<std::string>());
            }
            directions.push_back(dir.value());
        }

        std::unique_ptr<TreeDecorator> decorator = std::make_unique<AttachToLogsDecorator>(probability,
            std::make_unique<parser::BlockStateProviderHandle>(std::move(providerResult.value())),
            std::move(directions));
        return decorator;
    }

    return Error(ErrorCode::InvalidData, "unregistered tree decorator type: " + typeStr);
}

} // namespace decorator
} // namespace tree
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
