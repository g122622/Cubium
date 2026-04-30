#include <gtest/gtest.h>
#include <cstddef>
#include <algorithm>

#include "client/renderer/trident/entity/model/animal/AnimalModels.hpp"
#include "client/renderer/trident/entity/model/core/ModelRenderer.hpp"

using namespace mc::client::renderer::entity::model::animal;
using mc::client::renderer::entity::model::ModelVertex;

namespace mc::client::renderer {
namespace {

struct Bounds {
    f32 minX = 0.0f;
    f32 minY = 0.0f;
    f32 minZ = 0.0f;
    f32 maxX = 0.0f;
    f32 maxY = 0.0f;
    f32 maxZ = 0.0f;
};

Bounds computeBounds(const std::vector<ModelVertex>& vertices) {
    Bounds b;
    if (vertices.empty()) {
        return b;
    }

    b.minX = b.maxX = vertices[0].position.x;
    b.minY = b.maxY = vertices[0].position.y;
    b.minZ = b.maxZ = vertices[0].position.z;

    for (const auto& v : vertices) {
        b.minX = std::min(b.minX, v.position.x);
        b.minY = std::min(b.minY, v.position.y);
        b.minZ = std::min(b.minZ, v.position.z);
        b.maxX = std::max(b.maxX, v.position.x);
        b.maxY = std::max(b.maxY, v.position.y);
        b.maxZ = std::max(b.maxZ, v.position.z);
    }

    return b;
}

template <typename TModel>
Bounds buildDefaultPoseBounds(TModel& model) {
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;

    constexpr f32 kModelScale = 1.0f / 16.0f;
    model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, kModelScale);
    model.generateMesh(vertices, indices, kModelScale);

    EXPECT_FALSE(vertices.empty());
    EXPECT_FALSE(indices.empty());

    return computeBounds(vertices);
}

} // anonymous namespace

TEST(AnimalModelPose, CowCopyAnglesToSynchronizesMatchingParts) {
    CowModel sourceModel;
    CowModel targetModel;

    const auto& sourceParts = sourceModel.getParts();
    const auto& targetParts = targetModel.getParts();

    ASSERT_EQ(sourceParts.size(), targetParts.size());
    ASSERT_GE(sourceParts.size(), 2u);

    for (std::size_t index = 0; index < sourceParts.size(); ++index) {
        const f64 seed = static_cast<f64>(index + 1);

        sourceParts[index]->setRotateAngleX(seed * 0.11f);
        sourceParts[index]->setRotateAngleY(-seed * 0.22f);
        sourceParts[index]->setRotateAngleZ(seed * 0.33f);
        sourceParts[index]->setRotationPoint(seed * 1.0f, seed * 2.0f, seed * 3.0f);

        targetParts[index]->setRotateAngleX(-seed * 1.01f);
        targetParts[index]->setRotateAngleY(seed * 1.02f);
        targetParts[index]->setRotateAngleZ(-seed * 1.03f);
        targetParts[index]->setRotationPoint(-seed * 4.0f, -seed * 5.0f, -seed * 6.0f);
    }

    sourceModel.copyAnglesTo(targetModel);

    for (std::size_t index = 0; index < sourceParts.size(); ++index) {
        EXPECT_DOUBLE_EQ(sourceParts[index]->rotateAngleX(), targetParts[index]->rotateAngleX());
        EXPECT_DOUBLE_EQ(sourceParts[index]->rotateAngleY(), targetParts[index]->rotateAngleY());
        EXPECT_DOUBLE_EQ(sourceParts[index]->rotateAngleZ(), targetParts[index]->rotateAngleZ());
        EXPECT_DOUBLE_EQ(sourceParts[index]->rotationPointX(), targetParts[index]->rotationPointX());
        EXPECT_DOUBLE_EQ(sourceParts[index]->rotationPointY(), targetParts[index]->rotationPointY());
        EXPECT_DOUBLE_EQ(sourceParts[index]->rotationPointZ(), targetParts[index]->rotationPointZ());
    }
}

TEST(AnimalModelMesh, PigReasonableBoundsAndHorizontalBody) {
    PigModel model;
    const Bounds b = buildDefaultPoseBounds(model);

    const f32 width = b.maxX - b.minX;
    const f32 height = b.maxY - b.minY;
    const f32 depth = b.maxZ - b.minZ;

    EXPECT_GT(width, 0.5f);
    EXPECT_GT(height, 0.5f);
    EXPECT_GT(depth, 0.7f);

    EXPECT_LT(width, 0.8f);
    EXPECT_LT(height, 1.1f);
    EXPECT_LT(depth, 1.6f);

    // 回归保护：未调用 setAngles 时会出现“竖直长方体”趋势（depth 过小）
    EXPECT_GT(depth, width * 0.8f);
    EXPECT_LT(height, depth * 1.2f);
}

TEST(AnimalModelMesh, CowReasonableBoundsAndHorizontalBody) {
    CowModel model;
    const Bounds b = buildDefaultPoseBounds(model);

    const f32 width = b.maxX - b.minX;
    const f32 height = b.maxY - b.minY;
    const f32 depth = b.maxZ - b.minZ;

    EXPECT_GT(width, 0.65f);
    EXPECT_GT(height, 0.9f);
    EXPECT_GT(depth, 0.9f);

    EXPECT_LT(width, 0.95f);
    EXPECT_LT(height, 1.65f);
    EXPECT_LT(depth, 1.6f);

    EXPECT_GT(depth, width * 0.9f);
    EXPECT_LT(height, depth * 1.3f);
}

TEST(AnimalModelMesh, SheepReasonableBoundsAndHorizontalBody) {
    SheepModel model;
    const Bounds b = buildDefaultPoseBounds(model);

    const f32 width = b.maxX - b.minX;
    const f32 height = b.maxY - b.minY;
    const f32 depth = b.maxZ - b.minZ;

    EXPECT_GT(width, 0.35f);
    EXPECT_GT(height, 0.75f);
    EXPECT_GT(depth, 0.75f);

    EXPECT_LT(width, 0.7f);
    EXPECT_LT(height, 1.45f);
    EXPECT_LT(depth, 1.5f);

    EXPECT_GT(depth, width * 1.1f);
    EXPECT_LT(height, depth * 1.4f);
}

TEST(AnimalModelMesh, ChickenReasonableBounds) {
    ChickenModel model;
    const Bounds b = buildDefaultPoseBounds(model);

    const f32 width = b.maxX - b.minX;
    const f32 height = b.maxY - b.minY;
    const f32 depth = b.maxZ - b.minZ;

    EXPECT_GT(width, 0.25f);
    EXPECT_GT(height, 0.5f);
    EXPECT_GT(depth, 0.35f);

    EXPECT_LT(width, 0.55f);
    EXPECT_LT(height, 1.0f);
    EXPECT_LE(depth, 0.75f);
}

TEST(AnimalModelMesh, ChickenIncludesHeadBillAndChinInGeneratedMesh) {
    ChickenModel model;
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;

    constexpr f32 kModelScale = 1.0f / 16.0f;
    model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, kModelScale);
    model.generateMesh(vertices, indices, kModelScale);

    const bool hasHeadHeight = std::any_of(vertices.begin(), vertices.end(), [](const ModelVertex& vertex) {
        return vertex.position.y < 0.7f;
    });
    const bool hasBillOrChinForwardDepth = std::any_of(vertices.begin(), vertices.end(), [](const ModelVertex& vertex) {
        return vertex.position.z < -0.45f;
    });

    EXPECT_TRUE(hasHeadHeight);
    EXPECT_TRUE(hasBillOrChinForwardDepth);
}

TEST(AnimalModelMesh, RuntimeMeshUsesMinecraftModelUnits) {
    PigModel model;
    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;

    model.setAngles(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
    model.generateMesh(vertices, indices, 1.0f);

    const Bounds b = computeBounds(vertices);
    const f32 width = b.maxX - b.minX;
    const f32 depth = b.maxZ - b.minZ;

    EXPECT_GT(width, 8.0f);
    EXPECT_GT(depth, 20.0f);
    EXPECT_LT(width, 13.0f);
    EXPECT_LT(depth, 26.0f);
}

TEST(ModelRendererMesh, MirrorReversesXNormalLikeTexturedQuad) {
    mc::client::renderer::entity::model::ModelRenderer mirrored("mirrored");
    mirrored.setMirror(true);
    mirrored.setTextureOffset(0, 0).addBox(0.0f, 0.0f, 0.0f, 2.0f, 2.0f, 2.0f);

    std::vector<ModelVertex> vertices;
    std::vector<u32> indices;
    mirrored.generateMesh(vertices, indices, 1.0f / 16.0f);

    ASSERT_GE(vertices.size(), 4u);
    EXPECT_LT(vertices[0].normal.x, 0.0f);
}

} // namespace mc::client::renderer

