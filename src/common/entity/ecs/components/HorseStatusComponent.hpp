#pragma once

#include "common/core/Types.hpp"

namespace mc::ecs {

/**
 * @brief 马类状态标志组件（C 类同步字段）
 *
 * 承载 AbstractHorseEntity 的 6 个状态布尔（tame/saddled/bred/eating/rearing/mouthOpen）。
 * 仅 AbstractHorseEntity 子树 attach（Horse/Donkey/Mule/Llama/TraderLlama/SkeletonHorse/
 * ZombieHorse 共用，经基类构造自动获得）。
 *
 * 字段分类：C 类同步字段——组件为业务真相源，AbstractHorseEntity::_syncStatusFlags() 读
 * 6 bool 组合成 i8 写入 STATUS_PARAM DataParameter 镜像下发客户端。客户端 ClientEntity
 * 解析 STATUS_PARAM 6 bit 写入自有 horse 镜像成员（ClientEntity 不持 ECS 组件，见
 * ecs/README.md 坑10）。
 *
 * 持久化：tame/bred/saddled/eating 进 NBT（Tame/Bred/Saddle/EatingHaystack），
 * rearing/mouthOpen 为运行时动画状态不存盘。序列化器走 setter 触发 STATUS_PARAM 镜像
 * 副作用（C 类硬约束，见 HorseComponentSerialization）。
 *
 * 对齐 vanilla 1.21.11 AbstractHorse.DATA_FLAGS_ID（id 16，i8 位标志）：
 *   bit1(tame)/bit2(saddle)/bit3(bred)/bit4(eating)/bit5(rearing)/bit6(mouthOpen)。
 */
struct HorseStatusComponent {
    bool m_tame{false};      ///< 已驯服
    bool m_saddled{false};   ///< 已装备鞍
    bool m_bred{false};      ///< 已繁殖
    bool m_eating{false};    ///< 正在吃草
    bool m_rearing{false};   ///< 正在扬蹄
    bool m_mouthOpen{false}; ///< 嘴巴张开
};

} // namespace mc::ecs
