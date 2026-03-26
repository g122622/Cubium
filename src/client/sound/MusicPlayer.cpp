#include "MusicPlayer.hpp"
#include "client/sound/SoundEngine.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include "client/settings/ClientSettings.hpp"
#include <spdlog/spdlog.h>

namespace mc::client::sound {

// ============================================================================
// 音乐选择器常量
// ============================================================================

namespace {

// 主菜单音乐
const MusicPlayer::MusicSelector MENU_MUSIC = {
    ResourceLocation("minecraft:music.menu"),
    0,      // 立即播放
    0,
    true    // 替换当前
};

// 游戏音乐列表
const std::vector<MusicPlayer::MusicSelector> GAME_MUSIC = {
    { ResourceLocation("minecraft:music.game"), 12000, 24000, false },
    { ResourceLocation("minecraft:music.creative"), 12000, 24000, false },
    { ResourceLocation("minecraft:music.credits"), 12000, 24000, false }
};

// 创造模式音乐
const std::vector<MusicPlayer::MusicSelector> CREATIVE_MUSIC = {
    { ResourceLocation("minecraft:music.creative"), 12000, 24000, false },
    { ResourceLocation("minecraft:music.game"), 12000, 24000, false }
};

// 下界音乐
const std::vector<MusicPlayer::MusicSelector> NETHER_MUSIC = {
    { ResourceLocation("minecraft:music.nether.basalt_deltas"), 12000, 24000, false },
    { ResourceLocation("minecraft:music.nether.crimson_forest"), 12000, 24000, false },
    { ResourceLocation("minecraft:music.nether.nether_wastes"), 12000, 24000, false },
    { ResourceLocation("minecraft:music.nether.soul_sand_valley"), 12000, 24000, false },
    { ResourceLocation("minecraft:music.nether.warped_forest"), 12000, 24000, false }
};

// 末地音乐
const std::vector<MusicPlayer::MusicSelector> END_MUSIC = {
    { ResourceLocation("minecraft:music.end"), 12000, 24000, false }
};

// 水下音乐
const std::vector<MusicPlayer::MusicSelector> UNDERWATER_MUSIC = {
    { ResourceLocation("minecraft:music.under_water"), 12000, 24000, false }
};

// 末影龙战斗音乐
const MusicPlayer::MusicSelector DRAGON_MUSIC = {
    ResourceLocation("minecraft:music.dragon"),
    0,      // 立即播放
    0,
    true    // 替换当前
};

// 制作人员名单音乐
const MusicPlayer::MusicSelector CREDITS_MUSIC = {
    ResourceLocation("minecraft:music.credits"),
    0,      // 立即播放
    0,
    true    // 替换当前
};

// 空选择器
const MusicPlayer::MusicSelector EMPTY_SELECTOR = {
    ResourceLocation(""),
    0,
    0,
    false
};

} // anonymous namespace

// ============================================================================
// 构造函数和析构函数
// ============================================================================

MusicPlayer::MusicPlayer(SoundEngine& engine)
    : m_engine(engine)
    , m_rng(std::random_device{}())
{
    // 初始化音乐选择器列表
    m_gameMusicSelectors = GAME_MUSIC;
    m_creativeMusicSelectors = CREATIVE_MUSIC;
    m_netherMusicSelectors = NETHER_MUSIC;
    m_endMusicSelectors = END_MUSIC;
    m_underwaterMusicSelectors = UNDERWATER_MUSIC;
}

MusicPlayer::~MusicPlayer() {
    stop(false);
}

// ============================================================================
// 生命周期
// ============================================================================

void MusicPlayer::tick(bool isPaused, bool inMenu) {
    if (!m_enabled) {
        return;
    }

    // 检查当前音乐是否仍在播放
    if (m_currentSoundId != 0) {
        if (!m_engine.isPlaying(m_currentSoundId)) {
            // 音乐已结束
            m_currentSoundId = 0;
            m_currentType = MusicType::None;
        } else if (m_fadingOut) {
            // 更新淡出
            updateFade();
            return;
        }
    }

    // 如果暂停或正在播放，不开始新音乐
    if (isPaused && m_currentSoundId != 0) {
        return;
    }

    // 根据状态选择音乐类型
    MusicType desiredType = inMenu ? MusicType::Menu : MusicType::Game;

    // 如果当前没有音乐且延迟已过，开始新音乐
    if (m_currentSoundId == 0 && m_delayCounter == 0) {
        // 获取选择器
        const MusicSelector& selector = getSelector(desiredType);
        if (!selector.soundEventId.toString().empty()) {
            startPlaying(selector);
        }
    }

    // 更新延迟计数器
    if (m_delayCounter > 0) {
        --m_delayCounter;
    }
}

void MusicPlayer::stop(bool fadeOut) {
    if (m_currentSoundId == 0) {
        return;
    }

    if (fadeOut) {
        // 开始淡出
        m_fadingOut = true;
        m_fadeCounter = FADE_DURATION;
    } else {
        // 立即停止
        m_engine.stop(m_currentSoundId);
        m_currentSoundId = 0;
        m_currentType = MusicType::None;
        m_fadingOut = false;
    }
}

// ============================================================================
// 音乐控制
// ============================================================================

void MusicPlayer::setMusicType(MusicType type) {
    if (m_currentType == type) {
        return;
    }

    // 停止当前音乐
    stop(true);

    // 设置新类型
    m_currentType = type;

    // 重置延迟
    const MusicSelector& selector = getSelector(type);
    m_delayCounter = selectDelay(selector);
}

bool MusicPlayer::isPlaying() const noexcept {
    return m_currentSoundId != 0 && m_engine.isPlaying(m_currentSoundId);
}

// ============================================================================
// 私有方法
// ============================================================================

const MusicPlayer::MusicSelector& MusicPlayer::getSelector(MusicType type) const {
    switch (type) {
        case MusicType::Menu:
            return MENU_MUSIC;

        case MusicType::Game: {
            if (m_gameMusicSelectors.empty()) {
                return EMPTY_SELECTOR;
            }
            std::uniform_int_distribution<size_t> dist(0, m_gameMusicSelectors.size() - 1);
            return m_gameMusicSelectors[dist(m_rng)];
        }

        case MusicType::Creative: {
            if (m_creativeMusicSelectors.empty()) {
                return EMPTY_SELECTOR;
            }
            std::uniform_int_distribution<size_t> dist(0, m_creativeMusicSelectors.size() - 1);
            return m_creativeMusicSelectors[dist(m_rng)];
        }

        case MusicType::Nether: {
            if (m_netherMusicSelectors.empty()) {
                return EMPTY_SELECTOR;
            }
            std::uniform_int_distribution<size_t> dist(0, m_netherMusicSelectors.size() - 1);
            return m_netherMusicSelectors[dist(m_rng)];
        }

        case MusicType::End: {
            if (m_endMusicSelectors.empty()) {
                return EMPTY_SELECTOR;
            }
            std::uniform_int_distribution<size_t> dist(0, m_endMusicSelectors.size() - 1);
            return m_endMusicSelectors[dist(m_rng)];
        }

        case MusicType::Underwater: {
            if (m_underwaterMusicSelectors.empty()) {
                return EMPTY_SELECTOR;
            }
            std::uniform_int_distribution<size_t> dist(0, m_underwaterMusicSelectors.size() - 1);
            return m_underwaterMusicSelectors[dist(m_rng)];
        }

        case MusicType::Credits:
            return CREDITS_MUSIC;

        case MusicType::Dragon:
            return DRAGON_MUSIC;

        default:
            return EMPTY_SELECTOR;
    }
}

u32 MusicPlayer::selectDelay(const MusicSelector& selector) {
    if (selector.minDelayTicks >= selector.maxDelayTicks) {
        return selector.minDelayTicks;
    }

    std::uniform_int_distribution<u32> dist(selector.minDelayTicks, selector.maxDelayTicks);
    return dist(m_rng);
}

void MusicPlayer::startPlaying(const MusicSelector& selector) {
    // 如果有当前音乐且不允许替换
    if (m_currentSoundId != 0 && !selector.replaceCurrent) {
        if (m_engine.isPlaying(m_currentSoundId)) {
            return;
        }
        m_currentSoundId = 0;
    }

    // 停止当前音乐
    if (m_currentSoundId != 0) {
        m_engine.stop(m_currentSoundId);
        m_currentSoundId = 0;
    }

    // 创建音乐声音实例
    auto sound = SoundInstance::createMusic(selector.soundEventId);

    // 播放
    m_currentSoundId = m_engine.play(std::move(sound));

    if (m_currentSoundId == 0) {
        spdlog::warn("MusicPlayer: Failed to play music: {}", selector.soundEventId.toString());
        return;
    }

    spdlog::debug("MusicPlayer: Started playing: {}", selector.soundEventId.toString());

    // 设置下次延迟
    m_delayCounter = selectDelay(selector);
    m_fadingOut = false;
}

void MusicPlayer::updateFade() {
    if (!m_fadingOut || m_currentSoundId == 0) {
        return;
    }

    --m_fadeCounter;

    if (m_fadeCounter == 0) {
        // 淡出完成，停止音乐
        m_engine.stop(m_currentSoundId);
        m_currentSoundId = 0;
        m_currentType = MusicType::None;
        m_fadingOut = false;
    }
    // TODO: 实现音量淡出
    // 当前简化实现：直接停止
    // 未来可以通过 SoundEngine 设置音量实现平滑淡出
}

} // namespace mc::client::sound
