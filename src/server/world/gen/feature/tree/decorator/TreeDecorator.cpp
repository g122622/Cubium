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
#include "PlaceOnGroundDecorator.hpp"
#include "SimpleTreeDecorators.hpp"
#include "TrunkVineDecorator.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include "common/util/Direction.hpp"
#include "common/util/json/JsonRead.hpp"
#include "common/util/math/random/Random.hpp"
#include "common/util/property/BooleanProperty.hpp"
#include "common/world/block/base/types/BlockPos.hpp"
#include "common/world/block/registry/vanilla/VanillaBlocks.hpp"
#include "common/world/block/state/BlockState.hpp"
#include "common/world/blockentity/BlockEntity.hpp"
#include "server/world/gen/chunk/IChunkGenerator.hpp"
#include "server/world/gen/feature/parser/BlockStateProviderParser.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>
#include <nlohmann/json_fwd.hpp>

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
    ChunkPrimer* chunk,
    IChunkGenerator* generator,
    DecorationSetter setter,
    math::IRandom& random,
    std::vector<BlockPos> logs,
    std::vector<BlockPos> leaves,
    std::vector<BlockPos> roots)
    : m_region(region)
    , m_chunk(chunk)
    , m_generator(generator)
    , m_setter(std::move(setter))
    , m_random(random)
    , m_logs(std::move(logs))
    , m_leaves(std::move(leaves))
    , m_roots(std::move(roots))
{
    auto byY = [](const BlockPos& a, const BlockPos& b) {
        return getY(a) < getY(b);
    };
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

BlockEntity* TreeDecoratorContext::getBlockEntity(const BlockPos& pos) const
{
    return m_region.getBlockEntity(pos);
}

bool TreeDecoratorContext::checkBlock(
    const BlockPos& pos, const std::function<bool(const BlockState&)>& predicate) const
{
    const BlockState* state = m_region.getBlockState(pos);
    return state != nullptr && predicate(*state);
}

Result<std::unique_ptr<TreeDecorator>> parseDecorator(const nlohmann::json& decoratorJson)
{
    if (!decoratorJson.is_object() || !decoratorJson.contains("type")) {
        return Error(ErrorCode::InvalidData, "tree decorator missing 'type'");
    }
    std::string typeStr;
    MC_TRY_ASSIGN(typeStr, util::json::getString(decoratorJson, "type"));
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
        f32 probability = 0.0f;
        MC_TRY_ASSIGN(probability, util::json::getFloat32(decoratorJson, "probability"));
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
            std::string dirName;
            MC_TRY_ASSIGN(dirName, util::json::getString(dirJson));
            auto dir = Directions::fromName(dirName);
            if (!dir.has_value()) {
                return Error(ErrorCode::InvalidData, "attached_to_logs unknown direction: " + dirName);
            }
            directions.push_back(dir.value());
        }

        std::unique_ptr<TreeDecorator> decorator =
            std::make_unique<AttachToLogsDecorator>(probability, providerResult.value(), std::move(directions));
        return decorator;
    }

    if (typeStr == "place_on_ground") {
        // tries(正整数，默认128) / radius(非负，默认2) / height(非负，默认1) / block_state_provider
        i32 tries = 128;
        if (decoratorJson.contains("tries")) {
            MC_TRY_ASSIGN(tries, util::json::getInt32(decoratorJson, "tries"));
            if (tries <= 0) {
                return Error(ErrorCode::InvalidData, "place_on_ground 'tries' must be a positive integer");
            }
        }
        i32 radius = 2;
        if (decoratorJson.contains("radius")) {
            MC_TRY_ASSIGN(radius, util::json::getInt32(decoratorJson, "radius"));
            if (radius < 0) {
                return Error(ErrorCode::InvalidData, "place_on_ground 'radius' must be a non-negative integer");
            }
        }
        i32 height = 1;
        if (decoratorJson.contains("height")) {
            MC_TRY_ASSIGN(height, util::json::getInt32(decoratorJson, "height"));
            if (height < 0) {
                return Error(ErrorCode::InvalidData, "place_on_ground 'height' must be a non-negative integer");
            }
        }
        if (!decoratorJson.contains("block_state_provider")) {
            return Error(ErrorCode::InvalidData, "place_on_ground missing 'block_state_provider'");
        }
        auto groundProviderResult = parser::BlockStateProviderParser::parse(decoratorJson["block_state_provider"]);
        if (!groundProviderResult.success()) {
            return groundProviderResult.error();
        }
        return std::unique_ptr<TreeDecorator>(
            std::make_unique<PlaceOnGroundDecorator>(tries, radius, height, groundProviderResult.value()));
    }

    if (typeStr == "beehive") {
        // probability[0.0,1.0]
        if (!decoratorJson.contains("probability") || !decoratorJson["probability"].is_number()) {
            return Error(ErrorCode::InvalidData, "beehive missing 'probability'");
        }
        f32 probability = 0.0f;
        MC_TRY_ASSIGN(probability, util::json::getFloat32(decoratorJson, "probability"));
        if (probability < 0.0f || probability > 1.0f) {
            return Error(ErrorCode::InvalidData, "beehive probability out of range [0.0,1.0]");
        }
        return std::unique_ptr<TreeDecorator>(std::make_unique<BeehiveDecorator>(probability));
    }

    if (typeStr == "leave_vine") {
        if (!decoratorJson.contains("probability") || !decoratorJson["probability"].is_number()) {
            return Error(ErrorCode::InvalidData, "leave_vine missing 'probability'");
        }
        f32 probability = 0.0f;
        MC_TRY_ASSIGN(probability, util::json::getFloat32(decoratorJson, "probability"));
        if (probability < 0.0f || probability > 1.0f) {
            return Error(ErrorCode::InvalidData, "leave_vine probability out of range [0.0,1.0]");
        }
        return std::unique_ptr<TreeDecorator>(std::make_unique<LeaveVineDecorator>(probability));
    }

    if (typeStr == "cocoa") {
        if (!decoratorJson.contains("probability") || !decoratorJson["probability"].is_number()) {
            return Error(ErrorCode::InvalidData, "cocoa missing 'probability'");
        }
        f32 probability = 0.0f;
        MC_TRY_ASSIGN(probability, util::json::getFloat32(decoratorJson, "probability"));
        if (probability < 0.0f || probability > 1.0f) {
            return Error(ErrorCode::InvalidData, "cocoa probability out of range [0.0,1.0]");
        }
        return std::unique_ptr<TreeDecorator>(std::make_unique<CocoaDecorator>(probability));
    }

    if (typeStr == "alter_ground") {
        if (!decoratorJson.contains("provider")) {
            return Error(ErrorCode::InvalidData, "alter_ground missing 'provider'");
        }
        auto alterProviderResult = parser::BlockStateProviderParser::parse(decoratorJson["provider"]);
        if (!alterProviderResult.success()) {
            return alterProviderResult.error();
        }
        return std::unique_ptr<TreeDecorator>(std::make_unique<AlterGroundDecorator>(alterProviderResult.value()));
    }

    if (typeStr == "attached_to_leaves") {
        if (!decoratorJson.contains("probability") || !decoratorJson["probability"].is_number()) {
            return Error(ErrorCode::InvalidData, "attached_to_leaves missing 'probability'");
        }
        f32 probability = 0.0f;
        MC_TRY_ASSIGN(probability, util::json::getFloat32(decoratorJson, "probability"));
        if (probability < 0.0f || probability > 1.0f) {
            return Error(ErrorCode::InvalidData, "attached_to_leaves probability out of range [0.0,1.0]");
        }
        for (const char* field : {"exclusion_radius_xz", "exclusion_radius_y", "required_empty_blocks"}) {
            if (!decoratorJson.contains(field) || !decoratorJson[field].is_number_integer()) {
                return Error(ErrorCode::InvalidData, std::string("attached_to_leaves missing '") + field + "'");
            }
        }
        if (!decoratorJson.contains("block_provider")) {
            return Error(ErrorCode::InvalidData, "attached_to_leaves missing 'block_provider'");
        }
        auto leafProviderResult = parser::BlockStateProviderParser::parse(decoratorJson["block_provider"]);
        if (!leafProviderResult.success()) {
            return leafProviderResult.error();
        }
        if (!decoratorJson.contains("directions") || !decoratorJson["directions"].is_array() ||
            decoratorJson["directions"].empty()) {
            return Error(ErrorCode::InvalidData, "attached_to_leaves 'directions' must be a non-empty array");
        }
        std::vector<Direction> leafDirections;
        leafDirections.reserve(decoratorJson["directions"].size());
        for (const auto& dirJson : decoratorJson["directions"]) {
            if (!dirJson.is_string()) {
                return Error(ErrorCode::InvalidData, "attached_to_leaves direction entry must be a string");
            }
            std::string dirName;
            MC_TRY_ASSIGN(dirName, util::json::getString(dirJson));
            auto dir = Directions::fromName(dirName);
            if (!dir.has_value()) {
                return Error(ErrorCode::InvalidData, "attached_to_leaves unknown direction: " + dirName);
            }
            leafDirections.push_back(dir.value());
        }
        i32 exclusionRadiusXz = 0;
        i32 exclusionRadiusY = 0;
        i32 requiredEmptyBlocks = 0;
        MC_TRY_ASSIGN(exclusionRadiusXz, util::json::getInt32(decoratorJson, "exclusion_radius_xz"));
        MC_TRY_ASSIGN(exclusionRadiusY, util::json::getInt32(decoratorJson, "exclusion_radius_y"));
        MC_TRY_ASSIGN(requiredEmptyBlocks, util::json::getInt32(decoratorJson, "required_empty_blocks"));
        return std::unique_ptr<TreeDecorator>(std::make_unique<AttachedToLeavesDecorator>(probability,
            exclusionRadiusXz,
            exclusionRadiusY,
            leafProviderResult.value(),
            requiredEmptyBlocks,
            std::move(leafDirections)));
    }

    if (typeStr == "pale_moss") {
        // leaves_probability / trunk_probability / ground_probability（三者独立，无默认值）
        f32 leavesProbability = 0.0f;
        f32 trunkProbability = 0.0f;
        f32 groundProbability = 0.0f;
        MC_TRY_ASSIGN(leavesProbability, util::json::getFloat32(decoratorJson, "leaves_probability"));
        MC_TRY_ASSIGN(trunkProbability, util::json::getFloat32(decoratorJson, "trunk_probability"));
        MC_TRY_ASSIGN(groundProbability, util::json::getFloat32(decoratorJson, "ground_probability"));
        if (leavesProbability < 0.0f || leavesProbability > 1.0f) {
            return Error(ErrorCode::InvalidData, "pale_moss 'leaves_probability' out of range [0.0,1.0]");
        }
        if (trunkProbability < 0.0f || trunkProbability > 1.0f) {
            return Error(ErrorCode::InvalidData, "pale_moss 'trunk_probability' out of range [0.0,1.0]");
        }
        if (groundProbability < 0.0f || groundProbability > 1.0f) {
            return Error(ErrorCode::InvalidData, "pale_moss 'ground_probability' out of range [0.0,1.0]");
        }
        return std::unique_ptr<TreeDecorator>(
            std::make_unique<PaleMossDecorator>(leavesProbability, trunkProbability, groundProbability));
    }

    if (typeStr == "creaking_heart") {
        if (!decoratorJson.contains("probability") || !decoratorJson["probability"].is_number()) {
            return Error(ErrorCode::InvalidData, "creaking_heart missing 'probability'");
        }
        f32 creakProb = 0.0f;
        MC_TRY_ASSIGN(creakProb, util::json::getFloat32(decoratorJson, "probability"));
        if (creakProb < 0.0f || creakProb > 1.0f) {
            return Error(ErrorCode::InvalidData, "creaking_heart probability out of range [0.0,1.0]");
        }
        return std::unique_ptr<TreeDecorator>(std::make_unique<CreakingHeartDecorator>(creakProb));
    }

    return Error(ErrorCode::InvalidData, "unregistered tree decorator type: " + typeStr);
}

} // namespace decorator
} // namespace tree
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
