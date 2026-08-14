#pragma once

#include "common/entity/ecs/systems/ISystem.hpp"

namespace mc::ecs {

class EntityRegistry;
class EntityContext;

/**
 * @brief 每帧 tick 的系统接口
 *
 * 对齐基岩版 ITickingSystem（mc/deps/ecs/systems/ITickingSystem.h）。
 * tick(registry) 遍历整个 registry 的相关实体；singleTick 针对单实体。
 */
class ITickingSystem : public ISystem {
public:
    /** 全 registry tick */
    virtual void tick(EntityRegistry& registry) = 0;
};

} // namespace mc::ecs
