#include "MusicPlayer.hpp"
#include "client/settings/ClientSettings.hpp"
#include "client/sound/SoundEngine.hpp"
#include "client/sound/instance/SoundInstance.hpp"
#include <spdlog/spdlog.h>

namespace mc::client::sound {

// ============================================================================
// 音乐选择器常量
// ============================================================================

namespace {

// 主菜单音乐
// MC 1.16.5: minDelay=20, maxDelay=600, replaceCurrent=true
const MusicPlayer::MusicSelector MENU_MUSIC = {
    ResourceLocation("minecraft:music.menu"),
    20,  // minDelay: 20 ticks
    600, // maxDelay: 600 ticks
    true // 替换当前
};

// 游戏音乐列表（仅主世界生存模式）
// MC 1.16.5: 仅包含 music.game，创造模式和制作人员名单有独立选择器
const std::vector<MusicPlayer::MusicSelector> GAME_MUSIC = {
    {ResourceLocation("minecraft:music.game"), 12000, 24000, false}};

// 创造模式音乐
// MC 1.16.5: 仅包含 music.creative
const std::vector<MusicPlayer::MusicSelector> CREATIVE_MUSIC = {
    {ResourceLocation("minecraft:music.creative"), 12000, 24000, false}};

// 下界音乐（按生物群系选择）
// MC 1.16.5: 每个下界群系有专属音乐
const std::vector<MusicPlayer::MusicSelector> NETHER_MUSIC = {
    {ResourceLocation("minecraft:music.nether.basalt_deltas"), 12000, 24000, false},
    {ResourceLocation("minecraft:music.nether.crimson_forest"), 12000, 24000, false},
    {ResourceLocation("minecraft:music.nether.nether_wastes"), 12000, 24000, false},
    {ResourceLocation("minecraft:music.nether.soul_sand_valley"), 12000, 24000, false}
    // 注意：warped_forest 没有音乐（sounds.json 中为空数组）
};

// 末地音乐
// MC 1.16.5: minDelay=6000, maxDelay=24000, replaceCurrent=true
const std::vector<MusicPlayer::MusicSelector> END_MUSIC = {
    {ResourceLocation("minecraft:music.end"), 6000, 24000, true}};

// 水下音乐
const std::vector<MusicPlayer::MusicSelector> UNDERWATER_MUSIC = {
    {ResourceLocation("minecraft:music.under_water"), 12000, 24000, false}};

// 末影龙战斗音乐
const MusicPlayer::MusicSelector DRAGON_MUSIC = {
    ResourceLocation("minecraft:music.dragon"),
    0, // 立即播放
    0,
    true // 替换当前
};

// 制作人员名单音乐
const MusicPlayer::MusicSelector CREDITS_MUSIC = {
    ResourceLocation("minecraft:music.credits"),
    0, // 立即播放
    0,
    true // 替换当前
};

// 空选择器
const MusicPlayer::MusicSelector EMPTY_SELECTOR = {ResourceLocation(""), 0, 0, false};

} // anonymous namespace

// ============================================================================
// 构造函数和析构函数
// ============================================================================

MusicPlayer::MusicPlayer(SoundEngine& engine)
    : m_engine(engine)
    , m_rng(static_cast<u64>(std::chrono::steady_clock::now().time_since_epoch().count()))
{
    // 初始化音乐选择器列表
    m_gameMusicSelectors = GAME_MUSIC;
    m_creativeMusicSelectors = CREATIVE_MUSIC;
    m_netherMusicSelectors = NETHER_MUSIC;
    m_endMusicSelectors = END_MUSIC;
    m_underwaterMusicSelectors = UNDERWATER_MUSIC;
}

MusicPlayer::~MusicPlayer()
{
    stop(false);
}

// ============================================================================
// 生命周期
// ============================================================================

void MusicPlayer::tick(bool isPaused,
    bool inMenu,
    i32 dimension,
    bool inWater,
    bool inCreative,
    bool inBossFight,
    const std::optional<world::biome::BiomeMusic>& biomeMusic)
{
    if (!m_enabled) {
        return;
    }

    // 根据 MC 1.16.5 MusicTicker.func_238178_U_() 选择音乐类型
    MusicType desiredType = MusicType::None;
    std::optional<MusicSelector> biomeSelector;

    if (inMenu) {
        // 主菜单界面
        desiredType = MusicType::Menu;
    } else {
        // 游戏中
        if (dimension == 1) {
            // 末地维度
            desiredType = inBossFight ? MusicType::Dragon : MusicType::End;
        } else if (dimension == -1) {
            // 下界维度 - 使用生物群系音乐
            if (biomeMusic.has_value() && biomeMusic->isValid()) {
                // 生物群系有专属音乐（如玄武岩三角洲、绯红森林等）
                biomeSelector = MusicSelector::fromBiomeMusic(*biomeMusic);
                desiredType = MusicType::Biome;
            } else {
                // 诡异森林没有音乐（sounds.json 中为空数组）
                // MC 1.16.5: 返回空选择器，不播放音乐
                desiredType = MusicType::None;
            }
        } else if (inWater) {
            // 水下 - 检查是否在海洋或河流群系中
            // 注意：水下音乐需要在海洋或河流群系中才能播放
            // 当前简化实现：水下就播放水下音乐
            desiredType = MusicType::Underwater;
        } else if (inCreative) {
            // 创造模式
            desiredType = MusicType::Creative;
        } else {
            // 主世界普通游戏 - 检查生物群系是否有专属音乐
            if (biomeMusic.has_value() && biomeMusic->isValid()) {
                // 某些主世界生物群系可能有专属音乐（未来扩展）
                biomeSelector = MusicSelector::fromBiomeMusic(*biomeMusic);
                desiredType = MusicType::Biome;
            } else {
                // 默认游戏音乐
                desiredType = MusicType::Game;
            }
        }
    }

    // 获取目标选择器
    const MusicSelector& desiredSelector = biomeSelector.has_value() ? biomeSelector.value() : getSelector(desiredType);

    // MC 1.16.5: 检查是否需要替换当前音乐
    // 如果当前音乐正在播放，但新选择器要求替换且与当前不同
    if (m_currentSoundId != 0) {
        bool isPlaying = m_engine.isPlaying(m_currentSoundId);

        if (isPlaying) {
            // 检查是否需要因 replaceCurrent 而停止
            // MC 1.16.5 line 25-28:
            // if (!newSelector.sound.getName().equals(current.getSoundLocation()) && newSelector.replaceCurrent)
            if (m_currentType != desiredType && desiredSelector.replaceCurrent) {
                // 停止当前音乐，设置短延迟
                m_engine.stop(m_currentSoundId);
                m_currentSoundId = 0;
                m_currentType = MusicType::None;
                m_fadingOut = false;
                // 设置随机延迟 (0 to minDelay/2)
                m_delayCounter =
                    static_cast<u32>(m_rng.nextInt(0, static_cast<i32>(desiredSelector.minDelayTicks / 2)));
            }
        } else {
            // 音乐已播放完毕
            // MC 1.16.5 line 30-33:
            // currentMusic = null; timeUntilNextMusic = min(nextDelay, random(minDelay, maxDelay))
            m_currentSoundId = 0;
            m_currentType = MusicType::None;
            m_fadingOut = false;
            // 设置下次播放延迟
            m_delayCounter = std::min(m_delayCounter, selectDelay(desiredSelector));
        }
    }

    // 如果正在淡出，更新淡出并跳过播放新音乐
    if (m_fadingOut) {
        updateFade();
        return;
    }

    // 如果暂停，不开始新音乐
    if (isPaused) {
        return;
    }

    // MC 1.16.5 line 36: timeUntilNextMusic = min(timeUntilNextMusic, maxDelay)
    m_delayCounter = std::min(m_delayCounter, desiredSelector.maxDelayTicks);

    // MC 1.16.5 line 37-39: 如果没有当前音乐且延迟到了，播放新音乐
    if (m_currentSoundId == 0 && m_delayCounter > 0) {
        --m_delayCounter;
    }

    if (m_currentSoundId == 0 && m_delayCounter == 0) {
        // 获取选择器（可能随机选择）
        const MusicSelector& selector = getSelector(desiredType);
        if (!selector.soundEventId.toString().empty()) {
            startPlaying(selector);
        }
    }
}

void MusicPlayer::stop(bool fadeOut)
{
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

void MusicPlayer::setMusicType(MusicType type)
{
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

bool MusicPlayer::isPlaying() const noexcept
{
    return m_currentSoundId != 0 && m_engine.isPlaying(m_currentSoundId);
}

// ============================================================================
// 私有方法
// ============================================================================

const MusicPlayer::MusicSelector& MusicPlayer::getSelector(MusicType type) const
{
    switch (type) {
        case MusicType::Menu:
            return MENU_MUSIC;

        case MusicType::Game: {
            if (m_gameMusicSelectors.empty()) {
                return EMPTY_SELECTOR;
            }
            // 注意：nextInt(n) 返回 [0, n)，所以直接使用 size() 而不是 size()-1
            return m_gameMusicSelectors[m_rng.nextInt(static_cast<i32>(m_gameMusicSelectors.size()))];
        }

        case MusicType::Creative: {
            if (m_creativeMusicSelectors.empty()) {
                return EMPTY_SELECTOR;
            }
            return m_creativeMusicSelectors[m_rng.nextInt(static_cast<i32>(m_creativeMusicSelectors.size()))];
        }

        case MusicType::Nether: {
            if (m_netherMusicSelectors.empty()) {
                return EMPTY_SELECTOR;
            }
            return m_netherMusicSelectors[m_rng.nextInt(static_cast<i32>(m_netherMusicSelectors.size()))];
        }

        case MusicType::End: {
            if (m_endMusicSelectors.empty()) {
                return EMPTY_SELECTOR;
            }
            return m_endMusicSelectors[m_rng.nextInt(static_cast<i32>(m_endMusicSelectors.size()))];
        }

        case MusicType::Underwater: {
            if (m_underwaterMusicSelectors.empty()) {
                return EMPTY_SELECTOR;
            }
            return m_underwaterMusicSelectors[m_rng.nextInt(static_cast<i32>(m_underwaterMusicSelectors.size()))];
        }

        case MusicType::Credits:
            return CREDITS_MUSIC;

        case MusicType::Dragon:
            return DRAGON_MUSIC;

        default:
            return EMPTY_SELECTOR;
    }
}

u32 MusicPlayer::selectDelay(const MusicSelector& selector)
{
    if (selector.minDelayTicks >= selector.maxDelayTicks) {
        return selector.minDelayTicks;
    }

    return static_cast<u32>(
        m_rng.nextInt(static_cast<i32>(selector.minDelayTicks), static_cast<i32>(selector.maxDelayTicks)));
}

void MusicPlayer::startPlaying(const MusicSelector& selector)
{
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
    auto sound = std::make_unique<SoundInstance>(SoundInstance::createMusic(selector.soundEventId));

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

void MusicPlayer::updateFade()
{
    if (!m_fadingOut || m_currentSoundId == 0) {
        return;
    }

    --m_fadeCounter;

    // 计算淡出音量
    f32 fadeVolume = static_cast<f32>(m_fadeCounter) / static_cast<f32>(FADE_DURATION);

    // 更新声音音量
    ISoundInstance* sound = m_engine.getSoundInstance(m_currentSoundId);
    if (sound) {
        sound->setVolume(fadeVolume);
    }

    if (m_fadeCounter == 0) {
        // 淡出完成，停止音乐
        m_engine.stop(m_currentSoundId);
        m_currentSoundId = 0;
        m_currentType = MusicType::None;
        m_fadingOut = false;
    }
}

} // namespace mc::client::sound
