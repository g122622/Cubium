#include "world/blockentity/interactive/LecternEntity.hpp"

#include "item/core/Item.hpp"
#include "item/core/ItemStack.hpp"
#include "util/assert/AssertAll.hpp"
#include "world/IWorld.hpp"

namespace mc {
namespace blockentity {

namespace {

/**
 * @brief 判断物品是否属于讲台可接受的书籍类型。
 * @param item 物品指针。
 * @return true 表示该物品可放入讲台。
 * @note 当前项目尚未完整注册 BOOK/WRITTEN_BOOK 等专用物品，这里按命名后缀兼容。
 */
[[nodiscard]] bool isLecternBookItem(const Item* item) {
    if (item == nullptr) {
        return false;
    }

    const String& path = item->itemLocation().path();
    return path == "book" ||
           path == "written_book" ||
           path == "writable_book" ||
           path == "enchanted_book";
}

} // namespace

LecternEntity::LecternEntity(const BlockPos& pos)
    : BlockEntity(BlockEntityType::Lectern, pos)
    , m_inventory(1) {
}

LecternEntity::~LecternEntity() = default;

ItemStack LecternEntity::getBook() const {
    return m_inventory.getItem(SLOT_BOOK);
}

bool LecternEntity::setBook(const ItemStack& book) {
    if (!isValidBook(book)) {
        return false;
    }

    m_inventory.setItem(SLOT_BOOK, book);
    m_page = 0;
    setChanged();
    return true;
}

ItemStack LecternEntity::removeBook() {
    ItemStack book = m_inventory.extractItem(SLOT_BOOK);
    m_page = 0;
    setChanged();
    return book;
}

bool LecternEntity::hasBook() const {
    return !m_inventory.getItem(SLOT_BOOK).isEmpty();
}

i32 LecternEntity::getTotalPages() const {
    const ItemStack book = getBook();
    if (book.isEmpty()) {
        return 0;
    }

    const Item* item = book.getItem();
    if (item == nullptr) {
        return 0;
    }

    const String& path = item->itemLocation().path();
    if (path == "writable_book") {
        return 100;
    }

    if (path == "written_book") {
        // 当前阶段未接入书本 NBT，先使用原版上限作为稳定回退。
        return 100;
    }

    if (path == "enchanted_book") {
        return 1;
    }

    if (path == "book") {
        return 1;
    }

    return 1;
}

void LecternEntity::setPage(i32 page) {
    i32 totalPages = getTotalPages();
    if (totalPages == 0) {
        return;
    }

    page = std::max(0, std::min(page, totalPages - 1));
    if (m_page != page) {
        m_page = page;
        setChanged();
    }
}

bool LecternEntity::nextPage() {
    i32 totalPages = getTotalPages();
    if (totalPages == 0 || m_page >= totalPages - 1) {
        return false;
    }

    ++m_page;
    setChanged();
    return true;
}

bool LecternEntity::prevPage() {
    if (m_page <= 0) {
        return false;
    }

    --m_page;
    setChanged();
    return true;
}

i32 LecternEntity::getComparatorSignal() const {
    if (!hasBook()) {
        return 0;
    }

    const i32 totalPages = getTotalPages();
    if (totalPages <= 1) {
        return 1;
    }

    return static_cast<i32>((static_cast<f32>(m_page) / static_cast<f32>(totalPages - 1)) * 14.0f) + 1;
}

void LecternEntity::openContainer() {
    ++m_openCount;
    setChanged();
}

void LecternEntity::closeContainer() {
    if (m_openCount > 0) {
        --m_openCount;
        setChanged();
    }
}

void LecternEntity::tick(IWorld& world) {
    MC_UNUSED(world);
}

bool LecternEntity::isValidBook(const ItemStack& stack) {
    if (stack.isEmpty()) {
        return false;
    }

    return isLecternBookItem(stack.getItem());
}

void LecternEntity::updateBlockState(IWorld& world) {
    MC_UNUSED(world);
}

bool LecternEntity::load(const nlohmann::json& data) {
    if (!BlockEntity::load(data)) {
        return false;
    }

    m_inventory.clear();
    if (data.contains("Book") && data["Book"].is_object()) {
        const auto bookResult = ItemStack::fromJson(data["Book"]);
        if (bookResult.success()) {
            m_inventory.setItem(SLOT_BOOK, bookResult.value());
        }
    }

    if (data.contains("Page")) {
        setPage(data["Page"].get<i32>());
    } else {
        m_page = 0;
    }

    return true;
}

void LecternEntity::save(nlohmann::json& data) const {
    BlockEntity::save(data);

    const ItemStack book = m_inventory.getItem(SLOT_BOOK);
    if (!book.isEmpty()) {
        data["Book"] = book.toJson();
    }

    data["Page"] = m_page;
}

std::unique_ptr<BlockEntity> LecternEntity::clone() const {
    auto clone = std::make_unique<LecternEntity>(m_pos);
    clone->m_inventory.setItem(SLOT_BOOK, m_inventory.getItem(SLOT_BOOK));
    clone->m_page = m_page;
    clone->m_openCount = m_openCount;
    return clone;
}

} // namespace blockentity
} // namespace mc