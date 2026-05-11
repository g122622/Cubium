#pragma once

#include "common/core/Types.hpp"
#include "common/core/Result.hpp"
#include "Advancement.hpp"
#include "AdvancementList.hpp"
#include <memory>
#include <functional>

namespace mc::advancement {

/**
 * @brief 成就管理器
 *
 * 全局成就注册表，管理所有已加载的成就。
 * 参考 MC 1.16.5: net.minecraft.advancements.AdvancementManager
 *
 * 职责：
 * - 管理成就列表
 * - 提供成就查询接口
 * - 支持热重载
 *
 * 使用示例：
 * @code
 * auto& manager = AdvancementManager::instance();
 * auto advancement = manager.get(ResourceLocation("minecraft:story/mine_stone"));
 * for (auto root : manager.getRoots()) {
 *     // 遍历根成就
 * }
 * @endcode
 */
class AdvancementManager : public AdvancementList::IListener {
public:
    /**
     * @brief 获取单例实例
     */
    static AdvancementManager& instance();

    // 禁止拷贝
    AdvancementManager(const AdvancementManager&) = delete;
    AdvancementManager& operator=(const AdvancementManager&) = delete;

    // ========== 成就管理 ==========

    /**
     * @brief 注册成就
     * @param advancement 成就
     * @return 是否成功注册
     */
    bool registerAdvancement(Advancement::Ptr advancement);

    /**
     * @brief 移除成就
     * @param id 成就ID
     * @return 是否成功移除
     */
    bool removeAdvancement(const ResourceLocation& id);

    /**
     * @brief 清空所有成就
     */
    void clear();

    /**
     * @brief 获取成就
     * @param id 成就ID
     * @return 成就，如果不存在返回nullptr
     */
    [[nodiscard]] Advancement::Ptr get(const ResourceLocation& id) const;

    /**
     * @brief 检查成就是否存在
     */
    [[nodiscard]] bool contains(const ResourceLocation& id) const;

    /**
     * @brief 获取成就列表
     */
    [[nodiscard]] AdvancementList& getList() noexcept { return m_list; }
    [[nodiscard]] const AdvancementList& getList() const noexcept { return m_list; }

    /**
     * @brief 获取所有根成就
     */
    [[nodiscard]] const std::vector<Advancement::Ptr>& getRoots() const { return m_list.getRoots(); }

    /**
     * @brief 获取所有非根成就
     */
    [[nodiscard]] const std::vector<Advancement::Ptr>& getNonRoots() const { return m_list.getNonRoots(); }

    /**
     * @brief 遍历所有成就
     */
    void forEach(std::function<bool(Advancement::Ptr)> callback) const {
        m_list.forEach(std::move(callback));
    }

    // ========== 监听器 ==========

    /**
     * @brief 添加成就变化监听器
     */
    void addListener(AdvancementList::IListener* listener) {
        m_list.addListener(listener);
    }

    /**
     * @brief 移除成就变化监听器
     */
    void removeListener(AdvancementList::IListener* listener) {
        m_list.removeListener(listener);
    }

    // ========== 热重载 ==========

    /**
     * @brief 重新加载成就
     *
     * 清空当前成就并加载新成就，保持进度追踪。
     */
    void reload();

    // ========== AdvancementList::IListener 实现 ==========

    void onAdvancementAdded(Advancement::Ptr advancement) override;
    void onAdvancementRemoved(Advancement::Ptr advancement) override;
    void onAdvancementUpdated(Advancement::Ptr advancement) override;

private:
    AdvancementManager();
    ~AdvancementManager() = default;

    AdvancementList m_list;
};

} // namespace mc::advancement
