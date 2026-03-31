# 村庄模板资源 (Village Template Resources)

本目录包含村庄结构的模板配置文件。

## 目录结构

```
resources/data/minecraft/
├── worldgen/
│   ├── template_pool/
│   │   └── village/
│   │       ├── plains/
│   │       │   ├── village_plains.json       # 起始池
│   │       │   ├── houses.json               # 房屋模板池
│   │       │   ├── streets.json              # 街道模板池
│   │       │   ├── terminals.json            # 终端模板池
│   │       │   └── decorators.json           # 装饰模板池
│   │       ├── desert/
│   │       ├── savanna/
│   │       ├── taiga/
│   │       └── snowy/
│   └── structure/
│       └── village/
│           ├── plains/
│           │   ├── houses/
│           │   │   ├── plains_small_house_1.nbt
│           │   │   ├── plains_small_house_2.nbt
│           │   │   ├── plains_medium_house_1.nbt
│           │   │   └── ...
│           │   ├── streets/
│           │   │   ├── plains_street_1.nbt
│           │   │   └── ...
│           │   └── centers/
│           │       ├── plains_fountain_1.nbt
│           │       └── ...
│           ├── desert/
│           ├── savanna/
│           ├── taiga/
│           └── snowy/
```

## 模板池配置格式

### 起始池 (village_plains.json)

```json
{
    "name": "minecraft:village/plains/village_plains",
    "fallback": "minecraft:village/plains/terminators",
    "elements": [
        {
            "weight": 1,
            "element": {
                "location": "minecraft:village/plains/town_centers/plains_fountain_01",
                "processors": "minecraft:street_plains",
                "projection": "rigid",
                "element_type": "single_pool_element"
            }
        },
        {
            "weight": 1,
            "element": {
                "location": "minecraft:village/plains/town_centers/plains_meeting_point_01",
                "processors": "minecraft:street_plains",
                "projection": "rigid",
                "element_type": "single_pool_element"
            }
        }
    ]
}
```

### 房屋池 (houses.json)

```json
{
    "name": "minecraft:village/plains/houses",
    "fallback": "minecraft:village/plains/terminators",
    "elements": [
        {
            "weight": 4,
            "element": {
                "location": "minecraft:village/plains/houses/plains_small_house_01",
                "processors": "minecraft:empty",
                "projection": "rigid",
                "element_type": "single_pool_element"
            }
        },
        {
            "weight": 2,
            "element": {
                "location": "minecraft:village/plains/houses/plains_medium_house_01",
                "processors": "minecraft:empty",
                "projection": "rigid",
                "element_type": "single_pool_element"
            }
        },
        {
            "weight": 1,
            "element": {
                "location": "minecraft:village/plains/houses/plains_butcher_shop_01",
                "processors": "minecraft:empty",
                "projection": "rigid",
                "element_type": "single_pool_element"
            }
        }
    ]
}
```

## 村庄建筑类型

### 平原村庄
| 建筑 | 文件名 | 说明 |
|------|--------|------|
| 小屋 | plains_small_house_01-06 | 基本民居 |
| 中屋 | plains_medium_house_01-04 | 较大民居 |
| 大屋 | plains_large_house_01-02 | 富裕民居 |
| 屠夫店 | plains_butcher_shop_01-02 | 屠夫职业建筑 |
| 图书馆 | plains_library_01 | 图书管理员职业建筑 |
| 农田 | plains_farm_01-05 | 农作物农田 |
| 铁匠铺 | plains_blacksmith_01 | 铁匠职业建筑 |
| 教堂 | plains_church_01 | 教堂建筑 |
| 喷泉 | plains_fountain_01 | 中心广场喷泉 |
| 灯柱 | plains_lamp_post_01 | 照明灯柱 |

### 沙漠村庄
- 使用砂岩、切制砂岩、平滑砂岩
- 平顶建筑风格
- 悬挂地毯装饰

### 热带草原村庄
- 使用金合欢木、金合欢木板
- 阳台和平台设计
- 明亮的装饰风格

### 针叶林村庄
- 使用云杉木、云杉木板
- 圆石基础
- 倾斜屋顶设计

### 雪地村庄
- 使用云杉木、雪块
- 尖顶设计
- 冰窗装饰

## Jigsaw连接点

每个建筑模板通过Jigsaw连接点与其他建筑连接：

```
+------------------+
|     房屋         |
|                  |
|  [bottom] ---------> 连接到街道
|                  |
|  [side_left] -----> 连接到相邻房屋
|  [side_right] ----> 连接到相邻房屋
|                  |
+------------------+
```

### 常用连接点名称
- `bottom` - 向下连接（主入口）
- `top` - 向上连接（上层建筑）
- `side_left` - 左侧连接
- `side_right` - 右侧连接
- `front` - 前方连接
- `back` - 后方连接

## 处理器列表

### 街道处理器 (street_plains)
- 替换泥土路径为草径
- 替换草地为沙砾
- 添加灯柱

### 僵尸村庄处理器 (zombie_village)
- 替换木板为蜘蛛网
- 替换玻璃为空气
- 移除门
- 添加僵尸村民

## 实体配置

每个建筑可以包含预设实体：

```json
{
    "entities": [
        {
            "type": "minecraft:villager",
            "pos": [5, 0, 3],
            "nbt": {
                "Profession": "minecraft:butcher",
                "Level": 2
            }
        },
        {
            "type": "minecraft:cat",
            "pos": [2, 0, 2],
            "nbt": {}
        }
    ]
}
```

## 生成流程

```
1. 选择村庄类型（基于生物群系）
2. 加载对应的起始模板池
3. 从起始池随机选择一个中心建筑
4. 通过Jigsaw连接点递归扩展：
   a. 对于每个连接点，查找目标模板池
   b. 从池中随机选择模板
   c. 放置模板并处理旋转/镜像
   d. 重复直到达到深度限制或无更多连接点
5. 应用处理器（街道装饰、僵尸化等）
6. 生成村民和动物实体
```

## NBT文件格式

模板文件使用NBT格式存储：

```
CompoundTag:
- size: [x, y, z] (Int array)
- blocks: ListTag
  - pos: [x, y, z] (Int array)
  - state: index (Int)
  - nbt: CompoundTag (可选，方块实体数据)
- palette: ListTag
  - Name: "minecraft:block_id" (String)
  - Properties: CompoundTag (可选)
- entities: ListTag (可选)
  - pos: [x, y, z] (Double array)
  - blockPos: [x, y, z] (Int array)
  - nbt: CompoundTag
```

## TODO

- [ ] 创建平原村庄模板
- [ ] 创建沙漠村庄模板
- [ ] 创建热带草原村庄模板
- [ ] 创建针叶林村庄模板
- [ ] 创建雪地村庄模板
- [ ] 实现NBT模板加载器
- [ ] 实现Jigsaw组装算法
- [ ] 实现村庄处理器

## 参考

- MC 1.16.5 `data/minecraft/worldgen/template_pool/village/`
- MC 1.16.5 NBT Structure Format
