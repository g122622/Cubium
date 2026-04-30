#pragma once

#include "FoliagePlacer.hpp"
#include <memory>

namespace mc {

/**
 * @brief 松树树叶放置器
 *
 * 生成锥形树叶，从下到上逐渐变细。
 * 参考 MC 1.16.5: PineFoliagePlacer
 */
class PineFoliagePlacer : public FoliagePlacer {
public:
    PineFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset, i32 height);

    [[nodiscard]] i32 getFoliageHeight(math::Random& random, i32 trunkHeight) const override;
    [[nodiscard]] const char* name() const override { return "pine"; }
    [[nodiscard]] std::unique_ptr<FoliagePlacer> clone() const override;

protected:
    void placeFoliageInternal(
        WorldGenRegion& world,
        math::Random& random,
        i32 trunkHeight,
        const FoliagePosition& foliagePos,
        i32 foliageHeight,
        i32 radius,
        i32 offset,
        std::set<BlockPos>& foliageBlocks,
        const BlockState* foliageBlock
    ) override;

    [[nodiscard]] bool shouldSkip(
        math::Random& random,
        i32 dx, i32 dy, i32 dz,
        i32 radius,
        bool trunkTop
    ) const override;

private:
    i32 m_height;

    /**
     * @brief 计算指定高度的树叶半径
     */
    [[nodiscard]] i32 getRadiusAtHeight(i32 height, i32 foliageHeight) const;
};

/**
 * @brief 云杉树叶放置器
 *
 * 生成尖顶形状的树叶。
 * 参考 MC 1.16.5: SpruceFoliagePlacer
 */
class SpruceFoliagePlacer : public FoliagePlacer {
public:
    SpruceFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset, i32 height);

    [[nodiscard]] i32 getFoliageHeight(math::Random& random, i32 trunkHeight) const override;
    [[nodiscard]] const char* name() const override { return "spruce"; }
    [[nodiscard]] std::unique_ptr<FoliagePlacer> clone() const override;

protected:
    void placeFoliageInternal(
        WorldGenRegion& world,
        math::Random& random,
        i32 trunkHeight,
        const FoliagePosition& foliagePos,
        i32 foliageHeight,
        i32 radius,
        i32 offset,
        std::set<BlockPos>& foliageBlocks,
        const BlockState* foliageBlock
    ) override;

    [[nodiscard]] bool shouldSkip(
        math::Random& random,
        i32 dx, i32 dy, i32 dz,
        i32 radius,
        bool trunkTop
    ) const override;

private:
    i32 m_height;

    void placeFoliageLayer(
        WorldGenRegion& world,
        math::Random& random,
        const FoliagePosition& foliagePos,
        i32 radius,
        i32 yOffset,
        std::set<BlockPos>& foliageBlocks,
        const BlockState* foliageBlock,
        bool trunkTop);
};

/**
 * @brief 金合欢树叶放置器
 *
 * 生成伞形树叶。
 * 参考 MC 1.16.5: AcaciaFoliagePlacer
 */
class AcaciaFoliagePlacer : public FoliagePlacer {
public:
    AcaciaFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset);

    [[nodiscard]] i32 getFoliageHeight(math::Random& random, i32 trunkHeight) const override;
    [[nodiscard]] const char* name() const override { return "acacia"; }
    [[nodiscard]] std::unique_ptr<FoliagePlacer> clone() const override;

protected:
    void placeFoliageInternal(
        WorldGenRegion& world,
        math::Random& random,
        i32 trunkHeight,
        const FoliagePosition& foliagePos,
        i32 foliageHeight,
        i32 radius,
        i32 offset,
        std::set<BlockPos>& foliageBlocks,
        const BlockState* foliageBlock
    ) override;

    [[nodiscard]] bool shouldSkip(
        math::Random& random,
        i32 dx, i32 dy, i32 dz,
        i32 radius,
        bool trunkTop
    ) const override;

private:
    void placeFoliageLayer(
        WorldGenRegion& world,
        math::Random& random,
        const FoliagePosition& foliagePos,
        i32 radius,
        i32 yOffset,
        std::set<BlockPos>& foliageBlocks,
        const BlockState* foliageBlock,
        bool trunkTop);
};

/**
 * @brief 深色橡树树叶放置器
 *
 * 生成球形树叶，用于深色橡树。
 * 参考 MC 1.16.5: DarkOakFoliagePlacer
 */
class DarkOakFoliagePlacer : public FoliagePlacer {
public:
    DarkOakFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset, i32 height);

    [[nodiscard]] i32 getFoliageHeight(math::Random& random, i32 trunkHeight) const override;
    [[nodiscard]] const char* name() const override { return "dark_oak"; }
    [[nodiscard]] std::unique_ptr<FoliagePlacer> clone() const override;

protected:
    void placeFoliageInternal(
        WorldGenRegion& world,
        math::Random& random,
        i32 trunkHeight,
        const FoliagePosition& foliagePos,
        i32 foliageHeight,
        i32 radius,
        i32 offset,
        std::set<BlockPos>& foliageBlocks,
        const BlockState* foliageBlock
    ) override;

    [[nodiscard]] bool shouldSkip(
        math::Random& random,
        i32 dx, i32 dy, i32 dz,
        i32 radius,
        bool trunkTop
    ) const override;

private:
    i32 m_height;

    void placeFoliageLayer(
        WorldGenRegion& world,
        math::Random& random,
        const FoliagePosition& foliagePos,
        i32 radius,
        i32 yOffset,
        std::set<BlockPos>& foliageBlocks,
        const BlockState* foliageBlock,
        bool trunkTop);

    bool shouldSkipBase(
        math::Random& random,
        i32 dx, i32 dy, i32 dz,
        i32 radius,
        bool trunkTop) const;
};

/**
 * @brief 丛林木树叶放置器
 *
 * 生成单层树叶，用于丛林木。
 * 参考 MC 1.16.5: JungleFoliagePlacer
 */
class JungleFoliagePlacer : public FoliagePlacer {
public:
    JungleFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset, i32 height);

    [[nodiscard]] i32 getFoliageHeight(math::Random& random, i32 trunkHeight) const override;
    [[nodiscard]] const char* name() const override { return "jungle"; }
    [[nodiscard]] std::unique_ptr<FoliagePlacer> clone() const override;

protected:
    void placeFoliageInternal(
        WorldGenRegion& world,
        math::Random& random,
        i32 trunkHeight,
        const FoliagePosition& foliagePos,
        i32 foliageHeight,
        i32 radius,
        i32 offset,
        std::set<BlockPos>& foliageBlocks,
        const BlockState* foliageBlock
    ) override;

    [[nodiscard]] bool shouldSkip(
        math::Random& random,
        i32 dx, i32 dy, i32 dz,
        i32 radius,
        bool trunkTop
    ) const override;

private:
    i32 m_height;
};

/**
 * @brief 巨型松树树叶放置器
 *
 * 生成大型锥形树叶，用于巨型松树。
 * 参考 MC 1.16.5: MegaPineFoliagePlacer
 */
class MegaPineFoliagePlacer : public FoliagePlacer {
public:
    MegaPineFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset, i32 height);

    [[nodiscard]] i32 getFoliageHeight(math::Random& random, i32 trunkHeight) const override;
    [[nodiscard]] const char* name() const override { return "mega_pine"; }
    [[nodiscard]] std::unique_ptr<FoliagePlacer> clone() const override;

protected:
    void placeFoliageInternal(
        WorldGenRegion& world,
        math::Random& random,
        i32 trunkHeight,
        const FoliagePosition& foliagePos,
        i32 foliageHeight,
        i32 radius,
        i32 offset,
        std::set<BlockPos>& foliageBlocks,
        const BlockState* foliageBlock
    ) override;

    [[nodiscard]] bool shouldSkip(
        math::Random& random,
        i32 dx, i32 dy, i32 dz,
        i32 radius,
        bool trunkTop
    ) const override;

private:
    i32 m_height;
};

/**
 * @brief 灌木树叶放置器
 *
 * 生成单层球形树叶，用于灌木。
 * 参考 MC 1.16.5: BushFoliagePlacer
 */
class BushFoliagePlacer : public FoliagePlacer {
public:
    BushFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset);

    [[nodiscard]] i32 getFoliageHeight(math::Random& random, i32 trunkHeight) const override;
    [[nodiscard]] const char* name() const override { return "bush"; }
    [[nodiscard]] std::unique_ptr<FoliagePlacer> clone() const override;

protected:
    void placeFoliageInternal(
        WorldGenRegion& world,
        math::Random& random,
        i32 trunkHeight,
        const FoliagePosition& foliagePos,
        i32 foliageHeight,
        i32 radius,
        i32 offset,
        std::set<BlockPos>& foliageBlocks,
        const BlockState* foliageBlock
    ) override;

    [[nodiscard]] bool shouldSkip(
        math::Random& random,
        i32 dx, i32 dy, i32 dz,
        i32 radius,
        bool trunkTop
    ) const override;
};

/**
 * @brief 精美树叶放置器
 *
 * 生成更大更密集的球形树叶。
 * 参考 MC 1.16.5: FancyFoliagePlacer
 */
class FancyFoliagePlacer : public FoliagePlacer {
public:
    FancyFoliagePlacer(const FeatureSpread& radius, const FeatureSpread& offset, i32 height);

    [[nodiscard]] i32 getFoliageHeight(math::Random& random, i32 trunkHeight) const override;
    [[nodiscard]] const char* name() const override { return "fancy"; }
    [[nodiscard]] std::unique_ptr<FoliagePlacer> clone() const override;

protected:
    void placeFoliageInternal(
        WorldGenRegion& world,
        math::Random& random,
        i32 trunkHeight,
        const FoliagePosition& foliagePos,
        i32 foliageHeight,
        i32 radius,
        i32 offset,
        std::set<BlockPos>& foliageBlocks,
        const BlockState* foliageBlock
    ) override;

    [[nodiscard]] bool shouldSkip(
        math::Random& random,
        i32 dx, i32 dy, i32 dz,
        i32 radius,
        bool trunkTop
    ) const override;

private:
    i32 m_height;
};

} // namespace mc
