#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AI/EvaAdamBossCharacter.h"
#include "AI/EvaLearningSubsystem.h"
#include "AI/EvaZombieCharacter.h"
#include "Components/EvaPlayerTelemetryComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/EvaPrototypeGameMode.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "World/EvaResearchFacilityDirector.h"

namespace
{
    AStaticMeshActor* SpawnEvaTestFloor(UWorld* World)
    {
        if (!World)
        {
            return nullptr;
        }

        UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
        AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator);
        if (Floor && Floor->GetStaticMeshComponent())
        {
            Floor->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
            Floor->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
            Floor->GetStaticMeshComponent()->ComponentTags.AddUnique(TEXT("RuntimeFloor"));
            Floor->GetStaticMeshComponent()->ComponentTags.AddUnique(TEXT("EvaNavigableFloor"));
            Floor->GetStaticMeshComponent()->SetCanEverAffectNavigation(true);
            Floor->SetActorScale3D(FVector(24.0f, 24.0f, 0.5f));
        }
        return Floor;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaTelemetryClassificationTest,
    "AdaptiveHorror.EVA.TelemetryClassification",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaTelemetryClassificationTest::RunTest(const FString& Parameters)
{
    UEvaPlayerTelemetryComponent* Telemetry = NewObject<UEvaPlayerTelemetryComponent>();
    Telemetry->RecordShot(TEXT("Handgun"));
    Telemetry->RecordShot(TEXT("Handgun"));
    Telemetry->RecordHit(true, 2000.0f);

    TestTrue(TEXT("Accuracy is 50 percent"), FMath::IsNearlyEqual(Telemetry->GetAccuracy(), 0.5f));
    TestTrue(TEXT("Headshot rate is 100 percent"), FMath::IsNearlyEqual(Telemetry->GetHeadshotRate(), 1.0f));
    TestEqual(TEXT("Handgun is dominant"), Telemetry->GetDominantWeapon(), FName(TEXT("Handgun")));
    TestTrue(TEXT("Long-range sample classifies as Ranger"),
        Telemetry->ClassifyCombatStyle() == EEvaCombatStyle::Ranger);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaHunterLearningMultiplierTest,
    "AdaptiveHorror.EVA.HunterLearningMultiplier",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaHunterLearningMultiplierTest::RunTest(const FString& Parameters)
{
    UGameInstance* GameInstance = NewObject<UGameInstance>();
    UEvaLearningSubsystem* Learning = NewObject<UEvaLearningSubsystem>(GameInstance);
    Learning->SetHunterState(EEvaHunterState::DefeatedCooldown, 1);
    Learning->SetLearningSpeedMultiplier(0.3f);
    Learning->RecordShot(TEXT("Handgun"));

    TestTrue(TEXT("Hunter defeat multiplier is 0.3"),
        FMath::IsNearlyEqual(Learning->GetLearningSpeedMultiplier(), 0.3f));
    TestTrue(TEXT("Observation mass respects the multiplier"),
        FMath::IsNearlyEqual(Learning->GetEvaAnalysisRate(), 0.105f));
    TestTrue(TEXT("Hunter state is cooldown"),
        Learning->GetHunterState() == EEvaHunterState::DefeatedCooldown);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaAnalysisRateIncreaseTest,
    "AdaptiveHorror.EVA.AnalysisRateIncrease",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaAnalysisRateIncreaseTest::RunTest(const FString& Parameters)
{
    UGameInstance* GameInstance = NewObject<UGameInstance>();
    UEvaLearningSubsystem* Learning = NewObject<UEvaLearningSubsystem>(GameInstance);

    const float InitialRate = Learning->GetEvaAnalysisRate();
    Learning->RecordHit(true, 1800.0f);
    Learning->RecordKill();
    for (int32 Index = 0; Index < 4; ++Index)
    {
        Learning->RecordHunterObservation(EEvaCombatStyle::Ranger, 1600.0f, NAME_None, NAME_None);
    }

    TestTrue(TEXT("Analysis rate increases after hit, kill, and Hunter observation"),
        Learning->GetEvaAnalysisRate() > InitialRate);
    TestTrue(TEXT("Hunter observation can push EVA into an adaptation-capable state"),
        Learning->GetAnalysisStage() != EEvaAnalysisStage::Learning);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaEvolutionThresholdTest,
    "AdaptiveHorror.EVA.EvolutionThresholds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaEvolutionThresholdTest::RunTest(const FString& Parameters)
{
    UGameInstance* GameInstance = NewObject<UGameInstance>();
    UEvaLearningSubsystem* Learning = NewObject<UEvaLearningSubsystem>(GameInstance);

    for (int32 Index = 0; Index < 4; ++Index)
    {
        Learning->RecordHunterObservation(EEvaCombatStyle::Berserker, 400.0f, NAME_None, NAME_None);
    }
    TestTrue(TEXT("20 percent analysis recommends Fast evolution"),
        Learning->GetRecommendedEvolutionType() == EEvaEvolutionType::Fast);

    for (int32 Index = 0; Index < 4; ++Index)
    {
        Learning->RecordHunterObservation(EEvaCombatStyle::Ranger, 1800.0f, NAME_None, NAME_None);
    }
    TestTrue(TEXT("40 percent analysis recommends Armored evolution"),
        Learning->GetRecommendedEvolutionType() == EEvaEvolutionType::Armored);

    for (int32 Index = 0; Index < 4; ++Index)
    {
        Learning->RecordHunterObservation(EEvaCombatStyle::Explorer, 1200.0f, FName(TEXT("RouteA")), NAME_None);
    }
    TestTrue(TEXT("60 percent analysis recommends LongArm evolution"),
        Learning->GetRecommendedEvolutionType() == EEvaEvolutionType::LongArm);

    for (int32 Index = 0; Index < 4; ++Index)
    {
        Learning->RecordHunterObservation(EEvaCombatStyle::Ghost, 900.0f, NAME_None, FName(TEXT("HideA")));
    }
    TestTrue(TEXT("80 percent analysis recommends Composite evolution"),
        Learning->GetRecommendedEvolutionType() == EEvaEvolutionType::Composite);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaDirectorProgressionTest,
    "AdaptiveHorror.EVA.DirectorProgression",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaDirectorProgressionTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AEvaResearchFacilityDirector* Director = World->SpawnActor<AEvaResearchFacilityDirector>();
    TestTrue(TEXT("Director starts in Entry Lobby"),
        Director->GetCurrentZone() == EEvaFacilityZone::EntryLobby);

    Director->NotifyZoneEntered(EEvaFacilityZone::SecurityCorridor);
    TestTrue(TEXT("Director advances to Security Corridor"),
        Director->GetCurrentZone() == EEvaFacilityZone::SecurityCorridor);

    Director->NotifyZoneEntered(EEvaFacilityZone::ObservationLab);
    Director->NotifyZoneEntered(EEvaFacilityZone::ContainmentWard);
    TestTrue(TEXT("Containment Ward unlocks evolution"),
        Director->IsEvolutionUnlocked());
    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaStoryLogPickupTest,
    "AdaptiveHorror.EVA.StoryLogPickup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaStoryLogPickupTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AEvaResearchFacilityDirector* Director = World->SpawnActor<AEvaResearchFacilityDirector>();
    Director->NotifyStoryLogCollected(FName(TEXT("EVA_LOG_TEST")), TEXT("Test Log"), TEXT("Body"));

    TestTrue(TEXT("Director marks at least one EVA log as collected"), Director->HasAnyEvaLog());
    TestTrue(TEXT("Director stores log ID"), Director->HasCollectedStoryLog(FName(TEXT("EVA_LOG_TEST"))));
    TestEqual(TEXT("Director stores last log title"), Director->GetLastStoryLogTitle(), FString(TEXT("Test Log")));
    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaAdamPhaseTransitionTest,
    "AdaptiveHorror.EVA.AdamPhaseTransition",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaAdamPhaseTransitionTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AEvaAdamBossCharacter* Adam = World->SpawnActor<AEvaAdamBossCharacter>();

    TestFalse(TEXT("Adam should not enter phase two above 50 percent"),
        Adam->ShouldEnterPhaseTwo(1500.0f, 2500.0f));
    TestTrue(TEXT("Adam should enter phase two at 50 percent"),
        Adam->ShouldEnterPhaseTwo(1250.0f, 2500.0f));

    Adam->EnterPhaseTwo();
    TestTrue(TEXT("Adam phase two flag is set"), Adam->IsPhaseTwo());
    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaAdamDefeatStageClearTest,
    "AdaptiveHorror.EVA.AdamDefeatStageClear",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaAdamDefeatStageClearTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AEvaResearchFacilityDirector* Director = World->SpawnActor<AEvaResearchFacilityDirector>();
    Director->NotifyZoneEntered(EEvaFacilityZone::AdamArena);
    Director->NotifyAdamDefeated(nullptr);

    TestTrue(TEXT("Adam defeat completes the stage"), Director->IsStageClear());
    TestTrue(TEXT("Director zone becomes Clear"), Director->GetCurrentZone() == EEvaFacilityZone::Clear);
    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaSafeSpawnNoWorldFailsTest,
    "AdaptiveHorror.Spawn.SafeLocation.NoWorldFails",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaSafeSpawnNoWorldFailsTest::RunTest(const FString& Parameters)
{
    AEvaPrototypeGameMode* GameMode = NewObject<AEvaPrototypeGameMode>();
    FVector Location = FVector::ZeroVector;
    TestFalse(TEXT("Safe spawn fails cleanly without a world"),
        GameMode->FindSafeEnemySpawnLocation(FVector::ZeroVector, 400.0f, 800.0f, 300.0f, 500.0f, Location));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaSafeSpawnRequiresNavigationReadyTest,
    "AdaptiveHorror.Spawn.SafeLocation.RequiresNavigationReady",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaSafeSpawnRequiresNavigationReadyTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    SpawnEvaTestFloor(World);
    AEvaPrototypeGameMode* GameMode = World->SpawnActor<AEvaPrototypeGameMode>();

    FVector Location = FVector::ZeroVector;
    const bool bFound = GameMode->FindSafeEnemySpawnLocation(FVector::ZeroVector,
        300.0f, 700.0f, 0.0f, 0.0f, Location);

    TestFalse(TEXT("Safe spawn refuses floor candidates before runtime navigation is ready"), bFound);
    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaSafeSpawnRejectsUntaggedFloorTest,
    "AdaptiveHorror.Spawn.SafeLocation.RejectsUntaggedFloor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaSafeSpawnRejectsUntaggedFloorTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>(FVector::ZeroVector, FRotator::ZeroRotator);
    if (Floor && Floor->GetStaticMeshComponent())
    {
        Floor->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);
        Floor->GetStaticMeshComponent()->SetCollisionProfileName(TEXT("BlockAll"));
        Floor->SetActorScale3D(FVector(24.0f, 24.0f, 0.5f));
    }
    AEvaPrototypeGameMode* GameMode = World->SpawnActor<AEvaPrototypeGameMode>();

    FVector Location = FVector::ZeroVector;
    const bool bFound = GameMode->FindSafeEnemySpawnLocation(FVector::ZeroVector,
        300.0f, 700.0f, 0.0f, 0.0f, Location);

    TestFalse(TEXT("Safe spawn rejects untagged runtime geometry as a floor"), bFound);
    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaSafeSpawnRejectsWhenNavigationMissingTest,
    "AdaptiveHorror.Spawn.SafeLocation.RejectsWhenNavigationMissing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaSafeSpawnRejectsWhenNavigationMissingTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    SpawnEvaTestFloor(World);
    AEvaPrototypeGameMode* GameMode = World->SpawnActor<AEvaPrototypeGameMode>();

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
    World->SpawnActor<AEvaZombieCharacter>(AEvaZombieCharacter::StaticClass(), FVector(350.0f, 0.0f, 140.0f),
        FRotator::ZeroRotator, SpawnParameters);

    FVector Location = FVector::ZeroVector;
    const bool bFound = GameMode->FindSafeEnemySpawnLocation(FVector::ZeroVector,
        300.0f, 800.0f, 300.0f, 0.0f, Location);

    TestFalse(TEXT("Safe spawn does not use fallback floor traces when navigation is missing"), bFound);
    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaDebugCountersUpdateTest,
    "AdaptiveHorror.Spawn.DebugCounters",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaDebugCountersUpdateTest::RunTest(const FString& Parameters)
{
    AEvaPrototypeGameMode* GameMode = NewObject<AEvaPrototypeGameMode>();
    GameMode->NotifyFallbackMovementUsed();
    GameMode->NotifyEnemyStuck(TEXT("AutomationEnemy"));

    TestEqual(TEXT("Fallback counter increments"), GameMode->GetFallbackMovementCount(), 1);
    TestEqual(TEXT("Stuck counter increments"), GameMode->GetStuckEnemyCount(), 1);
    return true;
}

#endif
