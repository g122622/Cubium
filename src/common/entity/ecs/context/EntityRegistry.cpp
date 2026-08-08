#include "common/entity/ecs/context/EntityRegistry.hpp"

namespace mc::ecs {

EntityRegistry::EntityRegistry(std::string debugName) : m_debugName(std::move(debugName)) {}

} // namespace mc::ecs
