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

// open_grass_hall 结构尺寸（41×7×9），helper 相对坐标。四壁 glass 墙 + 内部/顶部全 air 露天草地长廊，
// y=0 grass_block 地板。猪灵逃避灵魂营火用：41 格长度容纳 AvoidBlockGoal ESCAPE_HORIZONTAL_RANGE=16 的逃跑位移，
// 玻璃墙在边界阻止猪灵跑出查询区域。creeper_pit(7×7) 太小，猪灵逃出 12 格安全距离即出结构边界。
const HALL_FROM = { x: 0, y: 0, z: 0 };
const HALL_VOLUME = { x: 41, y: 7, z: 9 };

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

// 猪灵惧怕灵魂营火并主动远离（wiki tech_猪灵.txt#其他：非敌对状态的猪灵会主动远离水平半径8格、
// 垂直半径4格范围内的灵魂火、灵魂火把、灵魂灯笼和灵魂营火）。
//
// C++ 链路：PiglinEntity::registerGoals（NetherEntities.cpp:487-496）goalSelector 优先级4 注册
// AvoidBlockGoal(this, BlockTags::PIGLIN_REPELLENTS(), 1.0, horizontalRange=8, verticalRange=4, validator)。
//   - PIGLIN_REPELLENTS 标签（BlockTags.cpp:1678-1684）含 soul_fire/soul_torch/soul_wall_torch/
//     soul_lantern/soul_campfire。validator 对 soul_campfire 额外检查 isLitCampfire（未点燃不排斥）。
//   - AvoidBlockGoal::shouldExecute（AvoidBlockGoal.cpp:74-92）：_findNearestRepellent 在猪灵当前
//     位置 ±8(水平)/±4(垂直) 范围搜索最近排斥方块 → _findEscapePosition 用
//     RandomPositionGenerator::findRandomTargetBlockAwayFrom(ESCAPE_HORIZONTAL_RANGE=16, ESCAPE_VERTICAL_RANGE=7)
//     计算逃跑位（_isEscapePositionValid 保证逃跑位比当前位置更远离排斥方块）→ nav 可用即执行。
//   - startExecuting/tick：navigator->moveTo(逃跑位, speed=1.0)，猪灵朝远离灵魂营火方向移动。
//   - shouldContinueExecuting（AvoidBlockGoal.cpp:94-124）：猪灵距排斥方块 3D 距离 > horizontalRange(=8)
//     时停止逃跑。对齐 MC 1.21.11 SetWalkTargetAwayFrom.pos(NEAREST_REPELLENT, 1.0F, 8, false) 的
//     desiredDistance=8 语义：距排斥方块 >=8 即停止逃离（原版 avoidRepellent() 第三个参数 8）。
//     故逃跑成功必然使猪灵距灵魂营火 >8 格（逃出检测范围）。
//
// 原版语义对照（PiglinAi.java:290-292 + SetWalkTargetAwayFrom.java）：
//   avoidRepellent() = SetWalkTargetAwayFrom.pos(NEAREST_REPELLENT, 1.0F, 8, false)
//   - 距排斥方块 <8 时每 tick 用 LandRandomPos.getPosAway(mob,16,7,repellentPos) 设新 WalkTarget（持续逃离）。
//   - 距 >=8 时不再设新目标，旧 WalkTarget 执行完生物停止。
//   Cubium AvoidBlockGoal 用 Goal 系统等效：shouldContinueExecuting 距 >=horizontalRange(8) 停止，
//   tick() 在路径结束且仍在 <8 格内时重设逃跑位继续逃。语义对齐。
//
// 优先级分析：AvoidBlockGoal(优先级4, 占 Move flag) > WaterAvoidingRandomWalkingGoal(优先级7)。
//   不 spawn 玩家→猪灵无 attackTarget（NearestAttackableTargetGoal<Player> 优先级2 不选目标）→
//   AvoidBlockGoal 是最高可执行 goal，独占 Move flag 驱动逃跑，RandomWalking 被抢占不干扰。
//   猪灵在主世界不僵尸化（僵尸化机制未实现，见 PiglinTests 同款注释），主世界测试安全。
//
// 环境选择：open_grass_hall（41×7×9 露天草地长廊，四壁玻璃墙）。
//   1. 41 格长度容纳逃跑位移：AvoidBlockGoal 逃跑位 ESCAPE_HORIZONTAL_RANGE=16，猪灵从 x=20 可逃到 x=4
//      或 x=36（距营火 x=22 最远 14-18 格 >8 检测范围），均在玻璃墙内（墙在 x=0/40）。
//      creeper_pit(7×7) 太小——猪灵逃出 8 格检测范围即接近结构边界，断言余量不足。
//   2. 玻璃墙阻止猪灵跑出 41×9 查询区域（开放坑无墙猪灵会跑出区域致 piglin=0 假失败）。
//   3. 灵魂营火须放在猪灵 ≤8 格内触发 shouldExecute。营火放 x=22（猪灵 x=20 旁 2 格）。
//   4. 灵魂营火需点燃状态（validator 检查 isLitCampfire，默认放置即为点燃）。
//      灵魂营火需 soul_sand/soul_soil 基座？——否，soul_campfire 可直接放置在任意方块上（基座仅影响
//      是否持续燃烧，放置时 isLitCampfire=true 即触发排斥）。
//
// 判定手段：pollUntilSucceed 每 4 tick 查区域内猪灵，断言猪灵与灵魂营火的 3D 距离 >8
//   （= horizontalRange 检测范围，逃跑成功必然满足，无论逃向哪方向）。逃跑导航约需 40-100 tick
//   （搜索+寻路+移动），maxTick=380 留余量。区域限定排除并行测试污染；type 用 "minecraft:piglin"。
//   距离用 3D 欧氏距离（shouldContinueExecuting 用 3D distSq 判定，含 y 分量；猪灵与营火同层 y 不变，
//   3D 距离≈水平距离）。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_猪灵.txt#其他（远离灵魂火/灵魂火把/灵魂灯笼/灵魂营火）
function piglinFleesFromSoulCampfire(test: Test): void {
  const piglinType = "piglin";
  // 灵魂营火放置位置（helper 相对坐标）。
  const campfirePos = { x: 22, y: 2, z: 4 };
  // 距灵魂营火 8 格为检测范围阈值（AvoidBlockGoal horizontalRange=8 = 原版 SetWalkTargetAwayFrom
  // desiredDistance=8）。猪灵逃离到距营火 >8 格（出检测范围）即证明排斥行为生效。
  const SAFE_DISTANCE = 8.0;

  // 猪灵 (20,2,4) spawn 在 open_grass_hall y=2 落到 y=1 草地顶。灵魂营火放 (22,2,4) 距猪灵 2 格 <8 触发排斥。
  // open_grass_hall helper-y=2 是 air 层，y=0 grass_block 地板。猪灵在中部 x=20，朝远离营火方向逃跑，
  // 16 格内到 x=4 或 x=36 仍在墙内（墙在 x=0/40）。灵魂营火默认放置即点燃（isLitCampfire=true）。
  test.spawn(piglinType, { x: 20, y: 2, z: 4 });
  test.setBlockType("minecraft:soul_campfire", campfirePos);

  const campfireWorld = test.worldLocation(campfirePos);

  // 轮询：猪灵距灵魂营火 3D 距离 >8（出检测范围，逃跑成功必然满足）。间隔 4 tick 捕获逃跑瞬间。
  pollUntilSucceed(test, () => {
    const piglins = test.getDimension().getEntities({
      type: "minecraft:piglin",
      location: test.worldLocation(HALL_FROM),
      volume: HALL_VOLUME,
    });
    if (piglins.length === 0) {
      return false;
    }
    const p = piglins[0].location;
    const dx = p.x - campfireWorld.x;
    const dy = p.y - campfireWorld.y;
    const dz = p.z - campfireWorld.z;
    const dist = Math.sqrt(dx * dx + dy * dy + dz * dz);
    return dist > SAFE_DISTANCE;
  }, {
    interval: 4,
    maxTick: 380,
    onTimeout: () => {
      const piglins = test.getDimension().getEntities({
        type: "minecraft:piglin",
        location: test.worldLocation(HALL_FROM),
        volume: HALL_VOLUME,
      });
      const pigPos = piglins.length > 0
        ? `(${piglins[0].location.x.toFixed(1)},${piglins[0].location.y.toFixed(1)},${piglins[0].location.z.toFixed(1)})`
        : "gone";
      const dist = piglins.length > 0
        ? Math.hypot(piglins[0].location.x - campfireWorld.x,
            piglins[0].location.y - campfireWorld.y,
            piglins[0].location.z - campfireWorld.z)
        : -1;
      test.assert(false,
        `piglin did not flee from soul campfire (piglin=${piglins.length}@${pigPos}, ` +
        `campfire=(${campfireWorld.x},${campfireWorld.y},${campfireWorld.z}), dist=${dist.toFixed(2)}, need>${SAFE_DISTANCE})`);
    },
  });
}

export function registerPiglinTests(): void {
  GameTest.register("MobBehaviorTests", "piglin_attacks_player_without_gold", piglinAttacksPlayerWithoutGold)
    .structureName("gametests:creeper_pit")
    .maxTicks(350);

  GameTest.register("MobBehaviorTests", "piglin_ignores_player_wearing_gold", piglinIgnoresPlayerWearingGold)
    .structureName("gametests:creeper_pit")
    .maxTicks(350);

  GameTest.register("MobBehaviorTests", "piglin_flees_from_soul_campfire", piglinFleesFromSoulCampfire)
    .structureName("gametests:open_grass_hall")
    .maxTicks(400);
}
