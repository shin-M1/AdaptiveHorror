#include "UI/EvaHUD.h"
#include "AI/EvaAdamBossAIController.h"
#include "AI/EvaAdamBossCharacter.h"
#include "AI/EvaLearningSubsystem.h"
#include "AI/EvaTelemetryTypes.h"
#include "AI/EvaZombieAIController.h"
#include "Characters/EvaPlayerCharacter.h"
#include "Components/EvaHealthComponent.h"
#include "Components/EvaPlayerTelemetryComponent.h"
#include "Core/EvaPrototypeGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Navigation/PathFollowingComponent.h"
#include "UI/EvaBossHUDWidget.h"
#include "Weapons/EvaWeaponBase.h"
#include "World/EvaResearchFacilityDirector.h"

void AEvaHUD::BeginPlay()
{
    Super::BeginPlay();
    EnsureBossHUDWidget();
}

void AEvaHUD::DrawHUD()
{
    Super::DrawHUD();
    if (!Canvas || !PlayerOwner)
    {
        return;
    }

    UpdateBossHUD();

    UWorld* World = GetWorld();
    const AEvaPrototypeGameMode* GameMode = World ? World->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr;
    if (GameMode &&
        (GameMode->GetGameFlowState() == EEvaGameFlowState::Title ||
            GameMode->GetGameFlowState() == EEvaGameFlowState::Loading))
    {
        return;
    }

    const AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(PlayerOwner->GetPawn());
    if (!Player)
    {
        DrawText(TEXT("Spawning EVA prototype..."), FLinearColor::White, 40.0f, 40.0f);
        return;
    }

    const UEvaHealthComponent* Health = Player->GetHealthComponent();
    const AEvaWeaponBase* Weapon = Player->GetCurrentWeapon();
    const UEvaPlayerTelemetryComponent* Telemetry = Player->GetTelemetryComponent();
    if (!Health || !Telemetry)
    {
        DrawText(TEXT("Initializing EVA player systems..."), FLinearColor::Yellow, 40.0f, 40.0f);
        return;
    }

    const FEvaTelemetrySnapshot Stats = Telemetry->GetTelemetry();
    DrawHorrorOverlay(Player, GameMode);

    FString StyleText = TEXT("Unknown");
    switch (Telemetry->ClassifyCombatStyle())
    {
    case EEvaCombatStyle::Berserker: StyleText = TEXT("Berserker"); break;
    case EEvaCombatStyle::Tactician: StyleText = TEXT("Tactician"); break;
    case EEvaCombatStyle::Ranger: StyleText = TEXT("Ranger"); break;
    case EEvaCombatStyle::Ghost: StyleText = TEXT("Ghost"); break;
    case EEvaCombatStyle::Explorer: StyleText = TEXT("Explorer"); break;
    default: break;
    }

    float AnalysisRate = 0.0f;
    float LearningMultiplier = 1.0f;
    EEvaHunterState HunterState = EEvaHunterState::Dormant;
    EEvaAnalysisStage AnalysisStage = EEvaAnalysisStage::Learning;
    EEvaAdaptationDirective AdaptationDirective = EEvaAdaptationDirective::None;
    EEvaEvolutionType NextEvolution = EEvaEvolutionType::None;
    FEvaPlayerAdaptationProfile AdaptationProfile;
    FEvaEnemyAdaptationTuning EnemyTuning;
    EEvaHunterCounterType HunterCounterType = EEvaHunterCounterType::None;
    int32 HunterTier = 0;
    if (const UGameInstance* GameInstance = World ? World->GetGameInstance() : nullptr)
    {
        if (const UEvaLearningSubsystem* Learning = GameInstance->GetSubsystem<UEvaLearningSubsystem>())
        {
            AnalysisRate = Learning->GetEvaAnalysisRate();
            LearningMultiplier = Learning->GetLearningSpeedMultiplier();
            HunterState = Learning->GetHunterState();
            HunterTier = Learning->GetHunterTier();
            AnalysisStage = Learning->GetAnalysisStage();
            AdaptationDirective = Learning->GetAdaptationDirective();
            NextEvolution = Learning->GetRecommendedEvolutionType();
            AdaptationProfile = Learning->GetCurrentAdaptationProfile();
            if (AdaptationProfile.bValid)
            {
                switch (AdaptationProfile.CombatStyle)
                {
                case EEvaCombatStyle::Berserker: StyleText = TEXT("Berserker"); break;
                case EEvaCombatStyle::Tactician: StyleText = TEXT("Tactician"); break;
                case EEvaCombatStyle::Ranger: StyleText = TEXT("Ranger"); break;
                case EEvaCombatStyle::Ghost: StyleText = TEXT("Ghost"); break;
                case EEvaCombatStyle::Explorer: StyleText = TEXT("Explorer"); break;
                default: StyleText = TEXT("Unknown"); break;
                }
            }
            EnemyTuning = Learning->BuildEnemyAdaptationTuning(NextEvolution);
            HunterCounterType = Learning->GetHunterCounterTypeForTier(HunterTier);
        }
    }

    auto HunterStateToString = [](const EEvaHunterState State)
    {
        switch (State)
        {
        case EEvaHunterState::Deployed: return FString(TEXT("DEPLOYED"));
        case EEvaHunterState::DefeatedCooldown: return FString(TEXT("DEFEATED / REINSERTING"));
        default: return FString(TEXT("DORMANT"));
        }
    };
    auto StageToString = [](const EEvaAnalysisStage Stage)
    {
        switch (Stage)
        {
        case EEvaAnalysisStage::Adapting: return FString(TEXT("ADAPTING"));
        case EEvaAnalysisStage::Evolving: return FString(TEXT("EVOLVING"));
        default: return FString(TEXT("LEARNING"));
        }
    };
    auto DirectiveToString = [](const EEvaAdaptationDirective Directive)
    {
        switch (Directive)
        {
        case EEvaAdaptationDirective::CounterCloseRange: return FString(TEXT("Keep Distance"));
        case EEvaAdaptationDirective::CounterLongRange: return FString(TEXT("Use Cover"));
        case EEvaAdaptationDirective::CounterStealth: return FString(TEXT("Widen Search"));
        case EEvaAdaptationDirective::CounterExplorer: return FString(TEXT("Ambush Routes"));
        default: return FString(TEXT("None"));
        }
    };
    auto EvolutionToString = [](const EEvaEvolutionType Evolution)
    {
        switch (Evolution)
        {
        case EEvaEvolutionType::Fast: return FString(TEXT("Fast"));
        case EEvaEvolutionType::Armored: return FString(TEXT("Armored"));
        case EEvaEvolutionType::LongArm: return FString(TEXT("Long Arm"));
        case EEvaEvolutionType::Composite: return FString(TEXT("Composite"));
        default: return FString(TEXT("None"));
        }
    };
    auto HunterCounterToString = [](const EEvaHunterCounterType Counter)
    {
        switch (Counter)
        {
        case EEvaHunterCounterType::AntiBerserker: return FString(TEXT("Anti-Berserker"));
        case EEvaHunterCounterType::AntiRanger: return FString(TEXT("Anti-Ranger"));
        case EEvaHunterCounterType::AntiGhost: return FString(TEXT("Anti-Ghost"));
        case EEvaHunterCounterType::AntiExplorer: return FString(TEXT("Anti-Explorer"));
        default: return FString(TEXT("None"));
        }
    };
    auto MoveStatusToString = [](const EPathFollowingStatus::Type Status)
    {
        switch (Status)
        {
        case EPathFollowingStatus::Idle: return FString(TEXT("Idle"));
        case EPathFollowingStatus::Waiting: return FString(TEXT("Waiting"));
        case EPathFollowingStatus::Paused: return FString(TEXT("Paused"));
        case EPathFollowingStatus::Moving: return FString(TEXT("Moving"));
        default: return FString(TEXT("Unknown"));
        }
    };

    float Y = 35.0f;
    const float LineHeight = 24.0f;
    auto DrawStat = [this, &Y, LineHeight](const FString& Text)
    {
        DrawText(Text, FLinearColor(0.8f, 1.0f, 0.85f), 35.0f, Y, nullptr, 1.1f);
        Y += LineHeight;
    };

    DrawStat(FString::Printf(TEXT("HP: %.0f / %.0f"), Health->GetCurrentHealth(), Health->GetMaxHealth()));
    DrawStat(FString::Printf(TEXT("AMMO: %d / %d%s"), Weapon ? Weapon->GetAmmoInMagazine() : 0,
        Weapon ? Weapon->GetReserveAmmo() : 0, Weapon && Weapon->IsReloading() ? TEXT("  RELOADING") : TEXT("")));
    DrawStat(FString::Printf(TEXT("EVA ANALYSIS: %.0f%%  x%.1f"), AnalysisRate, LearningMultiplier));
    DrawStat(FString::Printf(TEXT("EVA STAGE: %s"), *StageToString(AnalysisStage)));
    DrawStat(FString::Printf(TEXT("STYLE: %s"), *StyleText));
    DrawStat(FString::Printf(TEXT("HUNTER: %s  TIER %d"), *HunterStateToString(HunterState), HunterTier));

    const AEvaResearchFacilityDirector* Director = GameMode ? GameMode->GetResearchDirector() : nullptr;
    const bool bShowDebugHUD = GameMode && GameMode->IsGameplayActive() && GameMode->IsDebugHUDVisible();
    FString FirstEnemyIntent(TEXT("None"));
    FString FirstMoveStatus(TEXT("None"));
    if (bShowDebugHUD && World)
    {
        for (TActorIterator<AEvaZombieAIController> It(World); It; ++It)
        {
            const AEvaZombieAIController* ZombieController = *It;
            if (!ZombieController || !ZombieController->GetPawn() || !ZombieController->IsCombatEnabled())
            {
                continue;
            }

            const FString ControllerIntent = ZombieController->GetCurrentActionIntent();
            FirstEnemyIntent = ControllerIntent.IsEmpty() ? FString(TEXT("None")) : ControllerIntent;
            FirstMoveStatus = MoveStatusToString(ZombieController->GetMoveStatus());
            break;
        }
    }

    if (bShowDebugHUD)
    {
        const int32 PageCount = FMath::Max(1, GameMode->GetDebugHUDPageCount());
        const int32 PageIndex = FMath::Clamp(GameMode->GetDebugHUDPageIndex(), 0, PageCount - 1);
        float DebugY = 35.0f;
        const float DebugX = FMath::Max(520.0f, Canvas->ClipX - 545.0f);
        const float DebugLineHeight = 21.0f;
        auto DrawDebugStat = [this, &DebugY, DebugX, DebugLineHeight](const FString& Text)
        {
            DrawText(Text, FLinearColor(0.55f, 0.95f, 1.0f), DebugX, DebugY, nullptr, 0.92f);
            DebugY += DebugLineHeight;
        };

        DrawDebugStat(FString::Printf(TEXT("DEBUG %d/%d"), PageIndex + 1, PageCount));
        if (PageIndex == 0)
        {
            DrawDebugStat(TEXT("PAGE: EVA / GAMEPLAY"));
            DrawDebugStat(FString::Printf(TEXT("Style: %s"), *StyleText));
            DrawDebugStat(FString::Printf(TEXT("Analysis: %.0f%%"), AnalysisRate));
            DrawDebugStat(FString::Printf(TEXT("Stage: %s"), *StageToString(AnalysisStage)));
            DrawDebugStat(FString::Printf(TEXT("Preferred Dist: %.0f"), AdaptationProfile.PreferredCombatDistance));
            DrawDebugStat(FString::Printf(TEXT("Aggression: %.2f"), AdaptationProfile.AggressionScore));
            DrawDebugStat(FString::Printf(TEXT("Stealth: %.2f"), AdaptationProfile.StealthScore));
            DrawDebugStat(FString::Printf(TEXT("Exploration: %.2f"), AdaptationProfile.ExplorationScore));
            DrawDebugStat(FString::Printf(TEXT("Current Adapt: %s"), *DirectiveToString(AdaptationDirective)));
            DrawDebugStat(FString::Printf(TEXT("Next Evolution: %s"), *EvolutionToString(NextEvolution)));
            DrawDebugStat(FString::Printf(TEXT("HUNTER Counter: %s"), *HunterCounterToString(HunterCounterType)));
        }
        else if (PageIndex == 1)
        {
            DrawDebugStat(TEXT("PAGE: ENEMY ADAPTATION"));
            DrawDebugStat(FString::Printf(TEXT("Enemy Role: %s"),
                EnemyTuning.RoleLabel.IsEmpty() ? TEXT("CHASE") : *EnemyTuning.RoleLabel));
            DrawDebugStat(FString::Printf(TEXT("Action Intent: %s"), *FirstEnemyIntent));
            DrawDebugStat(FString::Printf(TEXT("Speed x%.2f"), EnemyTuning.MoveSpeedMultiplier));
            DrawDebugStat(FString::Printf(TEXT("Range x%.2f"), EnemyTuning.AttackRangeMultiplier));
            DrawDebugStat(FString::Printf(TEXT("Cooldown x%.2f"), EnemyTuning.AttackCooldownMultiplier));
            DrawDebugStat(FString::Printf(TEXT("Damage x%.2f"), EnemyTuning.DamageMultiplier));
            DrawDebugStat(FString::Printf(TEXT("Sidestep %.2f"), EnemyTuning.SidestepChance));
            DrawDebugStat(FString::Printf(TEXT("COMPOSITE: %s"),
                EnemyTuning.CompositeHybridType.IsEmpty() ? TEXT("None") : *EnemyTuning.CompositeHybridType));
            DrawDebugStat(FString::Printf(TEXT("Hybrid Roles: %d/2"), EnemyTuning.CompositeHybridRoleCount));
            DrawDebugStat(FString::Printf(TEXT("HUNTER Type: %s"), *HunterCounterToString(HunterCounterType)));
        }
        else
        {
            DrawDebugStat(TEXT("PAGE: NAV / SPAWN"));
            DrawDebugStat(FString::Printf(TEXT("Nav Ready: %s"), GameMode->IsNavMeshAvailable() ? TEXT("YES") : TEXT("NO")));
            DrawDebugStat(FString::Printf(TEXT("RecastNavMesh: %s"), GameMode->IsNavMeshAvailable() ? TEXT("READY") : TEXT("NOT READY")));
            DrawDebugStat(FString::Printf(TEXT("MoveTo: %s"), *FirstMoveStatus));
            DrawDebugStat(FString::Printf(TEXT("Fallback: %d"), GameMode->GetFallbackMovementCount()));
            DrawDebugStat(FString::Printf(TEXT("Stuck: %d"), GameMode->GetStuckEnemyCount()));
            DrawDebugStat(FString::Printf(TEXT("Active Z/H/A: %d / %d / %d"),
                GameMode->GetActiveZombieCount(), GameMode->GetActiveHunterCount(), GameMode->GetActiveAdamCount()));
            DrawDebugStat(FString::Printf(TEXT("Spawn: %s"), *GameMode->GetLastSpawnResult()));
            DrawDebugStat(FString::Printf(TEXT("Spawn Loc: %s"), *GameMode->GetLastSpawnLocation().ToCompactString()));
            DrawDebugStat(FString::Printf(TEXT("Zone: %s"), Director ? *Director->GetCurrentZoneName() : TEXT("None")));
            DrawDebugStat(FString::Printf(TEXT("Objective: %s"), Director ? *Director->GetObjectiveText() : TEXT("None")));
            DrawDebugStat(FString::Printf(TEXT("Objective Index: %d"), Director ? Director->GetObjectiveIndex() : -1));
            DrawDebugStat(FString::Printf(TEXT("Power: %s"), Director && Director->IsFacilityPowerOnline() ? TEXT("ONLINE") : TEXT("OFFLINE")));
            DrawDebugStat(FString::Printf(TEXT("Keycard: %s"), Director && Director->HasSecurityKeycard() ? TEXT("YES") : TEXT("NO")));
            DrawDebugStat(FString::Printf(TEXT("Door: %s"), Director && Director->IsObservationDoorOpen() ? TEXT("OPEN") : TEXT("LOCKED")));
            DrawDebugStat(FString::Printf(TEXT("Logs: %d"), Director ? Director->GetCollectedStoryLogCount() : 0));
            DrawDebugStat(FString::Printf(TEXT("Data Core: %s"), Director && Director->IsDataCoreAccessed() ? TEXT("DONE") : TEXT("LOCKED")));
            DrawDebugStat(FString::Printf(TEXT("Arena: %s"), Director && Director->IsAdamArenaUnlocked() ? TEXT("UNLOCKED") : TEXT("LOCKED")));
        }
    }

    if (Director && !bShowDebugHUD)
    {
        DrawStat(FString::Printf(TEXT("ZONE: %s"), *Director->GetCurrentZoneName()));
        DrawStat(FString::Printf(TEXT("OBJECTIVE: %s"), *Director->GetObjectiveText()));
        DrawStat(Director->GetObjectiveProgressText());
    }

    const float CenterX = Canvas->ClipX * 0.5f;
    const float CenterY = Canvas->ClipY * 0.5f;
    DrawLine(CenterX - 8.0f, CenterY, CenterX + 8.0f, CenterY, FLinearColor::White, 1.5f);
    DrawLine(CenterX, CenterY - 8.0f, CenterX, CenterY + 8.0f, FLinearColor::White, 1.5f);

    const FString InteractionPrompt = Player->GetInteractionPrompt();
    if (!InteractionPrompt.IsEmpty())
    {
        DrawText(InteractionPrompt, FLinearColor(0.95f, 0.92f, 0.45f),
            CenterX - 160.0f, CenterY + 58.0f, nullptr, 1.12f);
    }

    if (GameMode && GameMode->ShouldDisplayDebugStatusMessage())
    {
        DrawText(GameMode->GetDebugStatusMessage(), FLinearColor(0.3f, 0.95f, 1.0f),
            CenterX - 320.0f, 85.0f, nullptr, 1.15f);
    }

    if (Director && Director->ShouldDisplayStoryLog())
    {
        const float LogX = CenterX - 390.0f;
        const float LogY = Director->IsResearchLogOpen() ? Canvas->ClipY * 0.58f : Canvas->ClipY - 150.0f;
        DrawText(FString::Printf(TEXT("EVA LOG: %s"), *Director->GetLastStoryLogTitle()),
            FLinearColor(0.4f, 0.9f, 1.0f), LogX, LogY, nullptr, 1.2f);
        DrawText(Director->GetLastStoryLogBody(), FLinearColor::White, LogX, LogY + 30.0f, nullptr, 0.95f);
        if (Director->IsResearchLogOpen())
        {
            DrawText(TEXT("Press E to close"), FLinearColor(0.95f, 0.92f, 0.45f),
                LogX, LogY + 62.0f, nullptr, 0.95f);
        }
    }
}

void AEvaHUD::EnsureBossHUDWidget()
{
    if (BossHUDWidget || !PlayerOwner)
    {
        return;
    }

    TSubclassOf<UEvaBossHUDWidget> WidgetClass = BossHUDWidgetClass;
    if (!WidgetClass)
    {
        WidgetClass = UEvaBossHUDWidget::StaticClass();
    }

    BossHUDWidget = CreateWidget<UEvaBossHUDWidget>(PlayerOwner, WidgetClass);
    if (BossHUDWidget)
    {
        BossHUDWidget->AddToViewport(30);
        BossHUDWidget->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void AEvaHUD::UpdateBossHUD()
{
    UWorld* World = GetWorld();
    const AEvaPrototypeGameMode* GameMode = World ? World->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr;
    const AEvaResearchFacilityDirector* Director = GameMode ? GameMode->GetResearchDirector() : nullptr;
    AEvaAdamBossCharacter* Adam = Director && Director->IsAdamEncounterActive() ? Director->GetActiveAdam() : nullptr;
    const UEvaHealthComponent* AdamHealth = Adam ? Adam->GetHealthComponent() : nullptr;
    const bool bShowBossHUD = Director && Director->IsAdamEncounterActive() && Adam && AdamHealth &&
        !AdamHealth->IsDead() && GameMode && !GameMode->IsStageClear();

    if (!bShowBossHUD)
    {
        if (BossHUDWidget)
        {
            BossHUDWidget->ClearBossSnapshot();
            BossHUDWidget->SetVisibility(ESlateVisibility::Collapsed);
        }
        return;
    }

    EnsureBossHUDWidget();
    if (!BossHUDWidget)
    {
        return;
    }

    if (Canvas)
    {
        BossHUDWidget->SetDesiredSizeInViewport(FVector2D(Canvas->ClipX, Canvas->ClipY));
        BossHUDWidget->SetPositionInViewport(FVector2D::ZeroVector, false);
    }

    const AEvaPlayerCharacter* Player = PlayerOwner ? Cast<AEvaPlayerCharacter>(PlayerOwner->GetPawn()) : nullptr;
    const AEvaAdamBossAIController* AdamController = Cast<AEvaAdamBossAIController>(Adam->GetController());
    const float TargetDistance = AdamController ? AdamController->GetCurrentTargetDistance() :
        (Player ? FVector::Distance(Adam->GetActorLocation(), Player->GetActorLocation()) : -1.0f);
    const int32 SummonCount = FMath::Max(Adam->GetLastSummonCount(),
        AdamController ? AdamController->GetLastSummonCount() : 0);

    BossHUDWidget->SetBossSnapshot(true, TEXT("ADAM"), AdamHealth->GetHealthPercent(),
        AdamHealth->GetCurrentHealth(), AdamHealth->GetMaxHealth(),
        Adam->IsPhaseTwo() ? TEXT("Phase 2") : TEXT("Phase 1"),
        AdamController ? AdamController->GetCurrentAdamState() : TEXT("No Controller"),
        TargetDistance,
        SummonCount,
        AdamController ? AdamController->GetLastAdamEvent() : TEXT("None"),
        Adam->IsDebugBossEnabled());
    BossHUDWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
}

void AEvaHUD::DrawHorrorOverlay(const AEvaPlayerCharacter* Player, const AEvaPrototypeGameMode* GameMode)
{
    if (!Canvas || !Player)
    {
        return;
    }
    if (GameMode && GameMode->GetGameFlowState() != EEvaGameFlowState::Playing)
    {
        return;
    }

    const float W = Canvas->ClipX;
    const float H = Canvas->ClipY;
    const float Damage = Player->GetDamageFeedbackIntensity();
    const float LowHealth = Player->GetLowHealthVignetteIntensity();
    const float Blackout = GameMode ? GameMode->GetBlackoutOverlayIntensity() : 0.0f;
    const float Pulse = GameMode ? GameMode->GetHorrorPulseIntensity() : 0.0f;

    if (Blackout > 0.0f)
    {
        DrawRect(FLinearColor(0.0f, 0.0f, 0.0f, Blackout), 0.0f, 0.0f, W, H);
    }

    const float VignetteAlpha = FMath::Clamp(0.08f + LowHealth * 0.26f + Damage * 0.16f + Pulse * 0.08f,
        0.0f, 0.48f);
    if (VignetteAlpha > 0.01f)
    {
        const float EdgeX = W * 0.16f;
        const float EdgeY = H * 0.16f;
        const FLinearColor EdgeColor(0.0f, 0.0f, 0.0f, VignetteAlpha);
        DrawRect(EdgeColor, 0.0f, 0.0f, W, EdgeY);
        DrawRect(EdgeColor, 0.0f, H - EdgeY, W, EdgeY);
        DrawRect(EdgeColor, 0.0f, 0.0f, EdgeX, H);
        DrawRect(EdgeColor, W - EdgeX, 0.0f, EdgeX, H);
    }

    if (Damage > 0.01f)
    {
        const float RedAlpha = FMath::Clamp(0.10f + Damage * 0.24f, 0.0f, 0.34f);
        DrawRect(FLinearColor(0.75f, 0.02f, 0.0f, RedAlpha), 0.0f, 0.0f, W, H);
    }

    if (Pulse > 0.01f)
    {
        const float PulseAlpha = FMath::Clamp(Pulse * 0.12f, 0.0f, 0.16f);
        DrawRect(FLinearColor(1.0f, 0.04f, 0.02f, PulseAlpha), 0.0f, 0.0f, W, H);
    }

    if (GameMode && GameMode->ShouldDisplayHorrorWarning())
    {
        DrawText(GameMode->GetHorrorWarningText(), FLinearColor(1.0f, 0.12f, 0.08f),
            W * 0.5f - 260.0f, H * 0.22f, nullptr, 1.35f);
    }
}
