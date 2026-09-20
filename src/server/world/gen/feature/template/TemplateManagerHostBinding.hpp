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

#pragma once

#include "common/core/Types.hpp"

namespace mc {
namespace resource {
class DataPackRepository;
} // namespace resource
} // namespace mc

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

class IStructurePackSource;
class TemplateManager;

/**
 * @brief TemplateManager 宿主资源绑定令牌
 *
 * TemplateManager 在进程内是单例（JigsawAssembler::getTemplateManager()），
 * 而它加载模板所需的来源（数据包仓库、结构包资源源）都是宿主对象的非拥有指针。
 * 宿主一旦销毁，单例里的指针立即悬垂；此后任何一次模板加载（宿主存活期外的单元测试、
 * 或同一进程内启动的下一个服务端）都会解引用已释放对象，表现为 SIGSEGV，
 * 或 std::system_error("mutex lock failed: Invalid argument")。
 *
 * 本令牌把"绑定—解绑"收敛为一次 RAII：宿主把它作为成员持有，成员析构即自动解绑，
 * 因此不存在"忘记解绑"的可能；TemplateManager 的注入接口已私有化，只能经本令牌访问。
 *
 * 解绑采用"仅当当前绑定仍来自本令牌"语义，多个宿主先后绑定时不会互相清除对方的绑定。
 */
class TemplateManagerHostBinding final {
public:
    /**
     * @brief 构造令牌
     *
     * @param manager 被绑定的 TemplateManager（进程级单例）
     */
    explicit TemplateManagerHostBinding(TemplateManager& manager);

    /**
     * @brief 析构令牌：解绑本令牌注入的全部来源
     */
    ~TemplateManagerHostBinding();

    // 不拷贝、不移动：令牌地址与"是否已绑定"状态一一对应，移动会让解绑责任难以追踪
    TemplateManagerHostBinding(const TemplateManagerHostBinding&) = delete;
    TemplateManagerHostBinding& operator=(const TemplateManagerHostBinding&) = delete;
    TemplateManagerHostBinding(TemplateManagerHostBinding&&) = delete;
    TemplateManagerHostBinding& operator=(TemplateManagerHostBinding&&) = delete;

    /**
     * @brief 绑定数据包仓库（模板 .nbt 的主来源）
     *
     * 绑定会丢弃上一宿主遗留的模板缓存：模板内容取决于当前数据包集合。
     * 仓库须存活至本令牌析构（或 releaseAll()）为止。
     *
     * @param repository 数据包仓库
     */
    void bindDataPackRepository(const resource::DataPackRepository& repository);

    /**
     * @brief 绑定结构包资源源（基岩版行为包 .mcstructure 来源，优先级最高）
     *
     * @param source 结构包资源源，须存活至 unbindStructurePackSource()（或本令牌析构）为止
     */
    void bindStructurePackSource(const IStructurePackSource& source);

    /**
     * @brief 解绑结构包资源源
     *
     * 结构包资源源的所有者若先于宿主销毁（如 GameTestServer 的成员），必须在其析构前调用本方法。
     * 幂等：未绑定或已被其他宿主接管时为空操作。
     */
    void unbindStructurePackSource();

    /**
     * @brief 解绑本令牌注入的全部来源
     *
     * 幂等，可重复调用。宿主提前关闭（而非析构）时调用，可尽早摘除悬垂风险。
     */
    void releaseAll();

private:
    TemplateManager& m_manager;
    const resource::DataPackRepository* m_dataPackRepository = nullptr;
    const IStructurePackSource* m_structurePackSource = nullptr;
};

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
