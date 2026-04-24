#include "ZombieModel.hpp"
#include <cmath>

namespace mc::client::renderer::entity::model::monster {

namespace {
    constexpr f64 PI = 3.14159265359;
    constexpr f64 DEG_TO_RAD = PI / 180.0;
}

ZombieModel::ZombieModel()
    : BipedModel()
{
    // 僵尸使用标准玩家模型纹理尺寸
    // 但有些僵尸使用 64x32（普通僵尸），有些使用 64x64（尸壳、溺尸）
    setTextureSize(64, 64);
    setupParts();
}

void ZombieModel::setupParts() {
    // 僵尸的部件尺寸与玩家相同
    // 由 BipedModel 基类设置
    // 参考 MC 1.16.5 AbstractZombieModel -> BipedModel
}

void ZombieModel::setAngles(f64 limbSwing, f64 limbSwingAmount,
                             f64 ageInTicks, f64 netHeadYaw,
                             f64 headPitch, f64 scale) {
    // 调用基类设置基础动画
    BipedModel::setAngles(limbSwing, limbSwingAmount, ageInTicks, netHeadYaw, headPitch, scale);

    // 参考 MC 1.16.5 AbstractZombieModel.setRotationAngles
    // 僵尸特有：手臂向前伸，攻击时有摆动动画
    // ModelHelper.func_239105_a_ 是攻击动画辅助函数

    // 僵尸手臂基础角度：向前伸 -90 度（-PI/2）
    // 这在 BipedModel 中已经处理，但僵尸需要额外调整

    // 当僵尸攻击时，手臂会有特殊的摆动动画
    // 这是通过 swingProgress 参数控制的
    // 目前简化实现，仅保持手臂向前伸

    (void)ageInTicks;  // 暂时未使用
}

} // namespace mc::client::renderer::entity::model::monster
