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

#include "common/item/instrument/InstrumentRegistry.hpp"

#include "common/sound/SoundEvents.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "common/util/text/TranslationTextComponent.hpp"

#include <array>
#include <memory>
#include <string>
#include <utility>

namespace mc::item::instrument {
namespace {

/// 内置山羊角乐器定义：资源名、音效、翻译键
struct _VanillaGoatHorn {
    const char* id;
    const ResourceLocation* soundEvent;
    const char* translationKey;
};

/// 与 vanilla `Registries.INSTRUMENT` 登记顺序一致（八种山羊角音色）。
constexpr std::array<_VanillaGoatHorn, 8> VANILLA_GOAT_HORNS = {{
    {"minecraft:admire_goat_horn", &mc::SoundEvents::ITEM_GOAT_HORN_SOUND_4, "instrument.minecraft.admire_goat_horn"},
    {"minecraft:call_goat_horn", &mc::SoundEvents::ITEM_GOAT_HORN_SOUND_5, "instrument.minecraft.call_goat_horn"},
    {"minecraft:dream_goat_horn", &mc::SoundEvents::ITEM_GOAT_HORN_SOUND_7, "instrument.minecraft.dream_goat_horn"},
    {"minecraft:feel_goat_horn", &mc::SoundEvents::ITEM_GOAT_HORN_SOUND_3, "instrument.minecraft.feel_goat_horn"},
    {"minecraft:ponder_goat_horn",
        &mc::SoundEvents::ITEM_GOAT_HORN_SOUND_0,
        "instrument.minecraft.ponder_goat_horn"},
    {"minecraft:seek_goat_horn", &mc::SoundEvents::ITEM_GOAT_HORN_SOUND_2, "instrument.minecraft.seek_goat_horn"},
    {"minecraft:sing_goat_horn", &mc::SoundEvents::ITEM_GOAT_HORN_SOUND_1, "instrument.minecraft.sing_goat_horn"},
    {"minecraft:yearn_goat_horn", &mc::SoundEvents::ITEM_GOAT_HORN_SOUND_6, "instrument.minecraft.yearn_goat_horn"},
}};

/// 内置乐器参数（vanilla 八种山羊角一致：吹奏 7 秒、可听距离 256 格）。
constexpr f32 VANILLA_GOAT_HORN_USE_DURATION_SECONDS = 7.0f;
constexpr f32 VANILLA_GOAT_HORN_RANGE = 256.0f;

} // namespace

InstrumentRegistry& InstrumentRegistry::instance() noexcept
{
    static InstrumentRegistry s_instance;
    return s_instance;
}

void InstrumentRegistry::initialize()
{
    for (const auto& [id, soundEvent, translationKey] : VANILLA_GOAT_HORNS) {
        const ResourceLocation location(id);
        // 数据包已加载同名条目时保持数据包定义（与 vanilla "数据包覆盖内置"语义一致）。
        if (get(location) != nullptr) {
            continue;
        }
        registerInstrument(location,
            std::make_unique<InstrumentDefinition>(*soundEvent,
                VANILLA_GOAT_HORN_USE_DURATION_SECONDS,
                VANILLA_GOAT_HORN_RANGE,
                std::make_unique<text::TranslationTextComponent>(std::string(translationKey))));
    }
}

void InstrumentRegistry::clear()
{
    m_instruments.clear();
    m_indexById.clear();
}

void InstrumentRegistry::registerInstrument(const ResourceLocation& id, std::unique_ptr<InstrumentDefinition> instrument)
{
    MC_ASSERT_RELEASE_MSG(instrument != nullptr, "InstrumentRegistry: registering null instrument");
    const auto existing = m_indexById.find(id);
    if (existing != m_indexById.end()) {
        m_instruments[existing->second].second = std::move(instrument);
        return;
    }
    const std::size_t index = m_instruments.size();
    m_instruments.emplace_back(id, std::move(instrument));
    m_indexById.emplace(id, index);
}

const InstrumentDefinition* InstrumentRegistry::get(const ResourceLocation& id) const
{
    const auto it = m_indexById.find(id);
    return it == m_indexById.end() ? nullptr : m_instruments[it->second].second.get();
}

std::vector<std::pair<ResourceLocation, const InstrumentDefinition*>> InstrumentRegistry::getAll() const
{
    std::vector<std::pair<ResourceLocation, const InstrumentDefinition*>> result;
    result.reserve(m_instruments.size());
    for (const auto& [id, instrument] : m_instruments) {
        result.emplace_back(id, instrument.get());
    }
    return result;
}

} // namespace mc::item::instrument