#include "TemplateManager.hpp"

namespace mc {
namespace world {
namespace gen {
namespace feature {
namespace template_ {

TemplateManager::TemplateManager()
    : m_emptyTemplate(std::make_unique<Template>())
{
}

TemplateManager::~TemplateManager() = default;

void TemplateManager::setResourcePack(const IResourcePack* pack) {
    m_resourcePack = pack;
}

const Template* TemplateManager::getTemplate(const ResourceLocation& location) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_templates.find(location);
    if (it != m_templates.end()) {
        return it->second.get();
    }

    auto templ = loadTemplate(location);
    if (!templ) {
        return nullptr;
    }

    const Template* result = templ.get();
    m_templates[location] = std::move(templ);
    return result;
}

const Template& TemplateManager::getTemplateDefaulted(const ResourceLocation& location) {
    std::lock_guard<std::mutex> lock(m_mutex);

    auto it = m_templates.find(location);
    if (it != m_templates.end()) {
        return *it->second.get();
    }

    auto templ = loadTemplate(location);
    if (templ) {
        const Template& result = *templ;
        m_templates[location] = std::move(templ);
        return result;
    }

    return *m_emptyTemplate;
}

bool TemplateManager::hasTemplate(const ResourceLocation& location) const {
    std::lock_guard<std::mutex> lock(const_cast<std::mutex&>(m_mutex));
    return m_templates.find(location) != m_templates.end();
}

void TemplateManager::addTemplate(const ResourceLocation& location, std::unique_ptr<Template> templ) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_templates[location] = std::move(templ);
}

void TemplateManager::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_templates.clear();
}

std::unique_ptr<Template> TemplateManager::loadTemplate(const ResourceLocation& location) {
    // 尝试从资源包加载
    if (m_resourcePack) {
        auto templ = TemplateLoader::loadFromResourcePack(*m_resourcePack, location);
        if (templ) {
            return templ;
        }
    }

    // TODO: 尝试从文件系统加载（世界保存的结构模板）
    // 例如: <world>/generated/<namespace>/structures/<path>.nbt

    return nullptr;
}

std::unique_ptr<Template> TemplateManager::createProceduralTemplate(
    const String& name,
    i32 width,
    i32 height,
    i32 depth)
{
    auto templ = std::make_unique<Template>();
    templ->setSize(BlockPos(width, height, depth));
    return templ;
}

} // namespace template_
} // namespace feature
} // namespace gen
} // namespace world
} // namespace mc
