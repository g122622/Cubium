// 哞菇行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";

// creeper_pit 结构尺寸（7×5×7），helper 相对坐标。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 哞菇被闪电劈中红↔棕变种翻转（wiki tech_哞菇.txt#闪电：红色哞菇被雷击变棕色，棕色被雷击变红色）。
//
// C++ 链路：LightningBoltEntity::_damageEntities 对命中范围(±3 XZ)内实体先 hurt(5.0) 再调
// entity->onStruckByLightning()（EffectEntities.cpp:509-511）。MooshroomEntity::onStruckByLightning
// （MooshroomEntity.cpp:366-396）调 setMooshroomType(isRed() ? Brown : Red) 翻转变种 + 播放 convert
// 音效 + 客户端粒子。无难度门控（转化产物仍是哞菇，无和平消失问题，区别于 PigEntity 转化需非 Peaceful）。
// 哞菇 10 血，闪电伤害 5，存活 5 血，转化当 tick 完成，实体仍在（不 remove、不换 typeId）。
//
// 判定手段：读 minecraft:mark_variant 组件（Cubium 绑定 MinecraftModuleFactory.cpp getComponent，
// 对 MooshroomEntity 返回 MarkVariantComponent，readonly value 为 MooshroomType 枚举值 Red=0/Brown=1，
// 与 NBT "Type" 字段一致）。
//
// 对照组设计：生成两只哞菇——实验组 (2,2,3) 与闪电同格被劈（应翻转为棕色 value=1），
// 对照组 (5,2,5) 远离闪电（距实验组 >3 命中范围，未被劈，保持红色 value=0）。
// 双断言组合：实验组棕色 + 对照组红色，精确验证"被劈才翻转"——对照组排除 mark_variant 组件默认恒为
// 某值（如恒 1）的假通过，实验组排除默认棕色的假通过。两组均存活（闪电仅命中实验组范围）。
//
// 环境选择：creeper_pit（7×5×7 开放坑）y=0 grass_block 脚踩，无围墙不影响闪电命中。
// 实验组 (1,2,1) 与闪电同位，闪电 ±3 XZ 命中范围（DAMAGE_RADIUS_XZ=3.0，AABB [pos-3,pos+3]）
// 覆盖；对照组 (5,2,5) 距闪电 (1,2,1) 的 X/Z 均超出 ±3（闪电 X 范围[-2,4]，对照组 x=5>4 出界；
// 闪电 Z 范围[-2,4]，对照组 z=5>4 出界），确保对照组未被劈。
// 时序：闪电首 tick 即 _damageEntities → hurt(5) + onStruckByLightning → setMooshroomType(Brown)。
// 转化当 tick 完成。用 runAtTickTime 延迟若干 tick（待闪电触发 + 实体查询就绪）后断言。
// maxTicks=200 留闪电生成 + 首 tick 触发 + 余量。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_哞菇.txt#闪电（红↔棕变种翻转）
function mooshroomLightningConvert(test: Test): void {
  const mooshroomType = "mooshroom";
  const lightningType = "lightning_bolt";

  // 实验组 (1,2,1) 与闪电同位被劈；对照组 (5,2,5) 距闪电 X/Z 均超出 ±3 未被劈。
  // 两组脚踩结构内 y=0 grass_block（helper-y=2→结构内 y=1 空气）。
  const struck = test.spawn(mooshroomType, { x: 1, y: 2, z: 1 });
  const control = test.spawn(mooshroomType, { x: 5, y: 2, z: 5 });
  test.spawn(lightningType, { x: 1, y: 2, z: 1 });

  // 断言：实验组翻转为棕色（value=1）+ 对照组保持红色（value=0），两组均存活。
  // 用 spawn 返回的引用读组件——变种是哞菇自身状态，不依赖坐标查询，引用稳定。
  // runAtTickTime(20) 等待闪电首 tick 触发 + 实体查询就绪后断言并 succeed。
  test.runAtTickTime(20, () => {
    test.assertEntityPresentInArea(mooshroomType, true);

    const struckVariant = struck.getComponent("minecraft:mark_variant");
    test.assert(struckVariant !== undefined, "struck mooshroom has no mark_variant component");
    test.assert((struckVariant as any).value === 1,
      `struck mooshroom did not convert to brown, value=${(struckVariant as any).value}`);

    const controlVariant = control.getComponent("minecraft:mark_variant");
    test.assert(controlVariant !== undefined, "control mooshroom has no mark_variant component");
    test.assert((controlVariant as any).value === 0,
      `control mooshroom was not red, value=${(controlVariant as any).value}`);

    test.succeed();
  });
}

export function registerMooshroomTests(): void {
  GameTest.register("MobBehaviorTests", "mooshroom_lightning_convert", mooshroomLightningConvert)
    .structureName("gametests:creeper_pit")
    .maxTicks(200);
}
