/*
 * Copyright (c) 2026 Guo Yi
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 */

#include <gtest/gtest.h>

#include "entity/inventory/ISidedInventory.hpp"
#include "entity/inventory/InventoryRef.hpp"
#include "util/assert/AssertMacros.hpp"
#include "world/blockentity/core/SimpleInventory.hpp"

namespace mc {
namespace {

// ========== 测试用 ISidedInventory 实现 ==========

class TestSidedInventory : public ISidedInventory {
public:
    explicit TestSidedInventory(i32 size)
        : m_inventory(size)
    {}

    [[nodiscard]] i32 getContainerSize() const noexcept override { return m_inventory.getContainerSize(); }
    [[nodiscard]] bool isEmpty() const noexcept override { return m_inventory.isEmpty(); }
    [[nodiscard]] ItemStack getItem(i32 slot) const override { return m_inventory.getItem(slot); }
    void setItem(i32 slot, const ItemStack& stack) override { m_inventory.setItem(slot, stack); }
    ItemStack removeItem(i32 slot, i32 count) override { return m_inventory.removeItem(slot, count); }
    ItemStack removeItemNoUpdate(i32 slot) override { return m_inventory.removeItemNoUpdate(slot); }
    void clear() override { m_inventory.clear(); }
    void setChanged() override { m_inventory.setChanged(); }
    [[nodiscard]] std::vector<i32> getSlotsForFace(Direction side) const override
    {
        if (side == Direction::Up) {
            std::vector<i32> slots;
            for (i32 i = 0; i < getContainerSize(); ++i) {
                slots.push_back(i);
            }
            return slots;
        }
        return {};
    }
    [[nodiscard]] bool canInsertItem(i32 slot, const ItemStack& stack, Direction direction) const override
    {
        MC_UNUSED(slot);
        MC_UNUSED(stack);
        return direction == Direction::Up;
    }
    [[nodiscard]] bool canExtractItem(i32 slot, const ItemStack& stack, Direction direction) const override
    {
        MC_UNUSED(slot);
        MC_UNUSED(stack);
        return direction == Direction::Down;
    }

private:
    blockentity::SimpleInventory m_inventory;
};

} // namespace

// ========== 构造函数测试 ==========

TEST(InventoryRefTest, DefaultConstructor_CreatesEmptyRef)
{
    InventoryRef ref;
    EXPECT_EQ(ref.get(), nullptr);
    EXPECT_TRUE(ref.isEmpty());
    EXPECT_FALSE(ref.isOwning());
    EXPECT_FALSE(static_cast<bool>(ref));
    EXPECT_EQ(ref, nullptr);
}

TEST(InventoryRefTest, RawPointerConstructor_CreatesNonOwningRef)
{
    blockentity::SimpleInventory inventory(5);
    IInventory* rawPtr = &inventory;

    InventoryRef ref(rawPtr);
    EXPECT_EQ(ref.get(), rawPtr);
    EXPECT_FALSE(ref.isEmpty());
    EXPECT_FALSE(ref.isOwning());
    EXPECT_TRUE(static_cast<bool>(ref));
    EXPECT_NE(ref, nullptr);
}

TEST(InventoryRefTest, RawPointerConstructor_Nullptr)
{
    InventoryRef ref(nullptr);
    EXPECT_EQ(ref.get(), nullptr);
    EXPECT_TRUE(ref.isEmpty());
    EXPECT_FALSE(ref.isOwning());
}

TEST(InventoryRefTest, UniquePtrConstructor_CreatesOwningRef)
{
    auto owned = std::make_unique<TestSidedInventory>(3);
    ISidedInventory* rawPtr = owned.get();

    InventoryRef ref(std::move(owned));
    EXPECT_EQ(ref.get(), rawPtr);
    EXPECT_FALSE(ref.isEmpty());
    EXPECT_TRUE(ref.isOwning());
    EXPECT_NE(ref, nullptr);
    // owned 已经被 move 走
    EXPECT_EQ(owned.get(), nullptr);
}

TEST(InventoryRefTest, UniquePtrConstructor_NullUniquePtr)
{
    std::unique_ptr<ISidedInventory> nullPtr;
    InventoryRef ref(std::move(nullPtr));
    EXPECT_EQ(ref.get(), nullptr);
    EXPECT_TRUE(ref.isEmpty());
    EXPECT_FALSE(ref.isOwning());
}

// ========== 移动语义测试 ==========

TEST(InventoryRefTest, MoveConstructor_TransfersOwnership)
{
    auto owned = std::make_unique<TestSidedInventory>(3);
    ISidedInventory* rawPtr = owned.get();

    InventoryRef ref1(std::move(owned));
    InventoryRef ref2(std::move(ref1));

    // ref1 应该变为空
    EXPECT_EQ(ref1.get(), nullptr);
    EXPECT_TRUE(ref1.isEmpty());

    // ref2 应该拥有原来的指针
    EXPECT_EQ(ref2.get(), rawPtr);
    EXPECT_TRUE(ref2.isOwning());
}

TEST(InventoryRefTest, MoveConstructor_TransfersNonOwningRef)
{
    blockentity::SimpleInventory inventory(5);
    IInventory* rawPtr = &inventory;

    InventoryRef ref1(rawPtr);
    InventoryRef ref2(std::move(ref1));

    // ref1 应该变为空
    EXPECT_EQ(ref1.get(), nullptr);

    // ref2 应该持有非拥有引用
    EXPECT_EQ(ref2.get(), rawPtr);
    EXPECT_FALSE(ref2.isOwning());
}

TEST(InventoryRefTest, MoveAssignment_TransfersOwnership)
{
    auto owned = std::make_unique<TestSidedInventory>(3);
    ISidedInventory* rawPtr = owned.get();

    InventoryRef ref1(std::move(owned));
    InventoryRef ref2;
    ref2 = std::move(ref1);

    EXPECT_EQ(ref1.get(), nullptr);
    EXPECT_EQ(ref2.get(), rawPtr);
    EXPECT_TRUE(ref2.isOwning());
}

TEST(InventoryRefTest, MoveAssignment_SelfMoveIsNoop)
{
    auto owned = std::make_unique<TestSidedInventory>(3);
    ISidedInventory* rawPtr = owned.get();

    InventoryRef ref(std::move(owned));
    // 自移动是安全的（虽然不常见）
    ref = std::move(ref);
    // 行为实现定义，但不应崩溃
    EXPECT_NE(ref.get(), nullptr);
}

// ========== 所有权管理测试 ==========

TEST(InventoryRefTest, OwningRef_DestructsOwnedInventory)
{
    // 验证拥有引用析构时释放内存（不会泄漏或 double-free）
    ISidedInventory* rawPtr = nullptr;
    {
        auto owned = std::make_unique<TestSidedInventory>(3);
        rawPtr = owned.get();
        InventoryRef ref(std::move(owned));
        EXPECT_EQ(ref.get(), rawPtr);
        EXPECT_TRUE(ref.isOwning());
        // ref 析构时应该释放 TestSidedInventory
    }
    // rawPtr 现在是悬空指针，不访问它
    MC_UNUSED(rawPtr);
}

TEST(InventoryRefTest, NonOwningRef_DoesNotDestructInventory)
{
    // 验证非拥有引用析构时不释放内存
    auto inventory = std::make_unique<blockentity::SimpleInventory>(5);
    IInventory* rawPtr = inventory.get();
    {
        InventoryRef ref(rawPtr);
        EXPECT_EQ(ref.get(), rawPtr);
        EXPECT_FALSE(ref.isOwning());
        // ref 析构时不应释放 inventory
    }
    // inventory 应该仍然有效
    EXPECT_EQ(inventory->getContainerSize(), 5);
}

TEST(InventoryRefTest, MoveAssignment_OldOwningRefIsDestroyed)
{
    auto owned1 = std::make_unique<TestSidedInventory>(3);
    auto owned2 = std::make_unique<TestSidedInventory>(5);

    InventoryRef ref1(std::move(owned1));
    InventoryRef ref2(std::move(owned2));

    // ref2 赋值新值时，旧的 owned2 应该被销毁
    ref2 = std::move(ref1);
    EXPECT_EQ(ref1.get(), nullptr);
    EXPECT_TRUE(ref2.isOwning());
    EXPECT_EQ(ref2.get()->getContainerSize(), 3);
}

// ========== releaseOwnership 测试 ==========

TEST(InventoryRefTest, ReleaseOwnership_FromOwningRef)
{
    auto owned = std::make_unique<TestSidedInventory>(3);
    ISidedInventory* rawPtr = owned.get();

    InventoryRef ref(std::move(owned));
    EXPECT_TRUE(ref.isOwning());

    auto released = ref.releaseOwnership();
    EXPECT_EQ(released.get(), rawPtr);
    EXPECT_EQ(ref.get(), nullptr);
    EXPECT_FALSE(ref.isOwning());
    EXPECT_TRUE(ref.isEmpty());

    // released 现在拥有所有权
    EXPECT_EQ(released->getContainerSize(), 3);
}

TEST(InventoryRefTest, ReleaseOwnership_FromNonOwningRef)
{
    blockentity::SimpleInventory inventory(5);
    IInventory* rawPtr = &inventory;

    InventoryRef ref(rawPtr);
    EXPECT_FALSE(ref.isOwning());

    auto released = ref.releaseOwnership();
    EXPECT_EQ(released.get(), nullptr); // 非拥有引用不释放所有权
    EXPECT_EQ(ref.get(), nullptr);
    // 原始 inventory 仍然有效
    EXPECT_EQ(inventory.getContainerSize(), 5);
}

// ========== 操作符测试 ==========

TEST(InventoryRefTest, OperatorArrow_AccessesIInventory)
{
    blockentity::SimpleInventory inventory(5);
    InventoryRef ref(&inventory);

    EXPECT_EQ(ref->getContainerSize(), 5);
    EXPECT_TRUE(ref->isEmpty());
}

TEST(InventoryRefTest, OperatorDereference_AccessesIInventory)
{
    blockentity::SimpleInventory inventory(5);
    InventoryRef ref(&inventory);

    EXPECT_EQ((*ref).getContainerSize(), 5);
}

TEST(InventoryRefTest, OperatorBool_TrueWhenNonEmpty)
{
    blockentity::SimpleInventory inventory(5);
    InventoryRef ref(&inventory);
    EXPECT_TRUE(static_cast<bool>(ref));
}

TEST(InventoryRefTest, OperatorBool_FalseWhenEmpty)
{
    InventoryRef ref;
    EXPECT_FALSE(static_cast<bool>(ref));
}

TEST(InventoryRefTest, EqualityWithNullptr)
{
    blockentity::SimpleInventory inventory(5);
    InventoryRef ref(&inventory);
    InventoryRef emptyRef;

    EXPECT_FALSE(ref == nullptr);
    EXPECT_TRUE(ref != nullptr);
    EXPECT_TRUE(emptyRef == nullptr);
    EXPECT_FALSE(emptyRef != nullptr);
}

// ========== ownedSidedInventory 测试 ==========

TEST(InventoryRefTest, OwnedSidedInventory_ReturnsPtrWhenOwning)
{
    auto owned = std::make_unique<TestSidedInventory>(3);
    ISidedInventory* rawPtr = owned.get();

    InventoryRef ref(std::move(owned));
    EXPECT_EQ(ref.ownedSidedInventory(), rawPtr);
}

TEST(InventoryRefTest, OwnedSidedInventory_ReturnsNullptrWhenNonOwning)
{
    blockentity::SimpleInventory inventory(5);
    InventoryRef ref(&inventory);
    EXPECT_EQ(ref.ownedSidedInventory(), nullptr);
}

TEST(InventoryRefTest, OwnedSidedInventory_ReturnsNullptrWhenEmpty)
{
    InventoryRef ref;
    EXPECT_EQ(ref.ownedSidedInventory(), nullptr);
}

// ========== 综合场景测试 ==========

TEST(InventoryRefTest, Scenario_OwningRefLifetime)
{
    // 模拟 HopperEntity::getInventoryAtPosition 的使用场景
    // 创建一个拥有引用，使用它，然后让它自动销毁
    {
        auto owned = std::make_unique<TestSidedInventory>(3);
        InventoryRef ref(std::move(owned));

        // 通过 get() 访问 IInventory 接口
        IInventory* inv = ref.get();
        ASSERT_NE(inv, nullptr);
        EXPECT_EQ(inv->getContainerSize(), 3);

        // 通过 ownedSidedInventory() 访问 ISidedInventory 接口
        ISidedInventory* sidedInv = ref.ownedSidedInventory();
        ASSERT_NE(sidedInv, nullptr);
        EXPECT_EQ(sidedInv->getSlotsForFace(Direction::Up).size(), 3u);
        EXPECT_EQ(sidedInv->getSlotsForFace(Direction::Down).size(), 0u);
    }
    // ref 离开作用域，TestSidedInventory 被自动销毁
}

TEST(InventoryRefTest, Scenario_NonOwningRefLifetime)
{
    // 模拟 BlockEntity 路径返回的非拥有引用
    auto blockEntity = std::make_unique<blockentity::SimpleInventory>(5);
    IInventory* rawPtr = blockEntity.get();

    {
        InventoryRef ref(rawPtr);
        IInventory* inv = ref.get();
        ASSERT_NE(inv, nullptr);
        EXPECT_EQ(inv->getContainerSize(), 5);
        // ref 析构不会影响 blockEntity
    }

    // blockEntity 仍然有效
    EXPECT_EQ(blockEntity->getContainerSize(), 5);
}

TEST(InventoryRefTest, Scenario_Reassignment)
{
    // 先拥有一个，然后赋值另一个
    auto owned1 = std::make_unique<TestSidedInventory>(3);
    auto owned2 = std::make_unique<TestSidedInventory>(5);

    InventoryRef ref(std::move(owned1));
    EXPECT_EQ(ref->getContainerSize(), 3);

    ref = InventoryRef(std::move(owned2));
    EXPECT_EQ(ref->getContainerSize(), 5);
    // owned1 的 TestSidedInventory 在赋值时被销毁
}

} // namespace mc
