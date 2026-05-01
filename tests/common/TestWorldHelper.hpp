#pragma once

#include "common/world/tick/manager/TickManager.hpp"
#include "common/world/IWorld.hpp"
#include "common/world/fluid/Fluid.hpp"
#include "common/world/block/Block.hpp"

namespace mc {
namespace test {

/**
 * @brief 测试用的空 TickManager 实现
 *
 * 用于测试中需要 IWorld::tickManager() 但不需要实际功能的场景。
 * 注意：由于 TickManager 的方法不是 virtual，这里使用组合模式，
 * 但需要特殊处理来满足接口要求。
 */
class DummyTickManager : public world::tick::TickManager {
public:
    DummyTickManager();
};

} // namespace test
} // namespace mc
