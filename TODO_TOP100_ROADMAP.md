# TODO Top 100 补全路线图

本文基于全仓库扫描结果整理：`TODO` 706 条、`XXX` 13 条，共 719 条命中。  
其中 `XXX` 基本是模板语法示例，不计入技术债；`README.md` 型 backlog 也不纳入本清单。  
本文件按“低风险、局部改动、依赖已具备、适合优先收尾”的原则，合并展示约 100 个可优先补全的 TODO 点。

## 筛选规则

- 优先选择只改一个类或一个小子系统的 TODO。
- 优先选择现有接口已经存在，只差把分支接上或把数据读出来的 TODO。
- 优先选择容易写单测、容易加断言、容易对照原版 Java 行为的 TODO。
- 暂时回避红石传播、完整刷怪、复杂战斗 AI、整套维度/区块生成这类大改动。

## 建议顺序

1. 先收测试与资源加载类 TODO，快速补齐基础能力。
2. 再收方块、物品、UI 这类局部逻辑，收益高且风险低。
3. 最后再碰实体行为和 AI 目标，按“一个实体一个提交”推进。

## A. 测试与基础设施

- ✅ 已完成 `tests/common/test_biome.cpp:115`：补充了代表性生物群系注册样本，并将断言扩展到更多已注册生物群系名称与存在性检查。

## B. 方块系统

- ✅ 已完成 `src/common/world/block/blocks/AbstractFurnaceBlock.cpp:55`：补客户端/服务端分流，只让服务端触发熔炼与统一菜单逻辑。
- ✅ 已完成 `src/common/world/block/blocks/ChestBlock.cpp:95,166,171,223`：补流体 tick、锁箱音效、统一菜单打开、猫坐箱子判定。都属于已有能力的接线，不需要重做箱子系统。
- ✅ 已完成 `src/common/world/block/blocks/EnchantingTableBlock.cpp:53,66`：补服务端判定和附魔台 GUI 打开，直接走统一菜单入口。
- ✅ 已完成 `src/common/world/block/blocks/FurnaceBlocks.cpp:23,42,60`：补普通炉/高炉/烟熏炉菜单打开，复用同一套容器注册。
- ✅ 已完成 `src/common/world/block/blocks/agricultural/CropBlock.cpp:68,113`：补光照检查，并将骨粉增长改为基于世界种子和位置的确定性随机。
- ✅ 已完成 `src/common/world/block/blocks/agricultural/StemBlock.cpp:102`：将骨粉增长与果实触发改为基于世界种子和位置的确定性随机，移除全局 `rand()`。
- ✅ 已完成 `src/common/world/block/blocks/agricultural/FarmlandBlock.cpp:67,115`：补特殊支撑判定和下雨判定，直接接现有世界/天气接口。
- ✅ 已完成 `src/common/world/block/blocks/building/TrapDoorBlock.cpp:257`：补开合音效，并根据当前方块实例区分木/铁活板门。
- ✅ 已完成 `src/common/world/block/blocks/coral/CoralBlock.cpp:34,54,115,162,227`：补水源判定、邻近水检测、缺水回退和流体 tick，珊瑚扇/墙扇同步完成同类逻辑。
- `src/common/world/block/blocks/decorative/CampfireBlock.cpp:52,56,84,94`：营火不会被雨淋熄（MC 1.16.5 设计），TODO 已确认无需实现。烹饪逻辑和音效/粒子已有基础实现，待方块实体支持。
- ✅ 已完成 `src/common/world/block/blocks/decorative/PaneBlock.cpp:43,64,141`：补动态碰撞形状、水合状态和可连接方块类型。
- ✅ 已完成 `src/common/world/block/blocks/decorative/ScaffoldingBlock.cpp:37,39,60,72,94`：补距离计算、水合状态、底部状态更新、支撑过远判定、物品掉落和 FallingBlockEntity 创建逻辑。测试用例已完整覆盖 tick 行为、距离计算和实体生成。

## C. 资源 / UI / 菜单

- `src/common/resource/loader/LootTable.cpp:94,99`：补 JSON 解析与序列化，属于典型单文件数据流收尾。
- `src/common/resource/loader/ResourceLoader.cpp:252,262,274,284`：补模型与方块状态加载入口，先打通单文件版，再扩展批量加载。
- `src/client/sound/backend/AudioBuffer.cpp:63`：补音频加载，接口清晰，适合先接现有资源读取。
- `src/client/sound/handler/BiomeAmbientHandler.cpp:51`：从群系注册表取实际环境音效，直接对照原版环境音逻辑即可。
- `src/client/sound/MusicPlayer.cpp:306`：补音量淡出，典型局部状态机修补。
- `src/client/ui/kagero/template/parser/Parser.cpp:526`：补 ID 唯一性检查，依赖父文档但实现本身很局部。
- `src/client/ui/minecraft/resources/ResourceProvider.cpp:13`：补从资源路径加载纹理图集，属于资源路径接线。
- `src/client/ui/kagero/widget/ButtonWidget.hpp:222,262`：补按钮音效和实际渲染逻辑，先把 hover/pressed 态接全。
- `src/client/ui/kagero/widget/Viewport3DWidget.hpp:297,305,312,319`：补 Vulkan 帧缓冲、实体/物品/方块渲染，建议按“先能显示，再优化”推进。
- ✅ 已完成 `src/server/menu/CraftingMenu.cpp:129`：补工作台距离检查，按方块位置与玩家距离校验可访问范围。

## D. 物品 / NBT

- `src/common/item/attribute/ItemAttributeModifiers.cpp:38`：补属性注册表查找，避免调用点传裸指针。
- ✅ 已完成 `src/common/item/items/armor/ArmorItem.cpp:25`：补盔甲装备逻辑，直接接槽位系统即可。
- ✅ 已完成 `src/common/item/items/armor/DyeableArmorItem.cpp:12,22,29,34`：补染色盔甲的 NBT 读取、写入、移除与存在性检查，逻辑很适合做成一组单测。
- ✅ 已完成 `src/common/item/items/armor/ElytraItem.cpp:18,32,54`：补鞘翅装备、耐久消耗和滑翔状态检查。
- `src/common/item/items/block/BlockItem.cpp:72`：补世界边界检查，防止非法放置位置。
- ✅ 已完成 `src/common/item/items/potion/GlassBottleItem.cpp:13`：补水源/炼药锅取水，右键成功时返回水瓶。
- ✅ 已完成 `src/common/item/items/potion/LingeringPotionItem.cpp`：滞留药水投掷已实现，PotionEntity.onImpact() 已实现效果应用，AreaEffectCloud 实体待实现。
- ✅ 已完成 `src/common/item/items/potion/SplashPotionItem.cpp`：喷溅药水投掷与区域效果应用已完整实现。
- ✅ 已完成 `src/common/item/items/potion/ThrowablePotionItem.cpp`：提取喷溅/滞留药水共同代码到基类。
- `src/common/item/potion/PotionUtils.cpp:47`：补自定义效果读取，属于典型 NBT 解析收尾。

## E. 生物 / AI / 实体

- `src/common/entity/entities/passive/basic/AnimalEntity.cpp:46` 与 `AnimalEntity.hpp:70`：补统一的 `EntityType` 比较与通用交互入口，先把子类重复逻辑收敛掉。
- ✅ 已完成 `src/common/entity/entities/passive/basic/ChickenEntity.cpp:87`：补掉蛋生成物品实体并重置计时器。
- `src/common/entity/entities/passive/basic/CowEntity.hpp:50`：补挤奶逻辑，目标是把空桶和牛奶桶互转接通。
- `src/common/entity/entities/passive/basic/PigEntity.cpp:33` 与 `RabbitEntity.cpp:53,62`：补基础类型检查、食物判定和幼兔生成。
- ✅ 已完成 `src/common/entity/entities/passive/basic/SheepEntity.cpp:36,66,86,113`：补羊毛物品映射、父母颜色继承、吃草目标和羊毛再生。实现了 EatGrassGoal、颜色混合逻辑和 eatGrassBonus 调用。
- `src/common/entity/entities/passive/special/BeeEntity.cpp:40,47,75`：补花朵判定、幼蜂生成和水中判定。
- `src/common/entity/entities/passive/special/FoxEntity.cpp:91,99`：补浆果判定与幼狐生成。
- `src/common/entity/entities/passive/special/StriderEntity.cpp:32,63`：补熔岩判定与诡异菌判定。
- `src/common/entity/entities/passive/special/TurtleEntity.cpp:37,50`：补水中判定与幼龟生成。
- `src/common/entity/ai/brain/sensor/Sensors.hpp:47,105,148,204,266,309,353,416`：补附近玩家、生物、实体、伤害来源、工作站点、床/钟、幼年/成年实体和避险实体检测。
- ✅ 已完成 `src/common/entity/ai/goal/goals/attack/RangedAttackGoals.cpp:158`：补是否持有弓的检查，并补了对应回归测试。
- `src/common/entity/ai/goal/goals/BreedGoal.cpp:117,118,119`：补爱心粒子、繁殖音效和玩家经验值。
- `src/common/entity/ai/goal/goals/interact/TameableGoals.cpp:39,116,164,210`：补主人玩家查找、传送逻辑、找最近拿食物的玩家和食物判定。
- ✅ 已完成 `src/common/entity/ai/goal/goals/MeleeAttackGoal.cpp:132`：补快速逆平方根近似，属于纯性能小修。
- ✅ 已完成 `src/common/entity/ai/goal/goals/movement/MovementGoals.cpp:119`：补世界接口检查方块，并接入水/岩浆世界查询。
- ✅ 已完成 `src/common/entity/ai/goal/goals/PanicGoal.cpp:120`：补水判定，逻辑短且局部，并补了寻水回归测试。
- `src/common/entity/ai/goal/goals/target/TargetGoals.cpp:65,102,149,173,210,249,295`：补团队关系、附近实体搜索、最近攻击者、通知盟友和主人相关判定。
- ✅ 已完成 `src/common/entity/ai/goal/goals/TemptGoal.cpp:139`：补玩家手持物品检查，并让继续执行逻辑重新验证手持物品状态。
- `src/common/entity/entities/monster/end/EndermanEntity.cpp:42,54,69,75,81,105,111`：补瞬移、靠近目标瞬移、搬方块、受水伤害和随机搬放块。
- `src/common/entity/entities/monster/end/ShulkerEntity.cpp:13,39,48,89`：补移动控制器禁用、瞬移、子弹生成和受伤后瞬移。
- `src/common/entity/entities/monster/illager/EvokerEntity.cpp:49,54,77`：补尖牙施法、恼鬼召唤链和专用施法 goal。
- `src/common/entity/entities/monster/illager/IllagerEntities.cpp:21,27,32,40,63`：补弩攻击、装填、射击以及掠夺者/卫道士专用 AI。
- `src/common/entity/entities/monster/illager/IllusionerEntity.cpp:21,35,47,65`：补幻术师远程攻击、失明法术、分身法术和专用 goal。
- `src/common/entity/entities/monster/illager/PatrollerEntity.cpp:41`：补巡逻移动目标注册。
- `src/common/entity/entities/monster/illager/RavagerEntity.cpp:45,62`：补破坏前方方块和劫掠兽专用 AI。
- `src/common/entity/entities/monster/illager/SpellcastingIllagerEntity.cpp:36`：补施法粒子颜色反馈。
- `src/common/entity/entities/monster/illager/VexEntity.cpp:27,32,37,43`：补闪烁、伤害方法、noclip 标志和恼鬼专用 AI。
- `src/common/entity/entities/monster/illager/WitchEntity.cpp:41,61`：补治疗效果和女巫 AI。
- `src/common/entity/entities/monster/basic/CreeperEntity.cpp:21,54`：补当前位置爆炸和苦力怕 AI。
- `src/common/entity/entities/monster/basic/PhantomEntity.cpp:34`：补幻翼特有 AI 目标。
- `src/common/entity/entities/monster/basic/SlimeEntity.cpp:43,96`：补 2-4 个小史莱姆生成和史莱姆 AI。
- `src/common/entity/entities/monster/arthropod/SpiderEntity.cpp:22,31,45`：补光照、贴墙和蜘蛛 AI。
- `src/common/entity/entities/monster/arthropod/EndermiteEntity.cpp:26,33,56`：补正确实现后再 `discard()`、末影螨特有 AI 和蠹虫特有 AI。
- `src/common/entity/entities/monster/nether/BlazeEntity.cpp:21,46`：补火球发射和空中判定。
- 车辆、效果、杂项这几个目录也有较多命中，但更适合后续按子系统再拆，不建议在这一轮直接大改。

## F. 跨系统 / 核心小修

- `src/common/entity/loot/LootTable.cpp:94,99`：**待实现** JSON 解析与序列化。当前 `fromJson()` 返回 `Unsupported` 错误。需要创建 `LootSerializers` 类实现完整的 JSON 解析逻辑（支持 MC 1.16.5 掉落表格式）。README 已标注为"未来计划"。
- `src/common/entity/entities/passive/horse/AbstractHorseEntity.cpp:20`：补马鞍标志设置，逻辑很局部。
- `src/client/sound/backend/AudioBuffer.cpp:63`、`src/client/sound/handler/BiomeAmbientHandler.cpp:51`、`src/client/sound/MusicPlayer.cpp:306`：这三处都在收尾客户端音频链路，建议一起处理。
- `src/client/ui/kagero/template/parser/Parser.cpp:526` 与 `src/client/ui/kagero/template/parser/Ast.hpp` 中的 `bind:xxx / on:xxx / for:xxx` 示例：前者是真 TODO，后者是语法示例，不要混淆。
- `src/common/entity/core/Entity.cpp:332,617,630,648`：补溺水、车辆、视线阻挡等核心判定，建议先对照原版再补单测。
- `src/common/entity/core/LivingEntity.cpp:280,310,329,335`：补生命恢复、摔落保护、跳跃增强和冲刺跳跃，属于原版行为对齐点。
- `src/common/entity/combat/AttackContext.cpp:70,71`：补附魔伤害与保护。
- `src/common/entity/combat/PlayerAttackHelper.cpp:17,30,31,32,49,50,58,92,115,118`：补暴击、附魔、击退、点火等攻击结算细节，建议按原版攻击流程逐步对齐。
- `src/common/world/block/blocks/CauldronBlock.cpp:233,239,248,270,276,285,307,311,334,338,349,356`：补桶、药瓶、旗帜与音效链路，建议按“交互类型”拆成多组小单测。
- `src/common/world/block/blocks/mob/MobBlocks.cpp:18,32,38,75,123,142,154,170,187,230`：补蜂箱、海龟蛋、蜜蜂、蠹虫相关逻辑。
- `src/common/entity/entities/projectile/OtherProjectiles.cpp:39,74,101,105,113,172,192,233,236,322,338,344`：补钓鱼、尖牙、烟花、爆炸伤害等投射物行为。
- `src/common/entity/entities/projectile/ProjectileItemEntity.cpp:62,106,150,172`：补投射物伤害与掉落。
- `src/common/entity/entities/projectile/AbstractFireballEntity.cpp:43,84` 与 `AbstractArrowEntity.cpp:116,175`、`TridentEntity.cpp:117`：补命中点火、箭矢伤害与三叉戟结算。
- `src/common/entity/entities/projectile/Entity.cpp` 之外的这些投射物 TODO，大多都能沿着“命中 -> 伤害/效果 -> 掉落/销毁”三段式拆单测。
- `src/common/entity/entities/hanging/HangingEntity.cpp:54,106,205`：补背后支撑检查、画作物品生成和拴绳物品掉落。
- `src/common/entity/entities/item/ItemEntity.cpp:92,196,269`：补合并检测、水/岩浆检测和着火设置。
- `src/common/entity/entities/misc/MiscEntities.cpp:60,113,149`：补放置方块、爆炸和潮涌能量效果。
- `src/common/entity/entities/effect/EffectEntities.cpp:28,40,44,79,84,88,121`：补粒子、末影龙治疗、爆炸、闪电、火焰和范围效果。
- `src/common/entity/damage/CombatTracker.cpp:92`：补完整死亡消息系统。
- `src/common/entity/core/FlyingEntity.cpp:16`：补飞行移动逻辑。
- `src/common/entity/core/Entity.hpp:25`：旧枚举仍在，建议按注释目标继续收敛到 `mc::entity::EntityType`。
- `src/common/entity/core/VanillaEntities.hpp:653,664,677,688`：末影龙、凋灵、村民、流浪商人都标成未完成，建议作为后续大块拆分的入口。

## 验证模板

- 每补一个文件，优先加最小单测，覆盖“输入合法 / 输入边界 / 行为回归”三种情况。
- 每个关键分支加断言，尽量用 `MC_ASSERT_RELEASE` 暴露前置条件，而不是靠大量空指针保护掩盖问题。
- 对实体/方块交互类改动，优先补 `tests/common/...` 下的定向测试，而不是只靠手工跑游戏。
- 对需要原版行为的逻辑，先对照 `D:\Minecraft\MC研究\Minecraft1.16.5源码` 中同名或近似类，再做 C++ 风格化重构。

## 适合优先对照的原版类

- 实体攻击与战斗：`Player`, `LivingEntity`, `Entity`, `AttackContext`, `PlayerAttackHelper`
- 方块交互：`CauldronBlock`, `ChestBlock`, `CampfireBlock`, `ScaffoldingBlock`, `PaneBlock`
- 生物与 AI：`EndermanEntity`, `ShulkerEntity`, `EvokerEntity`, `IllusionerEntity`, `VillagerGoals`, `Sensors`
- 投射物：`AbstractArrowEntity`, `AbstractFireballEntity`, `TridentEntity`, `ProjectileItemEntity`, `OtherProjectiles`
- 资源与音频：`ResourceLoader`, `AudioBuffer`, `MusicManager`, `BiomeAmbientHandler`

## 维护建议

- 后续如果继续清 TODO，建议按“一个子系统一份清单”继续拆到 `docs/roadmap/`，根目录只保留总索引。
- 新增 TODO 时必须写清楚成因，是“依赖缺失”、“原版逻辑未补”还是“待重构”，避免再次混淆。
- 已经能通过断言或单测暴露的问题，不要再用防御性空判断盖住。

请你逐步收敛这些TODO，务必保证完整完成而不是继续开新的TODO。

尽可能准确复刻mc。系统设计必须优雅、考虑到未来拓展、结构清晰美观整洁、架构强大，你可参考mc源码（主要是为了学习mc真实游戏逻辑而非架构。mc的架构水平和代码质量并不高），但不要照搬，必须根据cpp的特性和当前项目的架构进行合理的架构设计和调整。
  
 ## 注意，你可随时访问mc java版本的源码来供自己参考：`D:\Minecraft\MC研究\Minecraft1.16.5源码`，这很重要，因为当前项目是一个复刻项目，目标是完全使用cpp尽可能一致地复刻java版mc的游戏体验，并在存档、数据包等层面上尽可能兼容和复用现有java版minecraf t生态。务必要有清晰、优雅、能让人赏心悦目的目录结构，不要把很多文件全部堆在一个目录下，要划分好细分的子目录。         

## 你需要先阅读readme文件了解怎么构建项目

## 需要断言+单测来保证代码质量；每个方法前都要附上doc注释说明方法的用法和注意事项（容易踩坑的地方）
## 务必要有清晰、优雅、能让人赏心悦目的目录结构，不要把很多文件全部堆在一个目录下，要划分好细分的子目录！

## 编译过程中遇到的warning你也要一并解决

## 需要使用命名空间隔离各个子系统的标识符。下面是最佳实践：

```cpp

namespace mr {
namespace entity {
namespace attribute {

/**
 * @brief 属性修改器操作类型
 *
 * 定义属性修改器如何影响基础值
 */
enum class Operation : u8 {
    // ...
}}}}

```

最后要运行 `cmake --build build --config RelWithDebInfo` 来构建项目，并确保所有的测试都通过。

当你完成一个TODO后，不要停下来，请继续做后面的TODO，直到所有TODO清空你才能停！
