/**
 * @file ReactiveStateTest.cpp
 * @brief Reactive<T>, Computed<T>, Binding<T> 单元测试
 *
 * 测试覆盖率目标：95%+
 *
 * 测试场景：
 * - Reactive 基本操作（get/set/observe）
 * - Reactive 观察者模式（observe/removeObserver/clearObservers）
 * - Reactive 修改器模式（modify/forceNotify）
 * - Reactive 隐式转换和赋值
 * - Computed 计算属性
 * - Binding 创建和使用
 */

#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <vector>
#include "client/ui/kagero/state/ReactiveState.hpp"

using namespace mc::client::ui::kagero::state;
using mc::i32;
using mc::std::string;
using mc::f32;

// ============================================================================
// Reactive 基本操作测试
// ============================================================================

TEST(ReactiveTest, DefaultConstructor) {
    Reactive<i32> count;
    EXPECT_EQ(count.get(), 0);
}

TEST(ReactiveTest, InitialValue) {
    Reactive<i32> count(42);
    EXPECT_EQ(count.get(), 42);

    Reactive<std::string> name("Steve");
    EXPECT_EQ(name.get(), "Steve");

    Reactive<f32> value(3.14f);
    EXPECT_FLOAT_EQ(value.get(), 3.14f);
}

TEST(ReactiveTest, SetSameValue) {
    Reactive<i32> count(10);

    int callCount = 0;
    count.observe([&](const i32&, const i32&) {
        callCount++;
    });

    count.set(10); // 设置相同的值
    EXPECT_EQ(callCount, 0); // 不应该通知
}

TEST(ReactiveTest, SetDifferentValue) {
    Reactive<i32> count(10);

    int callCount = 0;
    i32 oldVal = 0, newVal = 0;
    count.observe([&](const i32& old, const i32& newV) {
        callCount++;
        oldVal = old;
        newVal = newV;
    });

    count.set(20);
    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(oldVal, 10);
    EXPECT_EQ(newVal, 20);
}

TEST(ReactiveTest, SetMoveValue) {
    Reactive<std::string> name("Steve");

    int callCount = 0;
    name.observe([&](const std::string&, const std::string&) {
        callCount++;
    });

    std::string newName = "Alex";
    name.set(std::move(newName));
    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(name.get(), "Alex");
}

// ============================================================================
// 隐式转换测试
// ============================================================================

TEST(ReactiveTest, ImplicitConversion) {
    Reactive<i32> count(42);

    i32 value = count; // 隐式转换
    EXPECT_EQ(value, 42);

    const Reactive<i32>& ref = count;
    i32 value2 = ref; // const 引用的隐式转换
    EXPECT_EQ(value2, 42);
}

TEST(ReactiveTest, AssignmentOperator) {
    Reactive<i32> count(10);

    int callCount = 0;
    count.observe([&](const i32&, const i32&) {
        callCount++;
    });

    count = 20; // 赋值操作符
    EXPECT_EQ(count.get(), 20);
    EXPECT_EQ(callCount, 1);

    count = 20; // 相同值
    EXPECT_EQ(callCount, 1); // 不应该再通知
}

// ============================================================================
// 观察者模式测试
// ============================================================================

TEST(ReactiveTest, Observe) {
    Reactive<i32> health(100);

    std::vector<i32> oldValues;
    std::vector<i32> newValues;

    auto id = health.observe([&](const i32& old, const i32& newVal) {
        oldValues.push_back(old);
        newValues.push_back(newVal);
    });

    health.set(80);
    health.set(60);
    health.set(40);

    EXPECT_EQ(oldValues.size(), 3u);
    EXPECT_EQ(newValues.size(), 3u);

    EXPECT_EQ(oldValues[0], 100);
    EXPECT_EQ(newValues[0], 80);
    EXPECT_EQ(oldValues[1], 80);
    EXPECT_EQ(newValues[1], 60);
    EXPECT_EQ(oldValues[2], 60);
    EXPECT_EQ(newValues[2], 40);

    health.removeObserver(id);
}

TEST(ReactiveTest, MultipleObservers) {
    Reactive<i32> value(0);

    int count1 = 0, count2 = 0, count3 = 0;

    auto id1 = value.observe([&](const i32&, const i32&) { count1++; });
    auto id2 = value.observe([&](const i32&, const i32&) { count2++; });
    auto id3 = value.observe([&](const i32&, const i32&) { count3++; });

    value.set(10);

    EXPECT_EQ(count1, 1);
    EXPECT_EQ(count2, 1);
    EXPECT_EQ(count3, 1);

    value.removeObserver(id2);

    value.set(20);

    EXPECT_EQ(count1, 2);
    EXPECT_EQ(count2, 1); // 已移除
    EXPECT_EQ(count3, 2);

    value.removeObserver(id1);
    value.removeObserver(id3);
}

TEST(ReactiveTest, RemoveObserver) {
    Reactive<i32> value(0);

    int callCount = 0;
    auto id = value.observe([&](const i32&, const i32&) {
        callCount++;
    });

    EXPECT_TRUE(value.removeObserver(id));
    EXPECT_FALSE(value.removeObserver(id)); // 已移除
    EXPECT_FALSE(value.removeObserver(99999)); // 不存在的ID

    value.set(10);
    EXPECT_EQ(callCount, 0);
}

TEST(ReactiveTest, ClearObservers) {
    Reactive<i32> value(0);

    value.observe([&](const i32&, const i32&) {});
    value.observe([&](const i32&, const i32&) {});
    value.observe([&](const i32&, const i32&) {});

    EXPECT_EQ(value.observerCount(), 3u);

    value.clearObservers();

    EXPECT_EQ(value.observerCount(), 0u);
}

TEST(ReactiveTest, ObserverCount) {
    Reactive<i32> value(0);

    EXPECT_EQ(value.observerCount(), 0u);

    auto id1 = value.observe([](const i32&, const i32&) {});
    EXPECT_EQ(value.observerCount(), 1u);

    auto id2 = value.observe([](const i32&, const i32&) {});
    EXPECT_EQ(value.observerCount(), 2u);

    value.removeObserver(id1);
    EXPECT_EQ(value.observerCount(), 1u);

    value.removeObserver(id2);
    EXPECT_EQ(value.observerCount(), 0u);
}

TEST(ReactiveTest, ObserverModificationDuringNotification) {
    Reactive<i32> value(0);

    auto id1 = value.observe([&](const i32&, const i32& newVal) {
        if (newVal == 10) {
            value.set(20); // 在通知中修改
        }
    });

    auto id2 = value.observe([&](const i32&, const i32&) {
        // 第二个观察者
    });

    value.set(10);

    EXPECT_EQ(value.get(), 20);

    value.removeObserver(id1);
    value.removeObserver(id2);
}

// ============================================================================
// 修改器模式测试
// ============================================================================

TEST(ReactiveTest, Modify) {
    Reactive<std::vector<i32>> vec;

    vec.modify([](std::vector<i32>& v) {
        v.push_back(1);
        v.push_back(2);
        v.push_back(3);
    });

    EXPECT_EQ(vec.get().size(), 3u);
    EXPECT_EQ(vec.get()[0], 1);
    EXPECT_EQ(vec.get()[1], 2);
    EXPECT_EQ(vec.get()[2], 3);
}

TEST(ReactiveTest, ModifyTriggersNotification) {
    Reactive<std::vector<i32>> vec;
    int callCount = 0;

    vec.observe([&](const std::vector<i32>&, const std::vector<i32>&) {
        callCount++;
    });

    vec.modify([](std::vector<i32>& v) {
        v.push_back(1);
    });

    EXPECT_EQ(callCount, 1);
}

TEST(ReactiveTest, ModifyNoChange) {
    Reactive<std::vector<i32>> vec;
    int callCount = 0;

    vec.observe([&](const std::vector<i32>&, const std::vector<i32>&) {
        callCount++;
    });

    vec.modify([](std::vector<i32>&) {
        // 不修改
    });

    EXPECT_EQ(callCount, 0);
}

TEST(ReactiveTest, ForceNotify) {
    Reactive<i32> value(10);
    int callCount = 0;

    value.observe([&](const i32&, const i32&) {
        callCount++;
    });

    value.forceNotify();

    EXPECT_EQ(callCount, 1);
}

TEST(ReactiveTest, GetRef) {
    Reactive<i32> value(10);
    i32& ref = value.getRef();
    EXPECT_EQ(ref, 10);

    ref = 20;
    // 注意：直接修改引用不会触发通知
    EXPECT_EQ(value.get(), 20);
}

// ============================================================================
// Computed 测试
// ============================================================================

TEST(ComputedTest, BasicComputation) {
    Reactive<i32> a(10);
    Reactive<i32> b(20);

    Computed<i32> sum([&]() {
        return a.get() + b.get();
    });

    EXPECT_EQ(sum.get(), 30);
}

TEST(ComputedTest, DirtyMarking) {
    Reactive<i32> a(10);
    Reactive<i32> b(20);

    Computed<i32> sum([&]() {
        return a.get() + b.get();
    });

    EXPECT_EQ(sum.get(), 30);

    a.set(15);
    sum.markDirty();

    EXPECT_EQ(sum.get(), 35);
}

TEST(ComputedTest, Caching) {
    Reactive<i32> a(10);

    int computeCount = 0;

    Computed<i32> computed([&]() {
        computeCount++;
        return a.get() * 2;
    });

    EXPECT_EQ(computed.get(), 20);
    EXPECT_EQ(computeCount, 1);

    // 再次获取应该使用缓存
    EXPECT_EQ(computed.get(), 20);
    EXPECT_EQ(computeCount, 1);

    // 标记脏后重新计算
    a.set(15);
    computed.markDirty();
    EXPECT_EQ(computed.get(), 30);
    EXPECT_EQ(computeCount, 2);
}

TEST(ComputedTest, ImplicitConversion) {
    Reactive<i32> a(10);

    Computed<i32> doubled([&]() {
        return a.get() * 2;
    });

    i32 value = doubled; // 隐式转换
    EXPECT_EQ(value, 20);
}

TEST(ComputedTest, ComplexComputation) {
    Reactive<std::string> firstName("John");
    Reactive<std::string> lastName("Doe");

    Computed<std::string> fullName([&]() {
        return firstName.get() + " " + lastName.get();
    });

    EXPECT_EQ(fullName.get(), "John Doe");

    firstName.set("Jane");
    fullName.markDirty();
    EXPECT_EQ(fullName.get(), "Jane Doe");
}

// ============================================================================
// Binding 测试
// ============================================================================

TEST(BindingTest, ReadOnly) {
    Reactive<i32> value(42);

    Binding<i32> binding = Binding<i32>::readOnly([&value]() {
        return value.get();
    });

    EXPECT_EQ(binding.get(), 42);
    EXPECT_FALSE(binding.isWritable());
}

TEST(BindingTest, TwoWay) {
    Reactive<i32> value(42);

    Binding<i32> binding = Binding<i32>::twoWay(
        [&value]() { return value.get(); },
        [&value](const i32& v) { value.set(v); }
    );

    EXPECT_EQ(binding.get(), 42);
    EXPECT_TRUE(binding.isWritable());

    binding.set(100);
    EXPECT_EQ(value.get(), 100);
    EXPECT_EQ(binding.get(), 100);
}

TEST(BindingTest, FromReactive) {
    Reactive<i32> value(42);

    Binding<i32> binding = Binding<i32>::fromReactive(value);

    EXPECT_EQ(binding.get(), 42);
    EXPECT_TRUE(binding.isWritable());

    binding.set(100);
    EXPECT_EQ(value.get(), 100);

    value.set(200);
    EXPECT_EQ(binding.get(), 200);
}

TEST(BindingTest, Constant) {
    Binding<i32> binding = Binding<i32>::constant(42);

    EXPECT_EQ(binding.get(), 42);
    EXPECT_FALSE(binding.isWritable());
}

TEST(BindingTest, ImplicitConversion) {
    Binding<i32> binding = Binding<i32>::constant(42);

    i32 value = binding; // 隐式转换
    EXPECT_EQ(value, 42);
}

TEST(BindingTest, SetOnReadOnlyDoesNothing) {
    Binding<i32> binding = Binding<i32>::constant(42);

    // 设置只读绑定应该无效果，不应崩溃
    EXPECT_NO_THROW(binding.set(100));
    EXPECT_EQ(binding.get(), 42);
}

// ============================================================================
// 复杂场景测试
// ============================================================================

TEST(ReactiveTest, ChainedObservations) {
    Reactive<i32> a(10);
    Reactive<i32> b(0);

    // 当 a 变化时，更新 b
    a.observe([&](const i32&, const i32& newVal) {
        b.set(newVal * 2);
    });

    a.set(20);
    EXPECT_EQ(b.get(), 40);

    a.set(30);
    EXPECT_EQ(b.get(), 60);
}

TEST(ReactiveTest, ComplexTypeModification) {
    struct Data {
        i32 x = 0;
        i32 y = 0;

        bool operator==(const Data& other) const {
            return x == other.x && y == other.y;
        }

        bool operator!=(const Data& other) const {
            return !(*this == other);
        }
    };

    Reactive<Data> data;

    int callCount = 0;
    data.observe([&](const Data&, const Data&) {
        callCount++;
    });

    data.modify([](Data& d) {
        d.x = 10;
        d.y = 20;
    });

    EXPECT_EQ(callCount, 1);
    EXPECT_EQ(data.get().x, 10);
    EXPECT_EQ(data.get().y, 20);
}

TEST(ReactiveTest, NestedReactive) {
    struct Outer {
        Reactive<i32> inner{0};
    };

    Outer outer;
    outer.inner.set(42);

    EXPECT_EQ(outer.inner.get(), 42);
}
