#pragma once

#include "TargetInfo.hpp"

#include "common/core/BlockRaycastResult.hpp"
#include "common/util/math/Vector3.hpp"

#include <functional>

namespace mc::client {
class ClientEntityManager;
class ClientWorld;
} // namespace mc::client

namespace mc::client::ui::minecraft::targetinfo {

class TargetInfoResolver {
public:
    using PlayerNameLookup = std::function<std::string(EntityId)>;

    [[nodiscard]] static TargetInfoSnapshot resolve(const Vector3& eyePosition,
        const Vector3& forward,
        const ClientWorld& world,
        const ClientEntityManager& entityManager,
        const BlockRaycastResult& blockRaycast,
        f32 reachDistance,
        const PlayerNameLookup& playerNameLookup);
};

} // namespace mc::client::ui::minecraft::targetinfo