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

#include "common/core/Result.hpp"
#include "common/entity/effect/EffectInstance.hpp"
#include "common/util/assert/AssertAll.hpp"
#include "probe.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <memory>
#include <unordered_map>
#include <vector>
#include <nlohmann/json.hpp>

namespace mc::diff {
namespace {

using entity::effect::EffectInstance;
using entity::effect::EffectType;
using nlohmann::json;
constexpr std::array EFFECTS{EffectType::Speed, EffectType::Poison, EffectType::NightVision};

// 保留强引用，防止节点释放后地址被复用而产生伪身份。
class _IdentitySnapshot {
public:
    json capture(const std::vector<std::shared_ptr<EffectInstance>>& slots)
    {
        json roots = json::array();
        std::vector<std::shared_ptr<EffectInstance>> visit;
        auto enqueue = [&](const std::shared_ptr<EffectInstance>& instance) {
            if (std::ranges::find(visit, instance) == visit.end()) {
                visit.push_back(instance);
            }
        };
        for (const auto& slot : slots) {
            roots.push_back(_id(slot));
            enqueue(slot);
        }
        json nodes = json::array();
        for (size_t i = 0; i < visit.size(); ++i) {
            const auto instance = visit[i];
            const auto hidden = instance->hiddenEffect();
            json node{{"id", _id(instance)},
                {"effect", std::ranges::find(EFFECTS, instance->type()) - EFFECTS.begin()},
                {"duration", instance->duration()},
                {"amplifier", instance->amplifier()},
                {"ambient", instance->isAmbient()},
                {"visible", instance->isVisible()},
                {"icon", instance->showIcon()},
                {"hidden", nullptr}};
            if (hidden) {
                node["hidden"] = _id(hidden);
                enqueue(hidden);
            }
            nodes.push_back(std::move(node));
        }
        return json{{"slots", roots}, {"nodes", nodes}};
    }

private:
    size_t _id(const std::shared_ptr<EffectInstance>& instance)
    {
        const auto [iter, inserted] = m_ids.try_emplace(instance.get(), m_ids.size());
        if (inserted) {
            m_retained.push_back(instance);
        }
        return iter->second;
    }

    std::unordered_map<const EffectInstance*, size_t> m_ids;
    std::vector<std::shared_ptr<EffectInstance>> m_retained;
};

std::string _runLine(std::string_view line)
{
    const auto input = json::parse(line);
    std::vector<std::shared_ptr<EffectInstance>> slots;
    for (const auto& value : input.at("instances")) {
        auto instance = std::make_shared<EffectInstance>(EFFECTS.at(value.at("effect").get<size_t>()),
            value.at("duration").get<i32>(),
            value.at("amplifier").get<i32>(),
            value.at("ambient").get<bool>(),
            value.at("visible").get<bool>(),
            value.at("icon").get<bool>());
        const auto hidden = value.at("hidden").get<i32>();
        if (hidden >= 0) {
            instance->setHiddenEffect(slots.at(static_cast<size_t>(hidden)));
        }
        slots.push_back(std::move(instance));
    }
    _IdentitySnapshot snapshot;
    json result{{"unit", input.at("unit")}, {"initial", snapshot.capture(slots)}, {"trace", json::array()}};
    for (const auto& op : input.at("ops")) {
        const auto a = op.at("a").get<size_t>();
        const auto b = op.at("b").get<size_t>();
        const auto name = op.at("op").get<std::string>();
        json step{{"result", "ok"}};
        if (name == "alias") {
            slots[b] = slots[a];
        } else if (name == "copy") {
            slots[b] = std::make_shared<EffectInstance>(slots[a]->copyDetails());
        } else if (name == "scale") {
            slots[b] = std::make_shared<EffectInstance>(
                slots[a]->withScaledDuration(std::bit_cast<f32>(op.at("factor").get<u32>())));
        } else if (name == "update") {
            step["result"] = slots[a]->merge(*slots[b]);
        } else if (name == "eq") {
            step["result"] = *slots[a] == *slots[b];
        } else if (name == "same") {
            step["result"] = slots[a] == slots[b];
        } else if (name == "ends") {
            step["result"] = slots[a]->endsWithin(op.at("value").get<i32>());
        } else if (name == "infinite") {
            step["result"] = slots[a]->isPermanent();
        } else if (name == "tick") {
            const auto tick = slots[a]->advanceDuration();
            step["result"] = json{{"active", tick.active}, {"downgraded", tick.downgraded}};
        } else if (name == "map") {
            step["calls"] = json::array();
            const auto mapped = slots[a]->mapDuration([&](i32 duration) -> Result<i32> {
                step["calls"].push_back(duration);
                if (op.at("mutate").get<bool>()) {
                    slots[b]->merge(*slots.at(op.at("c").get<size_t>()));
                }
                if (op.at("fail").get<bool>()) {
                    return Error(ErrorCode::OperationFailed, "Duration callback failed");
                }
                return std::bit_cast<i32>(static_cast<u32>(duration) * op.at("value").get<u32>());
            });
            if (mapped.success()) {
                step["result"] = mapped.value();
            } else {
                step["result"] = "callback";
            }
        } else {
            MC_ASSERT_RELEASE_MSG(false, "Unknown status effect operation");
        }
        step["state"] = snapshot.capture(slots);
        result["trace"].push_back(std::move(step));
    }
    return result.dump();
}

} // namespace
MC_DIFF_DEFINE_PROBE(status_effect, nullptr, &_runLine)
} // namespace mc::diff
