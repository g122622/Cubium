// 疣猪兽（Zoglin）行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { assertEntityInVolume } from "../../../utils/entity/assert.js";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// creeper_pit 结构尺寸 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };
// grass_pen 结构尺寸 9×5×9（helper 相对坐标 x,z∈[0,8], y∈[0,4]）：
//   结构 y=0（helper y=1）：满铺 grass_block 地板
//   结构 y=1..2（helper y=2,3）：外圈玻璃墙 + 内部 7×7 air 腔（x,z∈[1,7]）
//   结构 y=3..4（helper y=4,5）：外圈玻璃墙 + 内部 air 腔
// 玻璃墙物理限制 zoglin/猪击退后留在区域内，消除开放坑击飞出区域致 pigX=n/a 的失败。
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// Shulker 的攻击会使 Zoglin 浮空至笼顶。
function zoglinFloat(test: Test): void {
  const zoglinType = "zoglin";
  const shulkerType = "shulker";

  test.spawn(zoglinType, { x: 5, y: 2, z: 5 });
  test.spawn(shulkerType, { x: 2, y: 2, z: 2 });

  test.succeedWhen(() => {
    // zoglin 是否已浮至笼顶？
    assertEntityInVolume(test, zoglinType, 1, 7, 1, 10, 10, 10);
  });
}

// 僵尸疣兽近战攻击非玩家目标（猪）并造成击退（wiki mob_疣猪兽_ED.txt#行为：僵尸疣兽对几乎所有生物敌对，
// 攻击会将目标击飞）。
//
// C++ 链路：ZoglinEntity::attackEntityAsMob override 自管完整攻击链（动画+随机化伤害+flingTarget+音效），
// 由 MeleeAttackGoal::_attackTarget 委托调用。历史上 Zoglin 把专用攻击逻辑放在未被调用的 attackLivingTarget
// （死代码），攻击退化为基类固定伤害无动画——改为 override attackEntityAsMob 后修复。
//
// 目标选择：Zoglin 成年对几乎所有 LivingEntity 敌对（NearestAttackableTargetGoal<LivingEntity>，
// 排除 Zoglin/Creeper），故用猪作为攻击目标（区别于 Hoglin 仅攻击玩家）。
//
// 判定手段：猪被击退移离原地 >1 格（水平位移）。注意本测试对"fling override vs 基类 causeExtraKnockback"
// 无判别性——基类 MobEntity::attackEntityAsMob 也有 ATTACK_KNOCKBACK=1.0 击退会让猪位移（回退验证：override
// 改走基类仍 PASS）。本测试的真正价值是：(1) 验证 Zoglin 对非玩家目标敌对选择（NearestAttackableTargetGoal<
// LivingEntity>），(2) 验证 MeleeAttackGoal→attackEntityAsMob 攻击链路通（猪被命中+击退）。
// flingTarget 抛飞的 velocity 应用由单元测试 FlingingSupportTypesTest 验证（直接断言 target.velocity 变化）。
// override 的伤害随机化/动画等差异集成测试难以判别，由单元测试覆盖。
//
// 环境选择：grass_pen（9×5×9 玻璃围栏，内部 7×7 air 腔 x,z∈[1,7]，y=1 满铺 grass_block 地板，
// y=2,3 外圈玻璃墙+内部 air，y=4 露天顶层 air）。ZoglinEntity 构造未调 setBurnsInDaylight(false)，
// 继承 MonsterEntity 默认 shouldBurnInDaylight=true，白天露天会燃——燃烧伤害会干扰击退判定（猪可能因
// Zoglin 燃烧传递着火掉血而非被击退）。故用 night batch 避免燃烧（night batch 下环境为夜晚，不燃）。
//
// 为何用 grass_pen 玻璃围栏而非 creeper_pit 开放坑：flingTarget 给猪施加 0.5 水平 + 0.25 垂直速度
// （ATTACK_KNOCKBACK=1.0，猪 KNOCKBACK_RESISTANCE=0 → knockbackStrength=1.0），把猪抛飞。creeper_pit
// 是 7×7 开放坑无围墙，猪被抛飞后超出 PIT_VOLUME 查询范围（飞出坑落到坑外草地）→ getEntities 返回空
// → pigX=n/a 超时失败。grass_pen 玻璃墙物理限制猪在 9×9 围栏内，抛飞后撞墙留在 PEN_VOLUME 查询范围内。
// 玻璃对 zoglin checkSight 透明（视线穿透），不影响目标选择与 MeleeAttackGoal 攻击。
//
// Zoglin (2,2,3) + 猪 (3,2,3) 紧邻 1 格。zoglin 在猪 -x 方向，flingTarget direction=(+x,0,0) 推猪 +x。
// grass_pen helper y=1 是 grass 地板，实体 spawn 在 helper y=2 站在 grass 上，无需 glass 支撑。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\mob_疣猪兽_ED.txt#行为（敌对攻击+击飞目标）
function zoglinKnocksBackTarget(test: Test): void {
  const zoglinType = "zoglin";
  const pigType = "pig";
  const pigInitX = 3;

  // zoglin (2,2,3) + 猪 (3,2,3)，紧邻 1 格。zoglin 在猪 -x 方向，击退推猪 +x。
  // grass_pen helper y=1 满铺 grass_block 地板，实体 spawn 在 helper y=2 站在 grass 上，无需额外支撑。
  test.spawn(zoglinType, { x: 2, y: 2, z: 3 });
  test.spawn(pigType, { x: 3, y: 2, z: 3 });

  // 轮询断言猪被水平击退移离原地 >1 格。zoglin 选目标 + 首攻冷却 20 tick，命中即 flingTarget。
  // 猪被击退后会来回弹（反复攻击），单次采样可能恰好回原位，故 pollUntilSucceed 多次采样取任一时刻位移。
  // grass_pen 玻璃墙限制猪在 PEN_VOLUME(9×9) 内，击退后猪撞墙留在查询范围内（不致 pigX=n/a）。
  // startTick=30 留 spawn + 选目标 + 首攻时间，maxTick=200 留余量。
  pollUntilSucceed(test, () => {
    const pigs = test.getDimension().getEntities({
      type: pigType,
      location: test.worldLocation(PEN_FROM),
      volume: PEN_VOLUME,
    });
    if (pigs.length === 0) return false;
    return Math.abs(pigs[0].location.x - pigInitX) > 1.0;
  }, {
    startTick: 30,
    interval: 5,
    maxTick: 200,
    onTimeout: () => {
      const pigs = test.getDimension().getEntities({
        type: pigType,
        location: test.worldLocation(PEN_FROM),
        volume: PEN_VOLUME,
      });
      const x = pigs.length > 0 ? pigs[0].location.x.toFixed(2) : "n/a";
      test.assert(false,
        `zoglin did not knock back pig (flingTarget not called in attackEntityAsMob), pigX=${x} (init=${pigInitX})`);
    },
  });
}

export function registerZoglinTests(): void {
  GameTest.register("MobBehaviorTests", "zoglin_float", zoglinFloat)
    .batch("night")
    .structureName("gametests:mediumglass")
    .maxTicks(210);

  GameTest.register("MobBehaviorTests", "zoglin_knocks_back_target", zoglinKnocksBackTarget)
    .batch("night")
    .structureName("gametests:grass_pen")
    .maxTicks(250);
}
