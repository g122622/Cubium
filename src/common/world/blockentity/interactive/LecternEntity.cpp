#include "world/blockentity/interactive/LecternEntity.hpp"
#include "world/IWorld.hpp"
#include "item/ItemStack.hpp"
#include "util/assert/AssertAll.hpp"

namespace mc {
namespace blockentity {

// ========== LecternEntity 实现 ==========

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
    m_page = 0;  // 重置页码
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
    if (!hasBook()) {
        return 0;
    }

    // TODO: 从书本获取页数
    // 书与笔: 最多50页
    // 成书: 实际页数
    // 附魔书: 0页
    const ItemStack& book = getBook();
    if (book.isEmpty()) {
        return 0;
    }

    // 暂时返回默认值
    MC_UNUSED(book);
    return 1;
}

void LecternEntity::setPage(i32 page) {
    i32 totalPages = getTotalPages();
    if (totalPages == 0) {
        return;
    }

    // 限制页码范围
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

    m_page++;
    setChanged();
    return true;
}

bool LecternEntity::prevPage() {
    if (m_page <= 0) {
        return false;
    }

    m_page--;
    setChanged();
    return true;
}

i32 LecternEntity::getComparatorSignal() const {
    if (!hasBook()) {
        return 0;
    }

    i32 totalPages = getTotalPages();
    if (totalPages <= 1) {
        return hasBook() ? 1 : 0;
    }

    // 比较器信号 = (当前页 / 总页数) * 14 + 1
    // 第一页: 1, 最后一页: 15
    return static_cast<i32>((static_cast<f32>(m_page) / static_cast<f32>(totalPages - 1)) * 14.0f) + 1;
}

void LecternEntity::openContainer() {
    m_openCount++;
    setChanged();
}

void LecternEntity::closeContainer() {
    if (m_openCount > 0) {
        m_openCount--;
        setChanged();
    }
}

void LecternEntity::tick(IWorld& world) {
    MC_UNUSED(world);
    // 讲台不需要tick更新
}

bool LecternEntity::isValidBook(const ItemStack& stack) {
    if (stack.isEmpty()) {
        return false;
    }

    // TODO: 检查物品是否是书与笔、成书或附魔书
    // 暂时返回true
    MC_UNUSED(stack);
    return true;
}

void LecternEntity::updateBlockState(IWorld& world) {
    // TODO: 更新方块的 HAS_BOOK 属性
    // BlockState state = world.getBlockState(m_pos);
    // if (state.hasProperty(BlockStateProperties::HAS_BOOK())) {
    //     world.setBlockState(m_pos, state.with(BlockStateProperties::HAS_BOOK(), hasBook()), 3);
    // }
    MC_UNUSED(world);
}

bool LecternEntity::load(const nlohmann::json& data) {
    if (!BlockEntity::load(data)) {
        return false;
    }

    // 加载书本
    if (data.contains("Book")) {
        // TODO: 加载ItemStack
        // m_inventory.load(data["Book"]);
    }

    // 加载页码
    if (data.contains("Page")) {
        m_page = data["Page"].get<i32>();
    }

    return true;
}

void LecternEntity::save(nlohmann::json& data) const {
    BlockEntity::save(data);

    // 保存书本
    if (!m_inventory.getItem(SLOT_BOOK).isEmpty()) {
        nlohmann::json bookJson;
        m_inventory.save(bookJson);
        data["Book"] = bookJson;
    }

    // 保存页码
    data["Page"] = m_page;
}

std::unique_ptr<BlockEntity> LecternEntity::clone() const {
    auto clone = std::make_unique<LecternEntity>(m_pos);
    clone->m_page = m_page;
    clone->m_openCount = m_openCount;
    // TODO: 复制物品
    return clone;
}

} // namespace blockentity
} // namespace mc
