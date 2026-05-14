#include "TestWorldHelper.hpp"

namespace mc {
namespace test {

DummyTickManager::DummyTickManager()
    : TickManager(dummyWorld()) {
}

IWorld& DummyTickManager::dummyWorld()
{
    class DummyWorld final : public BaseTestWorld {
    public:
        [[nodiscard]] world::tick::TickManager& tickManager() override {
            throw std::runtime_error("DummyWorld::tickManager not implemented");
        }

        [[nodiscard]] const world::tick::TickManager& tickManager() const override {
            throw std::runtime_error("DummyWorld::tickManager not implemented");
        }
    };

    static DummyWorld world;
    return world;
}

} // namespace test
} // namespace mc
