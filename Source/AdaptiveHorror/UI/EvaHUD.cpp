#include "UI/EvaHUD.h"
#include "AI/EvaAdamBossAIController.h"
#include "AI/EvaAdamBossCharacter.h"
#include "AI/EvaLearningSubsystem.h"
#include "AI/EvaTelemetryTypes.h"
#include "Characters/EvaPlayerCharacter.h"
#include "Components/EvaHealthComponent.h"
#include "Components/EvaPlayerTelemetryComponent.h"
#include "Core/EvaPrototypeGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
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

    if (GameMode && GameMode->IsDebugHUDVisible())
    {
        DrawStat(FString::Printf(TEXT("KILLS: %d"), Stats.KillCount));
        DrawStat(FString::Printf(TEXT("ACCURACY: %.1f%%"), Telemetry->GetAccuracy() * 100.0f));
        DrawStat(FString::Printf(TEXT("HEADSHOTS: %.1f%%"), Telemetry->GetHeadshotRate() * 100.0f));
        DrawStat(FString::Printf(TEXT("PREFERRED DIST: %.0f  CLOSE %.2f  LONG %.2f"),
            AdaptationProfile.PreferredCombatDistance,
            AdaptationProfile.CloseRangeRatio,
            AdaptationProfile.LongRangeRatio));
        DrawStat(FString::Printf(TEXT("PROFILE: Agg %.2f  Stealth %.2f  Explore %.2f  Sprint %.2f"),
            AdaptationProfile.AggressionScore,
            AdaptationProfile.StealthScore,
            AdaptationProfile.ExplorationScore,
            AdaptationProfile.SprintUsage));
        DrawStat(FString::Printf(TEXT("EVA ADAPT: %s"), *DirectiveToString(AdaptationDirective)));
        DrawStat(FString::Printf(TEXT("NEXT EVOLUTION: %s"), *EvolutionToString(NextEvolution)));
        DrawStat(FString::Printf(TEXT("ENEMY ROLE: %s"), *UEnum::GetValueAsString(EnemyTuning.BehaviorRole)));
        DrawStat(FString::Printf(TEXT("APPLIED: SPD %.2f RNG %.2f CD %.2f DMG %.2f SIDE %.2f"),
            EnemyTuning.MoveSpeedMultiplier,
            EnemyTuning.AttackRangeMultiplier,
            EnemyTuning.AttackCooldownMultiplier,
            EnemyTuning.DamageMultiplier,
            EnemyTuning.SidestepChance));
        DrawStat(FString::Printf(TEXT("HUNTER COUNTER: %s"), *UEnum::GetValueAsString(HunterCounterType)));
        DrawStat(FString::Printf(TEXT("ACTIVE ZOMBIES: %d  HUNTERS: %d  ADAM: %d"),
            GameMode->GetActiveZombieCount(), GameMode->GetActiveHunterCount(), GameMode->GetActiveAdamCount()));
        DrawStat(FString::Printf(TEXT("SPAWN: %s"), *GameMode->GetLastSpawnResult()));
        DrawStat(FString::Printf(TEXT("SPAWN LOC: %s"), *GameMode->GetLastSpawnLocation().ToCompactString()));
        DrawStat(FString::Printf(TEXT("NAVMESH: %s  NAV DEBUG: %s  FALLBACK: %d  STUCK: %d"),
            GameMode->IsNavMeshAvailable() ? TEXT("YES") : TEXT("NO"),
            GameMode->IsNavigationDebugVisible() ? TEXT("ON") : TEXT("OFF"),
            GameMode->GetFallbackMovementCount(),
            GameMode->GetStuckEnemyCount()));
    }
    const AEvaResearchFacilityDirector* Director = GameMode ? GameMode->GetResearchDirector() : nullptr;
    if (Director)
    {
        DrawStat(FString::Printf(TEXT("ZONE: %s"), *Director->GetCurrentZoneName()));
        DrawStat(FString::Printf(TEXT("OBJECTIVE: %s"), *Director->GetObjectiveText()));
        if (GameMode && GameMode->IsDebugHUDVisible())
        {
            DrawStat(FString::Printf(TEXT("EVA LOGS: %d / 5"), Director->GetCollectedStoryLogCount()));
        }
    }

    const float CenterX = Canvas->ClipX * 0.5f;
    const float CenterY = Canvas->ClipY * 0.5f;
    DrawLine(CenterX - 8.0f, CenterY, CenterX + 8.0f, CenterY, FLinearColor::White, 1.5f);
    DrawLine(CenterX, CenterY - 8.0f, CenterX, CenterY + 8.0f, FLinearColor::White, 1.5f);

    if (GameMode && GameMode->ShouldDisplayDebugStatusMessage())
    {
        DrawText(GameMode->GetDebugStatusMessage(), FLinearColor(0.3f, 0.95f, 1.0f),
            CenterX - 320.0f, 85.0f, nullptr, 1.15f);
    }

    if (Director && Director->ShouldDisplayStoryLog())
    {
        const float LogX = CenterX - 360.0f;
        const float LogY = Canvas->ClipY - 150.0f;
        DrawText(FString::Printf(TEXT("EVA LOG: %s"), *Director->GetLastStoryLogTitle()),
            FLinearColor(0.4f, 0.9f, 1.0f), LogX, LogY, nullptr, 1.2f);
        DrawText(Director->GetLastStoryLogBody(), FLinearColor::White, LogX, LogY + 30.0f, nullptr, 0.95f);
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
