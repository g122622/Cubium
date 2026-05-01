#include "TestWorldHelper.hpp"

namespace mc {
namespace test {

DummyTickManager::DummyTickManager()
    : TickManager(*static_cast<IWorld*>(nullptr)) {
}

} // namespace test
} // namespace mc
