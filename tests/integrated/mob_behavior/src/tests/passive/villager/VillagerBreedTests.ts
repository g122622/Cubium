// 村民自动拾取食物繁殖行为类 GameTest。

import * as GameTest from "@minecraft/server-gametest";
import type { Test } from "@minecraft/server-gametest";
import { pollUntilSucceed } from "../../../utils/test/poll.js";

// grass_pen 结构尺寸 9×5×9（helper 相对坐标 x,z∈[0,8], y∈[0,4]）。
// 用于 getEntities 的区域限定查询：location 取 (0,0,0) 角点，volume 取结构全尺寸 9×5×9。
// 必须区域限定——Cubium GameTest 批内并行 tick + 不清场，全维度 getEntities({type}) 跨测试污染。
const PEN_FROM = { x: 0, y: 0, z: 0 };
const PEN_VOLUME = { x: 9, y: 5, z: 9 };

// 村民自动拾取地上食物（面包）后达到繁殖食物门槛，互相靠近繁殖出幼年村民
// （wiki tech_村民.txt#繁殖：村民会拾取掉落的食物（面包/土豆/胡萝卜/甜菜根），当食物点数足够时
//   主动寻找配偶繁殖，无需玩家手动喂食。繁殖消耗食物、生成幼年村民、双亲进入冷却）。
//
// C++ 链路（本次补全，对齐 MC Java 1.21.11 Villager.pickUpItem/wantsToPickUp + InventoryCarrier.pickUpItem）：
//   1) VillagerEntity 构造函数 setCanPickUpLoot(true)（对齐 Java Villager.java:196），MobEntity::tick
//      looting 扫描段（MobEntity.cpp:363，对齐 Mob.aiStep）canPickUpLoot()&&isAlive()&&mobGriefing
//      → 扫描 AABB.inflate(getPickupReach=(1,0,1)) 内 ItemEntity → wantsToPickUp(stack)。
//   2) VillagerEntity::wantsToPickUp override（转调 canPickUpItem）：面包在可拾取列表 + 库存 canAddItem。
//   3) VillagerEntity::pickUpItem override（对齐 Java InventoryCarrier.pickUpItem）：
//      SimpleInventory::addItem 入库，全装入则 ItemEntity.remove()，部分装入则剩余 count 写回；
//      拾取后 countFoodPointsInInventory() >= WANTS_MORE_FOOD_THRESHOLD(12) → setWillingToBreed(true)
//      （用布尔标志模拟 Java canBreed() 的 foodLevel+countFoodPointsInInventory>=12 门槛）。
//   4) VillagerBreedGoal::shouldExecute → _isWillingToBreed()(=isWillingToBreed) → _findPartner
//      （8 格内找 willing+成年同类配偶）。
//   5) VillagerBreedGoal::tick → _moveToPartner，m_breedTicks++，当 distSq<=BREED_DISTANCE_SQ(4)
//      且 m_breedTicks>=60 → _spawnChild（_hasEnoughBeds 通过则 createChild+spawnEntity 生成幼年村民）。
//
// 食物点数：面包=4 点/个（VillagerEntity::foodPoints）。门槛 12 点需 3 个面包（3×4=12）。
//   每村民脚下放 3 个面包掉落物，looting 扫描每 tick 拾取一个（拾取后 ItemEntity.remove）。
//
// 床位要求：VillagerBreedGoal::_spawnChild 调 _hasEnoughBeds，world()->villageManager() 非空时
//   需 48 格内有未占用床 POI。GameTest 的 world 是 ServerWorld（持有 VillageManager），故须放床
//   注册床 POI（VillageManager::onBlockPlaced → POITypeHelper::fromBlockId(red_bed)=BedRed → registerPOI）。
//   放完整 red_bed（foot+head 配对，对齐 BedTests 范式）注册 1 个床 POI 即满足 availableBeds>0。
//
// 环境选择：grass_pen（9×5×9）+ 默认 day batch。
//   - 村民是被动生物不燃不刷怪干扰，不需 night/skyAccess。
//   - 玻璃围栏把两村民限制在相邻 2 格区域，防随机走动离开面包/配偶。
//
// 几何：
//   - 村民A (3,2,3) + 村民B (5,2,3) 相距 2 格（distSq=4 <= BREED_DISTANCE_SQ=4 在繁殖距离内）。
//   - 面包A×3 放 (3,2,3) 村民A 脚下同格（looting AABB.inflate(1,0,1) 覆盖自身格）。
//   - 面包B×3 放 (5,2,3) 村民B 脚下同格。
//   - 玻璃墙围栏：(2,2,3)(6,2,3) 两端墙 + (3,2,2)(3,2,4)(5,2,2)(5,2,4) 两侧墙，限制两村民在
//     (3,2,3)-(5,2,3) 走廊内，面包在脚下必然被 looting 扫描覆盖。
//   - 床 (1,2,1)foot + (1,2,2)head 配对放角落，远离村民但在 48 格 POI 搜索范围内。
//   - 封顶 (3,3,3)(4,3,3)(5,3,3) stone 防村民受惊跳起/光照干扰（非必需，双保险）。
//
// 判定手段：繁殖完成后区域内 villager 数 >=3（原 2 成年 + 1 幼年）。幼年村民 typeId=VILLAGER 可被
//   getEntities 查到。pollUntilSucceed 轮询。
//
// 时序：spawnItem 落地(pickupDelay 默认 10 tick 后可拾取) + 两村民各拾取 3 面包(约 30 tick) +
//   VillagerBreedGoal shouldExecute+findPartner + 60 tick BREED_TICKS + _spawnChild。startTick=60
//   留拾取时间，maxTick=900 留充足余量吸收拾取/寻路非确定性。
// Ref: docs\minecraft-wiki-source\minecraft_wiki\tech_村民.txt#繁殖（拾取食物自动繁殖）
// Ref: VillagerEntity.cpp pickUpItem/wantsToPickUp（对齐 Java InventoryCarrier.pickUpItem）
// Ref: VillagerBreedGoal.cpp _spawnChild/_hasEnoughBeds（床位 POI 门控）
function villagerBreedsAfterPickingUpBread(test: Test): void {
    const villagerType = "villager";

    // 床（角落，foot+head 配对注册床 POI 满足 _hasEnoughBeds）。foot 放支撑下方 stone。
    // facing=North 时 head 在 foot 的 z-1 方向。setBlockType 放 foot 默认 part=Foot，
    // setBlockWithStates 放 head part=head 配对（对齐 BedTests placeBedSetup 范式）。
    test.setBlockType("minecraft:stone", { x: 1, y: 1, z: 1 }); // head 支撑
    test.setBlockType("minecraft:stone", { x: 1, y: 1, z: 2 }); // foot 支撑
    test.setBlockType("minecraft:red_bed", { x: 1, y: 2, z: 2 }); // foot 半
    test.setBlockWithStates("minecraft:red_bed", { x: 1, y: 2, z: 1 }, "part=head,facing=north"); // head 半

    // 玻璃围栏：限制两村民在 (3,2,3)-(5,2,3) 走廊。
    test.setBlockType("minecraft:glass", { x: 2, y: 2, z: 3 }); // A 左端墙
    test.setBlockType("minecraft:glass", { x: 6, y: 2, z: 3 }); // B 右端墙
    test.setBlockType("minecraft:glass", { x: 3, y: 2, z: 2 }); // A 侧墙
    test.setBlockType("minecraft:glass", { x: 3, y: 2, z: 4 }); // A 侧墙
    test.setBlockType("minecraft:glass", { x: 5, y: 2, z: 2 }); // B 侧墙
    test.setBlockType("minecraft:glass", { x: 5, y: 2, z: 4 }); // B 侧墙
    // 封顶遮光双保险（防 daylight cycle 光照变化干扰，非必需）。
    test.setBlockType("minecraft:stone", { x: 3, y: 3, z: 3 });
    test.setBlockType("minecraft:stone", { x: 4, y: 3, z: 3 });
    test.setBlockType("minecraft:stone", { x: 5, y: 3, z: 3 });

    // 两村民相距 2 格（distSq=4 <= BREED_DISTANCE_SQ=4 在繁殖距离内，<8 格检测范围）。
    // 脚下 y=1 grass_block 支撑防下落。
    test.spawn(villagerType, { x: 3, y: 2, z: 3 });
    test.spawn(villagerType, { x: 5, y: 2, z: 3 });

    // 每村民脚下放 3 个面包掉落物（3×4=12 点达繁殖门槛）。
    // spawnItem 落地 pickupDelay 默认 10 tick 后可拾取，looting 扫描每 tick 拾取一个。
    for (let i = 0; i < 3; i++) {
        (test.spawnItem as any)("minecraft:bread", { x: 3, y: 2, z: 3 });
        (test.spawnItem as any)("minecraft:bread", { x: 5, y: 2, z: 3 });
    }

    // 轮询：繁殖完成后区域内 villager 数 >=3（原 2 成年 + 1 幼年）。
    pollUntilSucceed(test, () => {
        const villagers = test.getDimension().getEntities({
            type: villagerType,
            location: test.worldLocation(PEN_FROM),
            volume: PEN_VOLUME,
        });
        return villagers.length >= 3;
    }, {
        startTick: 60,
        interval: 15,
        maxTick: 900,
        onTimeout: () => {
            const villagers = test.getDimension().getEntities({
                type: villagerType,
                location: test.worldLocation(PEN_FROM),
                volume: PEN_VOLUME,
            });
            const items = test.getDimension().getEntities({
                type: "minecraft:item",
                location: test.worldLocation(PEN_FROM),
                volume: PEN_VOLUME,
            });
            // 区分失败原因：villager<2 说明村民流失，villager==2 说明未繁殖（拾取/意愿/床位/配偶链路断）。
            test.assert(false,
                `villager did not breed after picking up bread: villager=${villagers.length} ` +
                `items=${items.length} (villager==2: breed chain broken; <2: villager lost)`);
        },
    });
}

export function registerVillagerBreedTests(): void {
    GameTest.register("MobBehaviorTests", "villager_breeds_after_picking_up_bread", villagerBreedsAfterPickingUpBread)
        .structureName("gametests:grass_pen")
        .maxTicks(1000);
}
