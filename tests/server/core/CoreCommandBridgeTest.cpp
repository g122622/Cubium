#include <gtest/gtest.h>

#include "server/application/CoreCommandBridge.hpp"
#include "server/core/ServerCore.hpp"

namespace mc::server {

TEST(CoreCommandBridgeTest, GetSeedFallsBackToCoreConfigWhenWorldIsNull) {
    ServerCoreConfig config;
    config.seed = 987654321ULL;

    ServerCore core(config);
    CoreCommandBridge bridge(nullptr, &core);

    EXPECT_EQ(bridge.getSeed(), static_cast<i64>(config.seed));
}

} // namespace mc::server
