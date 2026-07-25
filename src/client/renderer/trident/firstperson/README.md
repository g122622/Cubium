# First Person Renderer

第一人称手部渲染器模块，负责渲染玩家视角下的手部和手持物品。

## 目录结构

```
firstperson/
├── README.md                    # 本文档
├── FirstPersonRenderer.hpp/cpp  # 第一人称渲染器主类（render 分支、tick、网格缓存）
├── FirstPersonTransforms.hpp/cpp # 纯变换自由函数（手臂/挥动/弓弩/地图/bobView/bobHurt/双手选择）
├── PlayerModel.hpp/cpp          # 玩家模型（双足模型扩展）
├── ItemInHandRenderer.hpp/cpp   # 手持物品渲染器（display 变换施加）
├── MatrixStack.hpp/cpp          # 矩阵栈（变换层级管理）
├── ItemCameraTransforms.hpp/cpp # 物品相机变换
└── ArmPose.hpp                  # 手臂姿态枚举
```

## 模块关系

- `TridentEngine` 负责初始化第一人称渲染器，并在每帧传入相机描述符集与 partial tick；同时注入 `ItemTextureAtlas`（普通物品）与 AtlasManager 的 blocks atlas 句柄（方块物品 3D 渲染切换图集，经 `setBlockAtlas` 下发）。
- `FirstPersonRenderer` 读取 `Player`、`PlayerInventory` 和 `ItemStack`，决定手臂姿态与手持物品表现。
- `FirstPersonTransforms` 承载所有与 MC 1.21.11 `ItemInHandRenderer`/`GameRenderer` 对齐的纯变换算法（手臂定位、挥动、弓/弩/三叉戟蓄力、地图倾斜、视野摇晃 `bobView`、伤害倾斜 `bobHurt`、双手渲染选择 `evaluateWhichHandsToRender`），可在无 Vulkan 依赖下单元测试。
- `ItemInHandRenderer` 从 `ItemModelCache` 获取物品模型的 display 变换并应用到矩阵栈（第一人称 display 变换的唯一施加点）。
- `EntityPipeline` 负责创建、绑定和销毁手臂/物品对应的 Vulkan 网格资源。
- `ItemMeshBuilder`（`trident/item/`）构建普通物品的 3D 模型网格；`BlockMeshBuilder`（`entity/util/`）构建方块物品的 3D 方块网格（逐面纹理）。
- `EntityTextureAtlas` 提供玩家皮肤区域，`ItemTextureAtlas` 提供普通物品 UV 区域，AtlasManager 的 blocks atlas 提供方块物品各面纹理 UV。
- `MatrixStack` 负责把相机朝向、bobView、bobHurt、挥手动画和持物变换组合成最终模型矩阵。

## 上下游依赖关系

### 上游依赖（本模块依赖）
- **BipedModel**: 双足模型基类
- **ModelRenderer**: 模型部件渲染器
- **ItemModelCache**: 物品模型缓存单例
- **ItemTextureAtlas**: 物品纹理图集（普通物品 3D 网格 UV）
- **AtlasManager blocks atlas**: 方块纹理图集（方块物品 3D 网格 UV，绘制时切换；句柄由 `TridentEngine` 经 `setBlockAtlas` 注入）
- **ItemMeshBuilder / BlockMeshBuilder**: 3D 物品/方块网格构建
- **EntityTextureAtlas**: 实体纹理图集（玩家皮肤）
- **TridentEngine**: 渲染引擎

### 下游依赖（依赖本模块）
- **TridentEngine**: 在 `render()` 中调用 `FirstPersonRenderer::render()` 渲染第一人称视角

## 容易踩的坑

1. **矩阵栈不平衡**: 每次 push 必须有对应的 pop，否则渲染错乱
2. **动画插值**: 必须使用 partialTick 进行插值，否则动画会卡顿
3. **主手/副手判断**: 需要根据玩家的主手设置来决定渲染位置
4. **物品变换覆盖**: 某些物品（地图）有特殊渲染逻辑
5. **透明度混合**: 手部和物品渲染顺序很重要
6. **变换顺序**: 不能手写"行列就地改值"去替代矩阵后乘，否则会出现水平转头时手臂自转
7. **物品网格不要共享缓存**: 主手和副手可能在同一帧持有不同物品，单缓存会导致每帧反复重建 GPU 网格；缓存键为 `(itemId, handSide, isBlockItem)`——左右手镜像几何不同，切换主手设置后同手位手侧会变，需重建
8. **旧网格不能立即销毁**: 需要按帧延迟回收，否则在飞帧可能仍在使用旧的 Vulkan 缓冲区
9. **ItemInHandRenderer 必须初始化**: 虽然 initialize() 不需要外部依赖，但必须调用才能正确设置默认变换参数
10. **第一人称根矩阵方向**: 使用"相机朝向基向量"构造，遵循 MC 前向定义：`forward = (-sin(yaw)*cos(pitch), sin(pitch), cos(yaw)*cos(pitch))`
11. **MatrixStack 后乘语义**: `current = current * transform`（PoseStack 语义）
12. **viewBobbing / damageTilt 作用在手部 PoseStack 而非相机**: `GameRenderer.renderItemInHand` 在相机对齐基之后依次应用 `bobHurt`（伤害倾斜）与 `bobView`（视野摇晃）。项目把这两者从相机层移到手部根矩阵 `baseStack` 上，避免走路时手不晃、受击时屏幕不倾斜。`damageTilt` 所需的 `hurtDir`/`hurtDuration` 由服务端 `LivingEntity::indicateDamage` 计算、经 `ir::play::HurtAnimation`（旧 `EntityAnimationPacket::TakeDamage` 已删除，统一走 IR；`TakeDamage` 枚举值现位于 `mc::network::EntityAnimation`，protocol/EntityEvents.hpp）同步到客户端 `animateHurt`，强度由 `ClientSettings.damageTiltStrength`（0-1，默认 1.0）控制。
13. **弓/弩/三叉戟使用中不挥动**: `renderArmWithItem` 分支——BOW/TRIDENT/SPEAR 使用中跳过通用 `applyItemArmTransform`+`swingArm`，改用各自专属基座（translate+rotate）后接蓄力；NONE/BUNDLE 与弩已装填空闲态才挥动。挥动拆为 `swingArm`（平移）+ `applyItemArmAttackTransform`（旋转）。
14. **equipProgress = 1 - height 语义**: height=1 表示完全可见，`equipProgress = swapAnimationScale*(1-lerp(partial, oHeight, height))` 作为"隐藏度"喂给 `applyItemArmTransform` 的 `equip*-0.6` 下落。
15. **3D 物品走 ItemMeshBuilder 不再烘焙 display 变换**: `_ensureItemMesh` 对普通物品调 `ItemMeshBuilder::buildHeldItemMesh(itemStack, transformType, false)`（`bakeTransforms=false`，返回原始几何），display 变换由 `_renderItemInHand` 末尾的 `ItemInHandRenderer::applyTransform` 在矩阵栈上单独施加——**避免与 ItemMeshBuilder 内部烘焙双重施加**。第三人称 `HeldItemLayer` 仍用默认 `bakeTransforms=true`（变换烘焙进顶点，仅传手臂相对矩阵）。
16. **方块物品 3D 渲染切换图集**: 方块物品（`BlockItem`）用 `BlockMeshBuilder::buildBlockMesh` 构建带逐面纹理的 3D 方块网格（UV 基于方块纹理图集），绘制前 `m_itemPipeline->setTextureAtlas(chunkAtlas...)` 切换图集、绘制后恢复为物品图集（与第三人称 `HeldBlockLayer` 同模式）。`BlockMeshBuilder` 依赖全局 `ChunkMesher::modelCache()`，需在区块渲染初始化后才有有效网格。
17. **Spyglass 抑制手部渲染**: `player->isScoping()`（使用中且物品 UseAction 为 Spyglass）时整个 `render()` 提前返回，不渲染手部（`isScoping()` 门控）。
18. **evaluateWhichHandsToRender 双手选择**: 持弓/弩且使用中→仅渲染使用手；主手用非弓弩且副手是已装填弩→仅主手；持弓/弩未使用→主手是已装填弩则仅主手，否则双手。替代了旧的 `isTwoHanded`+isEmpty 判定。

