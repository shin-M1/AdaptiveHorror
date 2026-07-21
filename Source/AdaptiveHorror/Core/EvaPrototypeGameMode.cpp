#include "Core/EvaPrototypeGameMode.h"
#include "AdaptiveHorror.h"
#include "AI/EvaHunterCharacter.h"
#include "AI/EvaAdamBossCharacter.h"
#include "AI/EvaLearningSubsystem.h"
#include "AI/EvaZombieAIController.h"
#include "AI/EvaZombieCharacter.h"
#include "Audio/EvaAudioFunctionLibrary.h"
#include "Camera/PlayerCameraManager.h"
#include "Characters/EvaPlayerCharacter.h"
#include "Characters/EvaPlayerController.h"
#include "Components/BoxComponent.h"
#include "Components/BrushComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/EvaHealthComponent.h"
#include "Components/EvaPlayerTelemetryComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/DirectionalLight.h"
#include "Engine/Engine.h"
#include "Engine/ExponentialHeightFog.h"
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
#include "HAL/IConsoleManager.h"
#include "InputCoreTypes.h"
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
#include "World/EvaFacilityInteractable.h"
#include "World/EvaFacilityZoneTrigger.h"
#include "World/EvaResearchFacilityDirector.h"
#include "Weapons/EvaWeaponBase.h"

namespace
{
    constexpr int32 SafeSpawnAttemptCount = 12;
    constexpr float DefaultEnemySeparation = 360.0f;
    constexpr float DefaultPlayerSeparation = 520.0f;
    constexpr float PresentationSafePlayerSeparation = 760.0f;
    constexpr float SpawnViewConeDotThreshold = 0.52f;
    constexpr float SpawnFrontalConeDotThreshold = 0.25f;
    constexpr float SpawnInteractionExclusionRadius = 300.0f;
    constexpr float SpawnCheckpointExclusionRadius = 700.0f;
    constexpr float ExpectedRuntimeFloorSurfaceZ = 25.0f;
    constexpr float RuntimeFloorHeightTolerance = 45.0f;
    constexpr float RuntimeFloorNormalMinZ = 0.9f;
    constexpr int32 MaxNavigationReadinessAttempts = 40;
    constexpr float NavigationReadinessInterval = 0.25f;

    const FName RuntimeFloorTag(TEXT("RuntimeFloor"));
    const FName EvaNavigableFloorTag(TEXT("EvaNavigableFloor"));

    constexpr int32 FacilityZoneCount = 6;
    constexpr float FacilityGateWallCenterY = 620.0f;
    constexpr float FacilityGateWallScaleY = 1.4f;
    constexpr float FacilityGateWallHalfY = FacilityGateWallScaleY * 50.0f;
    constexpr float FacilityGateOpeningHalfY = FacilityGateWallCenterY - FacilityGateWallHalfY;
    constexpr float FacilityGateWallOuterY = FacilityGateWallCenterY + FacilityGateWallHalfY;
    constexpr float FacilitySideWallScaleY = 0.35f;
    constexpr float FacilitySideWallHalfY = FacilitySideWallScaleY * 50.0f;
    constexpr float FacilityBoundaryWallOverlap = 35.0f;

    struct FFacilityBoundaryGeometry
    {
        float BridgeStartAbsY = 0.0f;
        float BridgeEndAbsY = 0.0f;
        float BridgeCenterAbsY = 0.0f;
        float BridgeScaleY = 0.0f;
        float SideWallCenterAbsY = 0.0f;
        float UnexpectedGapWidth = 0.0f;
        bool bClosedOutsideOpening = false;
    };

    FVector GetFacilityZoneCenter(const int32 ZoneIndex)
    {
        static const FVector ZoneCenters[] =
        {
            FVector(-4800.0f, 0.0f, 0.0f),
            FVector(-3000.0f, 0.0f, 0.0f),
            FVector(-1200.0f, 0.0f, 0.0f),
            FVector(600.0f, 0.0f, 0.0f),
            FVector(2400.0f, 0.0f, 0.0f),
            FVector(4200.0f, 0.0f, 0.0f)
        };
        return ZoneCenters[FMath::Clamp(ZoneIndex, 0, FacilityZoneCount - 1)];
    }

    FString GetFacilityZoneDisplayName(const int32 ZoneIndex)
    {
        switch (ZoneIndex)
        {
        case 0: return TEXT("Entry Lobby");
        case 1: return TEXT("Security Corridor");
        case 2: return TEXT("Observation Lab");
        case 3: return TEXT("Containment Ward");
        case 4: return TEXT("Data Core Room");
        case 5: return TEXT("Adam Arena");
        default: return TEXT("Unknown");
        }
    }

    FVector GetFacilityZoneFloorScale(const int32 ZoneIndex)
    {
        switch (ZoneIndex)
        {
        case 0: return FVector(18.0f, 21.0f, 0.5f); // Entry: wider lobby.
        case 1: return FVector(18.0f, 12.5f, 0.5f); // Security: still corridor-like.
        case 2: return FVector(18.0f, 19.0f, 0.5f); // Observation: room to circle equipment.
        case 3: return FVector(18.0f, 18.0f, 0.5f); // Containment: side cells plus cover.
        case 4: return FVector(18.0f, 19.0f, 0.5f); // Data Core: half-loop space.
        case 5: return FVector(24.0f, 24.0f, 0.5f); // Adam Arena: boss room.
        default: return FVector(18.0f, 14.0f, 0.5f);
        }
    }

    FFacilityBoundaryGeometry CalculateFacilityBoundaryGeometry(const float SideWallCenterAbsY)
    {
        FFacilityBoundaryGeometry Geometry;
        Geometry.SideWallCenterAbsY = SideWallCenterAbsY;
        Geometry.BridgeStartAbsY = FacilityGateWallOuterY - FacilityBoundaryWallOverlap;
        Geometry.BridgeEndAbsY = SideWallCenterAbsY + FacilitySideWallHalfY + FacilityBoundaryWallOverlap;
        Geometry.BridgeCenterAbsY = (Geometry.BridgeStartAbsY + Geometry.BridgeEndAbsY) * 0.5f;
        Geometry.BridgeScaleY = FMath::Max(0.1f, (Geometry.BridgeEndAbsY - Geometry.BridgeStartAbsY) / 100.0f);

        const float GapFromGateToBridge = FMath::Max(0.0f, Geometry.BridgeStartAbsY - FacilityGateWallOuterY);
        const float SideWallInnerAbsY = SideWallCenterAbsY - FacilitySideWallHalfY;
        const float GapFromBridgeToSideWall = FMath::Max(0.0f, SideWallInnerAbsY - Geometry.BridgeEndAbsY);
        Geometry.UnexpectedGapWidth = FMath::Max(GapFromGateToBridge, GapFromBridgeToSideWall);
        Geometry.bClosedOutsideOpening = Geometry.UnexpectedGapWidth <= KINDA_SMALL_NUMBER;
        return Geometry;
    }

    FString GetFacilityZoneShape(const int32 ZoneIndex)
    {
        switch (ZoneIndex)
        {
        case 0: return TEXT("Open");
        case 1: return TEXT("LShape");
        case 2: return TEXT("Central");
        case 3: return TEXT("Cell");
        case 4: return TEXT("Central");
        case 5: return TEXT("Arena");
        default: return TEXT("Linear");
        }
    }

    bool IsLivingEvaEnemy(const AEvaZombieCharacter* Enemy)
    {
        return Enemy && Enemy->GetHealthComponent() && !Enemy->GetHealthComponent()->IsDead();
    }

    FString BoolText(const bool bValue)
    {
        return bValue ? TEXT("true") : TEXT("false");
    }

    TAutoConsoleVariable<int32> CVarEvaReduceFlashing(
        TEXT("Eva.ReduceFlashing"),
        0,
        TEXT("Set to 1 to reduce emergency-light flicker intensity for comfort."));

    TAutoConsoleVariable<int32> CVarEvaReduceCameraShake(
        TEXT("Eva.ReduceCameraShake"),
        0,
        TEXT("Set to 1 to reduce prototype horror camera shake."));

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

    FString FacilityInteractableTypeToText(const EEvaFacilityInteractableType Type)
    {
        switch (Type)
        {
        case EEvaFacilityInteractableType::Keycard:
            return TEXT("Keycard");
        case EEvaFacilityInteractableType::LockedDoor:
            return TEXT("Door");
        case EEvaFacilityInteractableType::PowerConsole:
            return TEXT("PowerConsole");
        case EEvaFacilityInteractableType::ResearchLog:
            return TEXT("ResearchLog");
        case EEvaFacilityInteractableType::DataCoreConsole:
            return TEXT("DataCoreConsole");
        default:
            return TEXT("Unknown");
        }
    }

    FString CollisionEnabledToText(const ECollisionEnabled::Type CollisionEnabled)
    {
        switch (CollisionEnabled)
        {
        case ECollisionEnabled::NoCollision:
            return TEXT("NoCollision");
        case ECollisionEnabled::QueryOnly:
            return TEXT("QueryOnly");
        case ECollisionEnabled::PhysicsOnly:
            return TEXT("PhysicsOnly");
        case ECollisionEnabled::QueryAndPhysics:
            return TEXT("QueryAndPhysics");
        default:
            return TEXT("Unknown");
        }
    }

    FString CollisionResponseToText(const ECollisionResponse Response)
    {
        switch (Response)
        {
        case ECR_Ignore:
            return TEXT("Ignore");
        case ECR_Overlap:
            return TEXT("Overlap");
        case ECR_Block:
            return TEXT("Block");
        default:
            return TEXT("Unknown");
        }
    }

    FString CollisionObjectTypeToText(const ECollisionChannel Channel)
    {
        switch (Channel)
        {
        case ECC_WorldStatic:
            return TEXT("WorldStatic");
        case ECC_WorldDynamic:
            return TEXT("WorldDynamic");
        case ECC_Pawn:
            return TEXT("Pawn");
        case ECC_Visibility:
            return TEXT("Visibility");
        case ECC_Camera:
            return TEXT("Camera");
        default:
            return FString::Printf(TEXT("Channel%d"), static_cast<int32>(Channel));
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

    bool IsNearEvaInteractable(UWorld* World, const FVector& Location, const float Radius)
    {
        if (!World)
        {
            return false;
        }

        const float RadiusSq = FMath::Square(Radius);
        for (TActorIterator<AEvaFacilityInteractable> It(World); It; ++It)
        {
            const AEvaFacilityInteractable* Interactable = *It;
            if (IsValid(Interactable) && !Interactable->IsHidden() &&
                FVector::DistSquared(Location, Interactable->GetActorLocation()) <= RadiusSq)
            {
                return true;
            }
        }
        return false;
    }

    bool IsNavigationReachableAt(UWorld* World, const FVector& Location)
    {
        UNavigationSystemV1* NavigationSystem = World ? UNavigationSystemV1::GetCurrent(World) : nullptr;
        if (!NavigationSystem)
        {
            return false;
        }

        FNavLocation ProjectedLocation;
        return NavigationSystem->ProjectPointToNavigation(Location, ProjectedLocation,
            FVector(240.0f, 240.0f, 420.0f));
    }

    bool IsFloorValidAt(UWorld* World, const FVector& Location)
    {
        if (!World)
        {
            return false;
        }

        FHitResult FloorHit;
        const FVector TraceStart = Location + FVector(0.0f, 0.0f, 500.0f);
        const FVector TraceEnd = Location - FVector(0.0f, 0.0f, 900.0f);
        return World->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, ECC_WorldStatic) &&
            IsAcceptableEvaFloorHit(FloorHit);
    }
}

static FAutoConsoleCommandWithWorldAndArgs CCmdEvaDebugBlackout(
    TEXT("Eva.DebugBlackout"),
    TEXT("Triggers a short prototype blackout horror effect. Optional first argument is duration seconds."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic([](const TArray<FString>& Args, UWorld* World)
    {
        if (!World)
        {
            return;
        }

        float Duration = 3.5f;
        if (Args.Num() > 0)
        {
            Duration = FCString::Atof(*Args[0]);
        }
        if (AEvaPrototypeGameMode* GameMode = World->GetAuthGameMode<AEvaPrototypeGameMode>())
        {
            GameMode->TriggerBlackout(Duration, true);
        }
    }));

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

void AEvaPrototypeGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    StopAdaptationProfileUpdates();
    StopHorrorRuntimeEffects(true);
    Super::EndPlay(EndPlayReason);
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
    if (CurrentDirector)
    {
        CurrentDirector->CloseResearchLog();
    }
    SetGameFlowState(EEvaGameFlowState::PlayerDead);
    bGameOver = true;
    PlayerAwaitingRespawn = DeadPlayer;
    StopAdaptationProfileUpdates();
    StopHorrorRuntimeEffects(true);
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
    if (CurrentDirector)
    {
        CurrentDirector->CloseResearchLog();
    }
    StopHorrorRuntimeEffects(true);
    ClearStageClearTimers();
    StopAdaptationProfileUpdates();
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
    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        if (UEvaLearningSubsystem* Learning = GameInstance->GetSubsystem<UEvaLearningSubsystem>())
        {
            Learning->ResetLearning();
            Learning->SetProfileUpdatesEnabled(false);
        }
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
    StopHorrorRuntimeEffects(true);
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
            Learning->SetProfileUpdatesEnabled(true);
            Learning->UpdateAdaptationProfile(true);
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
    StartAdaptationProfileUpdates();
    BeginHorrorRuntimeEffects();
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

    if (CurrentDirector)
    {
        CurrentDirector->CloseResearchLog();
    }
    SetGameFlowState(EEvaGameFlowState::Paused);
    StopAdaptationProfileUpdates();
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
    StartAdaptationProfileUpdates();
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
    if (CurrentDirector)
    {
        CurrentDirector->CloseResearchLog();
    }
    ClearStageClearTimers();
    StopHorrorRuntimeEffects(true);
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
    BeginHorrorRuntimeEffects();
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
    if (CurrentDirector)
    {
        CurrentDirector->CloseResearchLog();
    }
    bStageClear = true;
    bGameOver = false;
    PlayerAwaitingRespawn = nullptr;
    StopAdaptationProfileUpdates();
    StopHorrorRuntimeEffects(true);
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
    StartAdaptationProfileUpdates();
    BeginHorrorRuntimeEffects();
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

    RuntimeFloorComponents.Reset();
    RuntimeFacilityBounds = FBox(EForceInit::ForceInit);
    SpawnedFacilityInteractableKeys.Reset();

    for (int32 ZoneIndex = 0; ZoneIndex < FacilityZoneCount; ++ZoneIndex)
    {
        BuildFacilityZone(GetFacilityZoneCenter(ZoneIndex), GetFacilityZoneDisplayName(ZoneIndex), ZoneIndex);
    }

    auto RegisterGeneratedFloor = [this](AActor* FloorActor)
    {
        if (FloorActor)
        {
            if (UStaticMeshComponent* MeshComponent = Cast<UStaticMeshComponent>(FloorActor->GetComponentByClass(
                UStaticMeshComponent::StaticClass())))
            {
                MeshComponent->ComponentTags.AddUnique(RuntimeFloorTag);
                MeshComponent->ComponentTags.AddUnique(EvaNavigableFloorTag);
                RegisterRuntimeFloorComponent(MeshComponent);
                return true;
            }
        }
        return false;
    };

    static const TCHAR* ZoneConnectionNames[] =
    {
        TEXT("Entry->Security"),
        TEXT("Security->Observation"),
        TEXT("Observation->Containment"),
        TEXT("Containment->DataCore"),
        TEXT("DataCore->Arena")
    };
    int32 BoundaryUnexpectedOpenings[FacilityZoneCount] = {};
    int32 BoundaryBridgeSegments[FacilityZoneCount] = {};

    for (int32 GateIndex = 0; GateIndex < 5; ++GateIndex)
    {
        const float GateX = -3900.0f + GateIndex * 1800.0f;
        const float ConnectorYScale = FMath::Max(GetFacilityZoneFloorScale(GateIndex).Y,
            GetFacilityZoneFloorScale(GateIndex + 1).Y);
        const float MaxSideWallY = ConnectorYScale * 50.0f + 60.0f;
        const FFacilityBoundaryGeometry BoundaryGeometry = CalculateFacilityBoundaryGeometry(MaxSideWallY);
        const bool bFloorConnected = RegisterGeneratedFloor(SpawnArenaBox(FVector(GateX, 0.0f, 0.0f),
            FVector(1.35f, ConnectorYScale, 0.55f)));
        const bool bWallA = SpawnTaggedArenaBox(FVector(GateX, FacilityGateWallCenterY, 160.0f),
            FVector(0.35f, FacilityGateWallScaleY, 3.2f), FName(TEXT("EvaGate"))) != nullptr;
        const bool bWallB = SpawnTaggedArenaBox(FVector(GateX, -FacilityGateWallCenterY, 160.0f),
            FVector(0.35f, FacilityGateWallScaleY, 3.2f), FName(TEXT("EvaGate"))) != nullptr;
        const bool bHeader = SpawnTaggedArenaBox(FVector(GateX, 0.0f, 360.0f), FVector(0.35f, 3.4f, 0.35f),
            FName(TEXT("EvaGate"))) != nullptr;
        const bool bNorthBoundaryBridge = SpawnTaggedArenaBox(
            FVector(GateX, BoundaryGeometry.BridgeCenterAbsY, 180.0f),
            FVector(0.35f, BoundaryGeometry.BridgeScaleY, 3.6f), FName(TEXT("EvaOuterBoundary"))) != nullptr;
        const bool bSouthBoundaryBridge = SpawnTaggedArenaBox(
            FVector(GateX, -BoundaryGeometry.BridgeCenterAbsY, 180.0f),
            FVector(0.35f, BoundaryGeometry.BridgeScaleY, 3.6f), FName(TEXT("EvaOuterBoundary"))) != nullptr;
        const int32 FloorSegments = bFloorConnected ? 1 : 0;
        const int32 WallSegments = (bWallA ? 1 : 0) + (bWallB ? 1 : 0) + (bHeader ? 1 : 0) +
            (bNorthBoundaryBridge ? 1 : 0) + (bSouthBoundaryBridge ? 1 : 0);
        const bool bBoundaryGeometryClosed = BoundaryGeometry.bClosedOutsideOpening;
        const bool bConnected = bFloorConnected && bWallA && bWallB && bHeader && bNorthBoundaryBridge &&
            bSouthBoundaryBridge && bBoundaryGeometryClosed;
        const int32 BridgeSegmentCount = (bNorthBoundaryBridge ? 1 : 0) + (bSouthBoundaryBridge ? 1 : 0);
        BoundaryBridgeSegments[GateIndex] += BridgeSegmentCount;
        BoundaryBridgeSegments[GateIndex + 1] += BridgeSegmentCount;
        if (!bNorthBoundaryBridge || !bSouthBoundaryBridge || !bBoundaryGeometryClosed)
        {
            ++BoundaryUnexpectedOpenings[GateIndex];
            ++BoundaryUnexpectedOpenings[GateIndex + 1];
        }
        UE_LOG(LogAdaptiveHorror, Log,
            TEXT("[ConnectionIntegrity] Link=%s Connected=%s GapDetected=%s FloorSegments=%d WallSegments=%d ConnectorX=%.0f ConnectorWidth=%.0f"),
            ZoneConnectionNames[GateIndex],
            *BoolText(bConnected),
            *BoolText(!bConnected),
            FloorSegments,
            WallSegments,
            GateX,
            ConnectorYScale * 100.0f);
        if (GateIndex <= 1)
        {
            UE_LOG(LogAdaptiveHorror, Log,
                TEXT("[BoundaryGeometry] Connection=%s LeftWallEnd=%.0f RightWallStart=%.0f OpeningMin=%.0f OpeningMax=%.0f ExpectedOpeningWidth=%.0f BridgeStartAbsY=%.0f BridgeEndAbsY=%.0f SideWallCenterAbsY=%.0f UnexpectedGapWidth=%.0f ClosedOutsideOpening=%s"),
                GateIndex == 0 ? TEXT("EntryToSecurity") : TEXT("SecurityToObservation"),
                -FacilityGateOpeningHalfY,
                FacilityGateOpeningHalfY,
                -FacilityGateOpeningHalfY,
                FacilityGateOpeningHalfY,
                FacilityGateOpeningHalfY * 2.0f,
                BoundaryGeometry.BridgeStartAbsY,
                BoundaryGeometry.BridgeEndAbsY,
                BoundaryGeometry.SideWallCenterAbsY,
                BoundaryGeometry.UnexpectedGapWidth,
                *BoolText(BoundaryGeometry.bClosedOutsideOpening && bNorthBoundaryBridge && bSouthBoundaryBridge));
        }
    }

    for (int32 ZoneIndex = 0; ZoneIndex < FacilityZoneCount; ++ZoneIndex)
    {
        const bool bBoundaryClosed = BoundaryUnexpectedOpenings[ZoneIndex] == 0;
        UE_LOG(LogAdaptiveHorror, Log,
            TEXT("[BoundaryIntegrity] Zone=%s OuterBoundaryClosed=%s UnexpectedOpenings=%d OuterSideWalls=2 BoundaryBridgeSegments=%d BoundaryBridgeAware=true"),
            *GetFacilityZoneDisplayName(ZoneIndex).Replace(TEXT(" "), TEXT("")),
            *BoolText(bBoundaryClosed),
            BoundaryUnexpectedOpenings[ZoneIndex],
            BoundaryBridgeSegments[ZoneIndex]);
    }

    const bool bZoneTrackingBoundsValid =
        GetFacilityZoneNameForLocation(GetFacilityZoneCenter(0)) == GetFacilityZoneDisplayName(0) &&
        GetFacilityZoneNameForLocation(GetFacilityZoneCenter(1)) == GetFacilityZoneDisplayName(1) &&
        GetFacilityZoneNameForLocation(GetFacilityZoneCenter(2)) == GetFacilityZoneDisplayName(2) &&
        GetFacilityZoneNameForLocation(GetFacilityZoneCenter(3)) == GetFacilityZoneDisplayName(3) &&
        GetFacilityZoneNameForLocation(GetFacilityZoneCenter(4)) == GetFacilityZoneDisplayName(4) &&
        GetFacilityZoneNameForLocation(GetFacilityZoneCenter(5)) == GetFacilityZoneDisplayName(5);
    UE_LOG(LogAdaptiveHorror, Log,
        TEXT("[ZoneTracking] ZoneBounds=%d BidirectionalTrackingEnabled=%s ObjectiveIndependent=%s BoundsValid=%s Source=GeneratedFacilityBounds"),
        FacilityZoneCount,
        FacilityZoneCount == 6 ? TEXT("true") : TEXT("false"),
        TEXT("true"),
        *BoolText(bZoneTrackingBoundsValid));

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

    SpawnFacilityInteractable(CurrentDirector, FVector(-3260.0f, 500.0f, 110.0f), FRotator(0.0f, -90.0f, 0.0f),
        EEvaFacilityInteractableType::PowerConsole, TEXT("POWER CONSOLE"));
    SpawnFacilityInteractable(CurrentDirector, FVector(-2750.0f, -240.0f, 96.0f), FRotator(0.0f, 25.0f, 0.0f),
        EEvaFacilityInteractableType::Keycard, TEXT("SECURITY KEYCARD"));
    SpawnFacilityInteractable(CurrentDirector, FVector(-2100.0f, 0.0f, 170.0f), FRotator::ZeroRotator,
        EEvaFacilityInteractableType::LockedDoor, TEXT("OBSERVATION LAB LOCK"));
    SpawnFacilityInteractable(CurrentDirector, FVector(-1200.0f, 520.0f, 86.0f), FRotator(0.0f, 180.0f, 0.0f),
        EEvaFacilityInteractableType::ResearchLog, TEXT("EVA LEARNING NOTES"),
        FName(TEXT("CONTENT_LOG_EVA")), TEXT("EVA Learning Notes"),
        TEXT("EVA correlates repeated player choices with survival outcomes. It is already classifying you."));
    SpawnFacilityInteractable(CurrentDirector, FVector(600.0f, 520.0f, 86.0f), FRotator(0.0f, 180.0f, 0.0f),
        EEvaFacilityInteractableType::ResearchLog, TEXT("HUNTER CONTAINMENT REPORT"),
        FName(TEXT("CONTENT_LOG_HUNTER")), TEXT("HUNTER Containment Report"),
        TEXT("HUNTER units record combat distance, hit bias, and escape routes with near-perfect fidelity."));
    SpawnFacilityInteractable(CurrentDirector, FVector(2400.0f, 520.0f, 86.0f), FRotator(0.0f, 180.0f, 0.0f),
        EEvaFacilityInteractableType::ResearchLog, TEXT("ADAM EXPERIMENT RECORD"),
        FName(TEXT("CONTENT_LOG_ADAM")), TEXT("Adam Experiment Record"),
        TEXT("ADAM is not a subject. It is EVA's preferred answer when observation alone is insufficient."));
    SpawnFacilityInteractable(CurrentDirector, FVector(2560.0f, -480.0f, 105.0f), FRotator(0.0f, 90.0f, 0.0f),
        EEvaFacilityInteractableType::DataCoreConsole, TEXT("DATA CORE CONSOLE"));
    LogFacilityInteractableSpawnStatus(TEXT("AfterAllFacilityInteractablesSpawned"));

    if (ADirectionalLight* DirectionalLight = GetWorld()->SpawnActor<ADirectionalLight>(
        FVector(-1600.0f, -2600.0f, 1800.0f), FRotator(-45.0f, 35.0f, 0.0f)))
    {
        if (UDirectionalLightComponent* LightComponent =
            Cast<UDirectionalLightComponent>(DirectionalLight->GetLightComponent()))
        {
            LightComponent->SetMobility(EComponentMobility::Movable);
            LightComponent->SetIntensity(1.35f);
            LightComponent->SetLightColor(FLinearColor(0.55f, 0.68f, 0.92f));
            RuntimeDirectionalLightComponent = LightComponent;
            RuntimeDirectionalLightBaseIntensity = 1.35f;
        }
    }

    if (ASkyLight* SkyLight = GetWorld()->SpawnActor<ASkyLight>(FVector(0.0f, 0.0f, 1600.0f), FRotator::ZeroRotator))
    {
        if (USkyLightComponent* SkyLightComponent = SkyLight->GetLightComponent())
        {
            SkyLightComponent->SetMobility(EComponentMobility::Movable);
            SkyLightComponent->SetIntensity(0.28f);
            SkyLightComponent->RecaptureSky();
            RuntimeSkyLightComponent = SkyLightComponent;
            RuntimeSkyLightBaseIntensity = 0.28f;
        }
    }

    if (APointLight* Light = GetWorld()->SpawnActor<APointLight>(FVector(0.0f, 0.0f, 900.0f), FRotator::ZeroRotator))
    {
        if (UPointLightComponent* PointLightComponent = Cast<UPointLightComponent>(Light->GetLightComponent()))
        {
            PointLightComponent->SetMobility(EComponentMobility::Movable);
            PointLightComponent->SetIntensity(4200.0f);
            PointLightComponent->SetAttenuationRadius(6200.0f);
            PointLightComponent->SetLightColor(FLinearColor(0.38f, 0.55f, 0.78f));
            RuntimeMainPointLightComponent = PointLightComponent;
            RuntimeMainPointLightBaseIntensity = 4200.0f;
        }
    }

    RuntimeEmergencyLightComponents.Reset();
    RuntimeEmergencyLightBaseIntensities.Reset();
    for (int32 ZoneIndex = 0; ZoneIndex < FacilityZoneCount; ++ZoneIndex)
    {
        const FVector EmergencyLightLocation(GetFacilityZoneCenter(ZoneIndex).X, GetFacilityZoneCenter(ZoneIndex).Y + 585.0f, 285.0f);
        if (APointLight* EmergencyLight = GetWorld()->SpawnActor<APointLight>(EmergencyLightLocation, FRotator::ZeroRotator))
        {
            if (UPointLightComponent* PointLightComponent = Cast<UPointLightComponent>(EmergencyLight->GetLightComponent()))
            {
                PointLightComponent->SetMobility(EComponentMobility::Movable);
                PointLightComponent->SetIntensity(1800.0f);
                PointLightComponent->SetAttenuationRadius(780.0f);
                PointLightComponent->SetLightColor(ZoneIndex == 5 ?
                    FLinearColor(1.0f, 0.12f, 0.04f) :
                    FLinearColor(0.85f, 0.08f, 0.04f));
                RuntimeEmergencyLightComponents.Add(PointLightComponent);
                RuntimeEmergencyLightBaseIntensities.Add(1800.0f);
            }
        }
    }

    SetFacilityPowerOnline(false);
    SpawnRuntimeFog();
    UEvaAudioFunctionLibrary::PlayPrototypeTone2D(this, 41.2f, 1.6f, 0.12f);

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
        LogFacilityInteractableSpawnStatus(TEXT("NavigationReady"));
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

FString AEvaPrototypeGameMode::GetFacilityZoneNameForLocation(const FVector& WorldLocation) const
{
    int32 BestZoneIndex = 0;
    float BestScore = TNumericLimits<float>::Max();
    for (int32 ZoneIndex = 0; ZoneIndex < FacilityZoneCount; ++ZoneIndex)
    {
        const FVector ZoneCenter = GetFacilityZoneCenter(ZoneIndex);
        const FVector ZoneScale = GetFacilityZoneFloorScale(ZoneIndex);
        const float HalfLengthX = ZoneScale.X * 50.0f;
        const float HalfWidthY = ZoneScale.Y * 50.0f;
        const float OutsideX = FMath::Max(0.0f, FMath::Abs(WorldLocation.X - ZoneCenter.X) - HalfLengthX);
        const float OutsideY = FMath::Max(0.0f, FMath::Abs(WorldLocation.Y - ZoneCenter.Y) - HalfWidthY);
        const float Score = OutsideX * OutsideX + OutsideY * OutsideY +
            FMath::Abs(WorldLocation.X - ZoneCenter.X) * 0.01f;
        if (Score < BestScore)
        {
            BestScore = Score;
            BestZoneIndex = ZoneIndex;
        }
    }

    return GetFacilityZoneDisplayName(BestZoneIndex);
}

void AEvaPrototypeGameMode::BuildFacilityZone(const FVector& Center, const FString& Label, const int32 ZoneIndex)
{
    const FVector FloorScale = GetFacilityZoneFloorScale(ZoneIndex);
    const float HalfWidth = FloorScale.Y * 50.0f;
    const float SideWallY = HalfWidth + 60.0f;
    const float WallHeightScale = ZoneIndex == 5 ? 4.3f : (ZoneIndex == 0 ? 3.9f : 3.6f);
    int32 ObstacleCount = 0;
    int32 LandmarkCount = 0;

    auto MarkObstacle = [&ObstacleCount](AActor* Actor)
    {
        if (Actor)
        {
            ++ObstacleCount;
        }
    };
    auto MarkLandmark = [&LandmarkCount](AActor* Actor)
    {
        if (Actor)
        {
            ++LandmarkCount;
        }
    };
    auto MarkObstacleLandmark = [&ObstacleCount, &LandmarkCount](AActor* Actor)
    {
        if (Actor)
        {
            ++ObstacleCount;
            ++LandmarkCount;
        }
    };

    AActor* Floor = SpawnArenaBox(FVector(Center.X, Center.Y, 0.0f), FloorScale);
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

    SpawnArenaBox(FVector(Center.X, SideWallY, WallHeightScale * 50.0f), FVector(FloorScale.X, 0.35f, WallHeightScale));
    SpawnArenaBox(FVector(Center.X, -SideWallY, WallHeightScale * 50.0f), FVector(FloorScale.X, 0.35f, WallHeightScale));

    MarkObstacle(SpawnTaggedArenaBox(FVector(Center.X - 360.0f, FMath::Min(500.0f, HalfWidth - 170.0f), 95.0f),
        FVector(2.0f, 1.0f, 1.9f), FName(TEXT("EvaCover")), FRotator(0.0f, 20.0f + ZoneIndex * 7.0f, 0.0f)));
    MarkObstacle(SpawnTaggedArenaBox(FVector(Center.X + 320.0f, -FMath::Min(500.0f, HalfWidth - 170.0f), 80.0f),
        FVector(1.4f, 2.0f, 1.6f), FName(TEXT("EvaCover")), FRotator(0.0f, -25.0f, 0.0f)));
    SpawnTaggedArenaBox(FVector(Center.X - 520.0f, -FMath::Max(260.0f, HalfWidth - 150.0f), 55.0f),
        FVector(0.5f, 0.5f, 0.5f), FName(TEXT("EvaEscapeRoute")));
    SpawnTaggedArenaBox(FVector(Center.X + 520.0f, FMath::Max(260.0f, HalfWidth - 150.0f), 55.0f),
        FVector(0.5f, 0.5f, 0.5f), FName(TEXT("EvaAmbushPoint")));
    SpawnTaggedArenaBox(FVector(Center.X, -FMath::Max(260.0f, HalfWidth - 110.0f), 70.0f),
        FVector(0.55f, 0.55f, 0.55f), FName(TEXT("EvaHideSpot")));

    switch (ZoneIndex)
    {
    case 0:
        MarkLandmark(SpawnTaggedArenaBox(FVector(Center.X - 520.0f, 600.0f, 75.0f),
            FVector(2.8f, 0.45f, 0.75f), FName(TEXT("EvaReceptionDesk"))));
        MarkLandmark(SpawnTaggedArenaBox(FVector(Center.X - 520.0f, 650.0f, 135.0f),
            FVector(1.5f, 0.25f, 0.25f), FName(TEXT("EvaReceptionDesk"))));
        break;
    case 1:
        MarkObstacleLandmark(SpawnTaggedArenaBox(FVector(Center.X - 410.0f, 160.0f, 105.0f),
            FVector(2.8f, 0.42f, 2.1f), FName(TEXT("EvaSecurityPartition"))));
        MarkObstacleLandmark(SpawnTaggedArenaBox(FVector(Center.X + 120.0f, -160.0f, 105.0f),
            FVector(2.6f, 0.42f, 2.1f), FName(TEXT("EvaSecurityPartition"))));
        break;
    case 2:
        MarkObstacleLandmark(SpawnTaggedArenaBox(FVector(Center.X, 0.0f, 115.0f),
            FVector(2.7f, 2.35f, 2.3f), FName(TEXT("EvaObservationEquipment"))));
        MarkLandmark(SpawnTaggedArenaBox(FVector(Center.X - 420.0f, SideWallY - 70.0f, 190.0f),
            FVector(1.4f, 0.12f, 2.4f), FName(TEXT("EvaObservationGlass"))));
        MarkLandmark(SpawnTaggedArenaBox(FVector(Center.X + 420.0f, SideWallY - 70.0f, 190.0f),
            FVector(1.4f, 0.12f, 2.4f), FName(TEXT("EvaObservationGlass"))));
        break;
    case 3:
        for (int32 CellIndex = 0; CellIndex < 3; ++CellIndex)
        {
            const float CellX = Center.X - 520.0f + CellIndex * 520.0f;
            MarkObstacleLandmark(SpawnTaggedArenaBox(FVector(CellX, SideWallY - 110.0f, 120.0f),
                FVector(1.65f, 0.18f, 2.4f), FName(TEXT("EvaContainmentCell"))));
            MarkObstacleLandmark(SpawnTaggedArenaBox(FVector(CellX, -SideWallY + 110.0f, 120.0f),
                FVector(1.65f, 0.18f, 2.4f), FName(TEXT("EvaContainmentCell"))));
        }
        MarkObstacle(SpawnTaggedArenaBox(FVector(Center.X + 360.0f, -140.0f, 70.0f),
            FVector(1.4f, 0.9f, 1.4f), FName(TEXT("EvaCover")), FRotator(0.0f, -35.0f, 0.0f)));
        break;
    case 4:
        MarkObstacleLandmark(SpawnTaggedArenaBox(FVector(Center.X, 0.0f, 170.0f),
            FVector(1.55f, 1.55f, 3.4f), FName(TEXT("EvaDataCore"))));
        MarkObstacle(SpawnTaggedArenaBox(FVector(Center.X - 300.0f, 320.0f, 70.0f),
            FVector(1.3f, 0.55f, 1.4f), FName(TEXT("EvaDataCoreHalfLoop"))));
        MarkObstacle(SpawnTaggedArenaBox(FVector(Center.X, 410.0f, 70.0f),
            FVector(1.3f, 0.55f, 1.4f), FName(TEXT("EvaDataCoreHalfLoop"))));
        MarkObstacle(SpawnTaggedArenaBox(FVector(Center.X + 300.0f, 320.0f, 70.0f),
            FVector(1.3f, 0.55f, 1.4f), FName(TEXT("EvaDataCoreHalfLoop"))));
        break;
    case 5:
        MarkLandmark(SpawnTaggedArenaBox(FVector(Center.X - 720.0f, 0.0f, 210.0f),
            FVector(0.28f, 5.6f, 0.35f), FName(TEXT("EvaAdamArenaGate"))));
        MarkObstacle(SpawnTaggedArenaBox(FVector(Center.X - 520.0f, 780.0f, 115.0f),
            FVector(1.0f, 1.0f, 2.3f), FName(TEXT("EvaArenaPillar"))));
        MarkObstacle(SpawnTaggedArenaBox(FVector(Center.X + 520.0f, -780.0f, 115.0f),
            FVector(1.0f, 1.0f, 2.3f), FName(TEXT("EvaArenaPillar"))));
        break;
    default:
        break;
    }

    if (ZoneIndex == 0)
    {
        SpawnArenaBox(FVector(Center.X - 880.0f, 0.0f, WallHeightScale * 50.0f),
            FVector(0.35f, FloorScale.Y, WallHeightScale));
    }
    if (ZoneIndex == 5)
    {
        SpawnArenaBox(FVector(Center.X + 880.0f, 0.0f, WallHeightScale * 50.0f),
            FVector(0.35f, FloorScale.Y, WallHeightScale));
    }

    AActor* LabelMarker = SpawnArenaBox(FVector(Center.X, 0.0f, 25.0f), FVector(0.35f, 0.35f, 0.08f));
    if (LabelMarker)
    {
        LabelMarker->Tags.Add(FName(*Label));
        LabelMarker->SetActorHiddenInGame(true);
        LabelMarker->SetActorEnableCollision(false);
    }

    UE_LOG(LogAdaptiveHorror, Log,
        TEXT("[ZoneIdentity] ZoneIndex=%d Zone=\"%s\" ZoneShape=%s FloorArea=%.0f ObstacleCount=%d LandmarkCount=%d AverageWidth=%.0f AverageHeight=%.0f"),
        ZoneIndex,
        *Label,
        *GetFacilityZoneShape(ZoneIndex),
        FloorScale.X * FloorScale.Y * 10000.0f,
        ObstacleCount,
        LandmarkCount,
        FloorScale.Y * 100.0f,
        WallHeightScale * 100.0f);
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

AEvaFacilityInteractable* AEvaPrototypeGameMode::SpawnFacilityInteractable(AEvaResearchFacilityDirector* Director,
    const FVector& Location, const FRotator& Rotation, const EEvaFacilityInteractableType Type,
    const FString& DisplayName, const FName LogId, const FString& LogTitle, const FString& LogBody)
{
    if (!GetWorld())
    {
        return nullptr;
    }

    const FString TypeText = FacilityInteractableTypeToText(Type);
    const FString EffectiveTitle = LogTitle.IsEmpty() ? DisplayName : LogTitle;
    const FString KeyName = LogId.IsNone() ? DisplayName : LogId.ToString();
    const FName SpawnKey(*FString::Printf(TEXT("%s_%s"), *TypeText, *KeyName));
    if (SpawnedFacilityInteractableKeys.Contains(SpawnKey))
    {
        UE_LOG(LogAdaptiveHorror, Warning,
            TEXT("[ContentSpawn] Type=%s Title=%s Location=%s Skipped=Duplicate Key=%s"),
            *TypeText,
            *EffectiveTitle,
            *Location.ToCompactString(),
            *SpawnKey.ToString());
        return nullptr;
    }

    AEvaFacilityInteractable* Interactable = GetWorld()->SpawnActor<AEvaFacilityInteractable>(
        AEvaFacilityInteractable::StaticClass(), Location, Rotation);
    if (Interactable)
    {
        SpawnedFacilityInteractableKeys.Add(SpawnKey);
        Interactable->ConfigureInteractable(Type, Director, DisplayName, LogId, LogTitle, LogBody);

        const bool bFloorValid = IsFloorValidAt(GetWorld(), Location);
        const bool bReachable = IsNavigationReachableAt(GetWorld(), Location);
        UE_LOG(LogAdaptiveHorror, Log,
            TEXT("[ContentSpawn] Type=%s Title=%s Location=%s FloorValid=%s Reachable=%s Key=%s"),
            *TypeText,
            *EffectiveTitle,
            *Location.ToCompactString(),
            *BoolText(bFloorValid),
            *BoolText(bReachable),
            *SpawnKey.ToString());
    }
    else
    {
        UE_LOG(LogAdaptiveHorror, Warning,
            TEXT("[ContentSpawn] Type=%s Title=%s Location=%s Failed=SpawnActorNull"),
            *TypeText,
            *EffectiveTitle,
            *Location.ToCompactString());
    }
    return Interactable;
}

void AEvaPrototypeGameMode::LogFacilityInteractableSpawnStatus(const FString& Context) const
{
    if (!GetWorld())
    {
        return;
    }

    int32 ResearchLogCount = 0;
    int32 RegisteredCount = 0;
    for (TActorIterator<AEvaFacilityInteractable> It(GetWorld()); It; ++It)
    {
        const AEvaFacilityInteractable* Interactable = *It;
        if (!IsValid(Interactable))
        {
            continue;
        }
        ++RegisteredCount;
    }

    const AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
    for (TActorIterator<AEvaFacilityInteractable> It(GetWorld()); It; ++It)
    {
        const AEvaFacilityInteractable* Interactable = *It;
        if (!IsValid(Interactable))
        {
            continue;
        }

        if (Interactable->GetInteractableType() == EEvaFacilityInteractableType::ResearchLog)
        {
            ++ResearchLogCount;
        }

        const bool bFloorValid = IsFloorValidAt(GetWorld(), Interactable->GetActorLocation());
        const bool bReachable = IsNavigationReachableAt(GetWorld(), Interactable->GetActorLocation());
        const UStaticMeshComponent* MeshComponent = Interactable->GetVisualComponentForDebug();
        const UPrimitiveComponent* TraceComponent = Interactable->GetInteractionCollisionComponentForDebug();
        const UPrimitiveComponent* CollisionComponent = TraceComponent ? TraceComponent : MeshComponent;
        const UStaticMesh* MeshAsset = MeshComponent ? MeshComponent->GetStaticMesh() : nullptr;
        const FString PromptText = Player ? Interactable->GetInteractionPrompt(Player) : FString(TEXT("NoPlayer"));
        const float InteractionDistance = Player ?
            FVector::Dist(Player->GetActorLocation(), Interactable->GetInteractionTraceLocation()) : -1.0f;
        const bool bMeshVisible = Interactable->IsMeshVisibleForDebug();
        const bool bInteractionEnabled = Interactable->IsInteractionCollisionEnabledForDebug();
        const bool bVisibilityBlock = CollisionComponent &&
            CollisionComponent->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Block;
        const FString ComponentBounds = CollisionComponent ?
            CollisionComponent->Bounds.GetBox().ToString() : FString(TEXT("None"));

        UE_LOG(LogAdaptiveHorror, Log,
            TEXT("[InteractableSpawn] Context=%s ActorName=%s ActorClass=%s Type=%s WorldLocation=%s MeshComponentValid=%s MeshAsset=%s MeshWorldLocation=%s MeshWorldScale=%s MeshVisible=%s HiddenInGame=%s ActorHidden=%s CollisionEnabled=%s CollisionObjectType=%s VisibilityResponse=%s CameraResponse=%s PawnResponse=%s ComponentBounds=%s PromptText=\"%s\" InteractionDistance=%.1f InteractableEnabled=%s InteractionEnabled=%s VisibilityBlock=%s RegisteredCount=%d FloorValid=%s Reachable=%s"),
            *Context,
            *Interactable->GetName(),
            *Interactable->GetClass()->GetName(),
            *FacilityInteractableTypeToText(Interactable->GetInteractableType()),
            *Interactable->GetActorLocation().ToCompactString(),
            *BoolText(MeshComponent != nullptr),
            MeshAsset ? *MeshAsset->GetPathName() : TEXT("None"),
            MeshComponent ? *MeshComponent->GetComponentLocation().ToCompactString() : TEXT("None"),
            MeshComponent ? *MeshComponent->GetComponentScale().ToCompactString() : TEXT("None"),
            *BoolText(bMeshVisible),
            MeshComponent ? *BoolText(MeshComponent->bHiddenInGame) : TEXT("None"),
            *BoolText(Interactable->IsHidden()),
            CollisionComponent ? *CollisionEnabledToText(CollisionComponent->GetCollisionEnabled()) : TEXT("None"),
            CollisionComponent ? *CollisionObjectTypeToText(CollisionComponent->GetCollisionObjectType()) : TEXT("None"),
            CollisionComponent ? *CollisionResponseToText(CollisionComponent->GetCollisionResponseToChannel(ECC_Visibility)) : TEXT("None"),
            CollisionComponent ? *CollisionResponseToText(CollisionComponent->GetCollisionResponseToChannel(ECC_Camera)) : TEXT("None"),
            CollisionComponent ? *CollisionResponseToText(CollisionComponent->GetCollisionResponseToChannel(ECC_Pawn)) : TEXT("None"),
            *ComponentBounds,
            *PromptText,
            InteractionDistance,
            *BoolText(Interactable->CanInteract(Player)),
            *BoolText(bInteractionEnabled),
            *BoolText(bVisibilityBlock),
            RegisteredCount,
            *BoolText(bFloorValid),
            *BoolText(bReachable));

        if (Interactable->GetInteractableType() == EEvaFacilityInteractableType::Keycard)
        {
            UE_LOG(LogAdaptiveHorror, Log,
                TEXT("[InteractableSpawn] Keycard MeshVisible=%s InteractionEnabled=%s VisibilityBlock=%s PromptText=\"%s\" Distance=%.1f"),
                *BoolText(bMeshVisible),
                *BoolText(bInteractionEnabled),
                *BoolText(bVisibilityBlock),
                *PromptText,
                InteractionDistance);
        }
        else if (Interactable->GetInteractableType() == EEvaFacilityInteractableType::ResearchLog)
        {
            UE_LOG(LogAdaptiveHorror, Log,
                TEXT("[InteractableSpawn] ResearchLog Title=%s MeshVisible=%s InteractionEnabled=%s VisibilityBlock=%s PromptText=\"%s\" Distance=%.1f"),
                *Interactable->GetDisplayName(),
                *BoolText(bMeshVisible),
                *BoolText(bInteractionEnabled),
                *BoolText(bVisibilityBlock),
                *PromptText,
                InteractionDistance);
        }

#if !UE_BUILD_SHIPPING
        if (Interactable->GetInteractableType() == EEvaFacilityInteractableType::Keycard ||
            Interactable->GetInteractableType() == EEvaFacilityInteractableType::ResearchLog)
        {
            const FColor DebugColor = Interactable->GetInteractableType() == EEvaFacilityInteractableType::Keycard ?
                FColor::Cyan : FColor::Orange;
            DrawDebugSphere(GetWorld(), Interactable->GetInteractionTraceLocation(), 38.0f, 12,
                DebugColor, false, 8.0f, 0, 2.0f);
        }
#endif

        UE_LOG(LogAdaptiveHorror, Log,
            TEXT("[ContentSpawn] Context=%s Type=%s Title=%s Location=%s FloorValid=%s Reachable=%s Hidden=%s Collision=%s"),
            *Context,
            *FacilityInteractableTypeToText(Interactable->GetInteractableType()),
            *Interactable->GetDisplayName(),
            *Interactable->GetActorLocation().ToCompactString(),
            *BoolText(bFloorValid),
            *BoolText(bReachable),
            *BoolText(Interactable->IsHidden()),
            *BoolText(Interactable->GetActorEnableCollision()));
        if (Interactable->GetInteractableType() == EEvaFacilityInteractableType::LockedDoor)
        {
            UE_LOG(LogAdaptiveHorror, Log,
                TEXT("[ContentSpawn] Context=%s DoorLockedCollision=%s DoorOpen=%s Location=%s"),
                *Context,
                *BoolText(Interactable->GetActorEnableCollision()),
                CurrentDirector && CurrentDirector->IsObservationDoorOpen() ? TEXT("true") : TEXT("false"),
                *Interactable->GetActorLocation().ToCompactString());
        }
    }

    UE_LOG(LogAdaptiveHorror, Log,
        TEXT("[ContentSpawn] Context=%s ResearchLogCount=%d RequiredResearchLogCount=3"),
        *Context,
        ResearchLogCount);
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
    const float MaxRadius, const float MinEnemySeparation, const float MinPlayerDistance, FVector& OutLocation,
    const bool bAvoidPlayerView) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }
    if (GameFlowState != EEvaGameFlowState::Playing || bGameOver || bStageClear)
    {
        UE_LOG(LogAdaptiveHorror, Log,
            TEXT("[SpawnAttempt] Type=Enemy Reason=SafeSearch Skipped=InactiveFlow Flow=%s GameOver=%s StageClear=%s Origin=%s"),
            *UEnum::GetValueAsString(GameFlowState),
            *BoolText(bGameOver),
            *BoolText(bStageClear),
            *Origin.ToCompactString());
        return false;
    }

    APlayerController* PlayerController = World->GetFirstPlayerController();
    const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    const FVector PlayerLocation = PlayerPawn ? PlayerPawn->GetActorLocation() : Origin;
    const APlayerCameraManager* CameraManager = PlayerController ? PlayerController->PlayerCameraManager : nullptr;
    const FVector CameraLocation = CameraManager ? CameraManager->GetCameraLocation() :
        (PlayerLocation + FVector(0.0f, 0.0f, 70.0f));
    const FVector CameraForward = CameraManager ? CameraManager->GetActorForwardVector().GetSafeNormal() :
        (PlayerPawn ? PlayerPawn->GetActorForwardVector().GetSafeNormal() : FVector::ForwardVector);
    const FVector PlayerForward = PlayerPawn ? PlayerPawn->GetActorForwardVector().GetSafeNormal2D() :
        CameraForward.GetSafeNormal2D();
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
    const float EffectivePlayerSeparation = bAvoidPlayerView ?
        FMath::Max(PlayerSeparation, PresentationSafePlayerSeparation) : PlayerSeparation;

    for (int32 AttemptIndex = 0; AttemptIndex < SafeSpawnAttemptCount; ++AttemptIndex)
    {
        FVector Candidate = Origin;
        if (AttemptIndex == 0 && PlayerPawn)
        {
            Candidate = PlayerLocation - PlayerForward * FMath::Clamp((RadiusMin + RadiusMax) * 0.5f, 520.0f, 900.0f);
        }
        else if (AttemptIndex == 1 && PlayerPawn)
        {
            Candidate = PlayerLocation - PlayerForward * RadiusMin + PlayerRight * 420.0f;
        }
        else if (AttemptIndex == 2 && PlayerPawn)
        {
            Candidate = PlayerLocation - PlayerForward * RadiusMin - PlayerRight * 420.0f;
        }
        else if (AttemptIndex == 3 && PlayerPawn)
        {
            Candidate = PlayerLocation + PlayerRight * FMath::Clamp((RadiusMin + RadiusMax) * 0.5f, 520.0f, 900.0f);
        }
        else if (AttemptIndex == 4 && PlayerPawn)
        {
            Candidate = PlayerLocation - PlayerRight * FMath::Clamp((RadiusMin + RadiusMax) * 0.5f, 520.0f, 900.0f);
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
        const bool bPassesPlayerDistance = !PlayerPawn || PlayerDistance >= EffectivePlayerSeparation;
        const bool bPassesEnemyDistance = NearestEnemyDistance >= EnemySeparation;
        const bool bNearPlayerStart = FVector::DistSquared(FinalLocation, LastCheckpointTransform.GetLocation()) <=
            FMath::Square(SpawnCheckpointExclusionRadius);
        const bool bNearInteractable = IsNearEvaInteractable(World, FinalLocation, SpawnInteractionExclusionRadius);

        bool bInCameraCone = false;
        bool bInFrontalCone = false;
        bool bDirectlyVisible = false;
        FString VisibilityHitName = TEXT("None");
        if (PlayerPawn && bAvoidPlayerView)
        {
            const FVector DirectionFromCamera = (FinalLocation + FVector(0.0f, 0.0f, 45.0f) -
                CameraLocation).GetSafeNormal();
            const float CameraDot = FVector::DotProduct(CameraForward, DirectionFromCamera);
            const float PlayerForwardDot = FVector::DotProduct(PlayerForward,
                (FinalLocation - PlayerLocation).GetSafeNormal2D());
            bInCameraCone = CameraDot >= SpawnViewConeDotThreshold;
            bInFrontalCone = PlayerForwardDot >= SpawnFrontalConeDotThreshold &&
                PlayerDistance <= FMath::Max(1100.0f, RadiusMax + 240.0f);

            FHitResult VisibilityHit;
            FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EvaSpawnVisibility), false, PlayerPawn);
            QueryParams.AddIgnoredActor(PlayerPawn);
            const bool bVisibilityBlocked = World->LineTraceSingleByChannel(VisibilityHit, CameraLocation,
                FinalLocation + FVector(0.0f, 0.0f, 60.0f), ECC_Visibility, QueryParams);
            VisibilityHitName = VisibilityHit.GetActor() ? VisibilityHit.GetActor()->GetName() : TEXT("None");
            bDirectlyVisible = !bVisibilityBlocked;
        }
        const bool bPassesPresentation = !bAvoidPlayerView ||
            (!bInCameraCone && !bInFrontalCone && !bDirectlyVisible && !bNearInteractable && !bNearPlayerStart);
        const bool bSpawnEligible = bNavProjected && bFloorAccepted && !bHasOverlap && bPassesPlayerDistance &&
            bPassesEnemyDistance && bPassesPresentation;

        UE_LOG(LogAdaptiveHorror, Log,
            TEXT("[SpawnAttempt] Type=Enemy Reason=SafeSearch Attempt=%d Requested=%s NavLocation=%s Final=%s NavProjected=%s FloorTrace=%s FloorAccepted=%s FloorActor=%s FloorComponent=%s FloorZ=%.1f NormalZ=%.2f Overlap=%s PlayerDistance=%.1f MinPlayerDistance=%.1f TooClose=%s NearestEnemyDistance=%.1f InView=%s Frontal=%s Visible=%s VisibilityHit=%s NearInteractable=%s NearPlayerStart=%s AvoidView=%s SpawnEligible=%s"),
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
            EffectivePlayerSeparation,
            *BoolText(!bPassesPlayerDistance),
            NearestEnemyDistance == TNumericLimits<float>::Max() ? -1.0f : NearestEnemyDistance,
            *BoolText(bInCameraCone),
            *BoolText(bInFrontalCone),
            *BoolText(bDirectlyVisible),
            *VisibilityHitName,
            *BoolText(bNearInteractable),
            *BoolText(bNearPlayerStart),
            *BoolText(bAvoidPlayerView),
            *BoolText(bSpawnEligible));

#if !UE_BUILD_SHIPPING
        DrawDebugSphere(World, FinalLocation, 42.0f, 12,
            bSpawnEligible ?
                FColor::Green : FColor::Red,
            false, 4.0f);
#endif

        if (bSpawnEligible)
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
    const bool bAvoidPlayerView = SpawnReason != TEXT("InitialEntryLobby") &&
        SpawnReason != TEXT("InitialVisibleZombie") &&
        SpawnReason != TEXT("AdamEncounter");
    const bool bFoundSafeLocation = FindSafeEnemySpawnLocation(Origin, MinRadius, MaxRadius,
        DefaultEnemySeparation, DefaultPlayerSeparation, FinalLocation, bAvoidPlayerView);
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
    if (bAvoidPlayerView)
    {
        UEvaAudioFunctionLibrary::PlayPrototypeToneAtLocation(this, FinalLocation, 46.0f, 0.35f, 0.10f);
        TWeakObjectPtr<AEvaZombieCharacter> WeakEnemy = Enemy;
        FTimerHandle DelayedPrimeHandle;
        GetWorldTimerManager().SetTimer(DelayedPrimeHandle, FTimerDelegate::CreateWeakLambda(this,
            [this, WeakEnemy]()
            {
                if (AEvaZombieCharacter* DelayedEnemy = WeakEnemy.Get())
                {
                    PrimeEnemyForPlayer(DelayedEnemy);
                }
            }), 0.35f, false);
    }
    else
    {
        PrimeEnemyForPlayer(Enemy);
    }

    const bool bPostSpawnOverlap = HasBlockingSpawnOverlap(GetWorld(), Enemy->GetActorLocation(), 56.0f, 110.0f);
    if (bPostSpawnOverlap)
    {
        FVector RetryLocation;
        if (FindSafeEnemySpawnLocation(Origin, MinRadius, MaxRadius + 240.0f, DefaultEnemySeparation,
            DefaultPlayerSeparation, RetryLocation, bAvoidPlayerView))
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
        ZombieController->ApplyCurrentGameplayAdaptation(true);
        ZombieController->EnsureCurrentActionIntent();
        Enemy->RefreshDebugIntentDisplay(true);
        UE_LOG(LogAdaptiveHorror, Log, TEXT("[AI] Enemy primed for pursuit Enemy=%s Controller=%s Target=%s Distance=%.1f"),
            *Enemy->GetName(),
            *ZombieController->GetName(),
            *PlayerPawn->GetName(),
            FVector::Dist(Enemy->GetActorLocation(), PlayerPawn->GetActorLocation()));
    }
    else
    {
        Enemy->RefreshDebugIntentDisplay(true);
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

void AEvaPrototypeGameMode::StartAdaptationProfileUpdates()
{
    if (!GetWorld() || !IsGameplayActive())
    {
        return;
    }

    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        if (UEvaLearningSubsystem* Learning = GameInstance->GetSubsystem<UEvaLearningSubsystem>())
        {
            Learning->SetProfileUpdatesEnabled(true);
            Learning->UpdateAdaptationProfile(true);
        }
    }

    if (!GetWorldTimerManager().IsTimerActive(AdaptationProfileTimer))
    {
        GetWorldTimerManager().SetTimer(AdaptationProfileTimer, this,
            &AEvaPrototypeGameMode::UpdateAdaptationProfileForGameplay,
            AdaptationProfileUpdateInterval, true, AdaptationProfileUpdateInterval);
    }
}

void AEvaPrototypeGameMode::StopAdaptationProfileUpdates()
{
    if (GetWorld())
    {
        GetWorldTimerManager().ClearTimer(AdaptationProfileTimer);
    }
    if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
    {
        if (UEvaLearningSubsystem* Learning = GameInstance->GetSubsystem<UEvaLearningSubsystem>())
        {
            Learning->SetProfileUpdatesEnabled(false);
        }
    }
}

void AEvaPrototypeGameMode::UpdateAdaptationProfileForGameplay()
{
    if (!GetWorld() || !IsGameplayActive() || UGameplayStatics::IsGamePaused(GetWorld()))
    {
        return;
    }

    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        if (UEvaLearningSubsystem* Learning = GameInstance->GetSubsystem<UEvaLearningSubsystem>())
        {
            Learning->UpdateAdaptationProfile(false);
        }
    }
}

void AEvaPrototypeGameMode::SyncEnemyDebugIntentDisplays(const bool bForceLog) const
{
#if !UE_BUILD_SHIPPING
    if (!GetWorld())
    {
        return;
    }

    for (TActorIterator<AEvaZombieCharacter> It(GetWorld()); It; ++It)
    {
        AEvaZombieCharacter* Enemy = *It;
        if (!Enemy)
        {
            continue;
        }
        if (AEvaZombieAIController* ZombieController = Cast<AEvaZombieAIController>(Enemy->GetController()))
        {
            ZombieController->EnsureCurrentActionIntent();
        }
        Enemy->RefreshDebugIntentDisplay(bForceLog);
    }
#else
    (void)bForceLog;
#endif
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
    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        if (UEvaLearningSubsystem* Learning = GameInstance->GetSubsystem<UEvaLearningSubsystem>())
        {
            Learning->UpdateAdaptationProfile(false);
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
    TriggerHunterArrivalEffect(Hunter->GetActorLocation());
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
    GetWorldTimerManager().ClearTimer(AdaptationProfileTimer);
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

bool AEvaPrototypeGameMode::CanRunHorrorEffect(const bool bAllowDuringPaused) const
{
    if (!GetWorld() || bGameOver || bStageClear)
    {
        return false;
    }

    return GameFlowState == EEvaGameFlowState::Playing ||
        (bAllowDuringPaused && GameFlowState == EEvaGameFlowState::Paused);
}

void AEvaPrototypeGameMode::BeginHorrorRuntimeEffects()
{
    if (!GetWorld() || !CanRunHorrorEffect(false))
    {
        return;
    }

    if (!GetWorldTimerManager().IsTimerActive(EmergencyLightFlickerTimer))
    {
        const float FlickerInterval = CVarEvaReduceFlashing.GetValueOnGameThread() != 0 ? 0.32f : 0.16f;
        GetWorldTimerManager().SetTimer(EmergencyLightFlickerTimer, this,
            &AEvaPrototypeGameMode::UpdateEmergencyLightFlicker, FlickerInterval, true);
    }
    if (!GetWorldTimerManager().IsTimerActive(AmbientPulseTimer))
    {
        GetWorldTimerManager().SetTimer(AmbientPulseTimer, this,
            &AEvaPrototypeGameMode::PlayAmbientPulse, 7.5f, true, 2.0f);
    }
}

void AEvaPrototypeGameMode::StopHorrorRuntimeEffects(const bool bRestoreLighting)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(EmergencyLightFlickerTimer);
        World->GetTimerManager().ClearTimer(BlackoutTimer);
        World->GetTimerManager().ClearTimer(AmbientPulseTimer);
    }

    bBlackoutActive = false;
    BlackoutEndTime = -1000.0f;
    HorrorPulseEndTime = -1000.0f;
    LastHorrorWarningText.Empty();
    LastAdamEntranceEffectActor.Reset();
    if (bRestoreLighting)
    {
        RestoreHorrorLighting();
    }
}

void AEvaPrototypeGameMode::RestoreHorrorLighting()
{
    const float PowerMultiplier = bFacilityPowerOnline ? 1.0f : 0.22f;
    if (RuntimeDirectionalLightComponent)
    {
        RuntimeDirectionalLightComponent->SetIntensity(RuntimeDirectionalLightBaseIntensity * (bFacilityPowerOnline ? 1.0f : 0.55f));
    }
    if (RuntimeSkyLightComponent)
    {
        RuntimeSkyLightComponent->SetIntensity(RuntimeSkyLightBaseIntensity * (bFacilityPowerOnline ? 1.0f : 0.45f));
    }
    if (RuntimeMainPointLightComponent)
    {
        RuntimeMainPointLightComponent->SetIntensity(RuntimeMainPointLightBaseIntensity * PowerMultiplier);
    }

    for (int32 Index = 0; Index < RuntimeEmergencyLightComponents.Num(); ++Index)
    {
        if (UPointLightComponent* LightComponent = RuntimeEmergencyLightComponents[Index])
        {
            const float BaseIntensity = RuntimeEmergencyLightBaseIntensities.IsValidIndex(Index) ?
                RuntimeEmergencyLightBaseIntensities[Index] : 1800.0f;
            LightComponent->SetIntensity(BaseIntensity * (bFacilityPowerOnline ? 1.0f : 0.55f));
        }
    }
}

void AEvaPrototypeGameMode::TriggerBlackout(const float Duration, const bool bForce)
{
    if (!CanRunHorrorEffect(false))
    {
        return;
    }
    if (bBlackoutActive && !bForce)
    {
        return;
    }

    bBlackoutActive = true;
    const float ClampedDuration = FMath::Clamp(Duration, 0.4f, 8.0f);
    BlackoutEndTime = GetWorld()->GetTimeSeconds() + ClampedDuration;
    HorrorPulseEndTime = FMath::Max(HorrorPulseEndTime, BlackoutEndTime);

    if (RuntimeDirectionalLightComponent)
    {
        RuntimeDirectionalLightComponent->SetIntensity(RuntimeDirectionalLightBaseIntensity * 0.12f);
    }
    if (RuntimeSkyLightComponent)
    {
        RuntimeSkyLightComponent->SetIntensity(RuntimeSkyLightBaseIntensity * 0.18f);
    }
    if (RuntimeMainPointLightComponent)
    {
        RuntimeMainPointLightComponent->SetIntensity(RuntimeMainPointLightBaseIntensity *
            (bFacilityPowerOnline ? 0.10f : 0.04f));
    }

    SetHorrorWarning(TEXT("FACILITY POWER DROP"), FMath::Min(ClampedDuration, 3.0f));
    UEvaAudioFunctionLibrary::PlayPrototypeTone2D(this, 38.0f, 0.9f, 0.30f);
    if (AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(
        GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr))
    {
        Player->TriggerCameraShakeFeedback(0.65f, 0.55f);
    }

    GetWorldTimerManager().ClearTimer(BlackoutTimer);
    GetWorldTimerManager().SetTimer(BlackoutTimer, this, &AEvaPrototypeGameMode::EndBlackout,
        ClampedDuration, false);
    BeginHorrorRuntimeEffects();
}

void AEvaPrototypeGameMode::EndBlackout()
{
    bBlackoutActive = false;
    BlackoutEndTime = -1000.0f;
    RestoreHorrorLighting();
    if (CanRunHorrorEffect(false))
    {
        UEvaAudioFunctionLibrary::PlayPrototypeTone2D(this, 196.0f, 0.16f, 0.18f);
        BeginHorrorRuntimeEffects();
    }
}

void AEvaPrototypeGameMode::UpdateEmergencyLightFlicker()
{
    if (!CanRunHorrorEffect(true))
    {
        return;
    }

    const bool bReduceFlashing = CVarEvaReduceFlashing.GetValueOnGameThread() != 0;
    for (int32 Index = 0; Index < RuntimeEmergencyLightComponents.Num(); ++Index)
    {
        UPointLightComponent* LightComponent = RuntimeEmergencyLightComponents[Index];
        if (!LightComponent)
        {
            continue;
        }

        const float BaseIntensity = RuntimeEmergencyLightBaseIntensities.IsValidIndex(Index) ?
            RuntimeEmergencyLightBaseIntensities[Index] : 1800.0f;
        const float PowerMultiplier = bFacilityPowerOnline ? 1.0f : 0.55f;
        const float BlackoutMultiplier = bBlackoutActive ? 1.65f : 1.0f;
        const float ComfortMultiplier = bReduceFlashing ? 0.82f : FMath::FRandRange(0.48f, 1.22f);
        const bool bDropout = !bReduceFlashing && FMath::FRand() < (bBlackoutActive ? 0.22f : 0.06f);
        LightComponent->SetIntensity(bDropout ? BaseIntensity * 0.18f * PowerMultiplier :
            BaseIntensity * PowerMultiplier * BlackoutMultiplier * ComfortMultiplier);
    }
}

void AEvaPrototypeGameMode::PlayAmbientPulse()
{
    if (!CanRunHorrorEffect(false))
    {
        return;
    }

    const float Frequency = CurrentDirector && CurrentDirector->IsAdamEncounterActive() ?
        FMath::FRandRange(31.0f, 46.0f) : FMath::FRandRange(48.0f, 76.0f);
    const float Volume = CurrentDirector && CurrentDirector->IsAdamEncounterActive() ? 0.24f : 0.12f;
    UEvaAudioFunctionLibrary::PlayPrototypeTone2D(this, Frequency, FMath::FRandRange(0.45f, 1.25f), Volume);
}

void AEvaPrototypeGameMode::SetHorrorWarning(const FString& Message, const float Duration)
{
    LastHorrorWarningText = Message;
    HorrorWarningDuration = FMath::Max(0.1f, Duration);
    LastHorrorWarningTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
}

bool AEvaPrototypeGameMode::ShouldDisplayHorrorWarning() const
{
    return GetWorld() && !LastHorrorWarningText.IsEmpty() &&
        GetWorld()->GetTimeSeconds() - LastHorrorWarningTime <= HorrorWarningDuration;
}

float AEvaPrototypeGameMode::GetBlackoutOverlayIntensity() const
{
    if (!bBlackoutActive || !GetWorld())
    {
        return 0.0f;
    }

    const float FadeOut = FMath::Clamp((BlackoutEndTime - GetWorld()->GetTimeSeconds()) / 0.65f, 0.0f, 1.0f);
    return 0.58f * FadeOut;
}

float AEvaPrototypeGameMode::GetHorrorPulseIntensity() const
{
    if (!GetWorld())
    {
        return 0.0f;
    }

    return FMath::Clamp((HorrorPulseEndTime - GetWorld()->GetTimeSeconds()) / 1.2f, 0.0f, 1.0f);
}

void AEvaPrototypeGameMode::TriggerHunterArrivalEffect(const FVector& ArrivalLocation)
{
    if (!CanRunHorrorEffect(false))
    {
        return;
    }

    HorrorPulseEndTime = FMath::Max(HorrorPulseEndTime, GetWorld()->GetTimeSeconds() + 1.8f);
    SetHorrorWarning(TEXT("EVA SIGNAL: HUNTER UNIT DEPLOYED"), 2.2f);
    UEvaAudioFunctionLibrary::PlayPrototypeToneAtLocation(this, ArrivalLocation, 52.0f, 0.45f, 0.70f);
    UEvaAudioFunctionLibrary::PlayPrototypeTone2D(this, 104.0f, 0.24f, 0.42f);
    if (AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(
        GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr))
    {
        Player->TriggerCameraShakeFeedback(0.45f, 0.35f);
    }
}

void AEvaPrototypeGameMode::TriggerAdamEntranceEffect(AEvaAdamBossCharacter* Adam)
{
    if (!CanRunHorrorEffect(false) || !Adam || LastAdamEntranceEffectActor.Get() == Adam)
    {
        return;
    }

    LastAdamEntranceEffectActor = Adam;
    SetHorrorWarning(TEXT("ADAM HAS ENTERED THE FACILITY"), 3.0f);
    TriggerBlackout(1.6f, true);
    UEvaAudioFunctionLibrary::PlayPrototypeToneAtLocation(this, Adam->GetActorLocation(), 29.0f, 1.25f, 0.82f);
    if (AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(
        GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr))
    {
        Player->TriggerCameraShakeFeedback(0.85f, 0.75f);
    }
}

void AEvaPrototypeGameMode::TriggerAdamChargeEffect(AEvaAdamBossCharacter* Adam)
{
    if (!CanRunHorrorEffect(false) || !Adam)
    {
        return;
    }

    HorrorPulseEndTime = FMath::Max(HorrorPulseEndTime, GetWorld()->GetTimeSeconds() + 0.9f);
    SetHorrorWarning(TEXT("ADAM: CHARGE"), 0.85f);
    UEvaAudioFunctionLibrary::PlayPrototypeToneAtLocation(this, Adam->GetActorLocation(), 41.0f, 0.34f, 0.74f);
    if (AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(
        GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr))
    {
        Player->TriggerCameraShakeFeedback(0.55f, 0.32f);
    }
}

void AEvaPrototypeGameMode::TriggerAdamRoarEffect(AEvaAdamBossCharacter* Adam)
{
    if (!CanRunHorrorEffect(false) || !Adam)
    {
        return;
    }

    HorrorPulseEndTime = FMath::Max(HorrorPulseEndTime, GetWorld()->GetTimeSeconds() + 1.4f);
    SetHorrorWarning(TEXT("ADAM: ROAR / SUMMON"), 1.25f);
    UEvaAudioFunctionLibrary::PlayPrototypeToneAtLocation(this, Adam->GetActorLocation(), 34.0f, 0.85f, 0.86f);
    if (AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(
        GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr))
    {
        Player->TriggerCameraShakeFeedback(0.70f, 0.62f);
    }
}

void AEvaPrototypeGameMode::TriggerAdamPhaseTwoEffect(AEvaAdamBossCharacter* Adam)
{
    if (!CanRunHorrorEffect(false) || !Adam)
    {
        return;
    }

    HorrorPulseEndTime = FMath::Max(HorrorPulseEndTime, GetWorld()->GetTimeSeconds() + 2.4f);
    SetHorrorWarning(TEXT("ADAM PHASE 2: BIOLOGICAL LIMITS REMOVED"), 3.2f);
    TriggerBlackout(1.15f, true);
    UEvaAudioFunctionLibrary::PlayPrototypeToneAtLocation(this, Adam->GetActorLocation(), 58.0f, 1.05f, 0.88f);
}

void AEvaPrototypeGameMode::TriggerDoorEffect(const FVector& Location, const FString& DoorLabel)
{
    if (!CanRunHorrorEffect(false))
    {
        return;
    }

    HorrorPulseEndTime = FMath::Max(HorrorPulseEndTime, GetWorld()->GetTimeSeconds() + 0.65f);
    UEvaAudioFunctionLibrary::PlayPrototypeToneAtLocation(this, Location, 120.0f, 0.18f, 0.38f);
    UEvaAudioFunctionLibrary::PlayPrototypeToneAtLocation(this, Location, 72.0f, 0.24f, 0.24f);
    UE_LOG(LogAdaptiveHorror, Log, TEXT("[HorrorFX] DoorEffect Label=%s Location=%s"),
        *DoorLabel, *Location.ToCompactString());
}

void AEvaPrototypeGameMode::TriggerPlayerDamageEffect(const float DamageAmount)
{
    if (!CanRunHorrorEffect(false))
    {
        return;
    }

    const float DamageScale = FMath::Clamp(DamageAmount / 35.0f, 0.15f, 1.0f);
    HorrorPulseEndTime = FMath::Max(HorrorPulseEndTime, GetWorld()->GetTimeSeconds() + 0.45f + DamageScale * 0.45f);
    if (AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(
        GetWorld()->GetFirstPlayerController() ? GetWorld()->GetFirstPlayerController()->GetPawn() : nullptr))
    {
        Player->TriggerCameraShakeFeedback(0.25f + DamageScale * 0.35f, 0.24f + DamageScale * 0.18f);
    }
}

void AEvaPrototypeGameMode::SpawnRuntimeFog()
{
    if (!GetWorld() || RuntimeFogComponent)
    {
        return;
    }

    if (AExponentialHeightFog* FogActor = GetWorld()->SpawnActor<AExponentialHeightFog>(
        FVector(0.0f, 0.0f, 45.0f), FRotator::ZeroRotator))
    {
        RuntimeFogComponent = FogActor->GetComponent();
        if (RuntimeFogComponent)
        {
            RuntimeFogComponent->SetMobility(EComponentMobility::Movable);
            RuntimeFogComponent->SetFogDensity(0.018f);
            RuntimeFogComponent->SetFogHeightFalloff(0.22f);
            RuntimeFogComponent->SetFogInscatteringColor(FLinearColor(0.09f, 0.12f, 0.16f));
            RuntimeFogComponent->SetStartDistance(120.0f);
            RuntimeFogComponent->SetFogMaxOpacity(0.42f);
        }
    }
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

void AEvaPrototypeGameMode::SetFacilityPowerOnline(const bool bOnline)
{
    bFacilityPowerOnline = bOnline;
    RestoreHorrorLighting();
    UE_LOG(LogAdaptiveHorror, Log, TEXT("[Content] FacilityPowerState Online=%s"),
        bFacilityPowerOnline ? TEXT("true") : TEXT("false"));
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
    if (APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr)
    {
        const bool bPressedNextPage = PlayerController->WasInputKeyJustPressed(EKeys::N) &&
            !PlayerController->WasInputKeyJustPressed(EKeys::F9);
        if (bPressedNextPage)
        {
            if (bDebugHUDVisible)
            {
                DebugHUDPageIndex = (DebugHUDPageIndex + 1) % DebugHUDPageCount;
            }
            else
            {
                bDebugHUDVisible = true;
                DebugHUDPageIndex = 0;
            }
            ShowDebugStatusMessage(FString::Printf(TEXT("DEBUG N: page %d/%d"),
                DebugHUDPageIndex + 1, DebugHUDPageCount), 3.0f);
            SyncEnemyDebugIntentDisplays(true);
            return;
        }

        bDebugHUDVisible = !bDebugHUDVisible;
        if (!bDebugHUDVisible)
        {
            DebugHUDPageIndex = 0;
            bNavigationDebugVisible = false;
        }
        else
        {
            bNavigationDebugVisible = !bNavigationDebugVisible;
        }

        PlayerController->ConsoleCommand(TEXT("Show Navigation"), true);
        PlayerController->ConsoleCommand(bNavigationDebugVisible ? TEXT("ShowFlag.Navigation 1") :
            TEXT("ShowFlag.Navigation 0"), true);
    }

    LogNavigationStatus(TEXT("DebugToggleNavigationVisualization"));
    SyncEnemyDebugIntentDisplays(true);
    ShowDebugStatusMessage(FString::Printf(TEXT("DEBUG F9: Debug HUD %s page %d/%d / Navigation visualization %s"),
        bDebugHUDVisible ? TEXT("ON") : TEXT("OFF"),
        DebugHUDPageIndex + 1,
        DebugHUDPageCount,
        bNavigationDebugVisible ? TEXT("ON") : TEXT("OFF")), 4.0f);
#endif
}
