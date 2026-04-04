#include <gtest/gtest.h>

#include "world/block/VanillaBlocks.hpp"
#include "world/chunk/ChunkPrimer.hpp"
#include "world/gen/chunk/IChunkGenerator.hpp"
#include "world/gen/feature/ocean/CoralFeature.hpp"
#include "world/gen/feature/ocean/KelpFeature.hpp"
#include "world/gen/feature/ocean/SeaPickleFeature.hpp"
#include "world/gen/feature/ocean/SeagrassFeature.hpp"

#include <array>
#include <memory>
#include <vector>

using namespace mc;

class OceanFeatureWorldTest : public ::testing::Test {
protected:
    void SetUp() override {
        VanillaBlocks::initialize();

        for (i32 relZ = -1; relZ <= 1; ++relZ) {
            for (i32 relX = -1; relX <= 1; ++relX) {
                const i32 index = (relZ + 1) * 3 + (relX + 1);
                auto chunk = std::make_unique<ChunkPrimer>(relX, relZ);

                // 构建海底测试场景：y=40 为地面，y=41..62 为水层。
                for (i32 x = 0; x < 16; ++x) {
                    for (i32 z = 0; z < 16; ++z) {
                        chunk->setBlock(x, 40, z, &VanillaBlocks::SAND->defaultState());
                        for (i32 y = 41; y <= 62; ++y) {
                            chunk->setBlock(x, y, z, &VanillaBlocks::WATER->defaultState());
                        }
                    }
                }

                m_chunks[static_cast<size_t>(index)] = chunk.get();
                m_ownedChunks.push_back(std::move(chunk));
            }
        }

        m_region = std::make_unique<WorldGenRegion>(0, 0, m_chunks);
    }

    void setWorldBlock(i32 x, i32 y, i32 z, const BlockState* state) {
        ASSERT_TRUE(m_region->setBlock(x, y, z, state));
    }

    [[nodiscard]] const BlockState* getWorldBlock(i32 x, i32 y, i32 z) const {
        return m_region->getBlock(x, y, z);
    }

    std::array<IChunk*, 9> m_chunks{};
    std::vector<std::unique_ptr<ChunkPrimer>> m_ownedChunks;
    std::unique_ptr<WorldGenRegion> m_region;
};

TEST_F(OceanFeatureWorldTest, KelpFeaturePlacesKelpInWater) {
    auto configured = KelpFeatures::createNormalKelp();
    ASSERT_NE(configured, nullptr);

    const KelpFeatureConfig& config = configured->getConfig();
    ASSERT_NE(config.kelpState, nullptr);
    ASSERT_NE(config.kelpTopState, nullptr);

    KelpFeature feature;
    math::Random random(12345);

    EXPECT_TRUE(feature.place(*m_region, random, BlockPos(0, 0, 0), config));

    bool foundKelp = false;
    for (i32 x = 0; x < 16 && !foundKelp; ++x) {
        for (i32 z = 0; z < 16 && !foundKelp; ++z) {
            for (i32 y = 41; y <= 62; ++y) {
                const BlockState* planted = getWorldBlock(x, y, z);
                if (planted == nullptr) {
                    continue;
                }
                const bool isKelp = (VanillaBlocks::KELP != nullptr && planted->is(VanillaBlocks::KELP));
                const bool isKelpPlant =
                    (VanillaBlocks::KELP_PLANT != nullptr && planted->is(VanillaBlocks::KELP_PLANT));
                if (isKelp || isKelpPlant) {
                    foundKelp = true;
                    break;
                }
            }
        }
    }
    EXPECT_TRUE(foundKelp);
}

TEST_F(OceanFeatureWorldTest, SeagrassMixedFeaturePlacesSeaPlant) {
    auto configured = SeagrassFeatures::createMixedSeagrass();
    ASSERT_NE(configured, nullptr);

    const SeagrassFeatureConfig& config = configured->getConfig();
    ASSERT_NE(config.seagrassState, nullptr);
    ASSERT_NE(config.tallSeagrassLowerState, nullptr);
    ASSERT_NE(config.tallSeagrassUpperState, nullptr);

    SeagrassFeature feature;
    math::Random random(22334);

    EXPECT_TRUE(feature.place(*m_region, random, BlockPos(0, 0, 0), config));

    bool foundSeagrass = false;
    for (i32 x = 0; x < 16 && !foundSeagrass; ++x) {
        for (i32 z = 0; z < 16 && !foundSeagrass; ++z) {
            for (i32 y = 41; y <= 42; ++y) {
                const BlockState* planted = getWorldBlock(x, y, z);
                if (planted == nullptr) {
                    continue;
                }
                const bool isSeagrass =
                    (VanillaBlocks::SEAGRASS != nullptr && planted->is(VanillaBlocks::SEAGRASS));
                const bool isTallSeagrass =
                    (VanillaBlocks::TALL_SEAGRASS != nullptr && planted->is(VanillaBlocks::TALL_SEAGRASS));
                if (isSeagrass || isTallSeagrass) {
                    foundSeagrass = true;
                    break;
                }
            }
        }
    }
    EXPECT_TRUE(foundSeagrass);
}

TEST_F(OceanFeatureWorldTest, CoralFeaturePlacesConfiguredCoralBlock) {
    CoralFeature feature;
    CoralFeatureConfig config(blocks::CoralColor::Tube, true);
    math::Random random(99887);

    ASSERT_NE(VanillaBlocks::TUBE_CORAL_BLOCK, nullptr);
    EXPECT_TRUE(feature.place(*m_region, random, BlockPos(0, 0, 0), config));

    bool foundCoralBlock = false;
    for (i32 x = 0; x < 16 && !foundCoralBlock; ++x) {
        for (i32 z = 0; z < 16 && !foundCoralBlock; ++z) {
            for (i32 y = 41; y <= 70; ++y) {
                const BlockState* state = getWorldBlock(x, y, z);
                if (state != nullptr && state->is(VanillaBlocks::TUBE_CORAL_BLOCK)) {
                    foundCoralBlock = true;
                    break;
                }
            }
        }
    }
    EXPECT_TRUE(foundCoralBlock);
}

TEST_F(OceanFeatureWorldTest, SeaPickleFeatureFailsOnNonCoralGround) {
    auto configured = SeaPickleFeatures::createNormalSeaPickle();
    ASSERT_NE(configured, nullptr);

    const SeaPickleFeatureConfig& config = configured->getConfig();
    ASSERT_NE(config.seaPickleState, nullptr);

    SeaPickleFeature feature;
    math::Random random(33445);

    EXPECT_FALSE(feature.place(*m_region, random, BlockPos(0, 0, 0), config));
}

TEST_F(OceanFeatureWorldTest, SeaPickleFeaturePlacesOnLivingCoral) {
    ASSERT_NE(VanillaBlocks::TUBE_CORAL_BLOCK, nullptr);
    for (i32 x = 0; x < 16; ++x) {
        for (i32 z = 0; z < 16; ++z) {
            setWorldBlock(x, 40, z, &VanillaBlocks::TUBE_CORAL_BLOCK->defaultState());
        }
    }

    auto configured = SeaPickleFeatures::createNormalSeaPickle();
    ASSERT_NE(configured, nullptr);

    SeaPickleFeature feature;
    math::Random random(44556);

    EXPECT_TRUE(feature.place(*m_region, random, BlockPos(0, 0, 0), configured->getConfig()));

    ASSERT_NE(VanillaBlocks::SEA_PICKLE, nullptr);
    bool foundSeaPickle = false;
    for (i32 x = 0; x < 16 && !foundSeaPickle; ++x) {
        for (i32 z = 0; z < 16 && !foundSeaPickle; ++z) {
            const BlockState* state = getWorldBlock(x, 41, z);
            if (state != nullptr && state->is(VanillaBlocks::SEA_PICKLE)) {
                foundSeaPickle = true;
            }
        }
    }
    EXPECT_TRUE(foundSeaPickle);
}
