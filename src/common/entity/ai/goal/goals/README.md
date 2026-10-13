# 生物行为目标

## 目录结构树

```text
goals/
├── AdditionalGoals.hpp/cpp     # 其他通用行为目标
├── AvoidBlockGoal.hpp/cpp      # 躲避指定方块
├── AvoidEntityGoal.hpp/cpp     # 躲避指定实体
├── BreedGoal.hpp/cpp           # 动物繁殖
├── EatGrassGoal.hpp/cpp        # 吃草行为
├── FindShelterGoal.hpp/cpp     # 寻找庇护所
├── FindWaterGoal.hpp/cpp       # 寻找水源
├── FishSwimGoal.hpp/cpp        # 鱼类游泳
├── FleeSunGoal.hpp/cpp         # 躲避日照
├── FlyGoal.hpp/cpp             # 飞行行为
├── FollowParentGoal.hpp/cpp    # 幼体跟随父母
├── LookAtGoal.hpp/cpp          # 注视实体与随机环顾
├── MeleeAttackGoal.hpp/cpp     # 近战追击与攻击
├── PanicGoal.hpp/cpp           # 受伤逃跑
├── RandomSwimmingGoal.hpp/cpp # 随机游泳
├── RandomWalkingGoal.hpp/cpp  # 随机漫步
├── RestrictSunGoal.hpp/cpp     # 限制导航经过日照区域
├── SwimGoal.hpp/cpp            # 水中漂浮
├── SwimUpGoal.hpp/cpp          # 向上游动
├── TemptGoal.hpp/cpp           # 跟随持有诱惑物品的玩家
├── attack/                    # 远程攻击目标
├── interact/                  # 驯服与玩家互动目标
├── movement/                  # 移动与跟随目标
├── special/                   # 特定实体的专属目标
├── target/                    # 攻击目标选择
├── villager/                  # 村民日常行为
└── README.md                  # 依赖关系与生命周期约束
```

## 内部模块关系

行为类继承 `Goal`，由 `GoalSelector` 按优先级和互斥标志选择、启动、更新和停止。目标选择类为生物设置攻击目标，攻击类消费该目标。

## 上下游外部依赖关系

实体注册并拥有目标对象。本目录依赖实体核心、`EntityReference`、导航、感知、控制器和世界查询接口；各行为子目录依赖对应的专属实体接口。

## 容易踩的坑

- 目标所属实体可能因模拟距离暂停 tick，目标缓存的存活不能依赖下一次 `resetTask()`。跨 tick 目标使用 `EntityReference`，在实体移除或析构时自动失效。
- 引用有效性与目标生命值分别检查；生命值归零但尚未移除时仍须判断 `isAlive()`。
- 失效目标应使 `shouldContinueExecuting()` 返回 false，随后由选择器清理导航和姿态，避免占用其他目标需要的互斥标志。
- 近战目标持续判定检查目标状态和追踪范围；非长期记忆目标还要求当前路径，长期记忆目标通过家范围检查后可无路径继续。不能复用带启动冷却的 `shouldExecute()`，否则持续追击会反复中断。
