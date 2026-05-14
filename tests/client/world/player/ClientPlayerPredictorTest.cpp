#include <gtest/gtest.h>

#include "client/world/player/ClientPlayerPredictor.hpp"
#include "common/core/Types.hpp"
#include "common/util/math/Vector3.hpp"

using namespace mc;
using namespace mc::client;

/**
 * @brief ClientPlayerPredictor 单元测试
 */
class ClientPlayerPredictorTest : public ::testing::Test {
protected:
    void SetUp() override { predictor = std::make_unique<ClientPlayerPredictor>(); }

    void TearDown() override { predictor.reset(); }

    std::unique_ptr<ClientPlayerPredictor> predictor;
};

TEST_F(ClientPlayerPredictorTest, InitialState)
{
    EXPECT_FALSE(predictor->hasServerPosition());
    EXPECT_EQ(predictor->currentSequence(), 0u);
}

TEST_F(ClientPlayerPredictorTest, Reset)
{
    Vector3 pos(100.0f, 64.0f, 200.0f);
    predictor->reset(pos, 45.0f, 30.0f);

    EXPECT_TRUE(predictor->hasServerPosition());

    auto predictedPos = predictor->predictedPosition();
    EXPECT_FLOAT_EQ(predictedPos.x, 100.0f);
    EXPECT_FLOAT_EQ(predictedPos.y, 64.0f);
    EXPECT_FLOAT_EQ(predictedPos.z, 200.0f);

    auto [yaw, pitch] = predictor->predictedRotation();
    EXPECT_FLOAT_EQ(yaw, 45.0f);
    EXPECT_FLOAT_EQ(pitch, 30.0f);

    // Server position should match
    auto serverPos = predictor->serverPosition();
    EXPECT_FLOAT_EQ(serverPos.x, 100.0f);
    EXPECT_FLOAT_EQ(serverPos.y, 64.0f);
    EXPECT_FLOAT_EQ(serverPos.z, 200.0f);
}

TEST_F(ClientPlayerPredictorTest, ResetClearsPendingInputs)
{
    // Generate some inputs
    predictor->handleMovementInput(1.0f, 0.0f, false, false);
    predictor->handleMovementInput(0.0f, 1.0f, true, false);
    predictor->handleMovementInput(1.0f, 1.0f, false, true);

    EXPECT_EQ(predictor->currentSequence(), 3u);

    // Reset should clear inputs
    Vector3 pos(0.0f, 0.0f, 0.0f);
    predictor->reset(pos, 0.0f, 0.0f);

    EXPECT_EQ(predictor->currentSequence(), 0u);
}

TEST_F(ClientPlayerPredictorTest, MovementInputUpdatesSequence)
{
    predictor->handleMovementInput(1.0f, 0.0f, false, false);
    EXPECT_EQ(predictor->currentSequence(), 1u);

    predictor->handleMovementInput(0.0f, 1.0f, false, false);
    EXPECT_EQ(predictor->currentSequence(), 2u);

    predictor->handleMovementInput(-1.0f, 0.0f, true, false);
    EXPECT_EQ(predictor->currentSequence(), 3u);
}

TEST_F(ClientPlayerPredictorTest, RotationInput)
{
    predictor->handleRotationInput(10.0f, 5.0f);

    auto [yaw, pitch] = predictor->predictedRotation();
    EXPECT_FLOAT_EQ(yaw, 10.0f);
    EXPECT_FLOAT_EQ(pitch, 5.0f);

    // Another rotation
    predictor->handleRotationInput(5.0f, -2.0f);
    auto [yaw2, pitch2] = predictor->predictedRotation();
    EXPECT_FLOAT_EQ(yaw2, 15.0f);
    EXPECT_FLOAT_EQ(pitch2, 3.0f);
}

TEST_F(ClientPlayerPredictorTest, PitchClamping)
{
    // Pitch should be clamped to [-90, 90]
    predictor->handleRotationInput(0.0f, 100.0f);
    auto [yaw1, pitch1] = predictor->predictedRotation();
    EXPECT_FLOAT_EQ(pitch1, 90.0f);

    predictor->handleRotationInput(0.0f, -200.0f);
    auto [yaw2, pitch2] = predictor->predictedRotation();
    EXPECT_FLOAT_EQ(pitch2, -90.0f);
}

TEST_F(ClientPlayerPredictorTest, YawNormalization)
{
    // Yaw should be normalized to [-180, 180]
    predictor->handleRotationInput(200.0f, 0.0f);
    auto [yaw1, pitch1] = predictor->predictedRotation();
    EXPECT_FLOAT_EQ(yaw1, -160.0f);

    predictor->handleRotationInput(-370.0f, 0.0f);
    auto [yaw2, pitch2] = predictor->predictedRotation();
    // -160 + (-370) = -530, normalized to -530 + 360 = -170
    EXPECT_FLOAT_EQ(yaw2, -170.0f);
}

TEST_F(ClientPlayerPredictorTest, ReceiveServerPosition)
{
    // Reset first
    Vector3 initialPos(0.0f, 64.0f, 0.0f);
    predictor->reset(initialPos, 0.0f, 0.0f);

    // Receive server position
    Vector3 serverPos(100.0f, 64.0f, 100.0f);
    predictor->receiveServerPosition(serverPos, 0.0f, 0.0f);

    EXPECT_TRUE(predictor->hasServerPosition());

    auto receivedServerPos = predictor->serverPosition();
    EXPECT_FLOAT_EQ(receivedServerPos.x, 100.0f);
    EXPECT_FLOAT_EQ(receivedServerPos.y, 64.0f);
    EXPECT_FLOAT_EQ(receivedServerPos.z, 100.0f);
}

TEST_F(ClientPlayerPredictorTest, SmallCorrectionThreshold)
{
    // Set small threshold
    predictor->setCorrectionThreshold(0.01f);

    Vector3 initialPos(0.0f, 64.0f, 0.0f);
    predictor->reset(initialPos, 0.0f, 0.0f);

    // Small position difference should trigger correction
    Vector3 serverPos(0.05f, 64.0f, 0.05f);
    predictor->receiveServerPosition(serverPos, 0.0f, 0.0f);

    // Prediction should be corrected to server position
    auto predictedPos = predictor->predictedPosition();
    EXPECT_FLOAT_EQ(predictedPos.x, 0.05f);
    EXPECT_FLOAT_EQ(predictedPos.y, 64.0f);
    EXPECT_FLOAT_EQ(predictedPos.z, 0.05f);
}

TEST_F(ClientPlayerPredictorTest, LargeCorrectionTriggersSmoothing)
{
    // Set large threshold
    predictor->setCorrectionThreshold(1.0f);

    Vector3 initialPos(0.0f, 64.0f, 0.0f);
    predictor->reset(initialPos, 0.0f, 0.0f);

    // Large position difference
    Vector3 serverPos(100.0f, 64.0f, 100.0f);
    predictor->receiveServerPosition(serverPos, 0.0f, 0.0f);

    // After receiving, predicted position should start correction
    auto predictedPos = predictor->predictedPosition();
    // Correction should jump to server position
    EXPECT_FLOAT_EQ(predictedPos.x, 100.0f);
}

TEST_F(ClientPlayerPredictorTest, TickUpdatesPrediction)
{
    Vector3 initialPos(0.0f, 64.0f, 0.0f);
    predictor->reset(initialPos, 0.0f, 0.0f);
    predictor->setMovementSpeed(10.0f);

    // Move forward
    predictor->handleMovementInput(1.0f, 0.0f, false, false);

    // Tick should update prediction
    predictor->tick(0.05f);

    auto predictedPos = predictor->predictedPosition();
    // Should have moved forward (Z direction based on yaw=0)
    EXPECT_GT(predictedPos.z, 0.0f);
}

TEST_F(ClientPlayerPredictorTest, AcknowledgeInput)
{
    predictor->handleMovementInput(1.0f, 0.0f, false, false);
    predictor->handleMovementInput(0.0f, 1.0f, false, false);
    predictor->handleMovementInput(-1.0f, 0.0f, false, false);

    EXPECT_EQ(predictor->currentSequence(), 3u);

    // Acknowledge up to sequence 2
    predictor->acknowledgeInput(2);

    // One input should remain (sequence 3)
    EXPECT_EQ(predictor->currentSequence(), 3u);
}

TEST_F(ClientPlayerPredictorTest, SetMovementSpeed)
{
    predictor->setMovementSpeed(100.0f);
    // Movement speed is applied in prediction updates
    // No direct getter, so we verify through tick behavior
    EXPECT_NO_THROW(predictor->tick(0.016f));
}

TEST_F(ClientPlayerPredictorTest, SetCorrectionThreshold)
{
    predictor->setCorrectionThreshold(0.5f);
    // Threshold affects when correction is triggered
    EXPECT_NO_THROW(predictor->tick(0.016f));
}

TEST_F(ClientPlayerPredictorTest, ClearPendingInputs)
{
    predictor->handleMovementInput(1.0f, 0.0f, false, false);
    predictor->handleMovementInput(0.0f, 1.0f, false, false);
    predictor->handleMovementInput(-1.0f, 0.0f, false, false);

    EXPECT_EQ(predictor->currentSequence(), 3u);

    predictor->clearPendingInputs();

    EXPECT_EQ(predictor->currentSequence(), 0u);
}

TEST_F(ClientPlayerPredictorTest, DiagonalMovement)
{
    Vector3 initialPos(0.0f, 64.0f, 0.0f);
    predictor->reset(initialPos, 0.0f, 0.0f);
    predictor->setMovementSpeed(10.0f);

    // Diagonal movement
    predictor->handleMovementInput(1.0f, 1.0f, false, false);
    predictor->tick(0.05f);

    auto predictedPos = predictor->predictedPosition();
    // Both X and Z should change for diagonal movement with yaw=0
    EXPECT_GT(std::abs(predictedPos.x), 0.0f);
    EXPECT_GT(std::abs(predictedPos.z), 0.0f);
}
