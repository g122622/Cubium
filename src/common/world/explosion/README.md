#爆炸系统(Explosion System)

##目录结构

``` explosion /
├── ExplosionMode.hpp #爆炸模式枚举定义（None / Break /
            Destroy）
├── ExplosionContext.hpp #爆炸上下文基类，用于自定义爆炸行为
├── ExplosionContext.cpp #ExplosionContext 实现
├── Explosion.hpp #爆炸核心类，执行完整爆炸流程
├── Explosion.cpp #Explosion 实现
└── README.md #本文档
```

            ##内部模块关系

``` ExplosionMode（枚举）
       │
       ▼ ExplosionContext ◄─────────── Explosion
       │                           │
       │                           │ 使用
       ▼                           ▼ EntityExplosionContext      ┌──────────────────┐
（实体爆炸上下文）          │ 射线追踪、伤害计算 │
                            │ 方块破坏、掉落生成 │
                            └──────────────────┘
```

        - **ExplosionMode**：定义爆炸对方块的影响方式，被 Explosion 使用 -
        **ExplosionContext**：抽象基类，允许自定义爆炸行为（如凋灵之首破坏基岩） -
        **Explosion**：核心类，协调整个爆炸流程，持有 ExplosionContext

         ##上下游外部依赖关系

         ## #上游依赖（本模块依赖）

    | 模块 | 用途 | | -- -- --| -- -- --| | `common / world / IWorld` | 世界接口，获取方块、实体、流体等 |
    | `common / world / block / Block` | 方块定义，爆炸抗性、掉落表等 | | `common / world / fluid / Fluid` |
    流体状态，影响爆炸抗性计算 | | `common / entity / core / Entity` | 实体基类，伤害、击退、免疫检测 |
    | `common / entity / damage / DamageSource` | 伤害来源，爆炸伤害类型 | | `common / item / loot / LootTableManager` |
    掉落表管理，方块掉落生成 | | `common / util / math / random / Random` | 随机数生成 |
    | `common / util / math / ray / Raycast` | 射线追踪，视线检测 | | `common / entity / utils / ItemDropHelper` |
    物品掉落工具类 | | `common / item / enchantment / EnchantmentHelper` | 附魔检测，爆炸保护附魔 |
    | `client / renderer / trident / particle / ParticleTypes` | 粒子效果 | | `common / sound / SoundCategory` |
    音效播放 |

    ## #下游依赖（被依赖）

    | 调用方 | 用途 | | -- -- -- --| -- -- --| | `ServerWorld` | 通过 `createExplosion()` 创建爆炸 |
    | `TNTBlock` | TNT 方块被点燃 / 爆炸时创建爆炸 | | `BedBlock` | 床在其他维度使用时创建爆炸 |
    | `RespawnAnchorBlock` | 重生锚在主世界使用时创建爆炸 | | `CreeperEntity` | 苦力怕爆炸 |
    | `WitherEntity` | 凋灵召唤 / 攻击时爆炸 | | `AbstractFireballEntity` | 恶魂火球 / 凋灵之首爆炸 |
    | `MinecartEntity`（TNT 矿车） | TNT 矿车爆炸 |

    ##容易踩的坑

            ## #1. LootTableManager 为空时不掉落物品

                ** 问题**：`Explosion` 构造时如果 `lootTableManager` 为 `nullptr`，`Destroy` 模式下不会生成任何掉落物。

                    ** 解决**：`ServerWorld::
                        createExplosion()` 会自动传入 `LootTableManager`；直接构造 `Explosion` 时需显式传入。

            ## #2. 爆炸保护附魔计算

                ** 问题**：爆炸保护附魔的 EPF（爆炸保护系数）有上限 20，减伤公式为 `damage × (1 - min(EPF, 20) / 25)`。

                    ** 解决**：代码中已正确处理，但自定义实体伤害逻辑时需注意此上限。

            ## #3. 流体爆炸抗性

                ** 问题**：水和岩浆的爆炸抗性为 100.0，会消耗大量爆炸强度。

            * *解决**：`ExplosionContext::getExplosionResistance()` 已处理流体情况，取方块和流体抗性的较大值。

              ## #4. 爆炸衰减公式

                  ** 问题**：物品存活概率为 `1 -
        1 / explosionRadius`，半径为 1 时物品 100 %
            消失。

                ** 解决**：这是 MC 1.16.5 的正确行为，恶魂火球（半径 1）爆炸不掉落物品。

            ## #5. 射线追踪使用随机种子

                ** 问题**：相同位置的爆炸如果使用相同种子，会产生相同的破坏模式。

                    ** 解决**：`Explosion` 使用位置坐标作为随机种子，保证相同位置的爆炸结果一致。

            ## #6. 玩家击退与游戏模式

                ** 问题**：观察者模式玩家不受击退，创造模式飞行中也不受击退。

                    ** 解决**：代码中已通过 `GameModeUtils::isSpectator()` 和 `abilities.flying` 检测。

            ## #7. 爆炸模式差异

    | 模式 | 破坏方块 | 生成掉落 | 用例 | | -- -- --| -- -- -- -- -| -- -- -- -- -| -- -- --| | None
    | ❌ | ❌ | mobGriefing = false 时的苦力怕 | | Break | ✅ | ❌ | TNT | | Destroy | ✅ | ✅ | 苦力怕、末地水晶 |

    ## #8. 爆炸路径中的 spawnAfterBreak

            * *要点 *
            *：`Explosion::_destroyBlocks` 在方块被移除后调用 `block.spawnAfterBreak(world, pos, state, nullptr, false)`。

        - `tool` 参数为 `nullptr`（爆炸无工具），`dropExp` 为 `false`（爆炸不产生经验）
        - 这意味着虫蚀方块（InfestedBlock）在爆炸中不会因为精准采集而不生成蠹虫（因为 tool =
                                  nullptr） - 但 `doTileDrops` 游戏规则仍然生效：`doTileDrops = false` 时不生成蠹虫
    - 调用顺序：`onBlockExploded` → `setBlockState(air)` → `spawnAfterBreak`，与 MC Java 一致
    - `onBlockExploded` 签名包含 `const Explosion * explosion` 参数，允许方块在爆炸回调中访问爆炸信息（如间接源实体）
