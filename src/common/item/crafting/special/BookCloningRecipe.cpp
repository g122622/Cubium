#include "item/crafting/special/BookCloningRecipe.hpp"

namespace mc {
namespace crafting {

BookCloningRecipe::BookCloningRecipe(const ResourceLocation& id)
    : SpecialRecipe(id) {
}

bool BookCloningRecipe::matches(const CraftingInventory& inventory) const {
    bool hasWrittenBook = false;
    bool hasWritableBook = false;

    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack stack = inventory.getItem(i);
        if (stack.isEmpty()) {
            continue;
        }

        if (isWrittenBook(stack)) {
            if (hasWrittenBook) {
                // 只能有一本成书
                return false;
            }
            hasWrittenBook = true;
            // 检查代数，最多复制到第二代
            if (getGeneration(stack) >= 2) {
                return false;
            }
        } else if (isWritableBook(stack)) {
            hasWritableBook = true;
        } else {
            // 有其他物品，不匹配
            return false;
        }
    }

    return hasWrittenBook && hasWritableBook;
}

ItemStack BookCloningRecipe::assemble(const CraftingInventory& inventory) const {
    ItemStack writtenBook;
    i32 writableBookCount = 0;

    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack stack = inventory.getItem(i);
        if (stack.isEmpty()) {
            continue;
        }

        if (isWrittenBook(stack)) {
            writtenBook = stack;
        } else if (isWritableBook(stack)) {
            writableBookCount += stack.getCount();
        }
    }

    if (writtenBook.isEmpty() || writableBookCount == 0) {
        return ItemStack::EMPTY;
    }

    // 检查代数
    i32 generation = getGeneration(writtenBook);
    if (generation >= 2) {
        return ItemStack::EMPTY;
    }

    // 创建复制的书
    ItemStack result = writtenBook.copy();
    result.setCount(writableBookCount);
    setGeneration(result, generation + 1);

    return result;
}

std::vector<ItemStack> BookCloningRecipe::getRemainingItems(const CraftingInventory& inventory) const {
    std::vector<ItemStack> remaining(inventory.getContainerSize());

    // 保留原书（只有一本）
    for (i32 i = 0; i < inventory.getContainerSize(); ++i) {
        ItemStack stack = inventory.getItem(i);
        if (isWrittenBook(stack)) {
            ItemStack originalBook = stack.copy();
            originalBook.setCount(1);
            remaining[i] = originalBook;
            break;
        }
    }

    return remaining;
}

bool BookCloningRecipe::isWrittenBook(const ItemStack& stack) {
    if (stack.isEmpty()) {
        return false;
    }
    // TODO: 检查物品是否为 WrittenBookItem
    // 需要实现物品类型检查
    return false;
}

bool BookCloningRecipe::isWritableBook(const ItemStack& stack) {
    if (stack.isEmpty()) {
        return false;
    }
    // TODO: 检查物品是否为 WritableBookItem
    // 需要实现物品类型检查
    return false;
}

i32 BookCloningRecipe::getGeneration(const ItemStack& stack) {
    // 从 NBT 标签获取代数
    const nlohmann::json* tag = stack.getTag();
    if (tag == nullptr || !tag->is_object()) {
        return 0;
    }

    auto it = tag->find("generation");
    if (it == tag->end() || !it->is_number()) {
        return 0;
    }

    return it->get<i32>();
}

void BookCloningRecipe::setGeneration(ItemStack& stack, i32 generation) {
    nlohmann::json& tag = stack.getOrCreateTag();
    tag["generation"] = generation;
}

} // namespace crafting
} // namespace mc
