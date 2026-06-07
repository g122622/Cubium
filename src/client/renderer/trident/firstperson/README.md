# First Person Renderer

第一人称手部渲染器模块，负责渲染玩家视角下的手部和手持物品。

## 目录结构

```
firstperson/
├── README.md                    # 本文档
├── FirstPersonRenderer.hpp/cpp  # 第一人称渲染器主类
├── PlayerModel.hpp/cpp          # 玩家模型（双足模型扩展）
├── ItemInHandRenderer.hpp/cpp   # 手持物品渲染器
├── MatrixStack.hpp/cpp          # 矩阵栈（变换层级管理）
├── ItemCameraTransforms.hpp/cpp # 物品相机变换
└── ArmPose.hpp                  # 手臂姿态枚举
```

## 模块关系

- `TridentEngine` 负责初始化第一人称渲染器，并在每帧传入相机描述符集与 partial tick。
- `FirstPersonRenderer` 读取 `Player`、`PlayerInventory` 和 `ItemStack`，决定手臂姿态与手持物品表现。
- `ItemInHandRenderer` 从 `ItemModelCache` 获取物品模型的自定义变换并应用到矩阵栈。
- `EntityPipeline` 负责创建、绑定和销毁手臂/物品对应的 Vulkan 网格资源。
- `EntityTextureAtlas` 提供玩家皮肤区域，`ItemTextureAtlas` 提供物品 UV 区域。
- `MatrixStack` 负责把相机朝向、挥手动画和持物变换组合成最终模型矩阵。

## 上下游依赖关系

### 上游依赖（本模块依赖）
- **BipedModel**: 双足模型基类
- **ModelRenderer**: 模型部件渲染器
- **ItemModelCache**: 物品模型缓存单例
- **ItemTextureAtlas**: 物品纹理图集
- **EntityTextureAtlas**: 实体纹理图集
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
7. **物品网格不要共享缓存**: 主手和副手可能在同一帧持有不同物品，单缓存会导致每帧反复重建 GPU 网格
8. **旧网格不能立即销毁**: 需要按帧延迟回收，否则在飞帧可能仍在使用旧的 Vulkan 缓冲区
9. **ItemInHandRenderer 必须初始化**: 虽然 initialize() 不需要外部依赖，但必须调用才能正确设置默认变换参数
10. **第一人称根矩阵方向**: 使用"相机朝向基向量"构造，遵循 MC 前向定义：`forward = (-sin(yaw)*cos(pitch), sin(pitch), cos(yaw)*cos(pitch))`
11. **MatrixStack 后乘语义**: `current = current * transform`（PoseStack 语义）
