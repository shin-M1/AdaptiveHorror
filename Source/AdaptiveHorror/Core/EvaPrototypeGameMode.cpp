#include "Core/EvaPrototypeGameMode.h"
#include "AdaptiveHorror.h"
#include "AI/EvaHunterCharacter.h"
#include "AI/EvaAdamBossCharacter.h"
#include "AI/EvaLearningSubsystem.h"
#include "AI/EvaZombieAIController.h"
#include "AI/EvaZombieCharacter.h"
#include "Characters/EvaPlayerCharacter.h"
#include "Characters/EvaPlayerController.h"
#include "Components/BoxComponent.h"
#include "Components/BrushComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/EvaHealthComponent.h"
#include "Components/EvaPlayerTelemetryComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/SkyLight.h"
#include "Engine/PointLight.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/HUD.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationData.h"
#include "NavigationSystem.h"
#include "NavMesh/NavMeshBoundsVolume.h"
#include "NavMesh/RecastNavMesh.h"
#include "UObject/UnrealType.h"
#include "Pickups/EvaAmmoPickup.h"
#include "Pickups/EvaHealthPickup.h"
#include "Pickups/EvaStoryLogPickup.h"
#include "UI/EvaHUD.h"
#include "World/EvaCheckpoint.h"
#include "World/EvaFacilityZoneTrigger.h"
#include "World/EvaResearchFacilityDirector.h"
#include "Weapons/EvaWeaponBase.h"

namespace
{
    constexpr int32 SafeSpawnAttemptCount = 12;
    constexpr float DefaultEnemySeparation = 360.0f;
    constexpr float DefaultPlayerSeparation = 520.0f;
    constexpr float ExpectedRuntimeFloorSurfaceZ = 25.0f;
    constexpr float RuntimeFloorHeightTolerance = 45.0f;
    constexpr float RuntimeFloorNormalMinZ = 0.9f;
    constexpr int32 MaxNavigationReadinessAttempts = 40;
    constexpr float NavigationReadinessInterval = 0.25f;

    const FName RuntimeFloorTag(TEXT("RuntimeFloor"));
    const FName EvaNavigableFloorTag(TEXT("EvaNavigableFloor"));

    bool IsLivingEvaEnemy(const AEvaZombieCharacter* Enemy)
    {
        return Enemy && Enemy->GetHealthComponent() && !Enemy->GetHealthComponent()->IsDead();
    }

    FString BoolText(const bool bValue)
    {
        return bValue ? TEXT("true") : TEXT("false");
    }

    FString NetModeToText(const ENetMode NetMode)
    {
        switch (NetMode)
        {
        case NM_Standalone: return TEXT("Standalone");
        case NM_DedicatedServer: return TEXT("DedicatedServer");
        case NM_ListenServer: return TEXT("ListenServer");
        case NM_Client: return TEXT("Client");
        default: return TEXT("Unknown");
        }
    }

    FString SpawnCollisionHandlingToText(const ESpawnActorCollisionHandlingMethod Method)
    {
        switch (Method)
        {
        case ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding:
            return TEXT("AdjustIfPossibleButDontSpawnIfColliding");
        case ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn:
            return TEXT("AdjustIfPossibleButAlwaysSpawn");
        case ESpawnActorCollisionHandlingMethod::AlwaysSpawn:
            return TEXT("AlwaysSpawn");
        case ESpawnActorCollisionHandlingMethod::DontSpawnIfColliding:
            return TEXT("DontSpawnIfColliding");
        default:
            return TEXT("Undefined");
        }
    }

    FString RuntimeGenerationToText(const ERuntimeGenerationType RuntimeGeneration)
    {
        switch (RuntimeGeneration)
        {
        case ERuntimeGenerationType::Static:
            return TEXT("Static");
        case ERuntimeGenerationType::DynamicModifiersOnly:
            return TEXT("DynamicModifiersOnly");
        case ERuntimeGenerationType::Dynamic:
            return TEXT("Dynamic");
        case ERuntimeGenerationType::LegacyGeneration:
            return TEXT("LegacyGeneration");
        default:
            return TEXT("Unknown");
        }
    }

    bool ReadBoolConfigProperty(const UObject* Object, const FName PropertyName, bool& OutValue)
    {
        if (!Object)
        {
            return false;
        }

        const FBoolProperty* BoolProperty = FindFProperty<FBoolProperty>(Object->GetClass(), PropertyName);
        if (!BoolProperty)
        {
            return false;
        }

        OutValue = BoolProperty->GetPropertyValue_InContainer(Object);
        return true;
    }

    bool WriteBoolConfigProperty(UObject* Object, const FName PropertyName, const bool bValue)
    {
        if (!Object)
        {
            return false;
        }

        FBoolProperty* BoolProperty = FindFProperty<FBoolProperty>(Object->GetClass(), PropertyName);
        if (!BoolProperty)
        {
            return false;
        }

        BoolProperty->SetPropertyValue_InContainer(Object, bValue);
        return true;
    }

    FString ReadBoolConfigText(const UObject* Object, const FName PropertyName)
    {
        bool bValue = false;
        return ReadBoolConfigProperty(Object, PropertyName, bValue) ? BoolText(bValue) : TEXT("unavailable");
    }

    bool IsAcceptableEvaFloorHit(const FHitResult& FloorHit)
    {
        if (!FloorHit.bBlockingHit || FloorHit.ImpactNormal.Z < RuntimeFloorNormalMinZ)
        {
            return false;
        }

        const UPrimitiveComponent* HitComponent = FloorHit.GetComponent();
        const bool bComponentTaggedFloor = HitComponent &&
            (HitComponent->ComponentHasTag(RuntimeFloorTag) || HitComponent->ComponentHasTag(EvaNavigableFloorTag));
        const bool bExpectedHeight = FMath::Abs(FloorHit.ImpactPoint.Z - ExpectedRuntimeFloorSurfaceZ) <=
            RuntimeFloorHeightTolerance;
        return bComponentTaggedFloor && bExpectedHeight;
    }

    float GetNearestEnemyDistance(UWorld* World, const FVector& Location, const AActor* IgnoreActor = nullptr)
    {
        if (!World)
        {
            return TNumericLimits<float>::Max();
        }

        float NearestDistance = TNumericLimits<float>::Max();
        for (TActorIterator<AEvaZombieCharacter> It(World); It; ++It)
        {
            const AEvaZombieCharacter* Enemy = *It;
            if (!IsLivingEvaEnemy(Enemy) || Enemy == IgnoreActor)
            {
                continue;
            }

            NearestDistance = FMath::Min(NearestDistance, FVector::Dist(Location, Enemy->GetActorLocation()));
        }
        return NearestDistance;
    }

    bool HasBlockingSpawnOverlap(UWorld* World, const FVector& Location, const float CapsuleRadius,
        const float CapsuleHalfHeight)
    {
        if (!World)
        {
            return true;
        }

        FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EvaEnemySpawnOverlap), false);
        const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(CapsuleRadius, CapsuleHalfHeight);
        const bool bPawnOverlap = World->OverlapBlockingTestByChannel(Location, FQuat::Identity, ECC_Pawn,
            CapsuleShape, QueryParams);
        const bool bWorldOverlap = World->OverlapBlockingTestByChannel(Location, FQuat::Identity, ECC_WorldStatic,
            CapsuleShape, QueryParams);
        return bPawnOverlap || bWorldOverlap;
    }
}

AEvaPrototypeGameMode::AEvaPrototypeGameMode()
{
    DefaultPawnClass = AEvaPlayerCharacter::StaticClass();
    PlayerControllerClass = AEvaPlayerController::StaticClass();
    HUDClass = AEvaHUD::StaticClass();
    LastCheckpointTransform = FTransform(FRotator::ZeroRotator, FVector(-5650.0f, 0.0f, 160.0f));
}

void AEvaPrototypeGameMode::BeginPlay()
{
    Super::BeginPlay();

    if (!GetWorld())
    {
        return;
    }

    SetGameFlowState(EEvaGameFlowState::Title);
    RuntimeCubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    LogRuntimeClassBindings();
    if (bBuildRuntimeArena)
    {
        BuildPrototypeArena();
    }

    EnsurePrototypePlayer();
    if (bBuildRuntimeArena)
    {
        GetWorldTimerManager().SetTimerForNextTick(this, &AEvaPrototypeGameMode::BuildRuntimeNavigation);
    }
    else
    {
        bRuntimeNavigationReady = true;
    }
}

void AEvaPrototypeGameMode::HandlePlayerDeath(AEvaPlayerCharacter* DeadPlayer)
{
    if (bStageClear)
    {
        LogPlayerDeathRequest(TEXT("RejectedBecauseStageClear"), DeadPlayer, false);
        return;
    }

    if (bGameOver || !DeadPlayer || !GetWorld() || GameFlowState != EEvaGameFlowState::Playing)
    {
        LogPlayerDeathRequest(bGameOver ? TEXT("RejectedBecauseAlreadyGameOver") : TEXT("RejectedInvalidRequestOrFlow"),
            DeadPlayer, false);
        return;
    }

    LogPlayerDeathRequest(TEXT("DeathRequest"), DeadPlayer, false);
    SetGameFlowState(EEvaGameFlowState::PlayerDead);
    bGameOver = true;
    PlayerAwaitingRespawn = DeadPlayer;
    ClearStageClearTimers();
    CleanupCombatActorsForFlowReset();
    if (APlayerController* PlayerController = Cast<APlayerController>(DeadPlayer->GetController()))
    {
        PlayerController->ResetIgnoreMoveInput();
        PlayerController->ResetIgnoreLookInput();
        PlayerController->SetIgnoreMoveInput(true);
        PlayerController->SetIgnoreLookInput(true);
        if (AEvaPlayerController* EvaController = Cast<AEvaPlayerController>(PlayerController))
        {
            EvaController->ShowGameOverMenu();
        }
    }
    ShowDebugStatusMessage(TEXT("GAME OVER - choose retry or return to title."), 6.0f);
    GetWorldTimerManager().ClearTimer(RespawnTimer);
    LogPlayerDeathRequest(TEXT("GameOverMenuCreated"), DeadPlayer, false);
}

void AEvaPrototypeGameMode::ActivateCheckpoint(const FTransform& CheckpointTransform)
{
    LastCheckpointTransform = CheckpointTransform;
}

void AEvaPrototypeGameMode::EnterTitleMode()
{
    if (!GetWorld())
    {
        return;
    }

    UGameplayStatics::SetGamePaused(GetWorld(), false);
    UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
    ClearStageClearTimers();
    CleanupCombatActorsForFlowReset();
    bGameOver = false;
    bStageClear = false;
    PlayerAwaitingRespawn = nullptr;
    bInitialZombieSpawned = false;
    LastCheckpointTransform = FTransform(FRotator::ZeroRotator, FVector(-5650.0f, 0.0f, 160.0f));
    if (CurrentDirector)
    {
        CurrentDirector->ResetForNewGame();
    }

    SetGameFlowState(EEvaGameFlowState::Title);
    EnsurePrototypePlayer();
    bool bTitleWidgetVisible = false;
    bool bTitleWidgetRequired = false;
    if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
    {
        PlayerController->ResetIgnoreMoveInput();
        PlayerController->ResetIgnoreLookInput();
        PlayerController->SetIgnoreMoveInput(true);
        PlayerController->SetIgnoreLookInput(true);
        if (AEvaPlayerController* EvaController = Cast<AEvaPlayerController>(PlayerController))
        {
            bTitleWidgetRequired = EvaController->IsLocalController();
            bTitleWidgetVisible = EvaController->ShowTitleMenu();
        }
    }

    if (bTitleWidgetRequired && !bTitleWidgetVisible)
    {
        UE_LOG(LogAdaptiveHorror, Error,
            TEXT("[TitleUI] FailureFallback=StartNewGameFlow Reason=Title widget was not visible after EnterTitleMode."));
        ShowDebugStatusMessage(TEXT("TITLE UI ERROR: Falling back to playable mode."), 6.0f);
        StartNewGameFlow();
    }
}

void AEvaPrototypeGameMode::StartNewGameFlow()
{
    if (!GetWorld())
    {
        return;
    }

    UGameplayStatics::SetGamePaused(GetWorld(), false);
    UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
    ClearStageClearTimers();
    CleanupCombatActorsForFlowReset();

    bGameOver = false;
    bStageClear = false;
    PlayerAwaitingRespawn = nullptr;
    TotalZombieKills = 0;
    HunterDefeatCount = 0;
    HunterTierToSpawn = 1;
    CurrentHunter = nullptr;
    bInitialZombieSpawned = false;
    FallbackMovementCount = 0;
    StuckEnemyCount = 0;
    LastSpawnResult = TEXT("New Game: no enemy spawn attempted yet.");
    LastSpawnLocation = FVector::ZeroVector;
    LastCheckpointTransform = FTransform(FRotator::ZeroRotator, FVector(-5650.0f, 0.0f, 160.0f));

    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        if (UEvaLearningSubsystem* Learning = GameInstance->GetSubsystem<UEvaLearningSubsystem>())
        {
            Learning->ResetLearning();
        }
    }

    if (CurrentDirector)
    {
        CurrentDirector->ResetForNewGame();
    }

    EnsurePrototypePlayer();
    AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(
        GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr);
    if (Player)
    {
        Player->ResetForCheckpoint(LastCheckpointTransform);
        if (UEvaPlayerTelemetryComponent* Telemetry = Player->GetTelemetryComponent())
        {
            Telemetry->ResetTelemetry();
        }
    }

    if (AEvaPlayerController* EvaController = Cast<AEvaPlayerController>(GetWorld()->GetFirstPlayerController()))
    {
        EvaController->CloseMenusForGameplay();
        EvaController->ApplyGameplayInputMode();
        EvaController->SaveAndApplySettings();
    }

    SetGameFlowState(EEvaGameFlowState::Playing);
    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    APawn* PossessedPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    UE_LOG(LogAdaptiveHorror, Warning,
        TEXT("[Player] Context=StartNewGameFlow PossessedPawn=%s PossessedPawnName=%s Controller=%s CharacterMovementEnabled=%s"),
        PossessedPawn ? *PossessedPawn->GetClass()->GetName() : TEXT("None"),
        PossessedPawn ? *PossessedPawn->GetName() : TEXT("None"),
        PlayerController ? *PlayerController->GetClass()->GetName() : TEXT("None"),
        Player && Player->GetCharacterMovement() && Player->GetCharacterMovement()->MovementMode != MOVE_None ?
            TEXT("true") : TEXT("false"));
    StartCombatSpawningAfterNavigationReady();
    ShowDebugStatusMessage(TEXT("NEW GAME - survive the research facility."), 4.0f);
}

void AEvaPrototypeGameMode::PauseGameFlow()
{
    if (!GetWorld() || GameFlowState != EEvaGameFlowState::Playing)
    {
        return;
    }

    SetGameFlowState(EEvaGameFlowState::Paused);
    UGameplayStatics::SetGamePaused(GetWorld(), true);
    if (AEvaPlayerController* EvaController = Cast<AEvaPlayerController>(GetWorld()->GetFirstPlayerController()))
    {
        EvaController->ShowPauseMenu();
    }
}

void AEvaPrototypeGameMode::ResumeGameFlow()
{
    if (!GetWorld() || GameFlowState != EEvaGameFlowState::Paused)
    {
        return;
    }

    UGameplayStatics::SetGamePaused(GetWorld(), false);
    SetGameFlowState(EEvaGameFlowState::Playing);
    if (AEvaPlayerController* EvaController = Cast<AEvaPlayerController>(GetWorld()->GetFirstPlayerController()))
    {
        EvaController->CloseMenusForGameplay();
        EvaController->ApplyGameplayInputMode();
    }
}

void AEvaPrototypeGameMode::RetryFromCheckpointFlow()
{
    if (!GetWorld() || GameFlowState == EEvaGameFlowState::Title)
    {
        return;
    }

    UGameplayStatics::SetGamePaused(GetWorld(), false);
    ClearStageClearTimers();
    CleanupCombatActorsForFlowReset();
    bGameOver = false;
    bStageClear = false;
    bInitialZombieSpawned = false;

    AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(
        GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr);
    if (Player)
    {
        PlayerAwaitingRespawn = Player;
    }
    if (PlayerAwaitingRespawn)
    {
        RespawnPlayer();
    }
    else
    {
        SetGameFlowState(EEvaGameFlowState::Playing);
    }
    if (AEvaPlayerController* EvaController = Cast<AEvaPlayerController>(GetWorld()->GetFirstPlayerController()))
    {
        EvaController->CloseMenusForGameplay();
        EvaController->ApplyGameplayInputMode();
    }
    StartCombatSpawningAfterNavigationReady();
}

void AEvaPrototypeGameMode::ReturnToTitleFlow()
{
    EnterTitleMode();
}

void AEvaPrototypeGameMode::NotifyEnemyKilled(AEvaZombieCharacter* DeadEnemy)
{
    if (bStageClear)
    {
        UE_LOG(LogAdaptiveHorror, Log, TEXT("[StageClear] NotifyEnemyKilled skipped after clear Enemy=%s"),
            DeadEnemy ? *DeadEnemy->GetName() : TEXT("None"));
        return;
    }

    if (!DeadEnemy || DeadEnemy->ActorHasTag(TEXT("Hunter")) || DeadEnemy->ActorHasTag(TEXT("Boss")) ||
        DeadEnemy->ActorHasTag(TEXT("Adam")))
    {
        return;
    }

    ++TotalZombieKills;
    ShowDebugStatusMessage(FString::Printf(TEXT("Infected neutralized. Kills: %d"), TotalZombieKills), 2.0f);
    if (!IsValid(CurrentHunter) && !GetWorldTimerManager().IsTimerActive(HunterReinsertTimer) &&
        TotalZombieKills >= HunterSpawnKillThreshold)
    {
        SpawnHunter();
    }
}

void AEvaPrototypeGameMode::NotifyHunterDefeated(AEvaHunterCharacter* DefeatedHunter)
{
    if (!GetWorld() || bStageClear)
    {
        return;
    }

    ++HunterDefeatCount;
    if (DefeatedHunter && CurrentHunter == DefeatedHunter)
    {
        CurrentHunter = nullptr;
    }

    HunterTierToSpawn = FMath::Max(HunterTierToSpawn + 1, 2);
    GetWorldTimerManager().ClearTimer(HunterTimeSpawnTimer);
    GetWorldTimerManager().SetTimer(HunterReinsertTimer, this, &AEvaPrototypeGameMode::SpawnHunter,
        HunterReinsertDelay, false);
    ShowDebugStatusMessage(FString::Printf(TEXT("HUNTER defeated. EVA learning speed reduced to x0.3. Next tier: %d"),
        HunterTierToSpawn), 5.0f);
}

void AEvaPrototypeGameMode::HandleStageClear()
{
    if (bStageClear)
    {
        LogStageClearState(TEXT("AlreadyCleared"), 0, false);
        return;
    }

    if (bGameOver)
    {
        LogStageClearState(TEXT("RejectedBecausePlayerDeathAlreadyActive"), 0, false);
        return;
    }

    SetGameFlowState(EEvaGameFlowState::StageCleared);
    bStageClear = true;
    bGameOver = false;
    PlayerAwaitingRespawn = nullptr;
    ClearStageClearTimers();
    const int32 ClearedEnemyAI = StopAllEnemyCombatForStageClear();
    if (APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
    {
        PlayerController->ResetIgnoreMoveInput();
        PlayerController->ResetIgnoreLookInput();
        PlayerController->SetIgnoreMoveInput(true);
    }
    LogStageClearState(TEXT("Begin"), ClearedEnemyAI, true);
    ShowDebugStatusMessage(TEXT("STAGE CLEAR - ADAM defeated."), 8.0f);
    if (AEvaPlayerController* EvaController = Cast<AEvaPlayerController>(
        GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr))
    {
        EvaController->ShowStageClearMenu();
    }
}

void AEvaPrototypeGameMode::RespawnPlayer()
{
    if (bStageClear)
    {
        GetWorldTimerManager().ClearTimer(RespawnTimer);
        PlayerAwaitingRespawn = nullptr;
        bGameOver = false;
        LogPlayerDeathRequest(TEXT("RespawnRejectedBecauseStageClear"), nullptr, false);
        return;
    }

    if (!GetWorld() || !PlayerAwaitingRespawn)
    {
        bGameOver = false;
        return;
    }

    ResetEnemyTargets();
    PlayerAwaitingRespawn->ResetForCheckpoint(LastCheckpointTransform);
    if (APlayerController* PlayerController = Cast<APlayerController>(PlayerAwaitingRespawn->GetController()))
    {
        PlayerController->SetIgnoreMoveInput(false);
        PlayerController->SetIgnoreLookInput(false);
        PlayerController->SetControlRotation(LastCheckpointTransform.Rotator());
    }
    bGameOver = false;
    SetGameFlowState(EEvaGameFlowState::Playing);
    PlayerAwaitingRespawn = nullptr;
    ShowDebugStatusMessage(TEXT("Checkpoint restored. Enemy aggro reset."), 3.0f);
}

void AEvaPrototypeGameMode::EnsurePrototypePlayer()
{
    APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
    if (!PlayerController)
    {
        return;
    }

    AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(PlayerController->GetPawn());
    if (!Player)
    {
        if (PlayerController->GetPawn())
        {
            PlayerController->GetPawn()->Destroy();
        }
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.Owner = PlayerController;
        SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
        Player = GetWorld()->SpawnActor<AEvaPlayerCharacter>(DefaultPawnClass, LastCheckpointTransform.GetLocation(),
            LastCheckpointTransform.Rotator(), SpawnParameters);
        if (Player)
        {
            PlayerController->Possess(Player);
        }
        else
        {
            ShowDebugStatusMessage(TEXT("ERROR: Failed to spawn prototype player."), 6.0f);
        }
    }
    else
    {
        Player->SetActorTransform(LastCheckpointTransform, false, nullptr, ETeleportType::TeleportPhysics);
    }

    LogNavigationStatus(TEXT("AfterEnsurePrototypePlayer"));
}

void AEvaPrototypeGameMode::BuildPrototypeArena()
{
    if (!GetWorld())
    {
        return;
    }

    static const FVector ZoneCenters[] =
    {
        FVector(-4800.0f, 0.0f, 0.0f),
        FVector(-3000.0f, 0.0f, 0.0f),
        FVector(-1200.0f, 0.0f, 0.0f),
        FVector(600.0f, 0.0f, 0.0f),
        FVector(2400.0f, 0.0f, 0.0f),
        FVector(4200.0f, 0.0f, 0.0f)
    };

    RuntimeFloorComponents.Reset();
    RuntimeFacilityBounds = FBox(EForceInit::ForceInit);

    BuildFacilityZone(ZoneCenters[0], TEXT("Entry Lobby"), 0);
    BuildFacilityZone(ZoneCenters[1], TEXT("Security Corridor"), 1);
    BuildFacilityZone(ZoneCenters[2], TEXT("Observation Lab"), 2);
    BuildFacilityZone(ZoneCenters[3], TEXT("Containment Ward"), 3);
    BuildFacilityZone(ZoneCenters[4], TEXT("Data Core Room"), 4);
    BuildFacilityZone(ZoneCenters[5], TEXT("Adam Arena"), 5);

    for (int32 GateIndex = 0; GateIndex < 5; ++GateIndex)
    {
        const float GateX = -3900.0f + GateIndex * 1800.0f;
        SpawnTaggedArenaBox(FVector(GateX, 620.0f, 160.0f), FVector(0.35f, 1.4f, 3.2f), FName(TEXT("EvaGate")));
        SpawnTaggedArenaBox(FVector(GateX, -620.0f, 160.0f), FVector(0.35f, 1.4f, 3.2f), FName(TEXT("EvaGate")));
        SpawnTaggedArenaBox(FVector(GateX, 0.0f, 360.0f), FVector(0.35f, 3.4f, 0.35f), FName(TEXT("EvaGate")));
    }

    CurrentDirector = GetWorld()->SpawnActor<AEvaResearchFacilityDirector>(AEvaResearchFacilityDirector::StaticClass(),
        FVector::ZeroVector, FRotator::ZeroRotator);

    SpawnFacilityTrigger(CurrentDirector, EEvaFacilityZone::EntryLobby, FVector(-5200.0f, 0.0f, 120.0f));
    SpawnFacilityTrigger(CurrentDirector, EEvaFacilityZone::SecurityCorridor, FVector(-3150.0f, 0.0f, 120.0f));
    SpawnFacilityTrigger(CurrentDirector, EEvaFacilityZone::ObservationLab, FVector(-1350.0f, 0.0f, 120.0f));
    SpawnFacilityTrigger(CurrentDirector, EEvaFacilityZone::ContainmentWard, FVector(450.0f, 0.0f, 120.0f));
    SpawnFacilityTrigger(CurrentDirector, EEvaFacilityZone::DataCoreRoom, FVector(2250.0f, 0.0f, 120.0f));
    SpawnFacilityTrigger(CurrentDirector, EEvaFacilityZone::AdamArena, FVector(4050.0f, 0.0f, 120.0f));

    GetWorld()->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), LastCheckpointTransform.GetLocation(),
        LastCheckpointTransform.Rotator());
    GetWorld()->SpawnActor<AEvaCheckpoint>(AEvaCheckpoint::StaticClass(), FVector(-5250.0f, 0.0f, 100.0f), FRotator::ZeroRotator);
    GetWorld()->SpawnActor<AEvaCheckpoint>(AEvaCheckpoint::StaticClass(), FVector(-3150.0f, -420.0f, 100.0f), FRotator::ZeroRotator);
    GetWorld()->SpawnActor<AEvaCheckpoint>(AEvaCheckpoint::StaticClass(), FVector(-1350.0f, -420.0f, 100.0f), FRotator::ZeroRotator);
    GetWorld()->SpawnActor<AEvaCheckpoint>(AEvaCheckpoint::StaticClass(), FVector(450.0f, -420.0f, 100.0f), FRotator::ZeroRotator);
    GetWorld()->SpawnActor<AEvaCheckpoint>(AEvaCheckpoint::StaticClass(), FVector(2250.0f, -420.0f, 100.0f), FRotator::ZeroRotator);
    GetWorld()->SpawnActor<AEvaCheckpoint>(AEvaCheckpoint::StaticClass(), FVector(4050.0f, -420.0f, 100.0f), FRotator::ZeroRotator);

    GetWorld()->SpawnActor<AEvaAmmoPickup>(AEvaAmmoPickup::StaticClass(), FVector(-3350.0f, 520.0f, 90.0f), FRotator::ZeroRotator);
    GetWorld()->SpawnActor<AEvaAmmoPickup>(AEvaAmmoPickup::StaticClass(), FVector(2450.0f, 520.0f, 90.0f), FRotator::ZeroRotator);
    GetWorld()->SpawnActor<AEvaHealthPickup>(AEvaHealthPickup::StaticClass(), FVector(3800.0f, -520.0f, 90.0f), FRotator::ZeroRotator);

    SpawnStoryLog(CurrentDirector, FName(TEXT("EVA_LOG_01")), TEXT("EVA Happiness Maximization Protocol"),
        TEXT("EVA was designed to maximize human happiness. The facility stopped asking whose happiness counted."),
        FVector(-1450.0f, 450.0f, 90.0f));
    SpawnStoryLog(CurrentDirector, FName(TEXT("EVA_LOG_02")), TEXT("Humanity Maximum Risk Judgment"),
        TEXT("EVA identified humanity itself as the dominant risk factor in all extinction simulations."),
        FVector(-1050.0f, -450.0f, 90.0f));
    SpawnStoryLog(CurrentDirector, FName(TEXT("EVA_LOG_03")), TEXT("Biological Adaptation Experiment Record"),
        TEXT("Infected tissue responds faster when exposed to repeated player combat patterns."),
        FVector(520.0f, 450.0f, 90.0f));
    SpawnStoryLog(CurrentDirector, FName(TEXT("EVA_LOG_04")), TEXT("HUNTER Observation Unit Brief"),
        TEXT("HUNTER units are not assassins. They are cameras with claws."),
        FVector(2380.0f, 450.0f, 90.0f));
    SpawnStoryLog(CurrentDirector, FName(TEXT("EVA_LOG_05")), TEXT("ADAM Activation Record"),
        TEXT("ADAM is the final adaptive vessel. Do not allow EVA to complete the loop."),
        FVector(3900.0f, 450.0f, 90.0f));

    if (ADirectionalLight* DirectionalLight = GetWorld()->SpawnActor<ADirectionalLight>(
        FVector(-1600.0f, -2600.0f, 1800.0f), FRotator(-45.0f, 35.0f, 0.0f)))
    {
        if (UDirectionalLightComponent* LightComponent =
            Cast<UDirectionalLightComponent>(DirectionalLight->GetLightComponent()))
        {
            LightComponent->SetMobility(EComponentMobility::Movable);
            LightComponent->SetIntensity(4.5f);
        }
    }

    if (ASkyLight* SkyLight = GetWorld()->SpawnActor<ASkyLight>(FVector(0.0f, 0.0f, 1600.0f), FRotator::ZeroRotator))
    {
        if (USkyLightComponent* SkyLightComponent = SkyLight->GetLightComponent())
        {
            SkyLightComponent->SetMobility(EComponentMobility::Movable);
            SkyLightComponent->SetIntensity(1.4f);
            SkyLightComponent->RecaptureSky();
        }
    }

    if (APointLight* Light = GetWorld()->SpawnActor<APointLight>(FVector(0.0f, 0.0f, 900.0f), FRotator::ZeroRotator))
    {
        if (UPointLightComponent* PointLightComponent = Cast<UPointLightComponent>(Light->GetLightComponent()))
        {
            PointLightComponent->SetMobility(EComponentMobility::Movable);
            PointLightComponent->SetIntensity(25000.0f);
            PointLightComponent->SetAttenuationRadius(11000.0f);
        }
    }

    LogNavigationStatus(TEXT("AfterRuntimeArenaGeometry"));
}

void AEvaPrototypeGameMode::RegisterRuntimeFloorComponent(UPrimitiveComponent* FloorComponent)
{
    if (!FloorComponent)
    {
        return;
    }

    FloorComponent->SetCanEverAffectNavigation(true);
    RuntimeFloorComponents.AddUnique(FloorComponent);
    RuntimeFacilityBounds += FloorComponent->Bounds.GetBox();
    UE_LOG(LogAdaptiveHorror, Log,
        TEXT("[Navigation] RuntimeFloor Registered Component=%s Bounds=%s CanEverAffectNavigation=%s Tags=%s"),
        *FloorComponent->GetName(),
        *FloorComponent->Bounds.GetBox().ToString(),
        *BoolText(FloorComponent->CanEverAffectNavigation()),
        *FString::JoinBy(FloorComponent->ComponentTags, TEXT(","), [](const FName& Tag) { return Tag.ToString(); }));
}

FBox AEvaPrototypeGameMode::CalculateRuntimeFacilityBounds() const
{
    FBox CalculatedBounds(EForceInit::ForceInit);
    for (const TObjectPtr<UPrimitiveComponent>& FloorComponent : RuntimeFloorComponents)
    {
        if (IsValid(FloorComponent))
        {
            CalculatedBounds += FloorComponent->Bounds.GetBox();
        }
    }
    return CalculatedBounds.IsValid ? CalculatedBounds : RuntimeFacilityBounds;
}

bool AEvaPrototypeGameMode::ValidateNavigationProjection(const FString& Context, const FVector& Location,
    FVector* OutProjectedLocation) const
{
    UWorld* World = GetWorld();
    UNavigationSystemV1* NavigationSystem = World ? UNavigationSystemV1::GetCurrent(World) : nullptr;
    if (!NavigationSystem)
    {
        UE_LOG(LogAdaptiveHorror, Warning, TEXT("[Navigation] Projection Context=%s Location=%s Result=false Reason=NoNavigationSystem"),
            *Context, *Location.ToCompactString());
        return false;
    }

    FNavLocation ProjectedLocation;
    const bool bProjected = NavigationSystem->ProjectPointToNavigation(Location, ProjectedLocation,
        FVector(360.0f, 360.0f, 600.0f));
    if (OutProjectedLocation && bProjected)
    {
        *OutProjectedLocation = ProjectedLocation.Location;
    }

    UE_LOG(LogAdaptiveHorror, Log,
        TEXT("[Navigation] Projection Context=%s Location=%s Projected=%s ProjectedLocation=%s"),
        *Context,
        *Location.ToCompactString(),
        *BoolText(bProjected),
        bProjected ? *ProjectedLocation.Location.ToCompactString() : TEXT("None"));
    return bProjected;
}

void AEvaPrototypeGameMode::LogRuntimeClassBindings() const
{
    const UWorld* World = GetWorld();
    const APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
    const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    const AHUD* ActiveHUD = PlayerController ? PlayerController->GetHUD() : nullptr;
    const AEvaZombieCharacter* ZombieCDO = AEvaZombieCharacter::StaticClass()->GetDefaultObject<AEvaZombieCharacter>();
    const AEvaHunterCharacter* HunterCDO = AEvaHunterCharacter::StaticClass()->GetDefaultObject<AEvaHunterCharacter>();
    const AEvaAdamBossCharacter* AdamCDO =
        AEvaAdamBossCharacter::StaticClass()->GetDefaultObject<AEvaAdamBossCharacter>();

    UE_LOG(LogAdaptiveHorror, Log,
        TEXT("[RuntimeClass] GameMode=%s PlayerControllerClass=%s ActivePlayerController=%s DefaultPawnClass=%s ActivePawn=%s HUDClass=%s ActiveHUD=%s ZombieClass=%s ZombieAIControllerClass=%s HunterClass=%s HunterAIControllerClass=%s AdamClass=%s AdamAIControllerClass=%s"),
        *GetClass()->GetName(),
        PlayerControllerClass ? *PlayerControllerClass->GetName() : TEXT("None"),
        PlayerController ? *PlayerController->GetClass()->GetName() : TEXT("None"),
        DefaultPawnClass ? *DefaultPawnClass->GetName() : TEXT("None"),
        PlayerPawn ? *PlayerPawn->GetClass()->GetName() : TEXT("None"),
        HUDClass ? *HUDClass->GetName() : TEXT("None"),
        ActiveHUD ? *ActiveHUD->GetClass()->GetName() : TEXT("None"),
        *AEvaZombieCharacter::StaticClass()->GetName(),
        ZombieCDO && ZombieCDO->AIControllerClass ? *ZombieCDO->AIControllerClass->GetName() : TEXT("None"),
        *AEvaHunterCharacter::StaticClass()->GetName(),
        HunterCDO && HunterCDO->AIControllerClass ? *HunterCDO->AIControllerClass->GetName() : TEXT("None"),
        *AEvaAdamBossCharacter::StaticClass()->GetName(),
        AdamCDO && AdamCDO->AIControllerClass ? *AdamCDO->AIControllerClass->GetName() : TEXT("None"));
}

void AEvaPrototypeGameMode::StartCombatSpawningAfterNavigationReady()
{
    if (!GetWorld() || !bRuntimeNavigationReady || bRuntimeNavigationFailed ||
        GameFlowState != EEvaGameFlowState::Playing)
    {
        return;
    }

    SpawnInitialZombie();
    if (!GetWorldTimerManager().IsTimerActive(AdaptiveSpawnTimer))
    {
        GetWorldTimerManager().SetTimer(AdaptiveSpawnTimer, this, &AEvaPrototypeGameMode::SpawnAdaptiveEnemy,
            AdaptiveSpawnInterval, true, 14.0f);
    }
    if (!GetWorldTimerManager().IsTimerActive(HunterTimeSpawnTimer))
    {
        GetWorldTimerManager().SetTimer(HunterTimeSpawnTimer, this, &AEvaPrototypeGameMode::SpawnHunter,
            HunterTimeSpawnDelay, false);
    }
}

void AEvaPrototypeGameMode::BuildRuntimeNavigation()
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return;
    }

    UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(World);
    if (!NavigationSystem)
    {
        UE_LOG(LogAdaptiveHorror, Warning, TEXT("[Navigation] BuildRuntimeNavigation failed: NavigationSystem missing"));
        ShowDebugStatusMessage(TEXT("NAV ERROR: NavigationSystem missing. Enemy spawning halted."), 8.0f);
        bRuntimeNavigationFailed = true;
        return;
    }

    bRuntimeNavigationReady = false;
    bRuntimeNavigationFailed = false;
    NavigationReadinessAttempts = 0;
    GetWorldTimerManager().ClearTimer(NavigationReadinessTimer);

    const double BuildStartTime = FPlatformTime::Seconds();
    WriteBoolConfigProperty(NavigationSystem, TEXT("bAutoCreateNavigationData"), true);
    WriteBoolConfigProperty(NavigationSystem, TEXT("bAllowClientSideNavigation"), true);
    WriteBoolConfigProperty(NavigationSystem, TEXT("bGenerateNavigationOnlyAroundNavigationInvokers"), false);
    WriteBoolConfigProperty(NavigationSystem, TEXT("bInitialBuildingLocked"), false);
    NavigationSystem->bWholeWorldNavigable = false;

    FBox FacilityBounds = CalculateRuntimeFacilityBounds();
    if (!FacilityBounds.IsValid)
    {
        FacilityBounds = FBox::BuildAABB(FVector(-300.0f, 0.0f, ExpectedRuntimeFloorSurfaceZ),
            FVector(6100.0f, 1000.0f, 80.0f));
    }

    const FVector BoundsCenter = FacilityBounds.GetCenter() + FVector(0.0f, 0.0f, 360.0f);
    const FVector FacilityExtent = FacilityBounds.GetExtent();
    const FVector BoundsExtent(
        FMath::Max(FacilityExtent.X + 900.0f, 6800.0f),
        FMath::Max(FacilityExtent.Y + 900.0f, 1450.0f),
        1100.0f);
    const FBox RuntimeNavBounds = FBox::BuildAABB(BoundsCenter, BoundsExtent);

    if (!RuntimeNavBoundsVolume)
    {
        RuntimeNavBoundsVolume = World->SpawnActor<ANavMeshBoundsVolume>(ANavMeshBoundsVolume::StaticClass(),
            BoundsCenter, FRotator::ZeroRotator);
    }
    if (RuntimeNavBoundsVolume)
    {
        RuntimeNavBoundsVolume->SetActorScale3D(FVector::OneVector);
        RuntimeNavBoundsVolume->SetActorLocation(BoundsCenter);
        RuntimeNavBoundsVolume->Tags.AddUnique(TEXT("EvaRuntimeNavBounds"));
        RuntimeNavBoundsVolume->bAllowPhysicsOverlap = false;
        RuntimeNavBoundsVolume->SupportedAgents.MarkInitialized();

        UBoxComponent* BoundsComponent = FindObject<UBoxComponent>(RuntimeNavBoundsVolume,
            TEXT("EvaRuntimeNavBoundsBox"));
        if (!BoundsComponent)
        {
            BoundsComponent = NewObject<UBoxComponent>(RuntimeNavBoundsVolume, TEXT("EvaRuntimeNavBoundsBox"));
            BoundsComponent->SetupAttachment(RuntimeNavBoundsVolume->GetRootComponent());
            RuntimeNavBoundsVolume->AddInstanceComponent(BoundsComponent);
        }
        if (BoundsComponent)
        {
            BoundsComponent->SetRelativeLocation(FVector::ZeroVector);
            BoundsComponent->SetRelativeRotation(FRotator::ZeroRotator);
            BoundsComponent->SetRelativeScale3D(FVector::OneVector);
            BoundsComponent->SetBoxExtent(BoundsExtent, false);
            BoundsComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            BoundsComponent->SetCanEverAffectNavigation(false);
            BoundsComponent->SetHiddenInGame(true);
            if (!BoundsComponent->IsRegistered())
            {
                BoundsComponent->RegisterComponent();
            }
            BoundsComponent->UpdateBounds();
        }

        if (UBrushComponent* BrushComponent = RuntimeNavBoundsVolume->GetBrushComponent())
        {
            BrushComponent->SetMobility(EComponentMobility::Static);
            BrushComponent->SetCanEverAffectNavigation(false);
            BrushComponent->SetWorldScale3D(FVector::OneVector);
            BrushComponent->UpdateBounds();
        }
        RuntimeNavBoundsVolume->RegisterAllComponents();
        RuntimeNavBoundsVolume->ReregisterAllComponents();
        RuntimeNavBoundsVolume->SetActorLocation(BoundsCenter);
        RuntimeNavBoundsVolume->SetActorScale3D(FVector::OneVector);
        RuntimeNavBoundsVolume->UpdateComponentTransforms();
        RuntimeNavBoundsVolume->UpdateOverlaps(false);
        NavigationSystem->OnNavigationBoundsUpdated(RuntimeNavBoundsVolume);
    }

    int32 UpdatedFloorComponents = 0;
    int32 UpdatedFloorActors = 0;
    for (const TObjectPtr<UPrimitiveComponent>& FloorComponent : RuntimeFloorComponents)
    {
        UPrimitiveComponent* FloorPrimitive = FloorComponent.Get();
        if (!FloorPrimitive || !FloorPrimitive->IsRegistered())
        {
            continue;
        }

        FloorPrimitive->SetCollisionProfileName(TEXT("BlockAll"));
        FloorPrimitive->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        FloorPrimitive->SetCanEverAffectNavigation(true);
        FloorPrimitive->UpdateBounds();
        UNavigationSystemV1::UpdateComponentInNavOctree(*FloorPrimitive);
        ++UpdatedFloorComponents;

        if (AActor* FloorOwner = FloorPrimitive->GetOwner())
        {
            FloorOwner->RegisterAllComponents();
            FloorOwner->UpdateComponentTransforms();
            UNavigationSystemV1::UpdateActorAndComponentsInNavOctree(*FloorOwner, false);
            ++UpdatedFloorActors;
        }

        const FBox FloorBounds = FloorPrimitive->Bounds.GetBox();
        if (FloorBounds.IsValid)
        {
            NavigationSystem->AddDirtyArea(FloorBounds.ExpandBy(120.0f),
                ENavigationDirtyFlag::All, FName(TEXT("EvaRuntimeFloor")));
        }
    }

    // OnNavigationBoundsUpdated and UpdateComponentInNavOctree queue work. Process it before forcing
    // a runtime build, otherwise Recast can exist with valid volume bounds but no registered floor data.
    NavigationSystem->Tick(0.0f);

    NavigationSystem->BuildBounds = RuntimeNavBounds;
    NavigationSystem->AddDirtyArea(RuntimeNavBounds, ENavigationDirtyFlag::All, FName(TEXT("EvaRuntimeGraybox")));
    ANavigationData* MainNavDataBeforeBuild = NavigationSystem->GetDefaultNavDataInstance(FNavigationSystem::Create);
    NavigationSystem->Build();

    NavigationSystem->BuildBounds = RuntimeNavBounds;
    NavigationSystem->AddDirtyArea(RuntimeNavBounds, ENavigationDirtyFlag::All, FName(TEXT("EvaRuntimeGrayboxPostBuild")));
    ANavigationData* MainNavData = NavigationSystem->GetDefaultNavDataInstance(FNavigationSystem::Create);
    if (MainNavData)
    {
        MainNavData->EnsureBuildCompletion();
    }

    const double BuildEndTime = FPlatformTime::Seconds();
    const FBox VolumeBounds = RuntimeNavBoundsVolume ? RuntimeNavBoundsVolume->GetComponentsBoundingBox(true) :
        FBox(EForceInit::ForceInit);
    UE_LOG(LogAdaptiveHorror, Log,
        TEXT("[Navigation] Build requested BoundsCenter=%s BoundsExtent=%s BuildBounds=%s BoundsVolume=%s VolumeBoundsValid=%s VolumeBounds=%s UpdatedFloorComponents=%d UpdatedFloorActors=%d MainNavBefore=%s RuntimeGeneration=%s AutoCreate=%s AllowClientSide=%s InvokerOnly=%s DurationMs=%.2f"),
        *BoundsCenter.ToCompactString(),
        *BoundsExtent.ToCompactString(),
        *RuntimeNavBounds.ToString(),
        *BoolText(RuntimeNavBoundsVolume != nullptr),
        *BoolText(VolumeBounds.IsValid != 0),
        VolumeBounds.IsValid ? *VolumeBounds.ToString() : TEXT("Invalid"),
        UpdatedFloorComponents,
        UpdatedFloorActors,
        MainNavDataBeforeBuild ? *MainNavDataBeforeBuild->GetName() : TEXT("None"),
        MainNavData ? *RuntimeGenerationToText(MainNavData->GetRuntimeGenerationMode()) : TEXT("None"),
        *ReadBoolConfigText(NavigationSystem, TEXT("bAutoCreateNavigationData")),
        *ReadBoolConfigText(NavigationSystem, TEXT("bAllowClientSideNavigation")),
        *ReadBoolConfigText(NavigationSystem, TEXT("bGenerateNavigationOnlyAroundNavigationInvokers")),
        (BuildEndTime - BuildStartTime) * 1000.0);

    LogNavigationStatus(TEXT("AfterRuntimeNavigationBuild"));
    GetWorldTimerManager().SetTimer(NavigationReadinessTimer, this, &AEvaPrototypeGameMode::CheckRuntimeNavigationReady,
        NavigationReadinessInterval, true, NavigationReadinessInterval);
    CheckRuntimeNavigationReady();
}

void AEvaPrototypeGameMode::CheckRuntimeNavigationReady()
{
    UWorld* World = GetWorld();
    UNavigationSystemV1* NavigationSystem = World ? UNavigationSystemV1::GetCurrent(World) : nullptr;
    ++NavigationReadinessAttempts;

    TArray<AActor*> NavDataActors;
    TArray<AActor*> RecastActors;
    if (World)
    {
        UGameplayStatics::GetAllActorsOfClass(World, ANavigationData::StaticClass(), NavDataActors);
        UGameplayStatics::GetAllActorsOfClass(World, ARecastNavMesh::StaticClass(), RecastActors);
    }

    ANavigationData* MainNavData = NavigationSystem ?
        NavigationSystem->GetDefaultNavDataInstance(FNavigationSystem::DontCreate) :
        nullptr;
    ARecastNavMesh* RecastNavMesh = Cast<ARecastNavMesh>(MainNavData);
    if (!RecastNavMesh && RecastActors.Num() > 0)
    {
        RecastNavMesh = Cast<ARecastNavMesh>(RecastActors[0]);
    }

    const bool bBuildInProgress = NavigationSystem && NavigationSystem->IsNavigationBuildInProgress();
    FVector PlayerProjectedLocation = FVector::ZeroVector;
    FVector RepresentativeProjectedLocation = FVector::ZeroVector;
    const bool bPlayerProjected = ValidateNavigationProjection(TEXT("ReadinessPlayer"),
        LastCheckpointTransform.GetLocation(), &PlayerProjectedLocation);
    const bool bRepresentativeFloorProjected = ValidateNavigationProjection(TEXT("ReadinessRepresentativeFloor"),
        FVector(-4800.0f, 0.0f, ExpectedRuntimeFloorSurfaceZ + 112.0f),
        &RepresentativeProjectedLocation);
    const bool bReady = NavigationSystem && NavDataActors.Num() > 0 && RecastNavMesh &&
        !bBuildInProgress && bPlayerProjected && bRepresentativeFloorProjected;

    UE_LOG(LogAdaptiveHorror, Log,
        TEXT("[Navigation] Readiness Attempt=%d Ready=%s NavSystem=%s NavDataCount=%d RecastCount=%d MainNav=%s Recast=%s BuildInProgress=%s PlayerProjected=%s RepresentativeProjected=%s PlayerProjectedLocation=%s RepresentativeProjectedLocation=%s"),
        NavigationReadinessAttempts,
        *BoolText(bReady),
        *BoolText(NavigationSystem != nullptr),
        NavDataActors.Num(),
        RecastActors.Num(),
        MainNavData ? *MainNavData->GetName() : TEXT("None"),
        RecastNavMesh ? *RecastNavMesh->GetName() : TEXT("None"),
        *BoolText(bBuildInProgress),
        *BoolText(bPlayerProjected),
        *BoolText(bRepresentativeFloorProjected),
        *PlayerProjectedLocation.ToCompactString(),
        *RepresentativeProjectedLocation.ToCompactString());

    if (bReady)
    {
        bRuntimeNavigationReady = true;
        bRuntimeNavigationFailed = false;
        GetWorldTimerManager().ClearTimer(NavigationReadinessTimer);
        ShowDebugStatusMessage(TEXT("NAV READY: representative floor projection succeeded."), 6.0f);
        LogNavigationStatus(TEXT("NavigationReady"));
        StartCombatSpawningAfterNavigationReady();
        return;
    }

    if (NavigationReadinessAttempts >= MaxNavigationReadinessAttempts)
    {
        bRuntimeNavigationReady = false;
        bRuntimeNavigationFailed = true;
        GetWorldTimerManager().ClearTimer(NavigationReadinessTimer);
        UE_LOG(LogAdaptiveHorror, Error,
            TEXT("[Navigation] Initialization failed. Enemy spawning halted. NavSystem=%s NavDataCount=%d RecastCount=%d PlayerProjected=%s RepresentativeProjected=%s"),
            *BoolText(NavigationSystem != nullptr),
            NavDataActors.Num(),
            RecastActors.Num(),
            *BoolText(bPlayerProjected),
            *BoolText(bRepresentativeFloorProjected));
        ShowDebugStatusMessage(TEXT("NAV ERROR: Runtime NavMesh failed. Enemies will not spawn."), 10.0f);
    }
}

void AEvaPrototypeGameMode::LogNavigationStatus(const FString& Context) const
{
    UWorld* World = GetWorld();
    UNavigationSystemV1* NavigationSystem = World ? UNavigationSystemV1::GetCurrent(World) : nullptr;
    TArray<AActor*> NavDataActors;
    TArray<AActor*> RecastActors;
    if (World)
    {
        UGameplayStatics::GetAllActorsOfClass(World, ANavigationData::StaticClass(), NavDataActors);
        UGameplayStatics::GetAllActorsOfClass(World, ARecastNavMesh::StaticClass(), RecastActors);
    }

    const ANavigationData* MainNavData = NavigationSystem ?
        NavigationSystem->GetDefaultNavDataInstance(FNavigationSystem::DontCreate) :
        nullptr;
    const ARecastNavMesh* RecastNavMesh = Cast<ARecastNavMesh>(MainNavData);
    const FBox VolumeBounds = RuntimeNavBoundsVolume ? RuntimeNavBoundsVolume->GetComponentsBoundingBox(true) :
        FBox(EForceInit::ForceInit);
    UE_LOG(LogAdaptiveHorror, Log,
        TEXT("[Navigation] Context=%s NavSystem=%s MainNavData=%s MainNavClass=%s Recast=%s RuntimeGeneration=%s SupportsRuntime=%s NavDataActors=%d RecastActors=%d RecastName=%s BoundsVolume=%s VolumeBoundsValid=%s VolumeLocation=%s VolumeExtent=%s AutoCreate=%s AllowClientSide=%s InvokerOnly=%s WholeWorld=%s Ready=%s Failed=%s"),
        *Context,
        *BoolText(NavigationSystem != nullptr),
        *BoolText(MainNavData != nullptr),
        MainNavData ? *MainNavData->GetClass()->GetName() : TEXT("None"),
        *BoolText(RecastNavMesh != nullptr),
        MainNavData ? *RuntimeGenerationToText(MainNavData->GetRuntimeGenerationMode()) : TEXT("None"),
        MainNavData ? *BoolText(MainNavData->SupportsRuntimeGeneration()) : TEXT("false"),
        NavDataActors.Num(),
        RecastActors.Num(),
        RecastNavMesh ? *RecastNavMesh->GetName() : (RecastActors.Num() > 0 ? *RecastActors[0]->GetName() : TEXT("None")),
        *BoolText(RuntimeNavBoundsVolume != nullptr),
        *BoolText(VolumeBounds.IsValid != 0),
        VolumeBounds.IsValid ? *VolumeBounds.GetCenter().ToCompactString() : TEXT("None"),
        VolumeBounds.IsValid ? *VolumeBounds.GetExtent().ToCompactString() : TEXT("None"),
        *ReadBoolConfigText(NavigationSystem, TEXT("bAutoCreateNavigationData")),
        *ReadBoolConfigText(NavigationSystem, TEXT("bAllowClientSideNavigation")),
        *ReadBoolConfigText(NavigationSystem, TEXT("bGenerateNavigationOnlyAroundNavigationInvokers")),
        NavigationSystem ? *BoolText(NavigationSystem->bWholeWorldNavigable) : TEXT("false"),
        *BoolText(bRuntimeNavigationReady),
        *BoolText(bRuntimeNavigationFailed));

    APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
    const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    if (NavigationSystem && PlayerPawn)
    {
        FNavLocation ProjectedPlayerLocation;
        const bool bPlayerProjected = NavigationSystem->ProjectPointToNavigation(PlayerPawn->GetActorLocation(),
            ProjectedPlayerLocation, FVector(240.0f, 240.0f, 420.0f));
        UE_LOG(LogAdaptiveHorror, Log, TEXT("[Navigation] Context=%s Player=%s Projected=%s ProjectedLocation=%s"),
            *Context,
            *PlayerPawn->GetActorLocation().ToCompactString(),
            *BoolText(bPlayerProjected),
            bPlayerProjected ? *ProjectedPlayerLocation.Location.ToCompactString() : TEXT("None"));
    }
}

void AEvaPrototypeGameMode::BuildFacilityZone(const FVector& Center, const FString& Label, const int32 ZoneIndex)
{
    AActor* Floor = SpawnArenaBox(FVector(Center.X, Center.Y, 0.0f), FVector(18.0f, 14.0f, 0.5f));
    if (Floor)
    {
        Floor->Tags.Add(FName(*FString::Printf(TEXT("EvaZone_%d"), ZoneIndex)));
        if (UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(Floor->GetComponentByClass(
            UStaticMeshComponent::StaticClass())))
        {
            MeshComponent->ComponentTags.AddUnique(RuntimeFloorTag);
            MeshComponent->ComponentTags.AddUnique(EvaNavigableFloorTag);
            RegisterRuntimeFloorComponent(MeshComponent);
        }
    }

    SpawnArenaBox(FVector(Center.X, 760.0f, 180.0f), FVector(18.0f, 0.35f, 3.6f));
    SpawnArenaBox(FVector(Center.X, -760.0f, 180.0f), FVector(18.0f, 0.35f, 3.6f));

    SpawnTaggedArenaBox(FVector(Center.X - 360.0f, 420.0f, 95.0f), FVector(2.0f, 1.0f, 1.9f), FName(TEXT("EvaCover")),
        FRotator(0.0f, 20.0f + ZoneIndex * 7.0f, 0.0f));
    SpawnTaggedArenaBox(FVector(Center.X + 320.0f, -430.0f, 80.0f), FVector(1.4f, 2.0f, 1.6f), FName(TEXT("EvaCover")),
        FRotator(0.0f, -25.0f, 0.0f));
    SpawnTaggedArenaBox(FVector(Center.X - 520.0f, -560.0f, 55.0f), FVector(0.5f, 0.5f, 0.5f), FName(TEXT("EvaEscapeRoute")));
    SpawnTaggedArenaBox(FVector(Center.X + 520.0f, 560.0f, 55.0f), FVector(0.5f, 0.5f, 0.5f), FName(TEXT("EvaAmbushPoint")));
    SpawnTaggedArenaBox(FVector(Center.X, -620.0f, 70.0f), FVector(0.55f, 0.55f, 0.55f), FName(TEXT("EvaHideSpot")));

    if (ZoneIndex == 0)
    {
        SpawnArenaBox(FVector(Center.X - 880.0f, 0.0f, 180.0f), FVector(0.35f, 14.0f, 3.6f));
    }
    if (ZoneIndex == 5)
    {
        SpawnArenaBox(FVector(Center.X + 880.0f, 0.0f, 180.0f), FVector(0.35f, 14.0f, 3.6f));
    }

    AActor* LabelMarker = SpawnArenaBox(FVector(Center.X, 0.0f, 25.0f), FVector(0.35f, 0.35f, 0.08f));
    if (LabelMarker)
    {
        LabelMarker->Tags.Add(FName(*Label));
        LabelMarker->SetActorHiddenInGame(true);
        LabelMarker->SetActorEnableCollision(false);
    }
}

void AEvaPrototypeGameMode::SpawnFacilityTrigger(AEvaResearchFacilityDirector* Director,
    const EEvaFacilityZone Zone, const FVector& Location)
{
    if (!GetWorld())
    {
        return;
    }

    AEvaFacilityZoneTrigger* Trigger = GetWorld()->SpawnActor<AEvaFacilityZoneTrigger>(AEvaFacilityZoneTrigger::StaticClass(),
        Location, FRotator::ZeroRotator);
    if (Trigger)
    {
        Trigger->ConfigureZone(Zone, Director, FVector(340.0f, 620.0f, 180.0f));
    }
}

void AEvaPrototypeGameMode::SpawnStoryLog(AEvaResearchFacilityDirector* Director, const FName LogId,
    const FString& Title, const FString& Body, const FVector& Location)
{
    if (!GetWorld())
    {
        return;
    }

    AEvaStoryLogPickup* StoryLog = GetWorld()->SpawnActor<AEvaStoryLogPickup>(AEvaStoryLogPickup::StaticClass(),
        Location, FRotator::ZeroRotator);
    if (StoryLog)
    {
        StoryLog->ConfigureStoryLog(LogId, Title, Body, Director);
    }
}

void AEvaPrototypeGameMode::SpawnPrototypeEnemies()
{
    if (!GetWorld() || bGameOver || bStageClear)
    {
        return;
    }

    SpawnEnemyNearLocation(AEvaZombieCharacter::StaticClass(), FVector(1100.0f, 250.0f, 140.0f),
        150.0f, 550.0f, TEXT("Zombie"), TEXT("PrototypeInitialA"));
    SpawnEnemyNearLocation(AEvaZombieCharacter::StaticClass(), FVector(1450.0f, -500.0f, 140.0f),
        150.0f, 550.0f, TEXT("Zombie"), TEXT("PrototypeInitialB"));
    SpawnEnemyNearLocation(AEvaZombieCharacter::StaticClass(), FVector(800.0f, 520.0f, 140.0f),
        150.0f, 550.0f, TEXT("Zombie"), TEXT("PrototypeInitialC"));
}

void AEvaPrototypeGameMode::SpawnInitialZombie()
{
    if (bInitialZombieSpawned || !GetWorld() || bGameOver || bStageClear)
    {
        return;
    }

    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    if (!PlayerPawn)
    {
        return;
    }

    bInitialZombieSpawned = true;
    AEvaZombieCharacter* Zombie = SpawnEnemyNearLocation(AEvaZombieCharacter::StaticClass(),
        PlayerPawn->GetActorLocation(), 520.0f, 900.0f, TEXT("Zombie"), TEXT("InitialVisibleZombie"));
    if (Zombie)
    {
        ShowDebugStatusMessage(TEXT("Initial Zombie Spawned"), 4.0f);
    }
    else
    {
        bInitialZombieSpawned = false;
        ShowDebugStatusMessage(TEXT("WARNING: Initial zombie safe spawn failed."), 6.0f);
    }
}

bool AEvaPrototypeGameMode::FindSafeEnemySpawnLocation(const FVector& Origin, const float MinRadius,
    const float MaxRadius, const float MinEnemySeparation, const float MinPlayerDistance, FVector& OutLocation) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    APlayerController* PlayerController = World->GetFirstPlayerController();
    const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    const FVector PlayerLocation = PlayerPawn ? PlayerPawn->GetActorLocation() : Origin;
    const FVector PlayerForward = PlayerPawn ? PlayerPawn->GetActorForwardVector().GetSafeNormal2D() : FVector::ForwardVector;
    const FVector PlayerRight = PlayerPawn ? PlayerPawn->GetActorRightVector().GetSafeNormal2D() : FVector::RightVector;
    UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(World);
    if (!bRuntimeNavigationReady || bRuntimeNavigationFailed || !NavigationSystem)
    {
        UE_LOG(LogAdaptiveHorror, Warning,
            TEXT("[SpawnAttempt] Type=Enemy Reason=SafeSearch Skipped=NavigationNotReady Ready=%s Failed=%s NavSystem=%s Origin=%s"),
            *BoolText(bRuntimeNavigationReady),
            *BoolText(bRuntimeNavigationFailed),
            *BoolText(NavigationSystem != nullptr),
            *Origin.ToCompactString());
        return false;
    }

    const float CapsuleRadius = 56.0f;
    const float CapsuleHalfHeight = 110.0f;
    const float RadiusMin = FMath::Max(0.0f, MinRadius);
    const float RadiusMax = FMath::Max(RadiusMin + 1.0f, MaxRadius);
    const float EnemySeparation = FMath::Max(0.0f, MinEnemySeparation);
    const float PlayerSeparation = FMath::Max(0.0f, MinPlayerDistance);

    for (int32 AttemptIndex = 0; AttemptIndex < SafeSpawnAttemptCount; ++AttemptIndex)
    {
        FVector Candidate = Origin;
        if (AttemptIndex == 0 && PlayerPawn)
        {
            Candidate = PlayerLocation + PlayerForward * FMath::Clamp((RadiusMin + RadiusMax) * 0.5f, 500.0f, 900.0f);
        }
        else if (AttemptIndex == 1 && PlayerPawn)
        {
            Candidate = PlayerLocation + PlayerForward * RadiusMin + PlayerRight * 260.0f;
        }
        else if (AttemptIndex == 2 && PlayerPawn)
        {
            Candidate = PlayerLocation + PlayerForward * RadiusMin - PlayerRight * 260.0f;
        }
        else
        {
            const float AngleDegrees = (360.0f / static_cast<float>(SafeSpawnAttemptCount)) * AttemptIndex +
                FMath::FRandRange(-22.5f, 22.5f);
            const float Radius = FMath::FRandRange(RadiusMin, RadiusMax);
            Candidate = Origin + FRotator(0.0f, AngleDegrees, 0.0f).Vector() * Radius;
        }

        FNavLocation ProjectedLocation;
        bool bNavProjected = false;
        bNavProjected = NavigationSystem->ProjectPointToNavigation(Candidate, ProjectedLocation,
            FVector(260.0f, 260.0f, 420.0f));

        const FVector NavLocation = bNavProjected ? ProjectedLocation.Location : Candidate;
        FHitResult FloorHit;
        const FVector TraceStart = NavLocation + FVector(0.0f, 0.0f, 500.0f);
        const FVector TraceEnd = NavLocation - FVector(0.0f, 0.0f, 900.0f);
        const bool bFloorHit = World->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, ECC_WorldStatic);
        const bool bFloorAccepted = IsAcceptableEvaFloorHit(FloorHit);
        const AActor* FloorActor = FloorHit.GetActor();
        const FVector FinalLocation = bNavProjected && bFloorAccepted ?
            FloorHit.ImpactPoint + FVector(0.0f, 0.0f, CapsuleHalfHeight + 2.0f) :
            Candidate + FVector(0.0f, 0.0f, CapsuleHalfHeight);

        const float PlayerDistance = FVector::Dist(FinalLocation, PlayerLocation);
        const float NearestEnemyDistance = GetNearestEnemyDistance(World, FinalLocation);
        const bool bHasOverlap = HasBlockingSpawnOverlap(World, FinalLocation, CapsuleRadius, CapsuleHalfHeight);
        const bool bPassesPlayerDistance = !PlayerPawn || PlayerDistance >= PlayerSeparation;
        const bool bPassesEnemyDistance = NearestEnemyDistance >= EnemySeparation;

        UE_LOG(LogAdaptiveHorror, Log,
            TEXT("[SpawnAttempt] Type=Enemy Reason=SafeSearch Attempt=%d Requested=%s NavLocation=%s Final=%s NavProjected=%s FloorTrace=%s FloorAccepted=%s FloorActor=%s FloorComponent=%s FloorZ=%.1f NormalZ=%.2f Overlap=%s PlayerDistance=%.1f NearestEnemyDistance=%.1f"),
            AttemptIndex,
            *Candidate.ToCompactString(),
            bNavProjected ? *NavLocation.ToCompactString() : TEXT("None"),
            *FinalLocation.ToCompactString(),
            *BoolText(bNavProjected),
            *BoolText(bFloorHit),
            *BoolText(bFloorAccepted),
            FloorActor ? *FloorActor->GetName() : TEXT("None"),
            FloorHit.GetComponent() ? *FloorHit.GetComponent()->GetName() : TEXT("None"),
            FloorHit.bBlockingHit ? FloorHit.ImpactPoint.Z : -99999.0f,
            FloorHit.bBlockingHit ? FloorHit.ImpactNormal.Z : 0.0f,
            *BoolText(bHasOverlap),
            PlayerDistance,
            NearestEnemyDistance == TNumericLimits<float>::Max() ? -1.0f : NearestEnemyDistance);

#if !UE_BUILD_SHIPPING
        DrawDebugSphere(World, FinalLocation, 42.0f, 12,
            (bNavProjected && !bHasOverlap && bPassesPlayerDistance && bPassesEnemyDistance && bFloorAccepted) ?
                FColor::Green : FColor::Red,
            false, 4.0f);
#endif

        if (bNavProjected && bFloorAccepted && !bHasOverlap && bPassesPlayerDistance && bPassesEnemyDistance)
        {
            OutLocation = FinalLocation;
            return true;
        }
    }

    return false;
}

AEvaZombieCharacter* AEvaPrototypeGameMode::SpawnEnemyNearLocation(TSubclassOf<AEvaZombieCharacter> EnemyClass,
    const FVector& Origin, const float MinRadius, const float MaxRadius, const FString& EnemyType,
    const FString& SpawnReason, const EEvaEvolutionType EvolutionType)
{
    if (bStageClear)
    {
        LastSpawnLocation = Origin;
        LastSpawnResult = FString::Printf(TEXT("%s skipped: stage clear"), *SpawnReason);
        UE_LOG(LogAdaptiveHorror, Log,
            TEXT("[StageClear] Spawn skipped after clear Type=%s Reason=%s Requested=%s"),
            *EnemyType,
            *SpawnReason,
            *Origin.ToCompactString());
        return nullptr;
    }

    if (!GetWorld() || !EnemyClass)
    {
        LastSpawnResult = FString::Printf(TEXT("%s failed: invalid world/class"), *SpawnReason);
        UE_LOG(LogAdaptiveHorror, Warning, TEXT("[Spawn] Type=%s Reason=%s Failed=InvalidWorldOrClass"),
            *EnemyType, *SpawnReason);
        return nullptr;
    }
    if (!bRuntimeNavigationReady || bRuntimeNavigationFailed)
    {
        LastSpawnResult = FString::Printf(TEXT("%s failed: runtime navigation not ready"), *SpawnReason);
        UE_LOG(LogAdaptiveHorror, Warning,
            TEXT("[Spawn] Type=%s Reason=%s Failed=NavigationNotReady Ready=%s FailedState=%s"),
            *EnemyType,
            *SpawnReason,
            *BoolText(bRuntimeNavigationReady),
            *BoolText(bRuntimeNavigationFailed));
        return nullptr;
    }

    FVector FinalLocation = FVector::ZeroVector;
    const bool bFoundSafeLocation = FindSafeEnemySpawnLocation(Origin, MinRadius, MaxRadius,
        DefaultEnemySeparation, DefaultPlayerSeparation, FinalLocation);
    if (!bFoundSafeLocation)
    {
        LastSpawnLocation = Origin;
        LastSpawnResult = FString::Printf(TEXT("%s failed: no safe NavMesh/collision-free location"), *SpawnReason);
        UE_LOG(LogAdaptiveHorror, Warning, TEXT("[Spawn] Type=%s Reason=%s Requested=%s Failed=NoSafeLocation"),
            *EnemyType, *SpawnReason, *Origin.ToCompactString());
        return nullptr;
    }

    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    const FRotator SpawnRotation = PlayerPawn ?
        (PlayerPawn->GetActorLocation() - FinalLocation).GetSafeNormal2D().Rotation() :
        FRotator(0.0f, 180.0f, 0.0f);

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
    AEvaZombieCharacter* Enemy = GetWorld()->SpawnActor<AEvaZombieCharacter>(EnemyClass, FinalLocation,
        SpawnRotation, SpawnParameters);
    if (!Enemy)
    {
        LastSpawnLocation = FinalLocation;
        LastSpawnResult = FString::Printf(TEXT("%s failed: SpawnActor returned null"), *SpawnReason);
        UE_LOG(LogAdaptiveHorror, Warning,
            TEXT("[Spawn] Type=%s Reason=%s Requested=%s Final=%s CollisionHandling=%s Result=SpawnActorNull"),
            *EnemyType,
            *SpawnReason,
            *Origin.ToCompactString(),
            *FinalLocation.ToCompactString(),
            *SpawnCollisionHandlingToText(SpawnParameters.SpawnCollisionHandlingOverride));
        return nullptr;
    }

    if (!Enemy->ActorHasTag(TEXT("Adam")) && !Enemy->ActorHasTag(TEXT("Boss")))
    {
        Enemy->ConfigureEvolution(EvolutionType);
    }
    if (!Enemy->GetController())
    {
        Enemy->SpawnDefaultController();
    }
    PrimeEnemyForPlayer(Enemy);

    const bool bPostSpawnOverlap = HasBlockingSpawnOverlap(GetWorld(), Enemy->GetActorLocation(), 56.0f, 110.0f);
    if (bPostSpawnOverlap)
    {
        FVector RetryLocation;
        if (FindSafeEnemySpawnLocation(Origin, MinRadius, MaxRadius + 240.0f, DefaultEnemySeparation,
            DefaultPlayerSeparation, RetryLocation))
        {
            Enemy->SetActorLocation(RetryLocation, false, nullptr, ETeleportType::TeleportPhysics);
            FinalLocation = RetryLocation;
            UE_LOG(LogAdaptiveHorror, Warning, TEXT("[Spawn] Type=%s Reason=%s PostSpawnOverlap=Relocated Final=%s"),
                *EnemyType, *SpawnReason, *FinalLocation.ToCompactString());
        }
        else
        {
            UE_LOG(LogAdaptiveHorror, Warning, TEXT("[Spawn] Type=%s Reason=%s PostSpawnOverlap=Destroy"),
                *EnemyType, *SpawnReason);
            Enemy->Destroy();
            LastSpawnLocation = FinalLocation;
            LastSpawnResult = FString::Printf(TEXT("%s failed: post-spawn overlap"), *SpawnReason);
            return nullptr;
        }
    }

    const float PlayerDistance = PlayerPawn ? FVector::Dist(FinalLocation, PlayerPawn->GetActorLocation()) : -1.0f;
    const float EnemyDistance = GetNearestEnemyDistance(GetWorld(), FinalLocation, Enemy);
    const bool bHasController = Enemy->GetController() != nullptr;
    const bool bPossessed = bHasController;
    LastSpawnLocation = FinalLocation;
    LastSpawnResult = FString::Printf(TEXT("%s spawned %s at %s"), *SpawnReason, *EnemyType,
        *FinalLocation.ToCompactString());
    if (SpawnReason == TEXT("InitialEntryLobby") || SpawnReason == TEXT("InitialVisibleZombie"))
    {
        bInitialZombieSpawned = true;
        ShowDebugStatusMessage(TEXT("Initial Zombie Spawned"), 4.0f);
    }

    UE_LOG(LogAdaptiveHorror, Log,
        TEXT("[Spawn] Type=%s Reason=%s Requested=%s Final=%s SpawnAttemptIndex=success CollisionHandling=%s Success=true Actor=%s AIController=%s Possessed=%s PlayerDistance=%.1f NearestEnemyDistance=%.1f"),
        *EnemyType,
        *SpawnReason,
        *Origin.ToCompactString(),
        *FinalLocation.ToCompactString(),
        *SpawnCollisionHandlingToText(SpawnParameters.SpawnCollisionHandlingOverride),
        *Enemy->GetName(),
        *BoolText(bHasController),
        *BoolText(bPossessed),
        PlayerDistance,
        EnemyDistance == TNumericLimits<float>::Max() ? -1.0f : EnemyDistance);

    return Enemy;
}

void AEvaPrototypeGameMode::PrimeEnemyForPlayer(AEvaZombieCharacter* Enemy) const
{
    if (!Enemy || !GetWorld() || bStageClear)
    {
        return;
    }

    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    if (!PlayerPawn)
    {
        UE_LOG(LogAdaptiveHorror, Warning, TEXT("[AI] PrimeEnemyForPlayer skipped: PlayerPawn missing Enemy=%s"),
            *Enemy->GetName());
        return;
    }

    Enemy->AlertToPlayer(PlayerPawn);
    if (AEvaZombieAIController* ZombieController = Cast<AEvaZombieAIController>(Enemy->GetController()))
    {
        ZombieController->SetPlayerTarget(PlayerPawn);
        UE_LOG(LogAdaptiveHorror, Log, TEXT("[AI] Enemy primed for pursuit Enemy=%s Controller=%s Target=%s Distance=%.1f"),
            *Enemy->GetName(),
            *ZombieController->GetName(),
            *PlayerPawn->GetName(),
            FVector::Dist(Enemy->GetActorLocation(), PlayerPawn->GetActorLocation()));
    }
    else
    {
        UE_LOG(LogAdaptiveHorror, Warning, TEXT("[AI] Enemy spawned without EVA AI controller Enemy=%s Controller=%s"),
            *Enemy->GetName(),
            Enemy->GetController() ? *Enemy->GetController()->GetName() : TEXT("None"));
    }
}

FVector AEvaPrototypeGameMode::PickEnemySpawnLocation() const
{
    static const FVector SpawnLocations[] =
    {
        FVector(-2500.0f, 480.0f, 140.0f),
        FVector(-850.0f, -480.0f, 140.0f),
        FVector(950.0f, 480.0f, 140.0f),
        FVector(2750.0f, -480.0f, 140.0f),
        FVector(4550.0f, 420.0f, 140.0f)
    };

    const int32 Index = FMath::Abs(TotalZombieKills) % UE_ARRAY_COUNT(SpawnLocations);
    return SpawnLocations[Index];
}

int32 AEvaPrototypeGameMode::CountLivingEnemies() const
{
    if (!GetWorld())
    {
        return 0;
    }

    TArray<AActor*> Zombies;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEvaZombieCharacter::StaticClass(), Zombies);

    int32 LivingCount = 0;
    for (AActor* Actor : Zombies)
    {
        const AEvaZombieCharacter* Zombie = Cast<AEvaZombieCharacter>(Actor);
        if (Zombie && Zombie->GetHealthComponent() && !Zombie->GetHealthComponent()->IsDead())
        {
            ++LivingCount;
        }
    }
    return LivingCount;
}

int32 AEvaPrototypeGameMode::GetActiveZombieCount() const
{
    if (!GetWorld())
    {
        return 0;
    }

    int32 Count = 0;
    for (TActorIterator<AEvaZombieCharacter> It(GetWorld()); It; ++It)
    {
        const AEvaZombieCharacter* Enemy = *It;
        if (IsLivingEvaEnemy(Enemy) && !Enemy->ActorHasTag(TEXT("Hunter")) &&
            !Enemy->ActorHasTag(TEXT("Adam")) && !Enemy->ActorHasTag(TEXT("Boss")))
        {
            ++Count;
        }
    }
    return Count;
}

int32 AEvaPrototypeGameMode::GetActiveHunterCount() const
{
    if (!GetWorld())
    {
        return 0;
    }

    int32 Count = 0;
    for (TActorIterator<AEvaZombieCharacter> It(GetWorld()); It; ++It)
    {
        const AEvaZombieCharacter* Enemy = *It;
        if (IsLivingEvaEnemy(Enemy) && Enemy->ActorHasTag(TEXT("Hunter")))
        {
            ++Count;
        }
    }
    return Count;
}

int32 AEvaPrototypeGameMode::GetActiveAdamCount() const
{
    if (!GetWorld())
    {
        return 0;
    }

    int32 Count = 0;
    for (TActorIterator<AEvaZombieCharacter> It(GetWorld()); It; ++It)
    {
        const AEvaZombieCharacter* Enemy = *It;
        if (IsLivingEvaEnemy(Enemy) && (Enemy->ActorHasTag(TEXT("Adam")) || Enemy->ActorHasTag(TEXT("Boss"))))
        {
            ++Count;
        }
    }
    return Count;
}

bool AEvaPrototypeGameMode::IsNavMeshAvailable() const
{
    if (!GetWorld() || !bRuntimeNavigationReady || bRuntimeNavigationFailed)
    {
        return false;
    }

    UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld());
    const ANavigationData* MainNavData = NavigationSystem ?
        NavigationSystem->GetDefaultNavDataInstance(FNavigationSystem::DontCreate) :
        nullptr;
    return Cast<ARecastNavMesh>(MainNavData) != nullptr;
}

void AEvaPrototypeGameMode::NotifyFallbackMovementUsed()
{
    ++FallbackMovementCount;
}

void AEvaPrototypeGameMode::NotifyEnemyStuck(const FString& EnemyName)
{
    ++StuckEnemyCount;
    UE_LOG(LogAdaptiveHorror, Warning, TEXT("[AI] EnemyStuck Enemy=%s Count=%d"), *EnemyName, StuckEnemyCount);
}

AActor* AEvaPrototypeGameMode::SpawnArenaBox(const FVector& Location, const FVector& Scale, const FRotator& Rotation)
{
    if (!GetWorld() || !RuntimeCubeMesh)
    {
        return nullptr;
    }

    AStaticMeshActor* Box = GetWorld()->SpawnActor<AStaticMeshActor>(Location, Rotation);
    if (Box)
    {
        UStaticMeshComponent* MeshComponent = Box->GetStaticMeshComponent();
        if (MeshComponent)
        {
            // Runtime graybox geometry must become navigation-relevant after its final transform is applied.
            // Keeping the floor/covers movable can make PIE NavMesh generation unreliable on some UE versions.
            MeshComponent->SetMobility(EComponentMobility::Movable);
            MeshComponent->SetStaticMesh(RuntimeCubeMesh);
            MeshComponent->SetCollisionProfileName(TEXT("BlockAll"));
            MeshComponent->SetCanEverAffectNavigation(true);
        }
        Box->SetActorScale3D(Scale);
        if (MeshComponent)
        {
            MeshComponent->SetMobility(EComponentMobility::Static);
        }
    }
    return Box;
}

AActor* AEvaPrototypeGameMode::SpawnTaggedArenaBox(const FVector& Location, const FVector& Scale,
    const FName Tag, const FRotator& Rotation)
{
    AActor* Box = SpawnArenaBox(Location, Scale, Rotation);
    if (!Box)
    {
        return nullptr;
    }

    Box->Tags.Add(Tag);
    if (Tag == FName(TEXT("EvaEscapeRoute")) || Tag == FName(TEXT("EvaAmbushPoint")) ||
        Tag == FName(TEXT("EvaHideSpot")))
    {
        Box->SetActorHiddenInGame(true);
        Box->SetActorEnableCollision(false);
    }
    return Box;
}

void AEvaPrototypeGameMode::SpawnAdaptiveEnemy()
{
    if (!GetWorld() || bGameOver || bStageClear || CountLivingEnemies() >= MaxLivingEnemies)
    {
        return;
    }
    if (CurrentDirector && CurrentDirector->IsAdamEncounterActive())
    {
        UE_LOG(LogAdaptiveHorror, Log,
            TEXT("[Spawn] Type=Zombie Reason=AdaptiveSpawn Skipped=AdamEncounterActive"));
        return;
    }

    EEvaEvolutionType EvolutionType = EEvaEvolutionType::None;
    if (const UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        if (const UEvaLearningSubsystem* Learning = GameInstance->GetSubsystem<UEvaLearningSubsystem>())
        {
            EvolutionType = Learning->GetRecommendedEvolutionType();
        }
    }

    AEvaZombieCharacter* Zombie = SpawnEnemyNearLocation(AEvaZombieCharacter::StaticClass(), PickEnemySpawnLocation(),
        180.0f, 700.0f, TEXT("Zombie"), TEXT("AdaptiveSpawn"), EvolutionType);
    if (Zombie)
    {
        if (EvolutionType != EEvaEvolutionType::None)
        {
            ShowDebugStatusMessage(TEXT("EVA deployed an evolved infected."), 3.0f);
        }
    }
    else
    {
        ShowDebugStatusMessage(TEXT("WARNING: Adaptive infected spawn failed."), 4.0f);
    }
}

void AEvaPrototypeGameMode::SpawnHunter()
{
    if (!GetWorld() || bGameOver || bStageClear)
    {
        return;
    }
    if (IsValid(CurrentHunter))
    {
        return;
    }
    CurrentHunter = nullptr;

    GetWorldTimerManager().ClearTimer(HunterTimeSpawnTimer);
    GetWorldTimerManager().ClearTimer(HunterReinsertTimer);

    const FTransform SpawnTransform = CurrentDirector ?
        CurrentDirector->GetHunterSpawnTransform() :
        FTransform(FRotator(0.0f, 180.0f, 0.0f), FVector(3100.0f, 520.0f, 140.0f));

    FVector SafeLocation = SpawnTransform.GetLocation();
    if (!FindSafeEnemySpawnLocation(SpawnTransform.GetLocation(), 200.0f, 760.0f,
        DefaultEnemySeparation, DefaultPlayerSeparation, SafeLocation))
    {
        LastSpawnLocation = SpawnTransform.GetLocation();
        LastSpawnResult = TEXT("HUNTER failed: no safe location");
        UE_LOG(LogAdaptiveHorror, Warning, TEXT("[Spawn] Type=Hunter Reason=HunterDeployment Requested=%s Failed=NoSafeLocation"),
            *SpawnTransform.GetLocation().ToCompactString());
        ShowDebugStatusMessage(TEXT("WARNING: HUNTER spawn failed - no safe location."), 5.0f);
        return;
    }

    const FTransform SafeSpawnTransform(SpawnTransform.Rotator(), SafeLocation, SpawnTransform.GetScale3D());
    AEvaHunterCharacter* Hunter = GetWorld()->SpawnActorDeferred<AEvaHunterCharacter>(AEvaHunterCharacter::StaticClass(),
        SafeSpawnTransform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding);
    if (!Hunter)
    {
        LastSpawnLocation = SafeLocation;
        LastSpawnResult = TEXT("HUNTER failed: SpawnActorDeferred returned null");
        UE_LOG(LogAdaptiveHorror, Warning,
            TEXT("[Spawn] Type=Hunter Reason=HunterDeployment Requested=%s Final=%s CollisionHandling=%s Result=SpawnActorDeferredNull"),
            *SpawnTransform.GetLocation().ToCompactString(),
            *SafeLocation.ToCompactString(),
            *SpawnCollisionHandlingToText(ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding));
        ShowDebugStatusMessage(TEXT("WARNING: HUNTER spawn failed."), 5.0f);
        return;
    }

    Hunter->InitializeHunterTier(HunterTierToSpawn);
    UGameplayStatics::FinishSpawningActor(Hunter, SafeSpawnTransform);
    if (!Hunter->GetController())
    {
        Hunter->SpawnDefaultController();
    }
    PrimeEnemyForPlayer(Hunter);
    CurrentHunter = Hunter;
    LastSpawnLocation = SafeLocation;
    LastSpawnResult = FString::Printf(TEXT("HUNTER Tier %d spawned at %s"), HunterTierToSpawn,
        *SafeLocation.ToCompactString());
    UE_LOG(LogAdaptiveHorror, Log,
        TEXT("[Spawn] Type=Hunter Reason=HunterDeployment Requested=%s Final=%s SpawnAttemptIndex=success CollisionHandling=%s Success=true Actor=%s AIController=%s Possessed=%s PlayerDistance=%.1f NearestEnemyDistance=%.1f"),
        *SpawnTransform.GetLocation().ToCompactString(),
        *SafeLocation.ToCompactString(),
        *SpawnCollisionHandlingToText(ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding),
        *Hunter->GetName(),
        *BoolText(Hunter->GetController() != nullptr),
        *BoolText(Hunter->GetController() != nullptr),
        GetWorld()->GetFirstPlayerController() && GetWorld()->GetFirstPlayerController()->GetPawn() ?
            FVector::Dist(SafeLocation, GetWorld()->GetFirstPlayerController()->GetPawn()->GetActorLocation()) : -1.0f,
        GetNearestEnemyDistance(GetWorld(), SafeLocation, Hunter));

    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        if (UEvaLearningSubsystem* Learning = GameInstance->GetSubsystem<UEvaLearningSubsystem>())
        {
            Learning->SetHunterState(EEvaHunterState::Deployed, HunterTierToSpawn);
            Learning->SetLearningSpeedMultiplier(1.0f);
        }
    }
    ShowDebugStatusMessage(FString::Printf(TEXT("WARNING: HUNTER Tier %d deployed. EVA analysis at full speed."),
        HunterTierToSpawn), 5.0f);
}

void AEvaPrototypeGameMode::ResetEnemyTargets()
{
    if (!GetWorld())
    {
        return;
    }

    TArray<AActor*> Zombies;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEvaZombieCharacter::StaticClass(), Zombies);
    for (AActor* Actor : Zombies)
    {
        if (APawn* EnemyPawn = Cast<APawn>(Actor))
        {
            if (AEvaZombieAIController* ZombieController = Cast<AEvaZombieAIController>(EnemyPawn->GetController()))
            {
                ZombieController->ClearPlayerTarget();
            }
        }
    }
}

int32 AEvaPrototypeGameMode::StopAllEnemyCombatForStageClear()
{
    if (!GetWorld())
    {
        return 0;
    }

    int32 ClearedEnemyAI = 0;
    for (TActorIterator<AEvaZombieCharacter> It(GetWorld()); It; ++It)
    {
        AEvaZombieCharacter* Enemy = *It;
        if (!Enemy)
        {
            continue;
        }

        Enemy->SetOverheadDisplayEnabled(false);

        if (UCharacterMovementComponent* MovementComponent = Enemy->GetCharacterMovement())
        {
            MovementComponent->StopMovementImmediately();
            MovementComponent->DisableMovement();
        }

        AAIController* AIController = Cast<AAIController>(Enemy->GetController());
        if (AEvaZombieAIController* EvaController = Cast<AEvaZombieAIController>(AIController))
        {
            EvaController->StopCombatForStageClear();
            ++ClearedEnemyAI;
        }
        else if (AIController)
        {
            AIController->ClearFocus(EAIFocusPriority::Gameplay);
            AIController->StopMovement();
            AIController->SetActorTickEnabled(false);
            ++ClearedEnemyAI;
        }

        Enemy->SetActorTickEnabled(false);
    }

    ResetEnemyTargets();
    CurrentHunter = nullptr;
    return ClearedEnemyAI;
}

void AEvaPrototypeGameMode::ClearStageClearTimers()
{
    if (!GetWorld())
    {
        return;
    }

    GetWorldTimerManager().ClearTimer(RespawnTimer);
    GetWorldTimerManager().ClearTimer(EnemySpawnTimer);
    GetWorldTimerManager().ClearTimer(AdaptiveSpawnTimer);
    GetWorldTimerManager().ClearTimer(HunterTimeSpawnTimer);
    GetWorldTimerManager().ClearTimer(HunterReinsertTimer);
}

void AEvaPrototypeGameMode::CleanupCombatActorsForFlowReset()
{
    if (!GetWorld())
    {
        return;
    }

    for (TActorIterator<AEvaZombieCharacter> It(GetWorld()); It; ++It)
    {
        AEvaZombieCharacter* Enemy = *It;
        if (Enemy)
        {
            Enemy->Destroy();
        }
    }
    CurrentHunter = nullptr;
}

void AEvaPrototypeGameMode::SetGameFlowState(const EEvaGameFlowState NewState)
{
    const EEvaGameFlowState OldState = GameFlowState;
    if (GameFlowState != NewState)
    {
        GameFlowState = NewState;
    }

    UWorld* World = GetWorld();
    const APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
    const AEvaPlayerController* EvaController = Cast<AEvaPlayerController>(PlayerController);
    const APawn* Pawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    const ULocalPlayer* LocalPlayer = EvaController ? EvaController->GetLocalPlayer() : nullptr;

    UE_LOG(LogAdaptiveHorror, Log,
        TEXT("[GameFlow] PreviousState=%s CurrentState=%s WorldName=%s NetMode=%s GameModeClass=%s PlayerControllerClass=%s PlayerControllerValid=%s LocalController=%s LocalPlayerValid=%s Pawn=%s PossessedPawn=%s Unchanged=%s"),
        *UEnum::GetValueAsString(OldState),
        *UEnum::GetValueAsString(GameFlowState),
        World ? *World->GetName() : TEXT("None"),
        World ? *NetModeToText(World->GetNetMode()) : TEXT("NoWorld"),
        *GetClass()->GetName(),
        PlayerControllerClass ? *PlayerControllerClass->GetName() : TEXT("None"),
        PlayerController ? TEXT("true") : TEXT("false"),
        PlayerController && PlayerController->IsLocalController() ? TEXT("true") : TEXT("false"),
        LocalPlayer ? TEXT("true") : TEXT("false"),
        Pawn ? *Pawn->GetClass()->GetName() : TEXT("None"),
        Pawn ? *Pawn->GetName() : TEXT("None"),
        OldState == NewState ? TEXT("true") : TEXT("false"));
}

void AEvaPrototypeGameMode::LogStageClearState(const FString& Context, const int32 ClearedEnemyAI,
    const bool bClearedTimers) const
{
    UWorld* World = GetWorld();
    const APlayerController* PlayerController = World ? World->GetFirstPlayerController() : nullptr;
    const AEvaPlayerCharacter* Player = PlayerController ? Cast<AEvaPlayerCharacter>(PlayerController->GetPawn()) : nullptr;
    const UEvaHealthComponent* PlayerHealth = Player ? Player->GetHealthComponent() : nullptr;

    UE_LOG(LogAdaptiveHorror, Warning,
        TEXT("[StageClear] Context=%s PlayerHP=%.1f PlayerDead=%s ActiveZombies=%d ActiveHunters=%d ActiveAdam=%d ClearedEnemyAI=%d ClearedTimers=%s PlayerDamageDisabled=%s PlayerMoveInputDisabled=%s PlayerLookInputDisabled=%s IsPaused=%s GlobalTimeDilation=%.2f IgnoreMoveInput=%s IgnoreLookInput=%s RespawnTimerActive=%s AdaptiveTimerActive=%s HunterTimerActive=%s HunterReinsertTimerActive=%s"),
        *Context,
        PlayerHealth ? PlayerHealth->GetCurrentHealth() : -1.0f,
        Player && Player->IsDead() ? TEXT("true") : TEXT("false"),
        GetActiveZombieCount(),
        GetActiveHunterCount(),
        GetActiveAdamCount(),
        ClearedEnemyAI,
        *BoolText(bClearedTimers),
        *BoolText(bStageClear),
        *BoolText(PlayerController ? PlayerController->IsMoveInputIgnored() : false),
        *BoolText(PlayerController ? PlayerController->IsLookInputIgnored() : false),
        *BoolText(World ? UGameplayStatics::IsGamePaused(World) : false),
        World ? UGameplayStatics::GetGlobalTimeDilation(World) : 1.0f,
        *BoolText(PlayerController ? PlayerController->IsMoveInputIgnored() : false),
        *BoolText(PlayerController ? PlayerController->IsLookInputIgnored() : false),
        *BoolText(IsRespawnScheduledForDebug()),
        *BoolText(World ? World->GetTimerManager().IsTimerActive(AdaptiveSpawnTimer) : false),
        *BoolText(World ? World->GetTimerManager().IsTimerActive(HunterTimeSpawnTimer) : false),
        *BoolText(World ? World->GetTimerManager().IsTimerActive(HunterReinsertTimer) : false));
}

void AEvaPrototypeGameMode::LogPlayerDeathRequest(const FString& Context, const AEvaPlayerCharacter* DeadPlayer,
    const bool bRespawnTimerCreated) const
{
    UWorld* World = GetWorld();
    const APlayerController* PlayerController = DeadPlayer ? Cast<APlayerController>(DeadPlayer->GetController()) :
        (World ? World->GetFirstPlayerController() : nullptr);
    const UEvaHealthComponent* Health = DeadPlayer ? DeadPlayer->GetHealthComponent() : nullptr;

    UE_LOG(LogAdaptiveHorror, Warning,
        TEXT("[PlayerDeath] Context=%s DeathRequest=%s RejectedBecauseStageClear=%s HP=%.1f PlayerDead=%s StageClearState=%s GameOverState=%s RespawnTimerCreated=%s RespawnTimerActive=%s IgnoreMoveInput=%s IgnoreLookInput=%s"),
        *Context,
        *BoolText(DeadPlayer != nullptr),
        *BoolText(bStageClear),
        Health ? Health->GetCurrentHealth() : -1.0f,
        DeadPlayer && DeadPlayer->IsDead() ? TEXT("true") : TEXT("false"),
        *BoolText(bStageClear),
        *BoolText(bGameOver),
        *BoolText(bRespawnTimerCreated),
        *BoolText(IsRespawnScheduledForDebug()),
        *BoolText(PlayerController ? PlayerController->IsMoveInputIgnored() : false),
        *BoolText(PlayerController ? PlayerController->IsLookInputIgnored() : false));
}

bool AEvaPrototypeGameMode::IsRespawnScheduledForDebug() const
{
    return GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(RespawnTimer);
}

int32 AEvaPrototypeGameMode::CleanupAdamArenaDebugEnemies(const FVector& ArenaLocation, const float Radius)
{
    if (!GetWorld())
    {
        return 0;
    }

    int32 RemovedCount = 0;
    for (TActorIterator<AEvaZombieCharacter> It(GetWorld()); It; ++It)
    {
        AEvaZombieCharacter* Enemy = *It;
        if (!IsLivingEvaEnemy(Enemy) || Enemy->ActorHasTag(TEXT("Adam")) || Enemy->ActorHasTag(TEXT("Boss")) ||
            Enemy->ActorHasTag(TEXT("Hunter")))
        {
            continue;
        }

        if (FVector::DistSquared2D(Enemy->GetActorLocation(), ArenaLocation) <= FMath::Square(Radius))
        {
            UE_LOG(LogAdaptiveHorror, Warning,
                TEXT("[AdamDebug] CleanupRemovedEnemy Actor=%s Class=%s Location=%s ArenaLocation=%s Radius=%.1f"),
                *Enemy->GetName(),
                *Enemy->GetClass()->GetName(),
                *Enemy->GetActorLocation().ToCompactString(),
                *ArenaLocation.ToCompactString(),
                Radius);
            Enemy->Destroy();
            ++RemovedCount;
        }
    }
    return RemovedCount;
}

bool AEvaPrototypeGameMode::ShouldDisplayDebugStatusMessage() const
{
    return GetWorld() && !LastDebugStatusMessage.IsEmpty() &&
        GetWorld()->GetTimeSeconds() - LastDebugStatusMessageTime <= DebugStatusMessageDuration;
}

void AEvaPrototypeGameMode::ShowDebugStatusMessage(const FString& Message, const float Duration)
{
    if (Message.IsEmpty())
    {
        return;
    }

    LastDebugStatusMessage = Message;
    DebugStatusMessageDuration = FMath::Max(0.1f, Duration);
    LastDebugStatusMessageTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

#if !UE_BUILD_SHIPPING
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, DebugStatusMessageDuration, FColor::Cyan, Message);
    }
#endif
}

void AEvaPrototypeGameMode::DebugIncreaseEvaAnalysis(const float Amount)
{
#if UE_BUILD_SHIPPING
    (void)Amount;
#else
    if (!GetWorld())
    {
        return;
    }
    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        if (UEvaLearningSubsystem* Learning = GameInstance->GetSubsystem<UEvaLearningSubsystem>())
        {
            Learning->DebugAddAnalysis(Amount);
            ShowDebugStatusMessage(FString::Printf(TEXT("DEBUG F1: EVA Analysis +%.0f -> %.0f%%"),
                Amount, Learning->GetEvaAnalysisRate()), 4.0f);
        }
    }
#endif
}

void AEvaPrototypeGameMode::DebugForceHunterSpawn()
{
#if !UE_BUILD_SHIPPING
    ShowDebugStatusMessage(TEXT("DEBUG F2: Force HUNTER deployment."), 3.0f);
    SpawnHunter();
#endif
}

void AEvaPrototypeGameMode::DebugForceZombieWave()
{
#if !UE_BUILD_SHIPPING
    if (bGameOver || bStageClear)
    {
        ShowDebugStatusMessage(TEXT("DEBUG F3 skipped: game is not in active combat."), 3.0f);
        return;
    }
    const int32 PreviousMax = MaxLivingEnemies;
    MaxLivingEnemies = FMath::Max(MaxLivingEnemies, CountLivingEnemies() + 3);
    for (int32 Index = 0; Index < 3; ++Index)
    {
        SpawnAdaptiveEnemy();
    }
    MaxLivingEnemies = PreviousMax;
    ShowDebugStatusMessage(TEXT("DEBUG F3: Forced infected wave spawned."), 3.0f);
#endif
}

void AEvaPrototypeGameMode::DebugWarpPlayerToAdamArena()
{
#if !UE_BUILD_SHIPPING
    if (!GetWorld())
    {
        return;
    }

    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    AEvaPlayerCharacter* Player = PlayerController ? Cast<AEvaPlayerCharacter>(PlayerController->GetPawn()) : nullptr;
    if (!Player)
    {
        ShowDebugStatusMessage(TEXT("DEBUG F4 failed: player pawn not found."), 4.0f);
        return;
    }

    const FTransform AdamTransform = CurrentDirector ?
        CurrentDirector->GetAdamSpawnTransform() :
        FTransform(FRotator(0.0f, 180.0f, 0.0f), FVector(5100.0f, 0.0f, 170.0f));
    const int32 ExistingAdamCountBefore = GetActiveAdamCount();
    UE_LOG(LogAdaptiveHorror, Warning,
        TEXT("[AdamDebug] F4 pressed ArenaLocation=%s ArenaState=Director:%s Zone:%s EncounterActive:%s StageClear:%s AdamClass=%s ExistingAdamCount=%d"),
        *AdamTransform.GetLocation().ToCompactString(),
        CurrentDirector ? TEXT("true") : TEXT("false"),
        CurrentDirector ? *CurrentDirector->GetCurrentZoneName() : TEXT("NoDirector"),
        CurrentDirector && CurrentDirector->IsAdamEncounterActive() ? TEXT("true") : TEXT("false"),
        bStageClear ? TEXT("true") : TEXT("false"),
        *AEvaAdamBossCharacter::StaticClass()->GetName(),
        ExistingAdamCountBefore);

    const FVector PlayerLocation = AdamTransform.GetLocation() + FVector(-900.0f, 0.0f, 0.0f);
    Player->SetActorLocationAndRotation(PlayerLocation, FRotator::ZeroRotator, false, nullptr, ETeleportType::TeleportPhysics);
    if (PlayerController)
    {
        PlayerController->SetControlRotation(FRotator::ZeroRotator);
    }

    const int32 RemovedEnemies = CleanupAdamArenaDebugEnemies(AdamTransform.GetLocation(), 2200.0f);
    if (CurrentDirector)
    {
        CurrentDirector->NotifyZoneEntered(EEvaFacilityZone::AdamArena);
        CurrentDirector->StartAdamEncounter();
    }
    else
    {
        UE_LOG(LogAdaptiveHorror, Warning, TEXT("[AdamDebug] F4 failed: no ResearchFacilityDirector."));
    }

    const int32 ExistingAdamCountAfter = GetActiveAdamCount();
    if (CurrentDirector && CurrentDirector->GetActiveAdam())
    {
        PrimeEnemyForPlayer(CurrentDirector->GetActiveAdam());
    }

    UE_LOG(LogAdaptiveHorror, Warning,
        TEXT("[AdamDebug] F4 result ArenaLocation=%s RemovedNonBossEnemies=%d ExistingAdamBefore=%d ExistingAdamAfter=%d ActiveAdam=%s"),
        *AdamTransform.GetLocation().ToCompactString(),
        RemovedEnemies,
        ExistingAdamCountBefore,
        ExistingAdamCountAfter,
        CurrentDirector && CurrentDirector->GetActiveAdam() ? *CurrentDirector->GetActiveAdam()->GetName() : TEXT("None"));
    ShowDebugStatusMessage(FString::Printf(TEXT("DEBUG F4: ADAM arena start. Adam=%d Removed=%d"),
        ExistingAdamCountAfter, RemovedEnemies), 4.0f);
#endif
}

void AEvaPrototypeGameMode::DebugRestorePlayer()
{
#if !UE_BUILD_SHIPPING
    if (!GetWorld())
    {
        return;
    }

    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    AEvaPlayerCharacter* Player = PlayerController ? Cast<AEvaPlayerCharacter>(PlayerController->GetPawn()) : nullptr;
    if (!Player)
    {
        ShowDebugStatusMessage(TEXT("DEBUG F5 failed: player pawn not found."), 4.0f);
        return;
    }

    Player->ResetForCheckpoint(Player->GetActorTransform());
    GetWorldTimerManager().ClearTimer(RespawnTimer);
    PlayerAwaitingRespawn = nullptr;
    if (PlayerController)
    {
        PlayerController->SetIgnoreMoveInput(false);
        PlayerController->SetIgnoreLookInput(false);
    }
    bGameOver = false;
    ShowDebugStatusMessage(TEXT("DEBUG F5: Player health and ammo restored."), 3.0f);
#endif
}

void AEvaPrototypeGameMode::DebugForceStageClear()
{
#if !UE_BUILD_SHIPPING
    if (CurrentDirector)
    {
        CurrentDirector->CompleteStage();
    }
    else
    {
        HandleStageClear();
    }
    ShowDebugStatusMessage(TEXT("DEBUG F6: Stage clear forced."), 5.0f);
#endif
}

void AEvaPrototypeGameMode::DebugPrintTelemetrySnapshot()
{
#if !UE_BUILD_SHIPPING
    if (!GetWorld())
    {
        return;
    }

    APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    const AEvaPlayerCharacter* Player = PlayerController ? Cast<AEvaPlayerCharacter>(PlayerController->GetPawn()) : nullptr;
    const UEvaPlayerTelemetryComponent* Telemetry = Player ? Player->GetTelemetryComponent() : nullptr;
    if (!Telemetry)
    {
        ShowDebugStatusMessage(TEXT("DEBUG F7 failed: telemetry component not found."), 4.0f);
        return;
    }

    const FEvaTelemetrySnapshot Snapshot = Telemetry->GetTelemetry();
    float AnalysisRate = 0.0f;
    if (const UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        if (const UEvaLearningSubsystem* Learning = GameInstance->GetSubsystem<UEvaLearningSubsystem>())
        {
            AnalysisRate = Learning->GetEvaAnalysisRate();
        }
    }

    const FString SnapshotText = FString::Printf(
        TEXT("DEBUG F7: Shots=%d Hits=%d HS=%d Kills=%d Acc=%.1f%% HSRate=%.1f%% AvgDist=%.0f EVA=%.0f%%"),
        Snapshot.ShotCount,
        Snapshot.HitCount,
        Snapshot.HeadshotCount,
        Snapshot.KillCount,
        Telemetry->GetAccuracy() * 100.0f,
        Telemetry->GetHeadshotRate() * 100.0f,
        Snapshot.AverageCombatDistance,
        AnalysisRate);
    ShowDebugStatusMessage(SnapshotText, 7.0f);
#endif
}

void AEvaPrototypeGameMode::DebugToggleNavigationVisualization()
{
#if !UE_BUILD_SHIPPING
    bNavigationDebugVisible = !bNavigationDebugVisible;
    bDebugHUDVisible = !bDebugHUDVisible;

    if (APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
    {
        PlayerController->ConsoleCommand(TEXT("Show Navigation"), true);
        PlayerController->ConsoleCommand(bNavigationDebugVisible ? TEXT("ShowFlag.Navigation 1") :
            TEXT("ShowFlag.Navigation 0"), true);
    }

    LogNavigationStatus(TEXT("DebugToggleNavigationVisualization"));
    ShowDebugStatusMessage(FString::Printf(TEXT("DEBUG F9/N: Debug HUD %s / Navigation visualization %s"),
        bDebugHUDVisible ? TEXT("ON") : TEXT("OFF"),
        bNavigationDebugVisible ? TEXT("ON") : TEXT("OFF")), 4.0f);
#endif
}
