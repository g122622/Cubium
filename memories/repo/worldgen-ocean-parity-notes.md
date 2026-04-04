- Build command: cmake --build build --config RelWithDebInfo --target mc_tests
- Targeted validation command:
  ./build/bin/RelWithDebInfo/mc_tests.exe --gtest_filter="OceanFeatureWorldTest.*:EndBiomeProviderTest.*:BiomeRegistryTest.*Ocean*GenerationSettings*:VegetationFeatureTest.OceanFeatureIdsAreOffsetAfterLandVegetation:VegetationFeatureTest.FeatureRegistryHasAllFeatures:VegetationFeatureTest.FeatureRegistryOceanAndNetherFeatureNames:VegetationFeatureTest.OceanBiomeSettings:VegetationFeatureTest.WarmOceanBiomeSettings:VegetationFeatureTest.ColdOceanBiomeSettings:VegetationFeatureTest.FrozenOceanBiomeSettings"
- Gotcha: Block::isValidPosition expects IBlockReader&, but WorldGenRegion only implements IWorldWriter. In feature code, use explicit local support/water checks instead of passing WorldGenRegion to isValidPosition.
- Gotcha: BlueIceFeature positive tests must satisfy the packed-ice neighbor precondition at the feature-selected start position, or placement returns false.
