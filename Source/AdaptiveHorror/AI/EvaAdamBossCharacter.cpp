#include "AI/EvaAdamBossCharacter.h"
#include "AI/EvaAdamBossAIController.h"
#include "AI/EvaLearningSubsystem.h"
#include "Components/CapsuleComponent.h"
#include "Components/EvaHealthComponent.h"
#include "Core/EvaPrototypeGameMode.h"
#include "Engine/Engine.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "World/EvaResearchFacilityDirector.h"

AEvaAdamBossCharacter::AEvaAdamBossCharacter()
{
    Tags.AddUnique(TEXT("Adam"));
    Tags.AddUnique(TEXT("Boss"));

    BaseHealth = AdamHP;
    BaseMovementSpeed = AdamMovementSpeed;
    MovementSpeed = AdamMovementSpeed;
    BaseAttackRange = 260.0f;
    BaseAttackDamage = AdamMeleeDamage;
    CurrentAttackRange = BaseAttackRange;
    CurrentAttackDamage = AdamMeleeDamage;
    RoarMinionClass = AEvaZombieCharacter::StaticClass();

    AIControllerClass = AEvaAdamBossAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
}

void AEvaAdamBossCharacter::BeginPlay()
{
    const float EffectiveMaxHealth = GetEffectiveAdamMaxHealth();
    BaseHealth = EffectiveMaxHealth;
    BaseMovementSpeed = AdamMovementSpeed;
    MovementSpeed = AdamMovementSpeed;
    BaseAttackDamage = AdamMeleeDamage;
    CurrentAttackDamage = AdamMeleeDamage;
    bDisplayOverheadHealthBar = false;
    Super::BeginPlay();

    if (HealthComponent)
    {
        HealthComponent->SetMaxHealth(EffectiveMaxHealth, true);
        HealthComponent->OnHealthChanged.AddDynamic(this, &AEvaAdamBossCharacter::HandleAdamHealthChanged);
    }
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCapsuleSize(70.0f, 130.0f);
    }
    if (BodyVisual)
    {
        BodyVisual->SetRelativeScale3D(FVector(1.15f, 0.85f, 1.6f));
    }
    if (HeadVisual)
    {
        HeadVisual->SetRelativeScale3D(FVector(0.75f));
    }
    if (LeftArmVisual)
    {
        LeftArmVisual->SetRelativeLocation(FVector(0.0f, -86.0f, 48.0f));
        LeftArmVisual->SetRelativeScale3D(FVector(0.34f, 0.30f, 1.38f));
    }
    if (RightArmVisual)
    {
        RightArmVisual->SetRelativeLocation(FVector(0.0f, 86.0f, 48.0f));
        RightArmVisual->SetRelativeScale3D(FVector(0.34f, 0.30f, 1.38f));
    }
    SetPrototypeDebugLabel(TEXT("ADAM"), FColor(255, 128, 0), 78.0f);
    SetOverheadHealthBarEnabled(false);
    LogPrototypeDebugLabelState(TEXT("AdamBeginPlayFinal"));
    if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
    {
        MovementComponent->MaxWalkSpeed = AdamMovementSpeed;
    }
    if (AEvaAdamBossAIController* AdamController = Cast<AEvaAdamBossAIController>(GetController()))
    {
        AdamController->ConfigureCombat(CurrentAttackRange, CurrentAttackDamage, AdamAttackInterval);
    }
    else
    {
        ApplyEvolutionToController();
    }
}

bool AEvaAdamBossCharacter::ShouldEnterPhaseTwo(const float CurrentHealth, const float MaxHealth) const
{
    return MaxHealth > 0.0f && CurrentHealth <= MaxHealth * 0.5f;
}

void AEvaAdamBossCharacter::EnterPhaseTwo()
{
    if (bPhaseTwo)
    {
        return;
    }

    bPhaseTwo = true;
    MovementSpeed = AdamMovementSpeed * 1.2f;
    CurrentAttackDamage = AdamMeleeDamage;
    CurrentAttackRange = BaseAttackRange;
    if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
    {
        MovementComponent->MaxWalkSpeed = MovementSpeed;
    }
    if (BodyVisual)
    {
        BodyVisual->SetRelativeScale3D(FVector(1.3f, 0.95f, 1.75f));
    }
    if (HeadVisual)
    {
        HeadVisual->SetRelativeScale3D(FVector(0.85f));
    }
    if (LeftArmVisual)
    {
        LeftArmVisual->SetRelativeLocation(FVector(0.0f, -96.0f, 44.0f));
        LeftArmVisual->SetRelativeScale3D(FVector(0.40f, 0.34f, 1.55f));
    }
    if (RightArmVisual)
    {
        RightArmVisual->SetRelativeLocation(FVector(0.0f, 96.0f, 44.0f));
        RightArmVisual->SetRelativeScale3D(FVector(0.40f, 0.34f, 1.55f));
    }
    SetPrototypeDebugLabel(TEXT("ADAM"), FColor::Red, 82.0f);
    SetOverheadHealthBarEnabled(false);
    LogPrototypeDebugLabelState(TEXT("AdamPhaseTwoLabel"));

#if !UE_BUILD_SHIPPING
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 5.0f, FColor::Red, TEXT("ADAM PHASE 2 - EVA is adapting the vessel."));
    }
#endif

    if (AEvaAdamBossAIController* AdamController = Cast<AEvaAdamBossAIController>(GetController()))
    {
        AdamController->ConfigureCombat(CurrentAttackRange, CurrentAttackDamage, AdamAttackInterval * 0.8f);
    }
    else
    {
        ApplyEvolutionToController();
    }

    if (!bPhaseTwoMinionSpawned)
    {
        bPhaseTwoMinionSpawned = true;
        SpawnRoarMinions(1, true);
    }
}

void AEvaAdamBossCharacter::SpawnRoarMinions(const int32 Count, const bool bForceEvolved)
{
    if (!GetWorld() || !RoarMinionClass || Count <= 0)
    {
        return;
    }

    LastSummonCount = Count;
    TotalSummonCount += Count;

    EEvaEvolutionType MinionEvolutionType = bForceEvolved ? EEvaEvolutionType::Composite : EEvaEvolutionType::None;
    if (!bForceEvolved)
    {
        if (const UGameInstance* GameInstance = GetWorld()->GetGameInstance())
        {
            if (const UEvaLearningSubsystem* Learning = GameInstance->GetSubsystem<UEvaLearningSubsystem>())
            {
                MinionEvolutionType = Learning->GetRecommendedEvolutionType();
            }
        }
    }

    for (int32 Index = 0; Index < Count; ++Index)
    {
        const float Angle = (360.0f / FMath::Max(1, Count)) * Index;
        const FVector Offset = FRotator(0.0f, Angle, 0.0f).Vector() * 420.0f;

        if (AEvaPrototypeGameMode* GameMode = GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>())
        {
            GameMode->SpawnEnemyNearLocation(RoarMinionClass, GetActorLocation() + Offset,
                220.0f, 680.0f, TEXT("Zombie"), TEXT("AdamRoarMinion"), MinionEvolutionType);
            continue;
        }

        FActorSpawnParameters SpawnParameters;
        SpawnParameters.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
        AEvaZombieCharacter* Minion = GetWorld()->SpawnActor<AEvaZombieCharacter>(RoarMinionClass,
            GetActorLocation() + Offset, FRotator::ZeroRotator, SpawnParameters);
        if (Minion)
        {
            Minion->ConfigureEvolution(MinionEvolutionType);
        }
    }
}

void AEvaAdamBossCharacter::HandleAdamHealthChanged(const float CurrentHealth, const float MaxHealth, const float Delta)
{
    if (!bPhaseTwo && ShouldEnterPhaseTwo(CurrentHealth, MaxHealth))
    {
        EnterPhaseTwo();
    }
}

void AEvaAdamBossCharacter::OnDefeated()
{
    bool bNotifiedDirector = false;
    TArray<AActor*> Directors;
    if (GetWorld())
    {
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEvaResearchFacilityDirector::StaticClass(), Directors);
    }
    if (Directors.Num() > 0)
    {
        if (AEvaResearchFacilityDirector* Director = Cast<AEvaResearchFacilityDirector>(Directors[0]))
        {
            Director->NotifyAdamDefeated(this);
            bNotifiedDirector = true;
        }
    }
    if (!bNotifiedDirector)
    {
        if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
        {
            GameMode->HandleStageClear();
        }
    }

    Super::OnDefeated();
}
