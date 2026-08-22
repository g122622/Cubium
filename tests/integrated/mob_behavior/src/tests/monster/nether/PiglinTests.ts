// 猪灵行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { ItemStack } from "@minecraft/server";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// creeper_pit / glass_pit 结构尺寸均为 7×5×7（helper 相对坐标 x,z∈[0,6], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构尺寸。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PIT_FROM = { x: 0, y: 0, z: 0 };
const PIT_VOLUME = { x: 7, y: 5, z: 7 };

// 猪灵主动攻击未穿金甲的玩家（wiki tech_猪灵.txt#敌对性：成年猪灵主动攻击未穿戴金装备的玩家）。
//
// C++ 链路：PiglinEntity::registerGoals（NetherEntities.cpp:509-519）targetSelector 优先级2 注册
// NearestAttackableTargetGoal<Player>，谓词 !PiglinAi::isWearingGold(player)（:517）。
//   - PiglinAi::isWearingGold 委托 Player::isWearingGoldArmor（Player.cpp:3007-3038），遍历四护甲槽，
//     dynamic_cast ArmorItem 比较材质 == ArmorMaterials::GOLD，任一金甲件返 true。
//   - 未穿金甲玩家：谓词 !false=true → 被选为 attackTarget。
//   - 穿金甲玩家：谓词 !true=false → 不被选为 attackTarget（见 piglin_ignores_player_wearing_gold）。
//
// 猪灵攻击链路：test.spawn 的猪灵主手无弩（构造不补装备），RangedCrossbowAttackGoal::shouldExecute
// 检查 _isHoldingCrossbow() 返 false 不执行（RangedAttackGoals.cpp:345），MeleeAttackGoal（优先级3）接管。
// 玩家被设为 attackTarget 后，MeleeAttackGoal 寻路接近 + 攻击，玩家 HP 降。
//
// 猪灵属性：MAX_HEALTH=16，MOVEMENT_SPEED=0.35，ATTACK_DAMAGE=5.0，FOLLOW_RANGE=16.0（NetherEntities.cpp:526-529）。
// 玩家紧邻 1 格在 FOLLOW_RANGE=16 内，NearestAttackableTargetGoal 选目标无延迟。
//
// 猪灵不僵尸化：AbstractPiglinEntity 无 tick override，主世界不转化（僵尸化机制未实现），主世界安全。
// 猪灵不日光燃烧：AbstractPiglinEntity setBurnsInDaylight(false)，无需 batch("night")。
//
// 判定手段：轮询断言玩家 HP<20（猪灵近战攻击造伤害 5）。攻击链路约 tick 20-40（选目标+寻路1格+攻击），
// maxTick=300 留余量。用 pollUntilSucceed（正向断言 HP 降，条件满足即 succeed）。
// Survival 玩家（gameMode=0）：创造/旁观玩家被 TargetGoal::isSuitableTarget 滤掉，必须 Survival。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_猪灵.txt#敌对性（攻击未穿金甲玩家）
function piglinAttacksPlayerWithoutGold(test: Test): void {
  const piglinType = "piglin";

  // 猪灵 (3,2,3)，Survival 玩家 (4,2,3)（直线 1 格，紧邻）。玩家未穿金甲，被猪灵选为 attackTarget。
  test.spawn(piglinType, { x: 3, y: 2, z: 3 });
  test.spawnSimulatedPlayer({ x: 4, y: 2, z: 3 }, "bait", 0 as any);

  // 轮询断言玩家 HP<20（猪灵攻击造伤害 5）。攻击约 tick 20-40，maxTick=300 留余量。
  pollUntilSucceed(test, () => {
    const players = test.getDimension().getEntities({
      type: "minecraft:player",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    if (players.length === 0) return false;
    const health = players[0].getComponent("minecraft:health");
    if (health === undefined) return false;
    return (health as any).currentValue < 20;
  }, {
    startTick: 20,
    interval: 10,
    maxTick: 300,
    onTimeout: () => {
      const players = test.getDimension().getEntities({
        type: "minecraft:player",
        location: test.worldLocation(PIT_FROM),
        volume: PIT_VOLUME,
      });
      const hp = players.length > 0
        ? (players[0].getComponent("minecraft:health") as any)?.currentValue
        : "player gone";
      test.assert(false,
        `piglin did not attack player without gold armor (isWearingGold predicate broken), player hp=${hp}`);
    },
  });
}

// 猪灵不攻击穿金甲的玩家（wiki tech_猪灵.txt#敌对性：穿戴金装备的玩家不被猪灵主动攻击）。
//
// 本测试是 piglin_attacks_player_without_gold 的**对照组**，交叉验证 isWearingGold 检测真正生效：
//   - 穿金甲玩家：NearestAttackableTargetGoal 谓词 !isWearingGold(player)=!true=false → 不选目标，
//     猪灵不攻击，玩家 HP 恒 20。本测试断言此行为。
//   - 未穿金甲玩家：谓词 true → 被选目标，猪灵攻击，HP 降（piglin_attacks_player_without_gold 验证）。
//
// 两测试互补：若 isWearingGold 检测失效（恒 false），穿金甲玩家也被攻击，本测试 FAIL；
// 若 isWearingGold 恒 true，未穿金甲玩家也不被攻击，对照测试 FAIL。交叉验证门控正确。
//
// 金甲穿戴链路：JS equippable.setEquipment 已支持穿戴 ItemStack（MinecraftModuleFactory.cpp
// setEquipment unwrap 路径，见 MobEquipmentDropTests），但对【玩家】装备仍优先用 SimulatedPlayer::setItem
// 直接写 PlayerInventory 装备槽——因 /replaceitem 命令对 SimulatedPlayer 失效：SimulatedPlayer 故意不注册到
// PlayerManager（SimulatedPlayer.cpp:52-53，无网络会话避免 keepalive/广播副作用），而 /replaceitem / /gamemode
// 的 @s 选择器经 resolvePlayerIds → PlayerManager.getPlayer 查不到（GameModeManager 报 "Player N not found"，
// 命令静默失败）。故用 SimulatedPlayer::setItem JS API 直接写 PlayerInventory 装备槽，绕过命令系统：
//   player.setItem(new ItemStack("minecraft:golden_chestplate", 1), 37)
// SimulatedPlayer::setItem（SimulatedPlayer.cpp:194-204）调 inventory().setItem(37, stack)，槽 37 即
// ARMOR_CHEST（Slot.hpp）。Player::getEquipment(Chest)（Player.cpp:1350）读 m_inventory.getChestplateRef()
// = m_items[37]，与 setItem 写入同槽，isWearingGoldArmor 能读到。setItem 是 JS API 直接调 C++，无权限检查，
// Survival 玩家也可用（不像 /replaceitem 需 permLevel=2）。
//
// 故玩家直接以 Survival 创建（gameMode=0），用 setItem 穿金甲，无需创造切换。Survival 玩家才能被
// TargetGoal 选为有效目标（创造/旁观被 isSuitableTarget 滤掉，无法测金甲门控）。
//
// 判定手段：玩家穿金甲后，等完整攻击窗口过断言 HP 恒 20（负向断言）。
// 猪灵若误攻击（isWearingGold 失效），HP 必降（伤害 5）。用 runAtTickTime 而非 pollUntilSucceed：
// 负向断言须等完整窗口，pollUntilSucceed 的"条件满足即 succeed"会在首检 HP=20 立即 succeed 漏判延迟攻击。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_猪灵.txt#敌对性（金甲玩家不被攻击）
function piglinIgnoresPlayerWearingGold(test: Test): void {
  const piglinType = "piglin";

  // 猪灵 (3,2,3)。Survival 玩家 (4,2,3)（gameMode=0，紧邻 1 格）。
  test.spawn(piglinType, { x: 3, y: 2, z: 3 });
  const player = test.spawnSimulatedPlayer({ x: 4, y: 2, z: 3 }, "bait", 0 as any);

  // 立即（tick 0，测试函数体同步执行）穿金胸甲：在猪灵首 tick AI 评估前完成穿戴。
  // 关键时序：若延迟穿甲（如 tick 5），猪灵可能在 tick 0-4 的 NearestAttackableTargetGoal 评估中
  // 选未穿金甲玩家为 attackTarget，之后即便穿金甲，shouldContinueExecuting 不重检 isWearingGold 谓词，
  // 猪灵保持目标继续攻击，导致测试偶发失败。tick 0 立即穿甲确保猪灵首次评估时玩家已是金甲状态。
  // setItem 写金胸甲到槽 37（ARMOR_CHEST）→ isWearingGoldArmor 返 true → 猪灵谓词 !true=false 不选目标。
  const goldenChestplate = new ItemStack("minecraft:golden_chestplate", 1);
  player.setItem(goldenChestplate as unknown as Parameters<typeof player.setItem>[0], 37);

  // tick 300 断言玩家 HP 仍满血 20（猪灵未攻击）。
  // 猪灵若误攻击（isWearingGold 失效或 setItem 未生效），约 tick 25-45 玩家 HP 降。
  // 等 300 tick 远超攻击时序，HP 仍 20 即证明金甲门控全程生效。
  test.runAtTickTime(300, () => {
    const players = test.getDimension().getEntities({
      type: "minecraft:player",
      location: test.worldLocation(PIT_FROM),
      volume: PIT_VOLUME,
    });
    test.assert(players.length > 0, "player disappeared");
    const health = players[0].getComponent("minecraft:health");
    test.assert(health !== undefined, "player has no health component");
    const hp = (health as any).currentValue;
    test.assert(hp >= 20,
      `piglin attacked player wearing gold armor (isWearingGold predicate broken or setItem ineffective), player hp=${hp}`);
    test.succeed();
  });
}

export function registerPiglinTests(): void {
  GameTest.register("MobBehaviorTests", "piglin_attacks_player_without_gold", piglinAttacksPlayerWithoutGold)
    .structureName("gametests:creeper_pit")
    .maxTicks(350);

  GameTest.register("MobBehaviorTests", "piglin_ignores_player_wearing_gold", piglinIgnoresPlayerWearingGold)
    .structureName("gametests:creeper_pit")
    .maxTicks(350);
}
