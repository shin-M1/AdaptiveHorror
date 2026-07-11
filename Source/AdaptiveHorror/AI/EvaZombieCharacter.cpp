#include "AI/EvaZombieCharacter.h"
#include "AdaptiveHorror.h"
#include "AI/EvaZombieAIController.h"
#include "Characters/EvaPlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/EvaHealthComponent.h"
#include "Components/EvaPlayerTelemetryComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Core/EvaPrototypeGameMode.h"
#include "Engine/Engine.h"
#include "UObject/ConstructorHelpers.h"

AEvaZombieCharacter::AEvaZombieCharacter()
{
    PrimaryActorTick.bCanEverTick = false;
    Tags.Add(TEXT("Zombie"));

    GetCapsuleComponent()->InitCapsuleSize(42.0f, 88.0f);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

    HealthComponent = CreateDefaultSubobject<UEvaHealthComponent>(TEXT("HealthComponent"));
    HealthComponent->SetMaxHealth(BaseHealth);

    BodyVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BodyVisual"));
    BodyVisual->SetupAttachment(GetCapsuleComponent());
    BodyVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    BodyVisual->SetRelativeLocation(FVector(0.0f, 0.0f, 10.0f));
    BodyVisual->SetRelativeScale3D(FVector(0.55f, 0.45f, 0.9f));

    HeadVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("HeadVisual"));
    HeadVisual->SetupAttachment(GetCapsuleComponent());
    HeadVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HeadVisual->SetRelativeLocation(FVector(0.0f, 0.0f, 105.0f));
    HeadVisual->SetRelativeScale3D(FVector(0.35f));

    TypeLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TypeLabel"));
    TypeLabel->SetupAttachment(GetCapsuleComponent());
    TypeLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TypeLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 165.0f));
    TypeLabel->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
    TypeLabel->SetHorizontalAlignment(EHTA_Center);
    TypeLabel->SetVerticalAlignment(EVRTA_TextCenter);
    TypeLabel->SetText(FText::FromString(TEXT("ZOMBIE")));
    TypeLabel->SetTextRenderColor(FColor::Green);
    TypeLabel->SetWorldSize(42.0f);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (CubeMesh.Succeeded())
    {
        BodyVisual->SetStaticMesh(CubeMesh.Object);
    }
    if (SphereMesh.Succeeded())
    {
        HeadVisual->SetStaticMesh(SphereMesh.Object);
    }

    TorsoHitbox = CreateDefaultSubobject<UBoxComponent>(TEXT("TorsoHitbox"));
    TorsoHitbox->SetupAttachment(GetCapsuleComponent());
    TorsoHitbox->SetBoxExtent(FVector(35.0f, 35.0f, 60.0f));
    TorsoHitbox->SetRelativeLocation(FVector(0.0f, 0.0f, 15.0f));
    TorsoHitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    TorsoHitbox->SetCollisionResponseToAllChannels(ECR_Ignore);
    TorsoHitbox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    TorsoHitbox->ComponentTags.Add(TEXT("Torso"));

    HeadHitbox = CreateDefaultSubobject<USphereComponent>(TEXT("HeadHitbox"));
    HeadHitbox->SetupAttachment(GetCapsuleComponent());
    HeadHitbox->SetSphereRadius(28.0f);
    HeadHitbox->SetRelativeLocation(FVector(0.0f, 0.0f, 105.0f));
    HeadHitbox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    HeadHitbox->SetCollisionResponseToAllChannels(ECR_Ignore);
    HeadHitbox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    HeadHitbox->ComponentTags.Add(TEXT("Head"));

    AIControllerClass = AEvaZombieAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
    GetCharacterMovement()->MaxWalkSpeed = MovementSpeed;
}

void AEvaZombieCharacter::BeginPlay()
{
    Super::BeginPlay();
    if (HealthComponent)
    {
        HealthComponent->OnDeath.AddDynamic(this, &AEvaZombieCharacter::HandleDeath);
    }
    ConfigureEvolution(EvolutionType);
    UE_LOG(LogAdaptiveHorror, Log,
        TEXT("[EnemyVisual] BeginPlay Actor=%s Class=%s EnemyType=%s DebugLabelComponent=%s LabelText=%s LabelVisible=%s CapsuleScale=%s BodyScale=%s HeadScale=%s ActorScale=%s AIControllerClass=%s AutoPossessAI=%d Controller=%s"),
        *GetName(),
        *GetClass()->GetName(),
        *UEnum::GetValueAsString(EvolutionType),
        TypeLabel ? TEXT("true") : TEXT("false"),
        TypeLabel ? *TypeLabel->Text.ToString() : TEXT("None"),
        TypeLabel && TypeLabel->IsVisible() ? TEXT("true") : TEXT("false"),
        *GetCapsuleComponent()->GetRelativeScale3D().ToCompactString(),
        BodyVisual ? *BodyVisual->GetRelativeScale3D().ToCompactString() : TEXT("None"),
        HeadVisual ? *HeadVisual->GetRelativeScale3D().ToCompactString() : TEXT("None"),
        *GetActorScale3D().ToCompactString(),
        AIControllerClass ? *AIControllerClass->GetName() : TEXT("None"),
        static_cast<int32>(AutoPossessAI),
        GetController() ? *GetController()->GetClass()->GetName() : TEXT("None"));
}

float AEvaZombieCharacter::TakeDamage(const float DamageAmount, const FDamageEvent& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    if (HealthComponent && HealthComponent->IsDead())
    {
        return 0.0f;
    }

    float AdjustedDamage = DamageAmount;
    if (HeadDamageMultiplier < 1.0f && DamageEvent.IsOfType(FPointDamageEvent::ClassID))
    {
        const FPointDamageEvent* PointDamageEvent = static_cast<const FPointDamageEvent*>(&DamageEvent);
        if (PointDamageEvent && PointDamageEvent->HitInfo.GetComponent() &&
            PointDamageEvent->HitInfo.GetComponent()->ComponentHasTag(TEXT("Head")))
        {
            AdjustedDamage *= HeadDamageMultiplier;
        }
    }

    LastDamageInstigator = EventInstigator;
    if (EventInstigator && EventInstigator->GetPawn())
    {
        AlertToPlayer(EventInstigator->GetPawn());
    }
    return Super::TakeDamage(AdjustedDamage, DamageEvent, EventInstigator, DamageCauser);
}

void AEvaZombieCharacter::AlertToPlayer(APawn* PlayerPawn)
{
    const AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(PlayerPawn);
    if (!PlayerPawn || (Player && Player->IsDead()))
    {
        return;
    }

    if (AEvaZombieAIController* ZombieController = Cast<AEvaZombieAIController>(GetController()))
    {
        ZombieController->SetPlayerTarget(PlayerPawn);
    }
}

void AEvaZombieCharacter::ConfigureEvolution(const EEvaEvolutionType NewEvolutionType)
{
    EvolutionType = NewEvolutionType;

    float NewMaxHealth = BaseHealth;
    MovementSpeed = BaseMovementSpeed;
    HeadDamageMultiplier = 1.0f;
    CurrentAttackRange = BaseAttackRange;
    CurrentAttackDamage = BaseAttackDamage;
    FVector BodyScale(0.55f, 0.45f, 0.9f);
    FVector HeadScale(0.35f);
    FVector ActorScale(1.0f);
    FString DebugLabel(TEXT("ZOMBIE"));
    FColor DebugLabelColor = FColor::Green;

    const bool bFast = NewEvolutionType == EEvaEvolutionType::Fast || NewEvolutionType == EEvaEvolutionType::Composite;
    const bool bArmored = NewEvolutionType == EEvaEvolutionType::Armored || NewEvolutionType == EEvaEvolutionType::Composite;
    const bool bLongArm = NewEvolutionType == EEvaEvolutionType::LongArm || NewEvolutionType == EEvaEvolutionType::Composite;

    if (bFast)
    {
        MovementSpeed *= 1.25f;
        BodyScale = FVector(0.45f, 0.35f, 1.05f);
        DebugLabel = TEXT("FAST");
        DebugLabelColor = FColor::Cyan;
    }
    if (bArmored)
    {
        NewMaxHealth *= 1.30f;
        HeadDamageMultiplier = 0.5f;
        BodyScale = FVector(FMath::Max(BodyScale.X, 0.7f), FMath::Max(BodyScale.Y, 0.55f), FMath::Max(BodyScale.Z, 1.05f));
        HeadScale = FVector(0.45f);
        DebugLabel = TEXT("ARMORED");
        DebugLabelColor = FColor::Yellow;
    }
    if (bLongArm)
    {
        CurrentAttackRange += 250.0f;
        CurrentAttackDamage *= 1.15f;
        ActorScale = FVector(1.05f, 1.05f, 1.15f);
        DebugLabel = TEXT("LONG ARM");
        DebugLabelColor = FColor(170, 90, 255);
    }
    if (NewEvolutionType == EEvaEvolutionType::Composite)
    {
        Tags.AddUnique(TEXT("EvolvedComposite"));
        DebugLabel = TEXT("COMPOSITE");
        DebugLabelColor = FColor::Magenta;
    }

    if (HealthComponent)
    {
        HealthComponent->SetMaxHealth(NewMaxHealth, true);
    }
    if (BodyVisual)
    {
        BodyVisual->SetRelativeScale3D(BodyScale);
    }
    if (HeadVisual)
    {
        HeadVisual->SetRelativeScale3D(HeadScale);
    }
    SetActorScale3D(ActorScale);
    SetPrototypeDebugLabel(DebugLabel, DebugLabelColor);
    if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
    {
        MovementComponent->MaxWalkSpeed = MovementSpeed;
    }
    ApplyEvolutionToController();
}

void AEvaZombieCharacter::SetPrototypeDebugLabel(const FString& Label, const FColor& Color, const float WorldSize)
{
    if (!TypeLabel)
    {
        return;
    }

    TypeLabel->SetText(FText::FromString(Label));
    TypeLabel->SetTextRenderColor(Color);
    TypeLabel->SetWorldSize(WorldSize);
}

void AEvaZombieCharacter::ApplyEvolutionToController()
{
    if (AEvaZombieAIController* ZombieController = Cast<AEvaZombieAIController>(GetController()))
    {
        ZombieController->ConfigureCombat(CurrentAttackRange, CurrentAttackDamage, 1.5f);
    }
}

void AEvaZombieCharacter::HandleDeath(AActor* DeadActor)
{
    OnDefeated();
}

void AEvaZombieCharacter::OnDefeated()
{
    if (bDefeatHandled)
    {
        return;
    }
    bDefeatHandled = true;

    AEvaPlayerCharacter* Player = LastDamageInstigator.IsValid() ?
        Cast<AEvaPlayerCharacter>(LastDamageInstigator->GetPawn()) : nullptr;
    if (!Player && GetWorld() && GetWorld()->GetFirstPlayerController())
    {
        Player = Cast<AEvaPlayerCharacter>(GetWorld()->GetFirstPlayerController()->GetPawn());
    }
    if (Player && Player->GetTelemetryComponent())
    {
        Player->GetTelemetryComponent()->RecordKill();
    }
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->NotifyEnemyKilled(this);
    }

    if (AAIController* ZombieController = Cast<AAIController>(GetController()))
    {
        ZombieController->StopMovement();
    }
    if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
    {
        MovementComponent->DisableMovement();
        MovementComponent->StopMovementImmediately();
    }
    if (UCapsuleComponent* Capsule = GetCapsuleComponent())
    {
        Capsule->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    if (TorsoHitbox)
    {
        TorsoHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    if (HeadHitbox)
    {
        HeadHitbox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    if (BodyVisual)
    {
        BodyVisual->SetVisibility(false, true);
    }
    if (HeadVisual)
    {
        HeadVisual->SetVisibility(false, true);
    }
    if (TypeLabel)
    {
        TypeLabel->SetVisibility(false, true);
    }
#if !UE_BUILD_SHIPPING
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Green,
            FString::Printf(TEXT("Infected defeated: %s"), *GetName()));
    }
#endif
    SetLifeSpan(2.0f);
}
