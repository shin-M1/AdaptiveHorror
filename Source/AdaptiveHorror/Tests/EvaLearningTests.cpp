#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "AI/EvaAdamBossCharacter.h"
#include "AI/EvaHunterCharacter.h"
#include "AI/EvaLearningSubsystem.h"
#include "AI/EvaZombieAIController.h"
#include "AI/EvaZombieCharacter.h"
#include "Characters/EvaPlayerCharacter.h"
#include "Components/EvaPlayerTelemetryComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Core/EvaPrototypeGameMode.h"
#include "Core/EvaSettingsSaveGame.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "World/EvaFacilityInteractable.h"
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

    TestTrue(TEXT("Power can be restored before lab progress"), Director->TryRestoreFacilityPower());
    TestTrue(TEXT("Keycard can be acquired before lab progress"), Director->TryAcquireSecurityKeycard());
    TestTrue(TEXT("Observation door can be opened after keycard"), Director->TryOpenObservationDoor());
    Director->NotifyZoneEntered(EEvaFacilityZone::ObservationLab);
    TestTrue(TEXT("Director advances to Observation Lab after door opens"),
        Director->GetCurrentZone() == EEvaFacilityZone::ObservationLab);
    TestTrue(TEXT("Required research log can be read before containment"),
        Director->TryReadResearchLog(FName(TEXT("EVA_LOG_03")), TEXT("Automation Log"), TEXT("Body")));
    Director->CloseResearchLog();
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaStageClearRejectsPlayerDeathTest,
    "AdaptiveHorror.StageClear.RejectsPlayerDeath",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaStageClearRejectsPlayerDeathTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AEvaPrototypeGameMode* GameMode = World->SpawnActor<AEvaPrototypeGameMode>();
    AEvaPlayerCharacter* Player = World->SpawnActor<AEvaPlayerCharacter>();

    GameMode->HandleStageClear();
    GameMode->HandlePlayerDeath(Player);

    TestTrue(TEXT("Stage clear remains active after a late death request"), GameMode->IsStageClear());
    TestFalse(TEXT("Late death request does not enter game over"), GameMode->IsGameOver());
    TestFalse(TEXT("Late death request does not schedule respawn"), GameMode->IsRespawnScheduledForDebug());

    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaStageClearSkipsSpawnsTest,
    "AdaptiveHorror.StageClear.SkipsSpawns",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaStageClearSkipsSpawnsTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AEvaPrototypeGameMode* GameMode = World->SpawnActor<AEvaPrototypeGameMode>();

    GameMode->HandleStageClear();
    AEvaZombieCharacter* SpawnedEnemy = GameMode->SpawnEnemyNearLocation(AEvaZombieCharacter::StaticClass(),
        FVector::ZeroVector, 100.0f, 200.0f, TEXT("Zombie"), TEXT("AutomationPostClear"));

    TestNull(TEXT("Post-clear spawn requests return null"), SpawnedEnemy);
    TestTrue(TEXT("Post-clear spawn records skipped reason"),
        GameMode->GetLastSpawnResult().Contains(TEXT("stage clear")));

    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaStageClearStopsEnemyCombatTest,
    "AdaptiveHorror.StageClear.StopsEnemyCombat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaStageClearStopsEnemyCombatTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AEvaPrototypeGameMode* GameMode = World->SpawnActor<AEvaPrototypeGameMode>();
    AEvaZombieCharacter* Zombie = World->SpawnActor<AEvaZombieCharacter>();
    AEvaZombieAIController* Controller = World->SpawnActor<AEvaZombieAIController>();

    TestNotNull(TEXT("Zombie spawns for stage clear combat-stop test"), Zombie);
    TestNotNull(TEXT("Controller spawns for stage clear combat-stop test"), Controller);
    if (!Zombie || !Controller)
    {
        World->DestroyWorld(false);
        return false;
    }

    Controller->Possess(Zombie);
    Zombie->SetDebugIntentText(TEXT("FLANK"));
    GameMode->HandleStageClear();

    TestFalse(TEXT("Stage clear disables EVA AI combat"), Controller->IsCombatEnabled());
    if (UTextRenderComponent* Label = Zombie->GetTypeLabelComponent())
    {
        TestFalse(TEXT("Stage clear hides overhead labels"), Label->IsVisible());
    }
    if (UTextRenderComponent* IntentLabel = Zombie->GetDebugIntentLabelComponent())
    {
        TestFalse(TEXT("Stage clear hides debug intent label"), IntentLabel->IsVisible());
    }

    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaStageClearIdempotentTest,
    "AdaptiveHorror.StageClear.Idempotent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaStageClearIdempotentTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AEvaPrototypeGameMode* GameMode = World->SpawnActor<AEvaPrototypeGameMode>();

    GameMode->HandleStageClear();
    GameMode->HandleStageClear();

    TestTrue(TEXT("Repeated stage clear calls keep stage clear active"), GameMode->IsStageClear());
    TestFalse(TEXT("Repeated stage clear calls do not enter game over"), GameMode->IsGameOver());
    TestFalse(TEXT("Repeated stage clear calls do not schedule respawn"), GameMode->IsRespawnScheduledForDebug());

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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaEnemyVisualLongArmUsesPartsTest,
    "AdaptiveHorror.Visual.Enemy.LongArmUsesPartScale",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaEnemyVisualLongArmUsesPartsTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AEvaZombieCharacter* Zombie = World->SpawnActor<AEvaZombieCharacter>();
    TestNotNull(TEXT("Zombie spawns for visual test"), Zombie);
    if (!Zombie)
    {
        World->DestroyWorld(false);
        return false;
    }

    Zombie->ConfigureEvolution(EEvaEvolutionType::LongArm);
    TestTrue(TEXT("LongArm keeps actor scale stable for capsule/navigation"),
        Zombie->GetActorScale3D().Equals(FVector::OneVector, KINDA_SMALL_NUMBER));
    TestNotNull(TEXT("LongArm has left arm visual"), Zombie->GetLeftArmVisualComponent());
    TestNotNull(TEXT("LongArm has body visual"), Zombie->GetBodyVisualComponent());
    if (Zombie->GetLeftArmVisualComponent() && Zombie->GetBodyVisualComponent())
    {
        TestTrue(TEXT("LongArm arm is visibly longer than body baseline"),
            Zombie->GetLeftArmVisualComponent()->GetRelativeScale3D().Z >
            Zombie->GetBodyVisualComponent()->GetRelativeScale3D().Z);
    }

    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaEnemyDebugLabelReadableDefaultTest,
    "AdaptiveHorror.Visual.Enemy.DebugLabelReadableDefault",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaEnemyDebugLabelReadableDefaultTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AEvaZombieCharacter* Zombie = World->SpawnActor<AEvaZombieCharacter>();
    TestNotNull(TEXT("Zombie spawns for label test"), Zombie);
    if (!Zombie)
    {
        World->DestroyWorld(false);
        return false;
    }

    UTextRenderComponent* Label = Zombie->GetTypeLabelComponent();
    TestNotNull(TEXT("Enemy debug label component exists"), Label);
    if (Label)
    {
        TestFalse(TEXT("Enemy debug label no longer starts with fixed mirrored yaw"),
            FMath::IsNearlyEqual(FRotator::NormalizeAxis(Label->GetRelativeRotation().Yaw), 180.0f, 0.1f));
        TestEqual(TEXT("Enemy debug label starts as ZOMBIE"), Label->Text.ToString(), FString(TEXT("ZOMBIE")));
    }

    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaGameFlowTitleModeBlocksCombatTest,
    "AdaptiveHorror.GameFlow.TitleModeBlocksCombat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaGameFlowTitleModeBlocksCombatTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AEvaPrototypeGameMode* GameMode = World->SpawnActor<AEvaPrototypeGameMode>();
    TestNotNull(TEXT("GameMode spawns for title flow test"), GameMode);
    if (!GameMode)
    {
        World->DestroyWorld(false);
        return false;
    }

    GameMode->EnterTitleMode();

    TestTrue(TEXT("Title mode sets flow state to Title"),
        GameMode->GetGameFlowState() == EEvaGameFlowState::Title);
    TestFalse(TEXT("Title mode is not active gameplay"), GameMode->IsGameplayActive());
    TestFalse(TEXT("Title mode blocks player damage"), GameMode->CanPlayerTakeDamage());
    TestFalse(TEXT("Title mode is not game over"), GameMode->IsGameOver());
    TestFalse(TEXT("Title mode is not stage clear"), GameMode->IsStageClear());

    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaGameFlowNewGameClearsTerminalStatesTest,
    "AdaptiveHorror.GameFlow.NewGameClearsTerminalStates",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaGameFlowNewGameClearsTerminalStatesTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AEvaPrototypeGameMode* GameMode = World->SpawnActor<AEvaPrototypeGameMode>();
    TestNotNull(TEXT("GameMode spawns for new game flow test"), GameMode);
    if (!GameMode)
    {
        World->DestroyWorld(false);
        return false;
    }

    GameMode->HandleStageClear();
    TestTrue(TEXT("Stage clear state is active before reset"), GameMode->IsStageClear());

    GameMode->StartNewGameFlow();

    TestTrue(TEXT("New Game sets flow state to Playing"),
        GameMode->GetGameFlowState() == EEvaGameFlowState::Playing);
    TestTrue(TEXT("New Game enables gameplay"), GameMode->IsGameplayActive());
    TestTrue(TEXT("New Game enables player damage"), GameMode->CanPlayerTakeDamage());
    TestFalse(TEXT("New Game clears game over"), GameMode->IsGameOver());
    TestFalse(TEXT("New Game clears stage clear"), GameMode->IsStageClear());

    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaGameFlowRetryClearsGameOverTest,
    "AdaptiveHorror.GameFlow.RetryClearsGameOver",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaGameFlowRetryClearsGameOverTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AEvaPrototypeGameMode* GameMode = World->SpawnActor<AEvaPrototypeGameMode>();
    AEvaPlayerCharacter* Player = World->SpawnActor<AEvaPlayerCharacter>();
    TestNotNull(TEXT("GameMode spawns for retry flow test"), GameMode);
    TestNotNull(TEXT("Player spawns for retry flow test"), Player);
    if (!GameMode || !Player)
    {
        World->DestroyWorld(false);
        return false;
    }

    GameMode->StartNewGameFlow();
    GameMode->HandlePlayerDeath(Player);
    TestTrue(TEXT("Death enters game over"), GameMode->IsGameOver());
    TestTrue(TEXT("Death sets flow state to PlayerDead"),
        GameMode->GetGameFlowState() == EEvaGameFlowState::PlayerDead);

    GameMode->RetryFromCheckpointFlow();

    TestTrue(TEXT("Retry returns to Playing"),
        GameMode->GetGameFlowState() == EEvaGameFlowState::Playing);
    TestFalse(TEXT("Retry clears game over"), GameMode->IsGameOver());
    TestFalse(TEXT("Retry does not schedule the legacy auto-respawn timer"), GameMode->IsRespawnScheduledForDebug());

    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaSettingsSaveDefaultsTest,
    "AdaptiveHorror.Settings.Defaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaSettingsSaveDefaultsTest::RunTest(const FString& Parameters)
{
    const UEvaSettingsSaveGame* Settings = NewObject<UEvaSettingsSaveGame>();
    TestNotNull(TEXT("Settings save object can be created"), Settings);
    if (!Settings)
    {
        return false;
    }

    TestTrue(TEXT("Master volume default is normalized"),
        Settings->MasterVolume >= 0.0f && Settings->MasterVolume <= 1.0f);
    TestTrue(TEXT("BGM volume default is normalized"),
        Settings->BGMVolume >= 0.0f && Settings->BGMVolume <= 1.0f);
    TestTrue(TEXT("SFX volume default is normalized"),
        Settings->SFXVolume >= 0.0f && Settings->SFXVolume <= 1.0f);
    TestTrue(TEXT("Mouse sensitivity default is gameplay-safe"),
        Settings->MouseSensitivity >= 0.1f && Settings->MouseSensitivity <= 5.0f);
    TestFalse(TEXT("Mouse Y inversion is disabled by default"), Settings->bInvertMouseY);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaHorrorBlackoutFlowGuardTest,
    "AdaptiveHorror.Horror.BlackoutFlowGuard",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaHorrorBlackoutFlowGuardTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AEvaPrototypeGameMode* GameMode = World->SpawnActor<AEvaPrototypeGameMode>();
    TestNotNull(TEXT("GameMode spawns for blackout guard test"), GameMode);
    if (!GameMode)
    {
        World->DestroyWorld(false);
        return false;
    }

    GameMode->EnterTitleMode();
    GameMode->TriggerBlackout(1.0f, true);
    TestFalse(TEXT("Blackout does not start in Title flow"), GameMode->IsBlackoutActive());

    GameMode->StartNewGameFlow();
    GameMode->TriggerBlackout(1.0f, true);
    TestTrue(TEXT("Blackout can start during active gameplay"), GameMode->IsBlackoutActive());
    TestTrue(TEXT("Blackout exposes a clamped overlay intensity"),
        GameMode->GetBlackoutOverlayIntensity() >= 0.0f && GameMode->GetBlackoutOverlayIntensity() <= 1.0f);

    GameMode->HandleStageClear();
    TestFalse(TEXT("Stage Clear stops active blackout"), GameMode->IsBlackoutActive());

    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaPlayerHorrorFeedbackClampTest,
    "AdaptiveHorror.Horror.PlayerFeedbackClamp",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaPlayerHorrorFeedbackClampTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AEvaPlayerCharacter* Player = World->SpawnActor<AEvaPlayerCharacter>();
    TestNotNull(TEXT("Player spawns for horror feedback test"), Player);
    if (!Player)
    {
        World->DestroyWorld(false);
        return false;
    }

    Player->TriggerDamageFeedback(999.0f);
    TestTrue(TEXT("Damage feedback intensity is clamped"),
        Player->GetDamageFeedbackIntensity() >= 0.0f && Player->GetDamageFeedbackIntensity() <= 1.0f);
    TestTrue(TEXT("Low-health vignette intensity is clamped"),
        Player->GetLowHealthVignetteIntensity() >= 0.0f && Player->GetLowHealthVignetteIntensity() <= 1.0f);

    Player->SetFlashlightEnabled(false);
    TestFalse(TEXT("Flashlight state can be disabled"), Player->IsFlashlightEnabled());
    Player->SetFlashlightEnabled(true);
    TestTrue(TEXT("Flashlight state can be enabled"), Player->IsFlashlightEnabled());

    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaAdaptationProfileClampTest,
    "AdaptiveHorror.GameplayPass.AdaptationProfileClamp",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaAdaptationProfileClampTest::RunTest(const FString& Parameters)
{
    UGameInstance* GameInstance = NewObject<UGameInstance>();
    UEvaLearningSubsystem* Learning = NewObject<UEvaLearningSubsystem>(GameInstance);

    Learning->DebugAddAnalysis(250.0f);
    Learning->RecordHit(true, 99999.0f);
    Learning->RecordDamageTaken(5000.0f, TEXT("Automation"));
    Learning->RecordSprintUsed();
    const FEvaPlayerAdaptationProfile Profile = Learning->UpdateAdaptationProfile(true);

    TestTrue(TEXT("Analysis is clamped"), Profile.AnalysisPercent >= 0.0f && Profile.AnalysisPercent <= 100.0f);
    TestTrue(TEXT("Accuracy is clamped"), Profile.Accuracy >= 0.0f && Profile.Accuracy <= 1.0f);
    TestTrue(TEXT("Headshot rate is clamped"), Profile.HeadshotRate >= 0.0f && Profile.HeadshotRate <= 1.0f);
    TestTrue(TEXT("Distance is clamped"), Profile.PreferredCombatDistance >= 0.0f && Profile.PreferredCombatDistance <= 5000.0f);
    TestTrue(TEXT("Damage taken rate is clamped"), Profile.DamageTakenRate >= 0.0f && Profile.DamageTakenRate <= 1.0f);
    TestTrue(TEXT("Sprint usage is clamped"), Profile.SprintUsage >= 0.0f && Profile.SprintUsage <= 1.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaAdaptationCombatStyleSelectionTest,
    "AdaptiveHorror.GameplayPass.CombatStyleSelection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaAdaptationCombatStyleSelectionTest::RunTest(const FString& Parameters)
{
    UGameInstance* GameInstance = NewObject<UGameInstance>();

    UEvaLearningSubsystem* BerserkerLearning = NewObject<UEvaLearningSubsystem>(GameInstance);
    BerserkerLearning->RecordHit(false, 260.0f);
    BerserkerLearning->RecordSprintUsed();
    TestTrue(TEXT("Close/sprint profile becomes Berserker"),
        BerserkerLearning->UpdateAdaptationProfile(true).CombatStyle == EEvaCombatStyle::Berserker);

    UEvaLearningSubsystem* RangerLearning = NewObject<UEvaLearningSubsystem>(GameInstance);
    RangerLearning->RecordShot(TEXT("Handgun"));
    RangerLearning->RecordHit(true, 2200.0f);
    TestTrue(TEXT("Accurate long-range profile becomes Ranger"),
        RangerLearning->UpdateAdaptationProfile(true).CombatStyle == EEvaCombatStyle::Ranger);

    UEvaLearningSubsystem* GhostLearning = NewObject<UEvaLearningSubsystem>(GameInstance);
    GhostLearning->RecordHideSpot(TEXT("HideAutomation"));
    TestTrue(TEXT("Hide spot profile becomes Ghost"),
        GhostLearning->UpdateAdaptationProfile(true).CombatStyle == EEvaCombatStyle::Ghost);

    UEvaLearningSubsystem* ExplorerLearning = NewObject<UEvaLearningSubsystem>(GameInstance);
    ExplorerLearning->RecordEscapeRoute(TEXT("RouteAutomation"));
    for (int32 Index = 0; Index < 6; ++Index)
    {
        ExplorerLearning->RecordHit(false, 900.0f);
    }
    TestTrue(TEXT("Route/zone movement profile becomes Explorer"),
        ExplorerLearning->UpdateAdaptationProfile(true).CombatStyle == EEvaCombatStyle::Explorer);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaEnemyAdaptationTuningTest,
    "AdaptiveHorror.GameplayPass.EnemyAdaptationTuning",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaEnemyAdaptationTuningTest::RunTest(const FString& Parameters)
{
    UGameInstance* GameInstance = NewObject<UGameInstance>();
    UEvaLearningSubsystem* Learning = NewObject<UEvaLearningSubsystem>(GameInstance);
    Learning->RecordShot(TEXT("Handgun"));
    Learning->RecordHit(true, 2200.0f);
    Learning->DebugAddAnalysis(70.0f);
    Learning->UpdateAdaptationProfile(true);

    const FEvaEnemyAdaptationTuning FastTuning = Learning->BuildEnemyAdaptationTuning(EEvaEvolutionType::Fast);
    TestTrue(TEXT("Fast tuning acts as flanker"), FastTuning.BehaviorRole == EEvaEnemyBehaviorRole::Flanker);
    TestTrue(TEXT("Fast tuning increases movement within clamp"),
        FastTuning.MoveSpeedMultiplier > 1.0f && FastTuning.MoveSpeedMultiplier <= 1.35f);

    const FEvaEnemyAdaptationTuning LongArmTuning = Learning->BuildEnemyAdaptationTuning(EEvaEvolutionType::LongArm);
    TestTrue(TEXT("Long Arm tuning becomes mid-range pressure"),
        LongArmTuning.BehaviorRole == EEvaEnemyBehaviorRole::MidRangePressure);
    TestTrue(TEXT("Long Arm range multiplier remains bounded"),
        LongArmTuning.AttackRangeMultiplier > 1.0f && LongArmTuning.AttackRangeMultiplier <= 1.55f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaCompositeAdaptationSelectionTest,
    "AdaptiveHorror.GameplayPass.CompositeAdaptationSelection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaCompositeAdaptationSelectionTest::RunTest(const FString& Parameters)
{
    UGameInstance* GameInstance = NewObject<UGameInstance>();
    UEvaLearningSubsystem* Learning = NewObject<UEvaLearningSubsystem>(GameInstance);
    Learning->RecordHideSpot(TEXT("HideAutomation"));
    Learning->DebugAddAnalysis(85.0f);
    Learning->UpdateAdaptationProfile(true);

    const FEvaEnemyAdaptationTuning Tuning = Learning->BuildEnemyAdaptationTuning(EEvaEvolutionType::Composite);
    TestTrue(TEXT("Composite uses adaptive role instead of maximizing all roles"),
        Tuning.BehaviorRole == EEvaEnemyBehaviorRole::CompositeAdaptive);
    TestTrue(TEXT("Composite speed stays fair"), Tuning.MoveSpeedMultiplier <= 1.35f);
    TestTrue(TEXT("Composite damage stays fair"), Tuning.DamageMultiplier <= 1.18f);
    TestTrue(TEXT("Composite produces debug summary"), !Tuning.DebugSummary.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaHunterCounterProfileTest,
    "AdaptiveHorror.GameplayPass.HunterCounterProfile",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaHunterCounterProfileTest::RunTest(const FString& Parameters)
{
    UGameInstance* GameInstance = NewObject<UGameInstance>();
    UEvaLearningSubsystem* Learning = NewObject<UEvaLearningSubsystem>(GameInstance);
    Learning->RecordShot(TEXT("Handgun"));
    Learning->RecordHit(true, 2400.0f);
    Learning->UpdateAdaptationProfile(true);

    TestTrue(TEXT("Tier 1 Hunter counters Ranger"),
        Learning->GetHunterCounterTypeForTier(1) == EEvaHunterCounterType::AntiRanger);

    Learning->RecordHunterDefeatedProfile();
    Learning->RecordHideSpot(TEXT("HideAfterHunter"));
    Learning->UpdateAdaptationProfile(true);
    TestTrue(TEXT("Tier 2 Hunter keeps previous defeated profile counter"),
        Learning->GetHunterCounterTypeForTier(2) == EEvaHunterCounterType::AntiRanger);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaLearningResetClearsProfileTest,
    "AdaptiveHorror.GameplayPass.NewGameProfileReset",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaLearningResetClearsProfileTest::RunTest(const FString& Parameters)
{
    UGameInstance* GameInstance = NewObject<UGameInstance>();
    UEvaLearningSubsystem* Learning = NewObject<UEvaLearningSubsystem>(GameInstance);
    Learning->RecordHit(false, 250.0f);
    TestTrue(TEXT("Profile becomes valid before reset"), Learning->UpdateAdaptationProfile(true).bValid);

    Learning->ResetLearning();
    const FEvaPlayerAdaptationProfile Profile = Learning->GetCurrentAdaptationProfile();
    TestFalse(TEXT("Reset clears adaptation profile validity"), Profile.bValid);
    TestTrue(TEXT("Reset returns analysis to zero"), FMath::IsNearlyZero(Learning->GetEvaAnalysisRate()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaZombieCompositeStatsBoundedTest,
    "AdaptiveHorror.GameplayPass.CompositeStatsBounded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaZombieCompositeStatsBoundedTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AEvaZombieCharacter* Zombie = World->SpawnActor<AEvaZombieCharacter>();
    TestNotNull(TEXT("Zombie spawns for composite bounds test"), Zombie);
    if (!Zombie)
    {
        World->DestroyWorld(false);
        return false;
    }

    Zombie->ConfigureEvolution(EEvaEvolutionType::Composite);
    TestTrue(TEXT("Composite keeps actor scale stable"), Zombie->GetActorScale3D().Equals(FVector::OneVector, KINDA_SMALL_NUMBER));
    TestTrue(TEXT("Composite label is visible on type component"),
        Zombie->GetTypeLabelComponent() && Zombie->GetTypeLabelComponent()->Text.ToString().Contains(TEXT("COMPOSITE")));
    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaGameplayDebugHUDPageBoundsTest,
    "AdaptiveHorror.GameplayPass.Polish.DebugHUDPageBounds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaGameplayDebugHUDPageBoundsTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AEvaPrototypeGameMode* GameMode = World->SpawnActor<AEvaPrototypeGameMode>();
    AEvaZombieCharacter* Zombie = World->SpawnActor<AEvaZombieCharacter>();

    TestNotNull(TEXT("GameMode spawns for debug HUD page test"), GameMode);
    TestNotNull(TEXT("Zombie spawns for debug intent visibility test"), Zombie);
    if (!GameMode || !Zombie)
    {
        World->DestroyWorld(false);
        return false;
    }

    TestEqual(TEXT("Debug HUD uses three pages"), GameMode->GetDebugHUDPageCount(), 3);
    TestTrue(TEXT("Debug HUD default page is in range"),
        GameMode->GetDebugHUDPageIndex() >= 0 && GameMode->GetDebugHUDPageIndex() < GameMode->GetDebugHUDPageCount());

    Zombie->SetDebugIntentText(TEXT("FLANK"));
    TestTrue(TEXT("Debug intent component exists"), Zombie->GetDebugIntentLabelComponent() != nullptr);
    if (UTextRenderComponent* IntentLabel = Zombie->GetDebugIntentLabelComponent())
    {
        TestFalse(TEXT("Debug intent stays hidden while Debug HUD is off"), IntentLabel->IsVisible());
    }

    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaGameplayRolePolishTuningTest,
    "AdaptiveHorror.GameplayPass.Polish.RoleTuning",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaGameplayRolePolishTuningTest::RunTest(const FString& Parameters)
{
    UGameInstance* GameInstance = NewObject<UGameInstance>();
    UEvaLearningSubsystem* Learning = NewObject<UEvaLearningSubsystem>(GameInstance);
    Learning->RecordShot(TEXT("Handgun"));
    Learning->RecordHit(true, 2200.0f);
    Learning->DebugAddAnalysis(75.0f);
    Learning->UpdateAdaptationProfile(true);

    const FEvaEnemyAdaptationTuning StandardTuning = Learning->BuildEnemyAdaptationTuning(EEvaEvolutionType::None);
    const FEvaEnemyAdaptationTuning FastTuning = Learning->BuildEnemyAdaptationTuning(EEvaEvolutionType::Fast);
    const FEvaEnemyAdaptationTuning ArmoredTuning = Learning->BuildEnemyAdaptationTuning(EEvaEvolutionType::Armored);
    const FEvaEnemyAdaptationTuning LongArmTuning = Learning->BuildEnemyAdaptationTuning(EEvaEvolutionType::LongArm);

    TestTrue(TEXT("FAST prioritizes flank/sidestep more than standard"),
        FastTuning.SidestepChance > StandardTuning.SidestepChance);
    TestTrue(TEXT("ARMORED sidesteps/disengages less than standard"),
        ArmoredTuning.SidestepChance < StandardTuning.SidestepChance);
    TestTrue(TEXT("LONG ARM attack range is longer than standard"),
        LongArmTuning.AttackRangeMultiplier > StandardTuning.AttackRangeMultiplier);

    const FEvaEnemyAdaptationTuning Tunings[] = { StandardTuning, FastTuning, ArmoredTuning, LongArmTuning };
    for (const FEvaEnemyAdaptationTuning& Tuning : Tunings)
    {
        TestTrue(TEXT("Speed multiplier remains clamped"),
            Tuning.MoveSpeedMultiplier >= 0.72f && Tuning.MoveSpeedMultiplier <= 1.35f);
        TestTrue(TEXT("Range multiplier remains clamped"),
            Tuning.AttackRangeMultiplier >= 0.90f && Tuning.AttackRangeMultiplier <= 1.55f);
        TestTrue(TEXT("Cooldown multiplier remains clamped"),
            Tuning.AttackCooldownMultiplier >= 0.75f && Tuning.AttackCooldownMultiplier <= 1.35f);
        TestTrue(TEXT("Damage multiplier remains clamped"),
            Tuning.DamageMultiplier >= 0.85f && Tuning.DamageMultiplier <= 1.18f);
        TestTrue(TEXT("Sidestep chance remains clamped"),
            Tuning.SidestepChance >= 0.0f && Tuning.SidestepChance <= 0.65f);
        TestTrue(TEXT("Role label is short and readable"), !Tuning.RoleLabel.IsEmpty() && Tuning.RoleLabel.Len() <= 24);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaCompositeHybridPolishTest,
    "AdaptiveHorror.GameplayPass.Polish.CompositeHybrid",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaCompositeHybridPolishTest::RunTest(const FString& Parameters)
{
    UGameInstance* GameInstance = NewObject<UGameInstance>();
    UEvaLearningSubsystem* Learning = NewObject<UEvaLearningSubsystem>(GameInstance);
    Learning->RecordShot(TEXT("Handgun"));
    Learning->RecordHit(true, 2400.0f);
    Learning->DebugAddAnalysis(90.0f);
    Learning->UpdateAdaptationProfile(true);

    const FEvaEnemyAdaptationTuning CompositeTuning =
        Learning->BuildEnemyAdaptationTuning(EEvaEvolutionType::Composite);

    TestTrue(TEXT("COMPOSITE remains adaptive role"),
        CompositeTuning.BehaviorRole == EEvaEnemyBehaviorRole::CompositeAdaptive);
    TestTrue(TEXT("COMPOSITE chooses at most two hybrid roles"),
        CompositeTuning.CompositeHybridRoleCount >= 1 && CompositeTuning.CompositeHybridRoleCount <= 2);
    TestTrue(TEXT("COMPOSITE exposes a hybrid type"), !CompositeTuning.CompositeHybridType.IsEmpty());
    TestTrue(TEXT("COMPOSITE hybrid has a nonzero hold duration"),
        CompositeTuning.CompositeHybridHoldSeconds >= 4.0f);
    TestTrue(TEXT("COMPOSITE tuning remains clamped"),
        CompositeTuning.MoveSpeedMultiplier <= 1.35f &&
        CompositeTuning.AttackRangeMultiplier <= 1.55f &&
        CompositeTuning.DamageMultiplier <= 1.18f &&
        CompositeTuning.SidestepChance <= 0.65f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaEnemyIntentInitializesAfterSpawnTest,
    "AdaptiveHorror.GameplayPass.Polish.IntentInitializesAfterSpawn",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaEnemyIntentInitializesAfterSpawnTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    const EEvaEvolutionType Types[] =
    {
        EEvaEvolutionType::None,
        EEvaEvolutionType::Fast,
        EEvaEvolutionType::Armored,
        EEvaEvolutionType::LongArm,
        EEvaEvolutionType::Composite
    };

    for (const EEvaEvolutionType Type : Types)
    {
        AEvaZombieCharacter* Zombie = World->SpawnActor<AEvaZombieCharacter>();
        TestNotNull(TEXT("Zombie spawns for intent initialization"), Zombie);
        if (!Zombie)
        {
            continue;
        }

        Zombie->ConfigureEvolution(Type);
        Zombie->RefreshDebugIntentDisplay(true);
        TestFalse(TEXT("Spawned enemy resolves a non-empty intent"), Zombie->GetResolvedDebugIntentText().IsEmpty());
        if (UTextRenderComponent* IntentLabel = Zombie->GetDebugIntentLabelComponent())
        {
            TestFalse(TEXT("Debug OFF keeps intent label hidden"), IntentLabel->IsVisible());
            TestFalse(TEXT("Intent label text is not empty after refresh"), IntentLabel->Text.ToString().IsEmpty());
        }
    }

    AEvaHunterCharacter* Hunter = World->SpawnActor<AEvaHunterCharacter>();
    TestNotNull(TEXT("Hunter spawns for intent initialization"), Hunter);
    if (Hunter)
    {
        Hunter->SetHunterCounterType(EEvaHunterCounterType::AntiRanger);
        Hunter->RefreshDebugIntentDisplay(true);
        TestTrue(TEXT("Hunter preserves counter intent"),
            Hunter->GetResolvedDebugIntentText().Contains(TEXT("ANTI-RANGER")));
    }

    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaEnemyIntentControllerFallbackTest,
    "AdaptiveHorror.GameplayPass.Polish.IntentControllerFallback",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaEnemyIntentControllerFallbackTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AEvaZombieCharacter* Zombie = World->SpawnActor<AEvaZombieCharacter>();
    AEvaZombieAIController* Controller = World->SpawnActor<AEvaZombieAIController>();

    TestNotNull(TEXT("Zombie spawns for controller intent fallback"), Zombie);
    TestNotNull(TEXT("Controller spawns for controller intent fallback"), Controller);
    if (!Zombie || !Controller)
    {
        World->DestroyWorld(false);
        return false;
    }

    Controller->Possess(Zombie);
    Controller->EnsureCurrentActionIntent();
    Zombie->RefreshDebugIntentDisplay(true);

    TestFalse(TEXT("Controller returns a non-empty fallback intent"), Controller->GetCurrentActionIntent().IsEmpty());
    TestFalse(TEXT("Existing enemy refresh resolves non-empty intent"), Zombie->GetResolvedDebugIntentText().IsEmpty());
    if (UTextRenderComponent* IntentLabel = Zombie->GetDebugIntentLabelComponent())
    {
        TestFalse(TEXT("Debug OFF hides refreshed intent label"), IntentLabel->IsVisible());
        TestFalse(TEXT("Refreshed intent label stores text"), IntentLabel->Text.ToString().IsEmpty());
    }

    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaContentPassObjectiveProgressionTest,
    "AdaptiveHorror.ContentPass.ObjectiveProgression",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaContentPassObjectiveProgressionTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AEvaResearchFacilityDirector* Director = World->SpawnActor<AEvaResearchFacilityDirector>();
    TestNotNull(TEXT("Director spawns for content objective test"), Director);
    if (!Director)
    {
        World->DestroyWorld(false);
        return false;
    }

    Director->ResetForNewGame();
    TestEqual(TEXT("New Game starts at objective 0"), Director->GetObjectiveIndex(), 0);
    TestTrue(TEXT("Power restore succeeds once"), Director->TryRestoreFacilityPower());
    TestEqual(TEXT("Power advances objective to keycard"), Director->GetObjectiveIndex(), 1);
    TestFalse(TEXT("Power restore cannot run twice"), Director->TryRestoreFacilityPower());
    TestTrue(TEXT("Keycard pickup succeeds once"), Director->TryAcquireSecurityKeycard());
    TestEqual(TEXT("Keycard advances objective to door"), Director->GetObjectiveIndex(), 2);
    TestFalse(TEXT("Keycard cannot be collected twice"), Director->TryAcquireSecurityKeycard());

    Director->CompleteStage();
    const int32 StageClearObjective = Director->GetObjectiveIndex();
    TestFalse(TEXT("Stage Clear blocks late keycard progress"), Director->TryAcquireSecurityKeycard());
    TestEqual(TEXT("Stage Clear objective remains unchanged"), Director->GetObjectiveIndex(), StageClearObjective);

    Director->ResetForNewGame();
    TestFalse(TEXT("New Game reset clears power"), Director->IsFacilityPowerOnline());
    TestFalse(TEXT("New Game reset clears keycard"), Director->HasSecurityKeycard());
    TestEqual(TEXT("New Game reset restores objective 0"), Director->GetObjectiveIndex(), 0);

    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaContentPassDoorAndDataCoreTest,
    "AdaptiveHorror.ContentPass.DoorAndDataCore",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaContentPassDoorAndDataCoreTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AEvaResearchFacilityDirector* Director = World->SpawnActor<AEvaResearchFacilityDirector>();
    TestNotNull(TEXT("Director spawns for content door/data core test"), Director);
    if (!Director)
    {
        World->DestroyWorld(false);
        return false;
    }

    Director->ResetForNewGame();
    TestFalse(TEXT("Door rejects without keycard"), Director->TryOpenObservationDoor());
    TestFalse(TEXT("Door remains locked without keycard"), Director->IsObservationDoorOpen());
    Director->TryAcquireSecurityKeycard();
    TestTrue(TEXT("Door opens after keycard"), Director->TryOpenObservationDoor());
    TestFalse(TEXT("Door cannot open twice"), Director->TryOpenObservationDoor());
    TestTrue(TEXT("Observation door state persists"), Director->IsObservationDoorOpen());

    TestFalse(TEXT("Data Core rejects before a research log"), Director->TryAccessDataCore());
    TestTrue(TEXT("First research log read succeeds"), Director->TryReadResearchLog(
        FName(TEXT("AutomationLog")), TEXT("Automation Log"), TEXT("Automation body.")));
    TestEqual(TEXT("First log increments collection"), Director->GetCollectedStoryLogCount(), 1);
    Director->CloseResearchLog();
    TestTrue(TEXT("Research log can be reread"), Director->TryReadResearchLog(
        FName(TEXT("AutomationLog")), TEXT("Automation Log"), TEXT("Automation body.")));
    TestEqual(TEXT("Reread does not double-count"), Director->GetCollectedStoryLogCount(), 1);
    Director->CloseResearchLog();

    TestTrue(TEXT("Data Core succeeds after log"), Director->TryAccessDataCore());
    TestTrue(TEXT("Adam Arena unlocks after Data Core"), Director->IsAdamArenaUnlocked());
    TestFalse(TEXT("Data Core cannot complete twice"), Director->TryAccessDataCore());

    World->DestroyWorld(false);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FEvaContentPassInteractablePromptTest,
    "AdaptiveHorror.ContentPass.InteractablePrompt",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FEvaContentPassInteractablePromptTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    AEvaResearchFacilityDirector* Director = World->SpawnActor<AEvaResearchFacilityDirector>();
    AEvaPlayerCharacter* Player = World->SpawnActor<AEvaPlayerCharacter>();
    AEvaFacilityInteractable* Keycard = World->SpawnActor<AEvaFacilityInteractable>();
    AEvaFacilityInteractable* Door = World->SpawnActor<AEvaFacilityInteractable>();

    TestNotNull(TEXT("Director spawns for prompt test"), Director);
    TestNotNull(TEXT("Player spawns for prompt test"), Player);
    TestNotNull(TEXT("Keycard interactable spawns"), Keycard);
    TestNotNull(TEXT("Door interactable spawns"), Door);
    if (!Director || !Player || !Keycard || !Door)
    {
        World->DestroyWorld(false);
        return false;
    }

    Director->ResetForNewGame();
    Keycard->ConfigureInteractable(EEvaFacilityInteractableType::Keycard, Director, TEXT("SECURITY KEYCARD"));
    Door->ConfigureInteractable(EEvaFacilityInteractableType::LockedDoor, Director, TEXT("OBSERVATION LAB LOCK"));

    TestEqual(TEXT("Keycard prompt is explicit"), Keycard->GetInteractionPrompt(Player),
        FString(TEXT("E - PICK UP KEYCARD")));
    TestEqual(TEXT("Locked door prompt reports missing keycard"), Door->GetInteractionPrompt(Player),
        FString(TEXT("E - KEYCARD REQUIRED")));
    TestTrue(TEXT("Keycard interact succeeds"), Keycard->Interact(Player));
    TestEqual(TEXT("Collected keycard hides prompt"), Keycard->GetInteractionPrompt(Player), FString(TEXT("")));
    TestEqual(TEXT("Door prompt changes after keycard"), Door->GetInteractionPrompt(Player),
        FString(TEXT("E - UNLOCK OBSERVATION LAB")));
    TestTrue(TEXT("Door interact succeeds after keycard"), Door->Interact(Player));

    World->DestroyWorld(false);
    return true;
}

#endif
