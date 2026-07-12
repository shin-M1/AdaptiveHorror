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
#include "NavigationSystem.h"
#include "Kismet/GameplayStatics.h"
#include "Pickups/EvaAmmoPickup.h"
#include "Pickups/EvaHealthPickup.h"

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

    SetObjectiveForZone(CurrentZone);
    if (!TriggeredZones.Contains(CurrentZone))
    {
        TriggeredZones.Add(CurrentZone);
        SpawnZoneEncounter(CurrentZone);
    }
}

void AEvaResearchFacilityDirector::NotifyZoneEntered(const EEvaFacilityZone NewZone)
{
    if (bStageClear || static_cast<uint8>(NewZone) < static_cast<uint8>(CurrentZone))
    {
        return;
    }

    CurrentZone = NewZone;
    SetObjectiveForZone(NewZone);
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->ShowDebugStatusMessage(FString::Printf(TEXT("Objective updated: %s"), *CurrentObjective), 4.0f);
    }
    if (!TriggeredZones.Contains(NewZone))
    {
        TriggeredZones.Add(NewZone);
        SpawnZoneEncounter(NewZone);
        SpawnSupportPickupsForZone(NewZone);
    }
}

void AEvaResearchFacilityDirector::NotifyStoryLogCollected(const FName LogId, const FString& Title,
    const FString& Body)
{
    if (!LogId.IsNone())
    {
        CollectedStoryLogs.AddUnique(LogId);
    }
    bEvaLogAcquired = true;
    LastStoryLogTitle = Title;
    LastStoryLogBody = Body;
    LastStoryLogTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;

    if (LogId == FName(TEXT("EVA_LOG_03")))
    {
        bEvolutionUnlocked = true;
    }
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->ShowDebugStatusMessage(FString::Printf(TEXT("EVA log recovered: %s"), *Title), 4.0f);
    }
}

void AEvaResearchFacilityDirector::NotifyAdamDefeated(AEvaAdamBossCharacter* Adam)
{
    ActiveAdam = nullptr;
    bAdamEncounterActive = false;
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
        CurrentObjective = TEXT("Defeat ADAM.");
        if (!ActiveAdam->GetController())
        {
            ActiveAdam->SpawnDefaultController();
        }
        if (APlayerController* PlayerController = GetWorld()->GetFirstPlayerController())
        {
            ActiveAdam->AlertToPlayer(PlayerController->GetPawn());
        }
        LogAdamEncounterState(TEXT("StartAdamEncounterExistingAdam"), false, ActiveAdam);
        return;
    }

    bAdamEncounterActive = false;
    CurrentZone = EEvaFacilityZone::AdamArena;
    CurrentObjective = TEXT("Defeat ADAM.");
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

    bStageClear = true;
    CurrentZone = EEvaFacilityZone::Clear;
    CurrentObjective = TEXT("Stage clear. TODO: return to title.");
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->ShowDebugStatusMessage(TEXT("Stage objective complete: escape the research facility."), 6.0f);
        GameMode->HandleStageClear();
    }
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
        StartAdamEncounter();
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

void AEvaResearchFacilityDirector::SetObjectiveForZone(const EEvaFacilityZone Zone)
{
    switch (Zone)
    {
    case EEvaFacilityZone::EntryLobby:
        CurrentObjective = TEXT("Find a weapon path and survive the first infected.");
        break;
    case EEvaFacilityZone::SecurityCorridor:
        CurrentObjective = TEXT("Push through the corridor. Fire, reload, keep moving.");
        break;
    case EEvaFacilityZone::ObservationLab:
        CurrentObjective = TEXT("Recover EVA logs. The system is watching.");
        break;
    case EEvaFacilityZone::ContainmentWard:
        CurrentObjective = TEXT("Containment breach. Evolved infected are appearing.");
        break;
    case EEvaFacilityZone::DataCoreRoom:
        CurrentObjective = TEXT("Reach the data core. HUNTER is being deployed.");
        break;
    case EEvaFacilityZone::AdamArena:
        CurrentObjective = TEXT("ADAM is awake. End the experiment.");
        break;
    case EEvaFacilityZone::Clear:
        CurrentObjective = TEXT("Stage clear. TODO: return to title.");
        break;
    default:
        CurrentObjective = TEXT("Proceed.");
        break;
    }
}
