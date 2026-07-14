#include "World/EvaResearchFacilityDirector.h"
#include "AdaptiveHorror.h"
#include "AI/EvaAdamBossCharacter.h"
#include "AI/EvaZombieAIController.h"
#include "AI/EvaLearningSubsystem.h"
#include "AI/EvaZombieCharacter.h"
#include "Components/EvaHealthComponent.h"
#include "Core/EvaPrototypeGameMode.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Pickups/EvaAmmoPickup.h"
#include "Pickups/EvaHealthPickup.h"
#include "World/EvaFacilityInteractable.h"

AEvaResearchFacilityDirector::AEvaResearchFacilityDirector()
{
    PrimaryActorTick.bCanEverTick = false;

    ZombieClass = AEvaZombieCharacter::StaticClass();
    AdamBossClass = AEvaAdamBossCharacter::StaticClass();
    AmmoPickupClass = AEvaAmmoPickup::StaticClass();
    HealthPickupClass = AEvaHealthPickup::StaticClass();

    HunterSpawnTransform = FTransform(FRotator(0.0f, 180.0f, 0.0f), FVector(3100.0f, 520.0f, 140.0f));
    AdamSpawnTransform = FTransform(FRotator(0.0f, 180.0f, 0.0f), FVector(5100.0f, 0.0f, 170.0f));

    Tags.Add(TEXT("EvaFacilityDirector"));
}

void AEvaResearchFacilityDirector::BeginPlay()
{
    Super::BeginPlay();

    SetObjectiveFromIndex();
    if (!TriggeredZones.Contains(CurrentZone))
    {
        TriggeredZones.Add(CurrentZone);
        SpawnZoneEncounter(CurrentZone);
    }
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->SetFacilityPowerOnline(bFacilityPowerOnline);
    }
    RefreshFacilityInteractables();
    UE_LOG(LogAdaptiveHorror, Log, TEXT("[Content] ObjectiveStart Index=%d Objective=%s"),
        ObjectiveIndex, *CurrentObjective);
}

void AEvaResearchFacilityDirector::NotifyZoneEntered(const EEvaFacilityZone NewZone)
{
    if (bStageClear || static_cast<uint8>(NewZone) < static_cast<uint8>(CurrentZone))
    {
        return;
    }

    FString RejectReason;
    if (!IsZoneEntryAllowed(NewZone, RejectReason))
    {
        if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
        {
            GameMode->ShowDebugStatusMessage(RejectReason, 3.0f);
        }
        UE_LOG(LogAdaptiveHorror, Warning, TEXT("[Content] ZoneEntryRejected Zone=%s Reason=%s"),
            *UEnum::GetValueAsString(NewZone), *RejectReason);
        return;
    }

    CurrentZone = NewZone;
    if (NewZone == EEvaFacilityZone::AdamArena)
    {
        AdvanceObjectiveTo(6, TEXT("ReachAdamArena"));
    }
    const bool bFirstVisit = !TriggeredZones.Contains(NewZone);
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->ShowDebugStatusMessage(FString::Printf(TEXT("Objective updated: %s"), *CurrentObjective), 4.0f);
        if (bFirstVisit && NewZone != EEvaFacilityZone::EntryLobby)
        {
            const APlayerController* PlayerController = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
            const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
            GameMode->TriggerDoorEffect(PlayerPawn ? PlayerPawn->GetActorLocation() : GetActorLocation(), GetCurrentZoneName());
            if (NewZone == EEvaFacilityZone::ObservationLab || NewZone == EEvaFacilityZone::DataCoreRoom)
            {
                GameMode->TriggerBlackout(NewZone == EEvaFacilityZone::DataCoreRoom ? 2.6f : 1.8f, false);
            }
        }
    }
    if (bFirstVisit)
    {
        TriggeredZones.Add(NewZone);
        SpawnZoneEncounter(NewZone);
        SpawnSupportPickupsForZone(NewZone);
    }
}

void AEvaResearchFacilityDirector::NotifyStoryLogCollected(const FName LogId, const FString& Title,
    const FString& Body)
{
    TryReadResearchLog(LogId, Title, Body);
}

bool AEvaResearchFacilityDirector::TryRestoreFacilityPower()
{
    if (bStageClear)
    {
        return false;
    }
    if (bFacilityPowerOnline)
    {
        return false;
    }

    bFacilityPowerOnline = true;
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->SetFacilityPowerOnline(true);
        GameMode->ShowDebugStatusMessage(TEXT("POWER RESTORED - emergency locks responding."), 4.0f);
    }
    AdvanceObjectiveTo(1, TEXT("PowerRestored"));
    RefreshFacilityInteractables();
    UE_LOG(LogAdaptiveHorror, Log, TEXT("[Content] PowerRestored ObjectiveIndex=%d"), ObjectiveIndex);
    return true;
}

bool AEvaResearchFacilityDirector::TryAcquireSecurityKeycard()
{
    if (bStageClear || bSecurityKeycardAcquired)
    {
        return false;
    }

    bSecurityKeycardAcquired = true;
    AdvanceObjectiveTo(2, TEXT("KeycardAcquired"));
    RefreshFacilityInteractables();
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->ShowDebugStatusMessage(TEXT("SECURITY KEYCARD ACQUIRED."), 4.0f);
    }
    UE_LOG(LogAdaptiveHorror, Log, TEXT("[Content] KeycardAcquired ObjectiveIndex=%d"), ObjectiveIndex);
    return true;
}

bool AEvaResearchFacilityDirector::TryOpenObservationDoor()
{
    if (bStageClear || bObservationDoorOpen)
    {
        return false;
    }
    if (!bSecurityKeycardAcquired)
    {
        if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
        {
            GameMode->ShowDebugStatusMessage(TEXT("KEYCARD REQUIRED"), 3.0f);
        }
        UE_LOG(LogAdaptiveHorror, Log, TEXT("[Content] DoorRejected Reason=MissingKeycard"));
        return false;
    }

    bObservationDoorOpen = true;
    AdvanceObjectiveTo(3, TEXT("ObservationDoorOpened"));
    RefreshFacilityInteractables();
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->TriggerDoorEffect(GetActorLocation(), TEXT("Observation Lab"));
        GameMode->ShowDebugStatusMessage(TEXT("OBSERVATION LAB UNLOCKED."), 4.0f);
    }
    UE_LOG(LogAdaptiveHorror, Log, TEXT("[Content] DoorUnlocked Door=ObservationLab ObjectiveIndex=%d"), ObjectiveIndex);
    return true;
}

bool AEvaResearchFacilityDirector::TryReadResearchLog(const FName LogId, const FString& Title, const FString& Body)
{
    if (bStageClear)
    {
        return false;
    }

    const bool bFirstRead = !LogId.IsNone() && !CollectedStoryLogs.Contains(LogId);
    if (bFirstRead)
    {
        CollectedStoryLogs.Add(LogId);
    }
    bEvaLogAcquired = true;
    bResearchLogOpen = true;
    LastStoryLogTitle = Title;
    LastStoryLogBody = Body;
    LastStoryLogTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

    if (LogId == FName(TEXT("EVA_LOG_03")) || LogId == FName(TEXT("CONTENT_LOG_HUNTER")))
    {
        bEvolutionUnlocked = true;
    }
    if (bFirstRead)
    {
        AdvanceObjectiveTo(4, TEXT("ResearchLogCollected"));
    }
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->ShowDebugStatusMessage(FString::Printf(TEXT("LOG OPEN: %s"), *Title), 4.0f);
    }
    UE_LOG(LogAdaptiveHorror, Log,
        TEXT("[Content] ResearchLogRead Log=%s FirstRead=%s Logs=%d ObjectiveIndex=%d"),
        *LogId.ToString(),
        bFirstRead ? TEXT("true") : TEXT("false"),
        CollectedStoryLogs.Num(),
        ObjectiveIndex);
    return true;
}

bool AEvaResearchFacilityDirector::TryAccessDataCore()
{
    if (bStageClear || bDataCoreAccessed)
    {
        return false;
    }
    if (CollectedStoryLogs.Num() <= 0)
    {
        if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
        {
            GameMode->ShowDebugStatusMessage(TEXT("CONTAINMENT RECORD REQUIRED"), 3.0f);
        }
        UE_LOG(LogAdaptiveHorror, Log, TEXT("[Content] DataCoreRejected Reason=NoResearchLog"));
        return false;
    }

    bDataCoreAccessed = true;
    bAdamArenaUnlocked = true;
    AdvanceObjectiveTo(5, TEXT("DataCoreAccessed"));
    RefreshFacilityInteractables();
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->ShowDebugStatusMessage(TEXT("DATA CORE ACCESSED - ADAM ARENA UNLOCKED."), 5.0f);
    }
    UE_LOG(LogAdaptiveHorror, Log,
        TEXT("[Content] DataCoreComplete ArenaUnlocked=%s ObjectiveIndex=%d"),
        bAdamArenaUnlocked ? TEXT("true") : TEXT("false"),
        ObjectiveIndex);
    return true;
}

void AEvaResearchFacilityDirector::CloseResearchLog()
{
    if (!bResearchLogOpen)
    {
        return;
    }
    bResearchLogOpen = false;
    LastStoryLogTime = -1000.0f;
    UE_LOG(LogAdaptiveHorror, Log, TEXT("[Content] ResearchLogClosed"));
}

void AEvaResearchFacilityDirector::NotifyAdamDefeated(AEvaAdamBossCharacter* Adam)
{
    ActiveAdam = nullptr;
    bAdamEncounterActive = false;
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        if (GameMode->IsGameOver() && !GameMode->IsStageClear())
        {
            UE_LOG(LogAdaptiveHorror, Warning,
                TEXT("[StageClear] Director rejected Adam defeat because player death is already active Adam=%s"),
                Adam ? *Adam->GetName() : TEXT("None"));
            return;
        }
    }
    CompleteStage();
}

void AEvaResearchFacilityDirector::StartAdamEncounter()
{
    if (!GetWorld() || bStageClear)
    {
        LogAdamEncounterState(TEXT("StartAdamEncounterSkipped"), false, ActiveAdam,
            bStageClear ? TEXT("StageClear") : TEXT("NoWorld"));
        return;
    }

    if (!AdamBossClass)
    {
        bAdamEncounterActive = false;
        LogAdamEncounterState(TEXT("StartAdamEncounterFailed"), false, nullptr, TEXT("AdamClassInvalid"));
        if (AEvaPrototypeGameMode* GameMode = GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>())
        {
            GameMode->ShowDebugStatusMessage(TEXT("WARNING: ADAM spawn failed - class invalid."), 5.0f);
        }
        return;
    }

    if (!IsValid(ActiveAdam) || (ActiveAdam->GetHealthComponent() && ActiveAdam->GetHealthComponent()->IsDead()))
    {
        ActiveAdam = FindExistingLivingAdam();
    }
    if (IsValid(ActiveAdam))
    {
        bAdamEncounterActive = true;
        CurrentZone = EEvaFacilityZone::AdamArena;
        AdvanceObjectiveTo(6, TEXT("AdamEncounterExisting"));
        if (!ActiveAdam->GetController())
        {
            ActiveAdam->SpawnDefaultController();
        }
        if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
        {
            ActiveAdam->AlertToPlayer(PlayerController->GetPawn());
        }
        if (AEvaPrototypeGameMode* GameMode = GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>())
        {
            GameMode->TriggerAdamEntranceEffect(ActiveAdam);
        }
        LogAdamEncounterState(TEXT("StartAdamEncounterExistingAdam"), false, ActiveAdam);
        return;
    }

    bAdamEncounterActive = false;
    CurrentZone = EEvaFacilityZone::AdamArena;
    AdvanceObjectiveTo(6, TEXT("AdamEncounterStarted"));
    if (AEvaPrototypeGameMode* GameMode = GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>())
    {
        GameMode->ShowDebugStatusMessage(TEXT("ADAM encounter started."), 5.0f);
    }

    ActiveAdam = nullptr;
    bool bSpawnAttempted = false;
    if (AEvaPrototypeGameMode* GameMode = GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>())
    {
        TSubclassOf<AEvaZombieCharacter> AdamAsEnemy = AdamBossClass;
        bSpawnAttempted = true;
        ActiveAdam = Cast<AEvaAdamBossCharacter>(GameMode->SpawnEnemyNearLocation(AdamAsEnemy,
            AdamSpawnTransform.GetLocation(), 220.0f, 760.0f, TEXT("ADAM"), TEXT("AdamEncounter")));
    }
    else
    {
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
        bSpawnAttempted = true;
        ActiveAdam = GetWorld()->SpawnActor<AEvaAdamBossCharacter>(AdamBossClass, AdamSpawnTransform.GetLocation(),
            AdamSpawnTransform.Rotator(), SpawnParameters);
    }
    if (!ActiveAdam)
    {
        bAdamEncounterActive = false;
        LogAdamEncounterState(TEXT("StartAdamEncounterFailed"), bSpawnAttempted, nullptr, TEXT("SpawnReturnedNull"));
        if (AEvaPrototypeGameMode* GameMode = GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>())
        {
            GameMode->ShowDebugStatusMessage(TEXT("WARNING: ADAM spawn failed."), 5.0f);
        }
        return;
    }

    bAdamEncounterActive = true;
    if (!ActiveAdam->GetController())
    {
        ActiveAdam->SpawnDefaultController();
    }
    if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
    {
        ActiveAdam->AlertToPlayer(PlayerController->GetPawn());
    }
    if (AEvaPrototypeGameMode* GameMode = GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>())
    {
        GameMode->TriggerAdamEntranceEffect(ActiveAdam);
    }
    LogAdamEncounterState(TEXT("StartAdamEncounterSpawned"), bSpawnAttempted, ActiveAdam);
}

AEvaAdamBossCharacter* AEvaResearchFacilityDirector::FindExistingLivingAdam() const
{
    if (!GetWorld())
    {
        return nullptr;
    }

    for (TActorIterator<AEvaAdamBossCharacter> It(GetWorld()); It; ++It)
    {
        AEvaAdamBossCharacter* Adam = *It;
        if (Adam && Adam->GetHealthComponent() && !Adam->GetHealthComponent()->IsDead())
        {
            return Adam;
        }
    }
    return nullptr;
}

int32 AEvaResearchFacilityDirector::CountExistingLivingAdam() const
{
    if (!GetWorld())
    {
        return 0;
    }

    int32 Count = 0;
    for (TActorIterator<AEvaAdamBossCharacter> It(GetWorld()); It; ++It)
    {
        const AEvaAdamBossCharacter* Adam = *It;
        if (Adam && Adam->GetHealthComponent() && !Adam->GetHealthComponent()->IsDead())
        {
            ++Count;
        }
    }
    return Count;
}

void AEvaResearchFacilityDirector::LogAdamEncounterState(const FString& Context, const bool bSpawnAttempted,
    AEvaAdamBossCharacter* SpawnResult, const FString& DestroyReason) const
{
    if (!GetWorld())
    {
        return;
    }

    FVector ProjectedLocation = FVector::ZeroVector;
    bool bNavProjected = false;
    if (UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld()))
    {
        FNavLocation NavLocation;
        bNavProjected = NavigationSystem->ProjectPointToNavigation(AdamSpawnTransform.GetLocation(), NavLocation,
            FVector(360.0f, 360.0f, 520.0f));
        if (bNavProjected)
        {
            ProjectedLocation = NavLocation.Location;
        }
    }

    const AEvaAdamBossCharacter* Adam = SpawnResult ? SpawnResult : ActiveAdam.Get();
    const AController* AdamController = Adam ? Adam->GetController() : nullptr;
    const AEvaZombieAIController* EvaAI = Cast<AEvaZombieAIController>(AdamController);
    const APlayerController* PlayerController = GetWorld()->GetFirstPlayerController();
    const APawn* PlayerPawn = PlayerController ? PlayerController->GetPawn() : nullptr;
    const bool bPlayerTarget = EvaAI && PlayerPawn && EvaAI->GetPlayerTarget() == PlayerPawn;
    const UEvaHealthComponent* Health = Adam ? Adam->GetHealthComponent() : nullptr;

    UE_LOG(LogAdaptiveHorror, Warning,
        TEXT("[AdamDebug] Context=%s ArenaLocation=%s ArenaState=Zone:%s Active:%s StageClear:%s AdamClass=%s ExistingAdamCount=%d SpawnAttempted=%s SpawnResult=%s FinalLocation=%s NavProjected=%s ProjectedLocation=%s AIController=%s Possessed=%s PlayerTarget=%s Health=%.1f/%.1f Phase=%s DestroyReason=%s"),
        *Context,
        *AdamSpawnTransform.GetLocation().ToCompactString(),
        *GetCurrentZoneName(),
        bAdamEncounterActive ? TEXT("true") : TEXT("false"),
        bStageClear ? TEXT("true") : TEXT("false"),
        AdamBossClass ? *AdamBossClass->GetName() : TEXT("None"),
        CountExistingLivingAdam(),
        bSpawnAttempted ? TEXT("true") : TEXT("false"),
        Adam ? *Adam->GetName() : TEXT("None"),
        Adam ? *Adam->GetActorLocation().ToCompactString() : TEXT("None"),
        bNavProjected ? TEXT("true") : TEXT("false"),
        *ProjectedLocation.ToCompactString(),
        AdamController ? *AdamController->GetClass()->GetName() : TEXT("None"),
        AdamController && AdamController->GetPawn() == Adam ? TEXT("true") : TEXT("false"),
        bPlayerTarget ? TEXT("true") : TEXT("false"),
        Health ? Health->GetCurrentHealth() : -1.0f,
        Health ? Health->GetMaxHealth() : -1.0f,
        Adam && Adam->IsPhaseTwo() ? TEXT("Phase2") : TEXT("Phase1"),
        DestroyReason.IsEmpty() ? TEXT("None") : *DestroyReason);
}

void AEvaResearchFacilityDirector::CompleteStage()
{
    if (bStageClear)
    {
        return;
    }

    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        if (GameMode->IsGameOver() && !GameMode->IsStageClear())
        {
            UE_LOG(LogAdaptiveHorror, Warning,
                TEXT("[StageClear] Director CompleteStage rejected because player death is already active."));
            return;
        }
    }

    bStageClear = true;
    CurrentZone = EEvaFacilityZone::Clear;
    ObjectiveIndex = 7;
    SetObjectiveFromIndex();
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->ShowDebugStatusMessage(TEXT("Stage objective complete: escape the research facility."), 6.0f);
        GameMode->HandleStageClear();
    }
}

void AEvaResearchFacilityDirector::ResetForNewGame()
{
    CurrentZone = EEvaFacilityZone::EntryLobby;
    ObjectiveIndex = 0;
    TriggeredZones.Reset();
    TriggeredZones.Add(CurrentZone);
    bEvaLogAcquired = false;
    bFacilityPowerOnline = false;
    bSecurityKeycardAcquired = false;
    bObservationDoorOpen = false;
    bDataCoreAccessed = false;
    bAdamArenaUnlocked = false;
    bResearchLogOpen = false;
    bEvolutionUnlocked = false;
    bHunterEventTriggered = false;
    bAdamEncounterActive = false;
    bStageClear = false;
    ActiveAdam = nullptr;
    CollectedStoryLogs.Reset();
    LastStoryLogTitle.Empty();
    LastStoryLogBody.Empty();
    LastStoryLogTime = -1000.0f;
    SetObjectiveFromIndex();
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->SetFacilityPowerOnline(false);
    }
    RefreshFacilityInteractables();
    UE_LOG(LogAdaptiveHorror, Log, TEXT("[Content] ProgressReset ObjectiveIndex=%d"), ObjectiveIndex);
}

FString AEvaResearchFacilityDirector::GetCurrentZoneName() const
{
    switch (CurrentZone)
    {
    case EEvaFacilityZone::EntryLobby: return TEXT("Entry Lobby");
    case EEvaFacilityZone::SecurityCorridor: return TEXT("Security Corridor");
    case EEvaFacilityZone::ObservationLab: return TEXT("Observation Lab");
    case EEvaFacilityZone::ContainmentWard: return TEXT("Containment Ward");
    case EEvaFacilityZone::DataCoreRoom: return TEXT("Data Core Room");
    case EEvaFacilityZone::AdamArena: return TEXT("Adam Arena");
    case EEvaFacilityZone::Clear: return TEXT("Clear");
    default: return TEXT("Unknown");
    }
}

bool AEvaResearchFacilityDirector::ShouldDisplayStoryLog() const
{
    if (LastStoryLogTitle.IsEmpty())
    {
        return false;
    }
    if (bResearchLogOpen)
    {
        return true;
    }
    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    return Now - LastStoryLogTime <= StoryLogDisplaySeconds;
}

void AEvaResearchFacilityDirector::SpawnZoneEncounter(const EEvaFacilityZone Zone)
{
    if (!GetWorld())
    {
        return;
    }

    switch (Zone)
    {
    case EEvaFacilityZone::EntryLobby:
        // The first visible infected is spawned by GameMode after the player pawn is guaranteed.
        // This keeps the initial enemy 500-900 cm in front of the player instead of relying on a fixed transform.
        break;
    case EEvaFacilityZone::SecurityCorridor:
        SpawnZombieAt(FVector(-3150.0f, -260.0f, 140.0f), EEvaEvolutionType::None, TEXT("ZoneSecurityA"));
        SpawnZombieAt(FVector(-2500.0f, 330.0f, 140.0f), EEvaEvolutionType::None, TEXT("ZoneSecurityB"));
        break;
    case EEvaFacilityZone::ObservationLab:
        SpawnZombieAt(FVector(-1100.0f, 380.0f, 140.0f), EEvaEvolutionType::None, TEXT("ZoneObservationA"));
        if (UGameInstance* GameInstance = GetWorld() ? GetWorld()->GetGameInstance() : nullptr)
        {
            if (UEvaLearningSubsystem* Learning = GameInstance->GetSubsystem<UEvaLearningSubsystem>())
            {
                Learning->RecordHunterObservation(EEvaCombatStyle::Tactician, 1200.0f, NAME_None, NAME_None);
            }
        }
        break;
    case EEvaFacilityZone::ContainmentWard:
        bEvolutionUnlocked = true;
        SpawnZombieAt(FVector(650.0f, -420.0f, 140.0f), EEvaEvolutionType::None, TEXT("ZoneContainmentA"));
        SpawnZombieAt(FVector(1150.0f, 280.0f, 140.0f), EEvaEvolutionType::Fast, TEXT("ZoneContainmentFast"));
        SpawnZombieAt(FVector(1500.0f, -180.0f, 140.0f), EEvaEvolutionType::Armored, TEXT("ZoneContainmentArmored"));
        break;
    case EEvaFacilityZone::DataCoreRoom:
        bHunterEventTriggered = true;
        SpawnZombieAt(FVector(2850.0f, -380.0f, 140.0f), EEvaEvolutionType::LongArm, TEXT("ZoneDataCoreLongArm"));
        if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
        {
            GameMode->SpawnHunter();
        }
        break;
    case EEvaFacilityZone::AdamArena:
        if (bAdamArenaUnlocked)
        {
            StartAdamEncounter();
        }
        break;
    default:
        break;
    }
}

AEvaZombieCharacter* AEvaResearchFacilityDirector::SpawnZombieAt(const FVector& Location,
    const EEvaEvolutionType EvolutionType, const FString& SpawnReason)
{
    if (!GetWorld() || !ZombieClass)
    {
        return nullptr;
    }

    if (AEvaPrototypeGameMode* GameMode = GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>())
    {
        return GameMode->SpawnEnemyNearLocation(ZombieClass, Location, 120.0f, 540.0f, TEXT("Zombie"),
            SpawnReason, EvolutionType);
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
    AEvaZombieCharacter* Zombie = GetWorld()->SpawnActor<AEvaZombieCharacter>(ZombieClass, Location,
        FRotator(0.0f, 180.0f, 0.0f), SpawnParameters);
    if (Zombie)
    {
        Zombie->ConfigureEvolution(EvolutionType);
    }
    return Zombie;
}

void AEvaResearchFacilityDirector::SpawnSupportPickupsForZone(const EEvaFacilityZone Zone)
{
    if (!GetWorld())
    {
        return;
    }

    const float ZoneX = [&]()
    {
        switch (Zone)
        {
        case EEvaFacilityZone::SecurityCorridor: return -3000.0f;
        case EEvaFacilityZone::ObservationLab: return -1200.0f;
        case EEvaFacilityZone::ContainmentWard: return 600.0f;
        case EEvaFacilityZone::DataCoreRoom: return 2400.0f;
        case EEvaFacilityZone::AdamArena: return 4200.0f;
        default: return -4800.0f;
        }
    }();

    if (AmmoPickupClass)
    {
        GetWorld()->SpawnActor<AEvaAmmoPickup>(AmmoPickupClass, FVector(ZoneX + 120.0f, 520.0f, 90.0f),
            FRotator::ZeroRotator);
    }
    if (HealthPickupClass && (Zone == EEvaFacilityZone::ContainmentWard || Zone == EEvaFacilityZone::AdamArena))
    {
        GetWorld()->SpawnActor<AEvaHealthPickup>(HealthPickupClass, FVector(ZoneX - 120.0f, -520.0f, 90.0f),
            FRotator::ZeroRotator);
    }
}

void AEvaResearchFacilityDirector::SetObjectiveFromIndex()
{
    switch (ObjectiveIndex)
    {
    case 0:
        CurrentObjective = TEXT("Restore Facility Power");
        break;
    case 1:
        CurrentObjective = TEXT("Find Security Keycard");
        break;
    case 2:
        CurrentObjective = TEXT("Unlock Observation Lab");
        break;
    case 3:
        CurrentObjective = TEXT("Search Containment Records");
        break;
    case 4:
        CurrentObjective = TEXT("Access Data Core");
        break;
    case 5:
        CurrentObjective = TEXT("Reach Adam Arena");
        break;
    case 6:
        CurrentObjective = TEXT("Defeat Adam");
        break;
    case 7:
        CurrentObjective = TEXT("Stage clear. TODO: return to title.");
        break;
    default:
        CurrentObjective = TEXT("Proceed.");
        break;
    }
}

bool AEvaResearchFacilityDirector::AdvanceObjectiveTo(const int32 NewObjectiveIndex, const FString& Reason)
{
    if (bStageClear || NewObjectiveIndex <= ObjectiveIndex)
    {
        return false;
    }

    ObjectiveIndex = FMath::Clamp(NewObjectiveIndex, 0, 7);
    SetObjectiveFromIndex();
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->ShowDebugStatusMessage(FString::Printf(TEXT("Objective: %s"), *CurrentObjective), 4.0f);
    }
    UE_LOG(LogAdaptiveHorror, Log, TEXT("[Content] ObjectiveAdvanced Index=%d Objective=%s Reason=%s"),
        ObjectiveIndex, *CurrentObjective, *Reason);
    return true;
}

bool AEvaResearchFacilityDirector::IsZoneEntryAllowed(const EEvaFacilityZone NewZone, FString& OutReason) const
{
    switch (NewZone)
    {
    case EEvaFacilityZone::ObservationLab:
        if (!bObservationDoorOpen)
        {
            OutReason = bSecurityKeycardAcquired ? TEXT("Unlock the Observation Lab door first.") :
                TEXT("Find the Security Keycard first.");
            return false;
        }
        break;
    case EEvaFacilityZone::ContainmentWard:
        if (CollectedStoryLogs.Num() <= 0)
        {
            OutReason = TEXT("Read at least one research log first.");
            return false;
        }
        break;
    case EEvaFacilityZone::AdamArena:
        if (!bAdamArenaUnlocked)
        {
            OutReason = TEXT("Access the Data Core first.");
            return false;
        }
        break;
    default:
        break;
    }

    OutReason.Empty();
    return true;
}

void AEvaResearchFacilityDirector::RefreshFacilityInteractables() const
{
    if (!GetWorld())
    {
        return;
    }

    for (TActorIterator<AEvaFacilityInteractable> It(GetWorld()); It; ++It)
    {
        if (AEvaFacilityInteractable* Interactable = *It)
        {
            Interactable->RefreshFromDirector();
        }
    }
}

FString AEvaResearchFacilityDirector::GetObjectiveProgressText() const
{
    return FString::Printf(TEXT("Logs %d/3 | Keycard %s | Power %s"),
        FMath::Min(CollectedStoryLogs.Num(), 3),
        bSecurityKeycardAcquired ? TEXT("YES") : TEXT("NO"),
        bFacilityPowerOnline ? TEXT("ONLINE") : TEXT("OFFLINE"));
}
