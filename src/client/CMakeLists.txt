# src/client/CMakeLists.txt
# 客户端可执行文件

find_package(glfw3 CONFIG REQUIRED)
find_package(Vulkan REQUIRED)
find_package(OpenAL CONFIG REQUIRED)

add_executable(minecraft-client
    main.cpp
    application/ClientApplication.cpp
    application/features/ClientApplicationHelpers.cpp
    application/features/ClientApplicationBootstrap.cpp
    application/features/ClientApplicationSession.cpp
    application/features/ClientApplicationAudio.cpp
    application/features/ClientApplicationTimeWeather.cpp
    application/features/ClientApplicationTargetInfo.cpp
    application/features/ClientApplicationTargetInfoUi.cpp
    application/features/ClientApplicationUiFrame.cpp
    application/features/ClientApplicationInput.cpp
    application/features/ClientApplicationNetwork.cpp
    application/features/ClientApplicationResource.cpp
    application/features/ClientApplicationUi.cpp
    application/features/ClientApplicationSettings.cpp
    application/features/MemoryTraceThread.cpp
    window/Window.cpp
    input/InputManager.cpp
    settings/ClientSettings.cpp
    # Trident 渲染引擎 API 层
    # (Types.hpp 只含 enum class 声明，无 .cpp 实现)
    # Trident 渲染引擎核心 (Vulkan 实现)
    renderer/trident/core/TridentContext.cpp
    renderer/trident/core/TridentSwapchain.cpp
    renderer/trident/core/TridentEngine.cpp
    renderer/trident/core/buffer/TridentBuffer.cpp
    renderer/trident/core/buffer/TridentStagingBufferPool.cpp
    renderer/trident/core/texture/TridentTexture.cpp
    renderer/trident/core/texture/AnimatedSprite.cpp
    renderer/trident/core/texture/TextureAtlasTicker.cpp
    renderer/trident/core/pipeline/TridentPipeline.cpp
    renderer/trident/core/render/RenderPassManager.cpp
    renderer/trident/core/render/FrameManager.cpp
    renderer/trident/core/render/DescriptorManager.cpp
    renderer/trident/core/render/UniformManager.cpp
    # 雾效果模块
    renderer/trident/fog/FogManager.cpp
    # 云渲染模块
    renderer/trident/cloud/CloudRenderer.cpp
    # 天气渲染模块
    renderer/trident/weather/WeatherRenderer.cpp
    # 光照贴图模块（16×16 lightmap，供天气渲染采样）
    renderer/trident/light/LightTextureManager.cpp
    # 粒子系统
    renderer/trident/particle/Particle.cpp
    renderer/trident/particle/ParticleManager.cpp
    renderer/trident/particle/ParticleTextureAtlas.cpp
    renderer/trident/particle/ParticleRegistry.cpp
    renderer/trident/particle/ParticleFactories.cpp
    renderer/trident/particle/data/BasicParticleData.cpp
    renderer/trident/particle/data/BlockParticleData.cpp
    renderer/trident/particle/data/DustParticleData.cpp
    renderer/trident/particle/data/EntityEffectParticleData.cpp
    renderer/trident/particle/data/ItemParticleData.cpp
    renderer/trident/particle/data/RedstoneParticleData.cpp
    renderer/trident/particle/data/VibrationParticleData.cpp
    renderer/trident/particle/data/TrailParticleData.cpp
    # 粒子实现 - 天气
    renderer/trident/particle/particles/RainParticle.cpp
    renderer/trident/particle/particles/SnowParticle.cpp
    renderer/trident/particle/particles/weather/SplashParticle.cpp
    renderer/trident/particle/particles/weather/FishingParticle.cpp
    # 粒子实现 - 环境
    renderer/trident/particle/particles/ambient/BubbleParticle.cpp
    renderer/trident/particle/particles/ambient/BubblePopParticle.cpp
    renderer/trident/particle/particles/ambient/UnderwaterParticle.cpp
    renderer/trident/particle/particles/ambient/CloudParticle.cpp
    renderer/trident/particle/particles/ambient/NetherSporeParticle.cpp
    renderer/trident/particle/particles/ambient/SporeBlossomParticle.cpp
    renderer/trident/particle/particles/ambient/SuspendedTownParticle.cpp
    # 粒子实现 - 效果
    renderer/trident/particle/particles/effect/AshParticle.cpp
    renderer/trident/particle/particles/effect/CampfireParticle.cpp
    renderer/trident/particle/particles/effect/CopperFireFlameParticle.cpp
    renderer/trident/particle/particles/effect/CritParticle.cpp
    renderer/trident/particle/particles/effect/DamageIndicatorParticle.cpp
    renderer/trident/particle/particles/effect/DragonBreathParticle.cpp
    renderer/trident/particle/particles/effect/DustParticle.cpp
    renderer/trident/particle/particles/effect/DustPlumeParticle.cpp
    renderer/trident/particle/particles/effect/EggCrackParticle.cpp
    renderer/trident/particle/particles/effect/ElderGuardianParticle.cpp
    renderer/trident/particle/particles/effect/ElectricSparkParticle.cpp
    renderer/trident/particle/particles/effect/EmitterParticle.cpp
    renderer/trident/particle/particles/effect/ExplosionParticle.cpp
    renderer/trident/particle/particles/effect/FireflyParticle.cpp
    renderer/trident/particle/particles/effect/FireworkParticle.cpp
    renderer/trident/particle/particles/effect/FlameParticle.cpp
    renderer/trident/particle/particles/effect/FlashParticle.cpp
    renderer/trident/particle/particles/effect/GlowParticle.cpp
    renderer/trident/particle/particles/effect/GustEmitterParticle.cpp
    renderer/trident/particle/particles/effect/GustParticle.cpp
    renderer/trident/particle/particles/effect/InfestedParticle.cpp
    renderer/trident/particle/particles/effect/ItemPickupParticle.cpp
    renderer/trident/particle/particles/effect/LavaParticle.cpp
    renderer/trident/particle/particles/effect/LightParticle.cpp
    renderer/trident/particle/particles/effect/NoteParticle.cpp
    renderer/trident/particle/particles/effect/OmenParticle.cpp
    renderer/trident/particle/particles/effect/PortalParticle.cpp
    renderer/trident/particle/particles/effect/PoofParticle.cpp
    renderer/trident/particle/particles/effect/RedstoneParticle.cpp
    renderer/trident/particle/particles/effect/SculkChargeParticle.cpp
    renderer/trident/particle/particles/effect/SculkSoulParticle.cpp
    renderer/trident/particle/particles/effect/ShriekParticle.cpp
    renderer/trident/particle/particles/effect/SmallFlameParticle.cpp
    renderer/trident/particle/particles/effect/SmokeParticle.cpp
    renderer/trident/particle/particles/effect/SonicBoomParticle.cpp
    renderer/trident/particle/particles/effect/SoulParticle.cpp
    renderer/trident/particle/particles/effect/SpellParticle.cpp
    renderer/trident/particle/particles/effect/TrialSpawnerParticle.cpp
    renderer/trident/particle/particles/effect/WhiteSmokeParticle.cpp
    # 粒子实现 - 生物
    renderer/trident/particle/particles/mob/HeartParticle.cpp
    renderer/trident/particle/particles/mob/SpitParticle.cpp
    renderer/trident/particle/particles/mob/SquidInkParticle.cpp
    renderer/trident/particle/particles/mob/TotemParticle.cpp
    renderer/trident/particle/particles/mob/VillagerParticle.cpp
    # 粒子实现 - 特殊
    renderer/trident/particle/particles/special/NautilusParticle.cpp
    renderer/trident/particle/particles/special/TrailParticle.cpp
    renderer/trident/particle/particles/special/VaultConnectionParticle.cpp
    renderer/trident/particle/particles/special/VibrationSignalParticle.cpp
    # 粒子实现 - 方块
    renderer/trident/particle/particles/block/ComposterParticle.cpp
    renderer/trident/particle/particles/block/DiggingParticle.cpp
    renderer/trident/particle/particles/block/DustPillarParticle.cpp
    renderer/trident/particle/particles/block/ItemParticle.cpp
    renderer/trident/particle/particles/block/LeavesParticle.cpp
    renderer/trident/particle/particles/block/ScrapeParticle.cpp
    renderer/trident/particle/particles/block/WaxParticle.cpp
    # 粒子实现 - 液体
    renderer/trident/particle/particles/liquid/CherryLeavesDripParticle.cpp
    renderer/trident/particle/particles/liquid/DripParticle.cpp
    renderer/trident/particle/particles/liquid/DripWaterParticle.cpp
    renderer/trident/particle/particles/liquid/DripstoneDripParticle.cpp
    renderer/trident/particle/particles/liquid/FallingNectarParticle.cpp
    # 方块破坏效果
    renderer/trident/block/BreakProgressManager.cpp
    renderer/trident/block/BreakProgressRenderer.cpp
    # 方块实体模型
    renderer/trident/blockentity/model/BlockEntityModel.cpp
    renderer/trident/blockentity/model/ChestModel.cpp
    renderer/trident/blockentity/model/BeaconBeamModel.cpp
    renderer/trident/blockentity/model/BannerModel.cpp
    # 方块实体渲染器
    renderer/trident/blockentity/BlockEntityRenderer.cpp
    renderer/trident/blockentity/BlockEntityRendererDispatcher.cpp
    renderer/trident/blockentity/renderers/PistonRenderer.cpp
    renderer/trident/blockentity/renderers/ChestRenderer.cpp
    renderer/trident/blockentity/renderers/BeaconRenderer.cpp
    renderer/trident/blockentity/renderers/BannerRenderer.cpp
    # 子渲染器
    renderer/trident/chunk/ChunkRenderer.cpp
    renderer/trident/chunk/ChunkMesher.cpp
    renderer/trident/chunk/AmbientOcclusionCalculator.cpp
    renderer/trident/sky/SkyRenderer.cpp
    renderer/trident/sky/CelestialCalculations.cpp
    renderer/trident/gui/GuiRenderer.cpp
    renderer/trident/gui/GuiAtlasRegistry.cpp
    renderer/trident/gui/GuiTextureAtlas.cpp
    renderer/trident/gui/GuiTextureManager.cpp
    renderer/trident/gui/GuiSpriteAtlas.cpp
    renderer/trident/gui/GuiSpriteManager.cpp
    renderer/trident/gui/GuiSpriteParser.cpp
    renderer/trident/gui/GuiSpriteRegistry.cpp
    renderer/trident/gui/GuiTextureLoader.cpp
    renderer/trident/item/ItemRenderer.cpp
    renderer/trident/item/ItemMeshBuilder.cpp
    # 实体渲染系统
    renderer/trident/entity/core/EntityRenderer.cpp
    renderer/trident/entity/core/EntityRendererManager.cpp
    renderer/trident/entity/core/AnimationContext.cpp
    renderer/trident/entity/core/AnimatedMeshCache.cpp
    renderer/trident/entity/core/RendererFactory.cpp
    renderer/trident/entity/pipeline/EntityPipeline.cpp
    renderer/trident/entity/pipeline/EntityTextureAtlas.cpp
    renderer/trident/entity/util/ShadowRenderer.cpp
    renderer/trident/entity/util/NameTagRenderer.cpp
    renderer/trident/entity/util/WorldTextRenderer.cpp
    renderer/trident/entity/util/BlockMeshBuilder.cpp
    # 实体模型
    renderer/trident/entity/model/core/EntityModel.cpp
    renderer/trident/entity/model/core/ModelRenderer.cpp
    renderer/trident/entity/model/core/ModelFactory.cpp
    renderer/trident/entity/model/core/AgeableModel.cpp
    renderer/trident/entity/model/core/SegmentedModel.cpp
    renderer/trident/entity/model/ModelRegistration.cpp
    renderer/trident/entity/model/base/BipedModel.cpp
    renderer/trident/entity/model/base/ElytraSpeedValue.cpp
    renderer/trident/entity/model/base/QuadrupedModel.cpp
    renderer/trident/entity/model/animal/ChickenModel.cpp
    renderer/trident/entity/model/animal/CowModel.cpp
    renderer/trident/entity/model/animal/PigModel.cpp
    renderer/trident/entity/model/animal/SheepModel.cpp
    renderer/trident/entity/model/animal/WolfModel.cpp
    renderer/trident/entity/model/animal/OcelotModel.cpp
    renderer/trident/entity/model/animal/CatModel.cpp
    renderer/trident/entity/model/animal/HorseModel.cpp
    renderer/trident/entity/model/animal/LlamaModel.cpp
    renderer/trident/entity/model/animal/VillagerModel.cpp
    renderer/trident/entity/model/animal/RabbitModel.cpp
    renderer/trident/entity/model/animal/BatModel.cpp
    renderer/trident/entity/model/animal/SquidModel.cpp
    renderer/trident/entity/model/animal/PolarBearModel.cpp
    renderer/trident/entity/model/monster/ZombieModel.cpp
    renderer/trident/entity/model/monster/SkeletonModel.cpp
    renderer/trident/entity/model/monster/CreeperModel.cpp
    renderer/trident/entity/model/monster/SpiderModel.cpp
    renderer/trident/entity/model/monster/EndermanModel.cpp
    renderer/trident/entity/model/monster/BlazeModel.cpp
    renderer/trident/entity/model/monster/SpecialMonsterModels.cpp
    renderer/trident/entity/model/monster/MoreMonsterModels.cpp
    renderer/trident/entity/model/monster/MonsterVariantModels.cpp
    renderer/trident/entity/model/monster/WitchModel.cpp
    renderer/trident/entity/model/nether/NetherModels.cpp
    renderer/trident/entity/model/aquatic/AquaticModels.cpp
    renderer/trident/entity/model/aquatic/PufferfishModel.cpp
    renderer/trident/entity/model/projectile/ProjectileModels.cpp
    renderer/trident/entity/model/player/PlayerModel.cpp
    # 实体渲染器注册
    renderer/trident/entity/renderer/RendererRegistration.cpp
    renderer/trident/entity/renderer/projectile/ItemEntityRenderer.cpp
    renderer/trident/entity/renderer/projectile/ExperienceOrbRenderer.cpp
    renderer/trident/entity/renderer/projectile/ProjectileRenderers.cpp
    renderer/trident/entity/renderer/projectile/BillboardRenderers.cpp
    renderer/trident/entity/renderer/projectile/FireballRenderers.cpp
    renderer/trident/entity/renderer/projectile/FishingBobberRenderer.cpp
    renderer/trident/entity/renderer/monster/MonsterRenderers.cpp
    renderer/trident/entity/renderer/monster/SpecialMonsterRenderers.cpp
    # renderer/trident/entity/renderer/monster/MoreMonsterRenderers.cpp
    renderer/trident/entity/renderer/monster/MonsterVariantRenderers.cpp
    renderer/trident/entity/renderer/nether/NetherRenderers.cpp
    renderer/trident/entity/renderer/aquatic/AquaticRenderers.cpp
    renderer/trident/entity/renderer/special/SpecialEntityRenderers.cpp
    renderer/trident/entity/renderer/animal/WolfRenderer.cpp
    renderer/trident/entity/renderer/animal/OcelotRenderer.cpp
    renderer/trident/entity/renderer/animal/CatRenderer.cpp
    renderer/trident/entity/renderer/animal/HorseRenderer.cpp
    renderer/trident/entity/renderer/animal/LlamaRenderer.cpp
    renderer/trident/entity/renderer/animal/VillagerRenderer.cpp
    renderer/trident/entity/renderer/player/PlayerRenderer.cpp
    renderer/trident/entity/renderer/player/PlayerArmPoseResolver.cpp
    renderer/trident/entity/renderer/vehicle/VehicleRenderers.cpp
    # 层渲染器
    renderer/trident/entity/layer/equipment/ArmorLayer.cpp
    renderer/trident/entity/layer/equipment/HeldItemLayer.cpp
    renderer/trident/entity/layer/equipment/HeadLayer.cpp
    renderer/trident/entity/layer/cosmetic/CapeLayer.cpp
    renderer/trident/entity/layer/cosmetic/ElytraLayer.cpp
    renderer/trident/entity/layer/entity/SaddleLayer.cpp
    renderer/trident/entity/layer/entity/SheepWoolLayer.cpp
    renderer/trident/entity/layer/entity/WolfCollarLayer.cpp
    renderer/trident/entity/layer/entity/ArrowLayer.cpp
    renderer/trident/entity/layer/entity/HeldBlockLayer.cpp
    renderer/trident/entity/layer/effect/EnergyGlintLayer.cpp
    renderer/trident/entity/layer/effect/EyesLayer.cpp
    # 实体特效
    renderer/trident/entity/effect/glow/GlowEffect.cpp
    renderer/trident/entity/effect/fire/FireTextureLoader.cpp
    renderer/trident/entity/effect/fire/FireAnimationState.cpp
    renderer/trident/entity/effect/fire/FireEffect.cpp
    renderer/trident/entity/effect/hurt/HurtFlashEffect.cpp
    # 第一人称手部渲染
    renderer/trident/firstperson/MatrixStack.cpp
    renderer/trident/firstperson/ItemCameraTransforms.cpp
    renderer/trident/firstperson/PlayerModel.cpp
    renderer/trident/firstperson/FirstPersonRenderer.cpp
    renderer/trident/firstperson/ItemInHandRenderer.cpp
    renderer/trident/firstperson/FirstPersonTransforms.cpp
    # 共享渲染组件
    renderer/mesh/MeshDataPool.cpp
    renderer/mesh/MeshResultQueue.cpp
    renderer/mesh/MeshBuildTask.cpp
    renderer/mesh/MeshBuildScheduler.cpp
    renderer/MeshTypes.cpp
    renderer/Camera.cpp
    # 资源加载
    resource/BlockModelLoader.cpp
    resource/BlockStateLoader.cpp
    resource/TextureAtlasBuilder.cpp
    resource/ResourceManager.cpp
    resource/BlockModelCache.cpp
    resource/ItemTextureAtlas.cpp
    resource/ItemModelLoader.cpp
    resource/ItemModelCache.cpp
    resource/EntityTextureLoader.cpp
    # 统一图集数据驱动（替换上述硬编码图集系统的纹理收集层）
    resource/atlas/AtlasSource.cpp
    resource/atlas/AtlasSourceParser.cpp
    resource/atlas/AtlasConfigLoader.cpp
    resource/atlas/AtlasHandle.cpp
    resource/atlas/AtlasManager.cpp
    resource/atlas/IdentifierPattern.cpp
    resource/atlas/MissingNo.cpp
    resource/atlas/Sources.cpp
    resource/atlas/SpriteLoader.cpp
    resource/atlas/TexturePathVariant.cpp
    # 声音系统
    sound/resource/SoundDefinition.cpp
    sound/resource/SoundRegistry.cpp
    sound/SoundHandler.cpp
    sound/SoundLoader.cpp
    sound/StbVorbisImpl.cpp
    sound/SoundPool.cpp
    sound/SoundEngine.cpp
    sound/MusicPlayer.cpp
    sound/AudioService.cpp
    sound/AudioBufferCache.cpp
    sound/backend/AudioBuffer.cpp
    sound/backend/OpenALBackend.cpp
    sound/instance/SoundInstance.cpp
    sound/instance/ElytraSound.cpp
    sound/instance/UnderwaterLoopSound.cpp
    sound/instance/MinecartSound.cpp
    sound/instance/MovingTickableSound.cpp
    sound/handler/BiomeAmbientHandler.cpp
    sound/handler/UnderwaterAmbientHandler.cpp
    sound/handler/BubbleColumnAmbientHandler.cpp
    sound/handler/EntitySoundHandler.cpp
    sound/handler/WeatherSoundHandler.cpp
    # 世界
    world/ClientWorld.cpp
    world/ClientMapDataCache.cpp
    # 地图渲染
    renderer/map/MapRenderer.cpp
    world/player/LocalPlayerIdentity.cpp
    world/player/PlayerIdentityRegistry.cpp
    world/color/BiomeColors.cpp
    world/color/blend/BiomeColorCache.cpp
    world/color/blend/BiomeColorBlender.cpp
    world/color/blend/ChunkBiomeAccessor.cpp
    # 维度
    dimension/ClientDimensionManager.cpp
    # 命令补全
    command/ClientCommandManager.cpp
    # 新网络层（1.21.11 IR）：客户端门面 + Play/Configuration visitor
    network/ClientNetwork.cpp
    network/ClientPlayVisitor.cpp
    # 聊天系统
    chat/ChatHistory.cpp
    # UI (平台无关)
    ui/FontTextureAtlas.cpp
    ui/Font.cpp
    ui/FontRenderer.cpp
    ui/DefaultAsciiFont.cpp
    ui/GuiScale.cpp
    ui/TridentCanvas.cpp
    ui/screen/ScreenManager.cpp
    ui/minecraft/screens/MapScreen.cpp
    ui/minecraft/screens/LoomScreen.cpp
    # UI Kagero模板系统
    ui/kagero/template/parser/Ast.cpp
    ui/kagero/template/parser/AstVisitor.cpp
    ui/kagero/template/parser/Lexer.cpp
    ui/kagero/template/parser/Parser.cpp
    ui/kagero/template/binder/BindingContext.cpp
    ui/kagero/template/compiler/TemplateCompiler.cpp
    ui/kagero/template/runtime/TemplateInstance.cpp
    ui/kagero/template/runtime/UpdateScheduler.cpp
    ui/kagero/template/bindings/BuiltinWidgets.cpp
    ui/kagero/template/bindings/BuiltinEvents.cpp
    ui/kagero/template/TemplateSystem.cpp
    # UI Kagero布局系统
    ui/kagero/layout/core/LayoutEngine.cpp
    ui/kagero/layout/algorithms/FlexLayout.cpp
    ui/kagero/layout/algorithms/GridLayout.cpp
    ui/kagero/layout/algorithms/AnchorLayout.cpp
    ui/kagero/layout/integration/WidgetLayoutAdaptor.cpp
    # UI Kagero Paint抽象
    ui/kagero/paint/Geometry.cpp
    ui/kagero/paint/PaintContext.cpp
    ui/kagero/paint/TextureImage.cpp
    ui/kagero/widget/ContainerWidget.cpp
    ui/kagero/widget/ListWidget.cpp
    ui/kagero/widget/Widget.cpp
    ui/kagero/widget/ItemTooltipBuilder.cpp
    # UI Kagero 引擎
    ui/kagero/KageroEngine.cpp
    # UI Kagero widgets
    # Minecraft UI业务层
    ui/minecraft/widgets/CrosshairWidget.cpp
    ui/minecraft/MinecraftUIContext.cpp
    ui/minecraft/resources/ResourceProvider.cpp
    ui/minecraft/widgets/SlotWidget.cpp
    ui/minecraft/widgets/InventorySlot.cpp
    ui/minecraft/widgets/HotbarWidget.cpp
    ui/minecraft/widgets/HealthBarWidget.cpp
    ui/minecraft/widgets/HungerBarWidget.cpp
    ui/minecraft/widgets/ExperienceBar.cpp
    ui/minecraft/widgets/Viewport3DWidget.cpp
    ui/minecraft/widgets/HudWidget.cpp
    ui/minecraft/widgets/TitleWidget.cpp
    ui/minecraft/targetinfo/TargetInfo.cpp
    ui/minecraft/targetinfo/TargetInfoResolver.cpp
    ui/minecraft/targetinfo/TargetInfoWidget.cpp
    ui/minecraft/widgets/ChatWidget.cpp
    ui/minecraft/widgets/ScreenStackWidget.cpp
    ui/minecraft/screens/Screen.cpp
    ui/minecraft/screens/ScreenManager.cpp
    ui/minecraft/screens/TemplateScreen.cpp
    ui/minecraft/screens/MainMenuScreen.cpp
    ui/minecraft/screens/WorldSelectionScreen.cpp
    ui/minecraft/screens/CreateWorldScreen.cpp
    ui/minecraft/screens/LoadingScreen.cpp
    ui/minecraft/screens/InventoryScreen.cpp
    ui/minecraft/screens/CreativeScreen.cpp
    ui/minecraft/screens/CraftingScreen.cpp
    ui/minecraft/screens/ChestScreen.cpp
    ui/minecraft/screens/FurnaceScreen.cpp
    ui/minecraft/screens/CartographyScreen.cpp
    ui/minecraft/screens/ContainerScreen.cpp
    ui/minecraft/screens/PauseScreen.cpp
    ui/minecraft/screens/ConfirmScreen.cpp
    ui/minecraft/screens/MessageScreen.cpp
    ui/minecraft/screens/OptionsScreen.cpp
    ui/minecraft/screens/DebugScreenWidget.cpp
    ui/minecraft/screens/SignEditScreen.cpp
    # 客户端实体管理
    world/entity/ClientEntity.cpp
    world/entity/ClientEntityManager.cpp
    # 本地玩家身份
    world/player/LocalPlayerIdentity.cpp
    # 玩家身份注册表（UUID↔entityId↔username 多向映射）
    world/player/PlayerIdentityRegistry.cpp
    # 客户端玩家预测器
    world/player/ClientPlayerPredictor.cpp
    # 客户端皮肤系统
    skin/ClientSkinManager.cpp
    skin/SkinTextureUploader.cpp
    # 内置服务端支持
    ${CMAKE_SOURCE_DIR}/src/server/application/IntegratedServer.cpp
    ${CMAKE_SOURCE_DIR}/src/server/application/MinecraftServer.cpp
    # 局域网发布所需的 TCP 网络模块（IntegratedServer::publishToLan 依赖）
    ${CMAKE_SOURCE_DIR}/src/server/network/TcpServer.cpp
    ${CMAKE_SOURCE_DIR}/src/server/network/TcpSession.cpp
    ${CMAKE_SOURCE_DIR}/src/server/network/TcpConnection.cpp
    # 新网络层服务端实现：IntegratedServer.cpp 引用 ServerNetwork/ServerHandshake/
    # ServerPlayRouter/RegistryDataBuilder，MinecraftServer.cpp 定义 routeInboundPlayPacket
    # 供 ServerPlayRouter 调用。客户端内置服需登记全部 4 个文件（与 server/CMakeLists.txt 一致）。
    ${CMAKE_SOURCE_DIR}/src/server/network/ServerNetwork.cpp
    ${CMAKE_SOURCE_DIR}/src/server/network/ServerHandshake.cpp
    ${CMAKE_SOURCE_DIR}/src/server/network/ServerPlayRouter.cpp
    ${CMAKE_SOURCE_DIR}/src/server/network/RegistryDataBuilder.cpp
    ${CMAKE_SOURCE_DIR}/src/server/mod/bedrock/addon/ServerScriptManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/mod/bedrock/addon/bridge/EventBridge.cpp
    ${CMAKE_SOURCE_DIR}/src/server/mod/bedrock/addon/bridge/ServerEventSignals.cpp
    ${CMAKE_SOURCE_DIR}/src/server/settings/ServerSettings.cpp
    ${CMAKE_SOURCE_DIR}/src/server/core/PlayerManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/core/ConnectionManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/core/TimeManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/core/TeleportManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/core/KeepAliveManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/core/PositionTracker.cpp
    ${CMAKE_SOURCE_DIR}/src/server/core/PacketHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/server/core/GameModeManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/core/WhitelistManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/core/BannedPlayerList.cpp
    ${CMAKE_SOURCE_DIR}/src/server/core/BannedIpList.cpp
    ${CMAKE_SOURCE_DIR}/src/server/core/OpListManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/ServerWorld.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/ChunkTaskScheduler.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/ChunkProgressionTask.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/ChunkLightingProvider.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/ServerChunkManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/SingleChunkLifecycleManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/ServerLightQueue.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/RuntimeLightingProvider.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/RuntimeLightTask.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/ChunkLoadLightTask.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/spawn/SpawnConditions.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/spawn/NaturalSpawner.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/spawn/DespawnManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/spawn/VillageSiege.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/entity/EntityTracker.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/entity/EntityChunkTracker.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/entity/ItemPickupManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/drop/BlockDropHandler.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/weather/WeatherManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/player/ServerPlayerEntityManager.cpp
    # 游戏事件系统（服务端实现）
    ${CMAKE_SOURCE_DIR}/src/server/world/gameevent/PositionSource.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/gameevent/GameEventListenerRegistry.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/gameevent/GameEventDispatcher.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/gameevent/DynamicGameEventListener.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/gameevent/VibrationSystemServer.cpp
    # 幽匿方块实体振动系统（服务端方法）
    ${CMAKE_SOURCE_DIR}/src/server/world/blockentity/sculk/SculkVibrationSystem.cpp
    ${CMAKE_SOURCE_DIR}/src/server/world/blockentity/sculk/SculkShriekerHelper.cpp
    ${CMAKE_SOURCE_DIR}/src/server/player/ServerPlayer.cpp
    ${CMAKE_SOURCE_DIR}/src/server/advancement/PlayerAdvancements.cpp
    # 事件系统
    ${CMAKE_SOURCE_DIR}/src/server/event/ServerEventBus.cpp
    # 统计系统
    ${CMAKE_SOURCE_DIR}/src/server/stats/StatType.cpp
    ${CMAKE_SOURCE_DIR}/src/server/stats/Stat.cpp
    ${CMAKE_SOURCE_DIR}/src/server/stats/StatRegistry.cpp
    ${CMAKE_SOURCE_DIR}/src/server/stats/StatisticsManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/menu/CraftingMenu.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/ServerCommandSource.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/CommandRegistry.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/support/PlayerResolver.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/support/EntityResolver.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/support/EffectResolver.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/support/FunctionSuggestionProvider.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/support/SpreadAlgorithm.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/data/DataAccessor.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/GameModeCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/DifficultyCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/DefaultGameModeCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/TimeCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/KillCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/ListCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/HelpCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/SeedCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/TeleportCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/GiveCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/ClearCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/WeatherCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/ExperienceCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/KickCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/SayCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/StopCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/SetIdleTimeoutCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/SummonCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/SetBlockCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/FillCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/ExecuteCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/OpCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/DeOpCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/BanCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/BanIpCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/PardonCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/PardonIpCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/BanListCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/WhitelistCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/SaveAllCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/SaveOnCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/SaveOffCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/SpawnPointCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/SetWorldSpawnCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/MessageCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/TellRawCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/TitleCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/PlaySoundCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/StopSoundCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/EffectCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/EnchantCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/GameRuleCommand.cpp
    # P3 命令
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/AdvancementCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/BossBarCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/DataPackCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/ForceLoadCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/LootCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/PublishCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/RecipeCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/ReloadCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/ReplaceItemCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/ScheduleCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/SpectateCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/SpreadPlayersCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/TagCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/TeamCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/TriggerCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/WardenSpawnTrackerCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/WorldBorderCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/ScoreboardCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/AttributeCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/CloneCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/DataCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/FunctionCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/LocateCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/LocateBiomeCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/MeCommand.cpp
    ${CMAKE_SOURCE_DIR}/src/server/command/commands/ParticleCommand.cpp
    # 交互管理器
    ${CMAKE_SOURCE_DIR}/src/server/interaction/BlockInteractionManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/interaction/MiningManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/interaction/ContainerManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/interaction/InventoryManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/interaction/SignCommandHelper.cpp
    # 维度模块
    ${CMAKE_SOURCE_DIR}/src/server/dimension/ServerDimension.cpp
    ${CMAKE_SOURCE_DIR}/src/server/dimension/ServerDimensionManager.cpp
    # 同步管理器
    ${CMAKE_SOURCE_DIR}/src/server/sync/EntitySyncManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/sync/BlockUpdateSyncManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/sync/ChunkSendManager.cpp
    # 记分板
    ${CMAKE_SOURCE_DIR}/src/server/scoreboard/ServerScoreboard.cpp
    ${CMAKE_SOURCE_DIR}/src/common/scoreboard/core/Scoreboard.cpp
    ${CMAKE_SOURCE_DIR}/src/common/scoreboard/core/ScoreObjective.cpp
    ${CMAKE_SOURCE_DIR}/src/common/scoreboard/core/Score.cpp
    ${CMAKE_SOURCE_DIR}/src/common/scoreboard/core/ScoreCriteria.cpp
    ${CMAKE_SOURCE_DIR}/src/common/scoreboard/core/ScoreCriteriaRenderType.cpp
    ${CMAKE_SOURCE_DIR}/src/common/scoreboard/core/ScorePlayerTeam.cpp
    ${CMAKE_SOURCE_DIR}/src/common/scoreboard/criteria/DummyCriteria.cpp
    ${CMAKE_SOURCE_DIR}/src/common/scoreboard/criteria/TriggerCriteria.cpp
    ${CMAKE_SOURCE_DIR}/src/common/scoreboard/criteria/DeathCountCriteria.cpp
    ${CMAKE_SOURCE_DIR}/src/common/scoreboard/criteria/KillCountCriteria.cpp
    ${CMAKE_SOURCE_DIR}/src/common/scoreboard/criteria/ReadOnlyCriteria.cpp
    ${CMAKE_SOURCE_DIR}/src/common/scoreboard/criteria/TeamKillCriteria.cpp
    ${CMAKE_SOURCE_DIR}/src/common/scoreboard/network/ScoreboardPackets.cpp
    ${CMAKE_SOURCE_DIR}/src/common/scoreboard/storage/ScoreboardSaveData.cpp
    # Boss栏系统
    ${CMAKE_SOURCE_DIR}/src/server/bossbar/BossInfo.cpp
    ${CMAKE_SOURCE_DIR}/src/server/bossbar/ServerBossInfo.cpp
    ${CMAKE_SOURCE_DIR}/src/server/bossbar/CustomServerBossInfo.cpp
    ${CMAKE_SOURCE_DIR}/src/server/bossbar/CustomServerBossInfoManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/bossbar/ServerDragonBossBar.cpp
    # 函数系统
    ${CMAKE_SOURCE_DIR}/src/server/function/CommandFunction.cpp
    ${CMAKE_SOURCE_DIR}/src/server/function/FunctionLoader.cpp
    ${CMAKE_SOURCE_DIR}/src/server/function/FunctionManager.cpp
    ${CMAKE_SOURCE_DIR}/src/server/function/MacroFunction.cpp
    ${CMAKE_SOURCE_DIR}/src/server/function/StringTemplate.cpp
    ${CMAKE_SOURCE_DIR}/src/server/function/TimerQueue.cpp
)

target_link_libraries(minecraft-client
    PRIVATE
        mc_common
        mc_bedrock_addon
        nlohmann_json::nlohmann_json
        glfw
        Vulkan::Vulkan
        OpenAL::OpenAL
        mc_offset_allocator
)

if(WIN32)
    target_link_libraries(minecraft-client PRIVATE user32 gdi32 ws2_32)
endif()

mc_set_compiler_warnings(minecraft-client)
mc_copy_runtime_dlls(minecraft-client)

target_compile_features(minecraft-client PRIVATE cxx_std_20)

target_compile_definitions(minecraft-client PRIVATE
    GLFW_INCLUDE_VULKAN
)

set_target_properties(minecraft-client PROPERTIES
    OUTPUT_NAME "minecraft-client"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# 依赖着色器编译
if(TARGET compile_shaders)
    add_dependencies(minecraft-client compile_shaders)
endif()
