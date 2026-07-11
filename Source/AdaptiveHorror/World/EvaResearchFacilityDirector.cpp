#include "World/EvaResearchFacilityDirector.h"
#include "AI/EvaAdamBossCharacter.h"
#include "AI/EvaLearningSubsystem.h"
#include "AI/EvaZombieCharacter.h"
#include "Core/EvaPrototypeGameMode.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
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
    if (!GetWorld() || !AdamBossClass || bAdamEncounterActive || bStageClear)
    {
        return;
    }

    bAdamEncounterActive = true;
    CurrentObjective = TEXT("Defeat ADAM.");
    if (AEvaPrototypeGameMode* GameMode = GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>())
    {
        GameMode->ShowDebugStatusMessage(TEXT("ADAM encounter started."), 5.0f);
    }

    ActiveAdam = nullptr;
    if (AEvaPrototypeGameMode* GameMode = GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>())
    {
        TSubclassOf<AEvaZombieCharacter> AdamAsEnemy = AdamBossClass;
        ActiveAdam = Cast<AEvaAdamBossCharacter>(GameMode->SpawnEnemyNearLocation(AdamAsEnemy,
            AdamSpawnTransform.GetLocation(), 220.0f, 760.0f, TEXT("ADAM"), TEXT("AdamEncounter")));
    }
    else
    {
        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
        ActiveAdam = GetWorld()->SpawnActor<AEvaAdamBossCharacter>(AdamBossClass, AdamSpawnTransform.GetLocation(),
            AdamSpawnTransform.Rotator(), SpawnParameters);
    }
    if (!ActiveAdam)
    {
        if (AEvaPrototypeGameMode* GameMode = GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>())
        {
            GameMode->ShowDebugStatusMessage(TEXT("WARNING: ADAM spawn failed."), 5.0f);
        }
    }
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
