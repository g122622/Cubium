# 容器菜单

## 目录结构

```text
container/
├── AnvilContainer.hpp/cpp       # 铁砧重命名、合并和费用
├── BeaconContainer.hpp/cpp      # 信标支付与效果属性
├── BrewingStandContainer.hpp/cpp # 酿造台槽位和进度
├── CartographyContainer.hpp/cpp # 制图台输入和地图结果
├── ChestContainer.hpp/cpp       # 箱子及通用存储菜单
├── CrafterContainer.hpp/cpp     # 自动合成器网格和预览
├── EnchantmentContainer.hpp/cpp # 附魔台选项、费用和种子
├── FurnaceContainer.hpp/cpp     # 熔炉槽位和进度
├── HopperContainer.hpp/cpp      # 漏斗菜单
├── LoomContainer.hpp/cpp        # 织布机图案选择
├── MerchantContainer.hpp/cpp    # 三格交易输入及结果库存
├── MerchantContainerMenu.hpp/cpp # 商人报价选择及自动填充
└── MerchantResultSlot.hpp/cpp   # 交易结果结算
```

## 内部模块关系

所有菜单继承 AbstractContainerMenu，共用 Slot 和 PlayerInventory。方块库存菜单引用方块实体库存；临时输入菜单自己持有库存。MerchantContainerMenu 关联商人和报价表，结果槽完成交易结算。

## 上下游外部依赖关系

服务端容器工厂、方块/实体交互创建菜单，Play 处理器操作菜单，客户端屏幕读取菜单。依赖物品、配方、附魔、方块实体与库存接口。

## 容易踩的坑

- 菜单槽号与背包内部下标不同，Slot 必须绑定所属菜单及正确编号。
- 物理菜单有效性需同时校验世界、方块、距离和关联实体，失效时关闭。
- 结果槽是预览，关闭不能返还；领取需校验费用、消耗输入并仅结算一次。
- QuickMove 的取出回调由基类统一执行，子类不要再次结算。
- 临时输入关闭时返还或掉落；信标支付槽属于菜单，关闭按规则掉落，不能留在方块中。
- 信标属性效果 ID 使用注册 ID 加一、零表示无；set_beacon 的可选 holder 使用纯注册 ID。
- 织布机按钮是可选图案列表的零基索引；图案卷可重复使用，基础图案由配置标签排序。
- 附魔台同步十项属性；展示等级与实际扣费不同，创造模式免支付物品和等级。
