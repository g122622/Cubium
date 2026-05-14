#pragma once

#include "Advancement.hpp"
#include "common/core/Result.hpp"
#include "common/core/Types.hpp"
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace mc::advancement {

/**
 * @brief 成就列表管理器
 *
 * 管理所有成就，维护父子关系，提供根节点和非根节点集合。
 * 参考 MC 1.16.5: net.minecraft.advancements.AdvancementList
 *
 * 职责：
 * - 管理成就注册表
 * - 解析父子关系
 * - 维护根节点和非根节点集合
 * - 提供成就查询接口
 */
class AdvancementList {
public:
    /**
     * @brief 成就变化监听器
     */
    class IListener {
    public:
        virtual ~IListener() = default;

        /**
         * @brief 成就添加回调
         * @param advancement 添加的成就
         */
        virtual void onAdvancementAdded(Advancement::Ptr advancement) = 0;

        /**
         * @brief 成就移除回调
         * @param advancement 移除的成就
         */
        virtual void onAdvancementRemoved(Advancement::Ptr advancement) = 0;

        /**
         * @brief 成就更新回调（子成就添加等）
         * @param advancement 更新的成就
         */
        virtual void onAdvancementUpdated(Advancement::Ptr advancement) = 0;
    };

    /**
     * @brief 默认构造
     */
    AdvancementList() = default;

    // 禁止拷贝
    AdvancementList(const AdvancementList&) = delete;
    AdvancementList& operator=(const AdvancementList&) = delete;

    // ========== 成就管理 ==========

    /**
     * @brief 添加成就
     * @param advancement 成就
     * @return 是否成功添加
     */
    bool add(Advancement::Ptr advancement);

    /**
     * @brief 移除成就
     * @param id 成就ID
     * @return 是否成功移除
     */
    bool remove(const ResourceLocation& id);

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
     * @param id 成就ID
     */
    [[nodiscard]] bool contains(const ResourceLocation& id) const;

    /**
     * @brief 获取所有成就
     */
    [[nodiscard]] const std::unordered_map<ResourceLocation, Advancement::Ptr>& getAll() const noexcept
    {
        return m_advancements;
    }

    /**
     * @brief 获取成就数量
     */
    [[nodiscard]] size_t size() const noexcept { return m_advancements.size(); }

    // ========== 根节点 ==========

    /**
     * @brief 获取所有根成就
     *
     * 根成就是没有父成就的成就，通常是每个标签页的第一个成就。
     */
    [[nodiscard]] const std::vector<Advancement::Ptr>& getRoots() const noexcept { return m_roots; }

    /**
     * @brief 获取所有非根成就
     */
    [[nodiscard]] const std::vector<Advancement::Ptr>& getNonRoots() const noexcept { return m_nonRoots; }

    // ========== 遍历 ==========

    /**
     * @brief 遍历所有成就
     * @param callback 回调函数，返回false停止遍历
     */
    void forEach(std::function<bool(Advancement::Ptr)> callback) const;

    /**
     * @brief 遍历根成就
     * @param callback 回调函数，返回false停止遍历
     */
    void forEachRoot(std::function<bool(Advancement::Ptr)> callback) const;

    // ========== 监听器 ==========

    /**
     * @brief 添加监听器
     */
    void addListener(IListener* listener);

    /**
     * @brief 移除监听器
     */
    void removeListener(IListener* listener);

    // ========== 父子关系解析 ==========

    /**
     * @brief 重新解析父子关系
     *
     * 当所有成就加载完成后调用，建立父子关系。
     */
    void rebuildRelations();

private:
    /**
     * @brief 尝试建立父子关系
     * @param advancement 成就
     * @return 是否成功建立
     */
    bool trySetParent(Advancement::Ptr advancement);

    /**
     * @brief 通知监听器成就添加
     */
    void notifyAdvancementAdded(Advancement::Ptr advancement);

    /**
     * @brief 通知监听器成就移除
     */
    void notifyAdvancementRemoved(Advancement::Ptr advancement);

    /**
     * @brief 通知监听器成就更新
     */
    void notifyAdvancementUpdated(Advancement::Ptr advancement);

    std::unordered_map<ResourceLocation, Advancement::Ptr> m_advancements;
    std::vector<Advancement::Ptr> m_roots;
    std::vector<Advancement::Ptr> m_nonRoots;
    std::vector<IListener*> m_listeners;

    // 等待父成就的成就（父成就尚未加载）
    std::unordered_map<ResourceLocation, std::vector<Advancement::Ptr>> m_waitingForParent;
};

} // namespace mc::advancement
