#include "AI/EvaZombieCharacter.h"
#include "AdaptiveHorror.h"
#include "AI/EvaZombieAIController.h"
#include "Audio/EvaAudioFunctionLibrary.h"
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
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Core/EvaPrototypeGameMode.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

#if !UE_BUILD_SHIPPING
static TAutoConsoleVariable<int32> CVarEvaDebugEnemyHealthNumbers(
    TEXT("Eva.DebugEnemyHealthNumbers"),
    0,
    TEXT("Shows CurrentHP / MaxHP under normal enemy overhead HP bars when set to 1."));
#endif

namespace
{
const FName EvaVisualActionAttack(TEXT("Attack"));
const FName EvaVisualActionCharge(TEXT("Charge"));
const FName EvaVisualActionRoar(TEXT("Roar"));

void ApplyColorToComponent(UStaticMeshComponent* Component, const FLinearColor& Color)
{
    if (!Component)
    {
        return;
    }

    UMaterialInstanceDynamic* DynamicMaterial = Component->CreateAndSetMaterialInstanceDynamic(0);
    if (DynamicMaterial)
    {
        DynamicMaterial->SetVectorParameterValue(TEXT("Color"), Color);
        DynamicMaterial->SetVectorParameterValue(TEXT("BaseColor"), Color);
        DynamicMaterial->SetVectorParameterValue(TEXT("Tint"), Color);
    }
}
}

AEvaZombieCharacter::AEvaZombieCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.TickInterval = 0.10f;
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

    LeftArmVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftArmVisual"));
    LeftArmVisual->SetupAttachment(GetCapsuleComponent());
    LeftArmVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    LeftArmVisual->SetRelativeLocation(FVector(0.0f, -42.0f, 46.0f));
    LeftArmVisual->SetRelativeScale3D(FVector(0.16f, 0.16f, 0.58f));

    RightArmVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightArmVisual"));
    RightArmVisual->SetupAttachment(GetCapsuleComponent());
    RightArmVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RightArmVisual->SetRelativeLocation(FVector(0.0f, 42.0f, 46.0f));
    RightArmVisual->SetRelativeScale3D(FVector(0.16f, 0.16f, 0.58f));

    LeftLegVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftLegVisual"));
    LeftLegVisual->SetupAttachment(GetCapsuleComponent());
    LeftLegVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    LeftLegVisual->SetRelativeLocation(FVector(0.0f, -18.0f, -52.0f));
    LeftLegVisual->SetRelativeScale3D(FVector(0.16f, 0.16f, 0.68f));

    RightLegVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightLegVisual"));
    RightLegVisual->SetupAttachment(GetCapsuleComponent());
    RightLegVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RightLegVisual->SetRelativeLocation(FVector(0.0f, 18.0f, -52.0f));
    RightLegVisual->SetRelativeScale3D(FVector(0.16f, 0.16f, 0.68f));

    LeftShoulderVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftShoulderVisual"));
    LeftShoulderVisual->SetupAttachment(GetCapsuleComponent());
    LeftShoulderVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    LeftShoulderVisual->SetRelativeLocation(FVector(0.0f, -45.0f, 72.0f));
    LeftShoulderVisual->SetRelativeScale3D(FVector(0.22f, 0.18f, 0.18f));

    RightShoulderVisual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightShoulderVisual"));
    RightShoulderVisual->SetupAttachment(GetCapsuleComponent());
    RightShoulderVisual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RightShoulderVisual->SetRelativeLocation(FVector(0.0f, 45.0f, 72.0f));
    RightShoulderVisual->SetRelativeScale3D(FVector(0.22f, 0.18f, 0.18f));

    TypeLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("TypeLabel"));
    TypeLabel->SetupAttachment(GetCapsuleComponent());
    TypeLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TypeLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 165.0f));
    TypeLabel->SetRelativeRotation(FRotator::ZeroRotator);
    TypeLabel->SetHorizontalAlignment(EHTA_Center);
    TypeLabel->SetVerticalAlignment(EVRTA_TextCenter);
    TypeLabel->SetText(FText::FromString(TEXT("ZOMBIE")));
    TypeLabel->SetTextRenderColor(FColor::Green);
    TypeLabel->SetWorldSize(42.0f);
    TypeLabel->SetVisibility(true, true);
    TypeLabel->SetHiddenInGame(false, true);
    TypeLabel->SetOwnerNoSee(false);
    TypeLabel->SetOnlyOwnerSee(false);

    HealthBarLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("HealthBarLabel"));
    HealthBarLabel->SetupAttachment(GetCapsuleComponent());
    HealthBarLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HealthBarLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 139.0f));
    HealthBarLabel->SetRelativeRotation(FRotator::ZeroRotator);
    HealthBarLabel->SetHorizontalAlignment(EHTA_Center);
    HealthBarLabel->SetVerticalAlignment(EVRTA_TextCenter);
    HealthBarLabel->SetText(FText::FromString(TEXT("[##########]")));
    HealthBarLabel->SetTextRenderColor(FColor::Green);
    HealthBarLabel->SetWorldSize(24.0f);
    HealthBarLabel->SetVisibility(true, true);
    HealthBarLabel->SetHiddenInGame(false, true);
    HealthBarLabel->SetOwnerNoSee(false);
    HealthBarLabel->SetOnlyOwnerSee(false);

    HealthValueLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("HealthValueLabel"));
    HealthValueLabel->SetupAttachment(GetCapsuleComponent());
    HealthValueLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    HealthValueLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 119.0f));
    HealthValueLabel->SetRelativeRotation(FRotator::ZeroRotator);
    HealthValueLabel->SetHorizontalAlignment(EHTA_Center);
    HealthValueLabel->SetVerticalAlignment(EVRTA_TextCenter);
    HealthValueLabel->SetText(FText::FromString(TEXT("100 / 100")));
    HealthValueLabel->SetTextRenderColor(FColor::White);
    HealthValueLabel->SetWorldSize(18.0f);
    HealthValueLabel->SetVisibility(false, true);
    HealthValueLabel->SetHiddenInGame(true, true);
    HealthValueLabel->SetOwnerNoSee(false);
    HealthValueLabel->SetOnlyOwnerSee(false);

    DebugIntentLabel = CreateDefaultSubobject<UTextRenderComponent>(TEXT("DebugIntentLabel"));
    DebugIntentLabel->SetupAttachment(GetCapsuleComponent());
    DebugIntentLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    DebugIntentLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 193.0f));
    DebugIntentLabel->SetRelativeRotation(FRotator::ZeroRotator);
    DebugIntentLabel->SetHorizontalAlignment(EHTA_Center);
    DebugIntentLabel->SetVerticalAlignment(EVRTA_TextCenter);
    DebugIntentLabel->SetText(FText::GetEmpty());
    DebugIntentLabel->SetTextRenderColor(FColor::Cyan);
    DebugIntentLabel->SetWorldSize(22.0f);
    DebugIntentLabel->SetVisibility(false, true);
    DebugIntentLabel->SetHiddenInGame(true, true);
    DebugIntentLabel->SetOwnerNoSee(false);
    DebugIntentLabel->SetOnlyOwnerSee(false);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (CubeMesh.Succeeded())
    {
        BodyVisual->SetStaticMesh(CubeMesh.Object);
        LeftArmVisual->SetStaticMesh(CubeMesh.Object);
        RightArmVisual->SetStaticMesh(CubeMesh.Object);
        LeftLegVisual->SetStaticMesh(CubeMesh.Object);
        RightLegVisual->SetStaticMesh(CubeMesh.Object);
        LeftShoulderVisual->SetStaticMesh(CubeMesh.Object);
        RightShoulderVisual->SetStaticMesh(CubeMesh.Object);
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

void AEvaZombieCharacter::Tick(const float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    UpdatePrototypeVisualAnimation(DeltaSeconds);
    UpdatePrototypeDebugLabelFacing();
}

void AEvaZombieCharacter::BeginPlay()
{
    Super::BeginPlay();
    if (HealthComponent)
    {
        HealthComponent->OnDeath.AddDynamic(this, &AEvaZombieCharacter::HandleDeath);
    }
    EnsurePrototypeDebugLabelInitialized();
    ConfigureEvolution(EvolutionType);
    EnsurePrototypeDebugLabelInitialized();
    LogPrototypeDebugLabelState(TEXT("BeginPlay"));
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
    FVector ArmScale(0.16f, 0.16f, 0.58f);
    FVector LeftArmLocation(0.0f, -42.0f, 46.0f);
    FVector RightArmLocation(0.0f, 42.0f, 46.0f);
    FVector LegScale(0.16f, 0.16f, 0.68f);
    FVector LeftLegLocation(0.0f, -18.0f, -52.0f);
    FVector RightLegLocation(0.0f, 18.0f, -52.0f);
    FVector ShoulderScale(0.22f, 0.18f, 0.18f);
    FVector LeftShoulderLocation(0.0f, -45.0f, 72.0f);
    FVector RightShoulderLocation(0.0f, 45.0f, 72.0f);
    FString DebugLabel(TEXT("ZOMBIE"));
    FColor DebugLabelColor = FColor::Green;
    FLinearColor VisualColor(0.18f, 0.65f, 0.22f, 1.0f);

    const bool bFast = NewEvolutionType == EEvaEvolutionType::Fast;
    const bool bArmored = NewEvolutionType == EEvaEvolutionType::Armored;
    const bool bLongArm = NewEvolutionType == EEvaEvolutionType::LongArm;

    if (bFast)
    {
        MovementSpeed *= 1.25f;
        BodyScale = FVector(0.42f, 0.32f, 1.12f);
        HeadScale = FVector(0.30f);
        ArmScale = FVector(0.12f, 0.12f, 0.70f);
        LeftArmLocation = FVector(0.0f, -36.0f, 48.0f);
        RightArmLocation = FVector(0.0f, 36.0f, 48.0f);
        LegScale = FVector(0.12f, 0.12f, 0.92f);
        LeftLegLocation = FVector(0.0f, -14.0f, -62.0f);
        RightLegLocation = FVector(0.0f, 14.0f, -62.0f);
        ShoulderScale = FVector(0.18f, 0.12f, 0.14f);
        LeftShoulderLocation = FVector(0.0f, -34.0f, 78.0f);
        RightShoulderLocation = FVector(0.0f, 34.0f, 78.0f);
        DebugLabel = TEXT("FAST");
        DebugLabelColor = FColor::Cyan;
        VisualColor = FLinearColor(0.08f, 0.75f, 0.95f, 1.0f);
    }
    if (bArmored)
    {
        NewMaxHealth *= 1.30f;
        MovementSpeed *= 0.92f;
        HeadDamageMultiplier = 0.5f;
        BodyScale = FVector(FMath::Max(BodyScale.X, 0.82f), FMath::Max(BodyScale.Y, 0.66f), FMath::Max(BodyScale.Z, 1.10f));
        HeadScale = FVector(FMath::Max(HeadScale.X, 0.48f));
        ArmScale = FVector(FMath::Max(ArmScale.X, 0.28f), FMath::Max(ArmScale.Y, 0.24f), FMath::Max(ArmScale.Z, 0.72f));
        LeftArmLocation = FVector(0.0f, -58.0f, 46.0f);
        RightArmLocation = FVector(0.0f, 58.0f, 46.0f);
        LegScale = FVector(FMath::Max(LegScale.X, 0.26f), FMath::Max(LegScale.Y, 0.22f), FMath::Max(LegScale.Z, 0.78f));
        LeftLegLocation = FVector(0.0f, -24.0f, -54.0f);
        RightLegLocation = FVector(0.0f, 24.0f, -54.0f);
        ShoulderScale = FVector(0.48f, 0.30f, 0.32f);
        LeftShoulderLocation = FVector(0.0f, -66.0f, 78.0f);
        RightShoulderLocation = FVector(0.0f, 66.0f, 78.0f);
        DebugLabel = TEXT("ARMORED");
        DebugLabelColor = FColor::Yellow;
        VisualColor = FLinearColor(0.95f, 0.82f, 0.22f, 1.0f);
    }
    if (bLongArm)
    {
        CurrentAttackRange += 250.0f;
        CurrentAttackDamage *= 1.15f;
        BodyScale = FVector(FMath::Max(BodyScale.X, 0.58f), FMath::Max(BodyScale.Y, 0.42f), FMath::Max(BodyScale.Z, 1.04f));
        ArmScale = FVector(FMath::Max(ArmScale.X, 0.18f), FMath::Max(ArmScale.Y, 0.18f), 1.24f);
        LeftArmLocation = FVector(0.0f, -56.0f, 34.0f);
        RightArmLocation = FVector(0.0f, 56.0f, 34.0f);
        ShoulderScale = FVector(FMath::Max(ShoulderScale.X, 0.28f), FMath::Max(ShoulderScale.Y, 0.20f), FMath::Max(ShoulderScale.Z, 0.22f));
        LeftShoulderLocation = FVector(0.0f, -58.0f, 74.0f);
        RightShoulderLocation = FVector(0.0f, 58.0f, 74.0f);
        DebugLabel = TEXT("LONG ARM");
        DebugLabelColor = FColor(170, 90, 255);
        VisualColor = FLinearColor(0.55f, 0.22f, 0.95f, 1.0f);
    }
    if (NewEvolutionType == EEvaEvolutionType::Composite)
    {
        Tags.AddUnique(TEXT("EvolvedComposite"));
        MovementSpeed *= 1.10f;
        NewMaxHealth *= 1.12f;
        HeadDamageMultiplier = 0.75f;
        CurrentAttackRange += 120.0f;
        CurrentAttackDamage *= 1.05f;
        BodyScale = FVector(0.66f, 0.52f, 1.16f);
        HeadScale = FVector(0.40f);
        ArmScale = FVector(0.18f, 0.18f, 0.96f);
        LeftArmLocation = FVector(0.0f, -54.0f, 40.0f);
        RightArmLocation = FVector(0.0f, 54.0f, 40.0f);
        LegScale = FVector(0.16f, 0.15f, 0.84f);
        LeftLegLocation = FVector(0.0f, -20.0f, -58.0f);
        RightLegLocation = FVector(0.0f, 20.0f, -58.0f);
        ShoulderScale = FVector(0.36f, 0.24f, 0.28f);
        LeftShoulderLocation = FVector(0.0f, -60.0f, 78.0f);
        RightShoulderLocation = FVector(0.0f, 60.0f, 78.0f);
        DebugLabel = TEXT("COMPOSITE");
        DebugLabelColor = FColor::Magenta;
        VisualColor = FLinearColor(1.0f, 0.12f, 0.82f, 1.0f);
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
    if (LeftArmVisual)
    {
        LeftArmVisual->SetRelativeLocation(LeftArmLocation);
        LeftArmVisual->SetRelativeScale3D(ArmScale);
    }
    if (RightArmVisual)
    {
        RightArmVisual->SetRelativeLocation(RightArmLocation);
        RightArmVisual->SetRelativeScale3D(ArmScale);
    }
    if (LeftLegVisual)
    {
        LeftLegVisual->SetRelativeLocation(LeftLegLocation);
        LeftLegVisual->SetRelativeScale3D(LegScale);
    }
    if (RightLegVisual)
    {
        RightLegVisual->SetRelativeLocation(RightLegLocation);
        RightLegVisual->SetRelativeScale3D(LegScale);
    }
    if (LeftShoulderVisual)
    {
        LeftShoulderVisual->SetRelativeLocation(LeftShoulderLocation);
        LeftShoulderVisual->SetRelativeScale3D(ShoulderScale);
    }
    if (RightShoulderVisual)
    {
        RightShoulderVisual->SetRelativeLocation(RightShoulderLocation);
        RightShoulderVisual->SetRelativeScale3D(ShoulderScale);
    }
    ApplyPrototypeVisualColor(VisualColor);
    SetActorScale3D(FVector::OneVector);
    SetPrototypeDebugLabel(DebugLabel, DebugLabelColor);
    LogPrototypeDebugLabelState(TEXT("ConfigureEvolution"));
    if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
    {
        MovementComponent->MaxWalkSpeed = MovementSpeed;
    }
    ApplyEvolutionToController();
}

void AEvaZombieCharacter::SetPrototypeDebugLabel(const FString& Label, const FColor& Color, const float WorldSize)
{
    EnsurePrototypeDebugLabelInitialized();
    if (!TypeLabel)
    {
        return;
    }

    TypeLabel->SetText(FText::FromString(Label));
    TypeLabel->SetTextRenderColor(Color);
    TypeLabel->SetWorldSize(WorldSize);
    TypeLabel->SetVisibility(bDisplayOverheadVisuals, true);
    TypeLabel->SetHiddenInGame(!bDisplayOverheadVisuals, true);
    TypeLabel->SetOwnerNoSee(false);
    TypeLabel->SetOnlyOwnerSee(false);
}

void AEvaZombieCharacter::SetDebugIntentText(const FString& IntentText)
{
    EnsurePrototypeDebugLabelInitialized();
    if (CurrentDebugIntentText == IntentText)
    {
        return;
    }

    CurrentDebugIntentText = IntentText;
    if (DebugIntentLabel)
    {
        DebugIntentLabel->SetText(FText::FromString(CurrentDebugIntentText));
        const bool bShouldShow = ShouldShowDebugIntentLabel();
        DebugIntentLabel->SetVisibility(bShouldShow, true);
        DebugIntentLabel->SetHiddenInGame(!bShouldShow, true);
    }
}

void AEvaZombieCharacter::SetOverheadDisplayEnabled(const bool bEnabled)
{
    bDisplayOverheadVisuals = bEnabled;
    if (TypeLabel)
    {
        TypeLabel->SetVisibility(bEnabled, true);
        TypeLabel->SetHiddenInGame(!bEnabled, true);
    }
    if (DebugIntentLabel)
    {
        DebugIntentLabel->SetVisibility(false, true);
        DebugIntentLabel->SetHiddenInGame(true, true);
    }
    UpdatePrototypeHealthBar();
}

void AEvaZombieCharacter::SetOverheadHealthBarEnabled(const bool bEnabled)
{
    bDisplayOverheadHealthBar = bEnabled;
    UpdatePrototypeHealthBar();
}

void AEvaZombieCharacter::SetDebugHealthNumbersVisible(const bool bVisible)
{
    bDebugHealthNumbersVisible = bVisible;
    UpdatePrototypeHealthBar();
}

void AEvaZombieCharacter::PlayPrototypeAttackFeedback()
{
    StartPrototypeVisualAction(EvaVisualActionAttack, 0.34f, 196.0f, 0.42f);
}

void AEvaZombieCharacter::StartPrototypeVisualAction(const FName ActionName, const float Duration,
    const float Frequency, const float VolumeScale)
{
    if (HealthComponent && HealthComponent->IsDead())
    {
        return;
    }

    PrototypeVisualAction = ActionName;
    PrototypeVisualActionEndTime = GetWorld() ? GetWorld()->GetTimeSeconds() + FMath::Max(0.05f, Duration) : -1000.0f;
    UEvaAudioFunctionLibrary::PlayPrototypeToneAtLocation(this, GetActorLocation(), Frequency, Duration, VolumeScale);
}

void AEvaZombieCharacter::UpdatePrototypeVisualAnimation(const float DeltaSeconds)
{
    if (!bEnablePrototypeAnimation)
    {
        return;
    }

    if (HealthComponent && HealthComponent->IsDead())
    {
        return;
    }

    PrototypeVisualAnimTime += DeltaSeconds;
    const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    const bool bActionActive = Now <= PrototypeVisualActionEndTime && PrototypeVisualAction != NAME_None;
    if (!bActionActive)
    {
        PrototypeVisualAction = NAME_None;
    }

    const float MoveSpeed = GetVelocity().Size2D();
    const bool bWalking = MoveSpeed > 12.0f;
    const float WalkSpeedScale = bWalking ? 8.0f : 2.0f;
    const float Swing = FMath::Sin(PrototypeVisualAnimTime * WalkSpeedScale);
    const float Bob = FMath::Sin(PrototypeVisualAnimTime * WalkSpeedScale * 0.5f);
    const float WalkAmount = bWalking ? 1.0f : 0.25f;

    FRotator BodyRotation(0.0f, 0.0f, Swing * 3.0f * WalkAmount);
    FRotator HeadRotation(0.0f, Swing * 3.5f * WalkAmount, 0.0f);
    FRotator LeftArmRotation(Swing * 28.0f * WalkAmount, 0.0f, 0.0f);
    FRotator RightArmRotation(-Swing * 28.0f * WalkAmount, 0.0f, 0.0f);
    FRotator LeftLegRotation(-Swing * 24.0f * WalkAmount, 0.0f, 0.0f);
    FRotator RightLegRotation(Swing * 24.0f * WalkAmount, 0.0f, 0.0f);
    BodyRotation.Yaw = Bob * 1.5f * WalkAmount;

    if (bActionActive && PrototypeVisualAction == EvaVisualActionAttack)
    {
        BodyRotation = FRotator(-8.0f, 0.0f, 0.0f);
        HeadRotation = FRotator(-6.0f, 0.0f, 0.0f);
        LeftArmRotation = FRotator(-72.0f, -18.0f, 0.0f);
        RightArmRotation = FRotator(-72.0f, 18.0f, 0.0f);
    }
    else if (bActionActive && PrototypeVisualAction == EvaVisualActionCharge)
    {
        BodyRotation = FRotator(-16.0f, 0.0f, 0.0f);
        HeadRotation = FRotator(-12.0f, 0.0f, 0.0f);
        LeftArmRotation = FRotator(42.0f, -16.0f, 0.0f);
        RightArmRotation = FRotator(42.0f, 16.0f, 0.0f);
        LeftLegRotation = FRotator(-28.0f, 0.0f, 0.0f);
        RightLegRotation = FRotator(28.0f, 0.0f, 0.0f);
    }
    else if (bActionActive && PrototypeVisualAction == EvaVisualActionRoar)
    {
        BodyRotation = FRotator(10.0f, 0.0f, Swing * 2.0f);
        HeadRotation = FRotator(22.0f, 0.0f, 0.0f);
        LeftArmRotation = FRotator(-118.0f, -42.0f, -18.0f);
        RightArmRotation = FRotator(-118.0f, 42.0f, 18.0f);
    }

    if (BodyVisual)
    {
        BodyVisual->SetRelativeRotation(BodyRotation);
    }
    if (HeadVisual)
    {
        HeadVisual->SetRelativeRotation(HeadRotation);
    }
    if (LeftArmVisual)
    {
        LeftArmVisual->SetRelativeRotation(LeftArmRotation);
    }
    if (RightArmVisual)
    {
        RightArmVisual->SetRelativeRotation(RightArmRotation);
    }
    if (LeftLegVisual)
    {
        LeftLegVisual->SetRelativeRotation(LeftLegRotation);
    }
    if (RightLegVisual)
    {
        RightLegVisual->SetRelativeRotation(RightLegRotation);
    }
    if (LeftShoulderVisual)
    {
        LeftShoulderVisual->SetRelativeRotation(FRotator(0.0f, 0.0f, -Swing * 2.0f * WalkAmount));
    }
    if (RightShoulderVisual)
    {
        RightShoulderVisual->SetRelativeRotation(FRotator(0.0f, 0.0f, Swing * 2.0f * WalkAmount));
    }
}

void AEvaZombieCharacter::ApplyPrototypeVisualColor(const FLinearColor& Color)
{
    ApplyColorToComponent(BodyVisual, Color);
    ApplyColorToComponent(HeadVisual, Color * 0.9f + FLinearColor::White * 0.1f);
    ApplyColorToComponent(LeftArmVisual, Color * 0.85f);
    ApplyColorToComponent(RightArmVisual, Color * 0.85f);
    ApplyColorToComponent(LeftLegVisual, Color * 0.8f);
    ApplyColorToComponent(RightLegVisual, Color * 0.8f);
    ApplyColorToComponent(LeftShoulderVisual, Color * 1.15f);
    ApplyColorToComponent(RightShoulderVisual, Color * 1.15f);
}

void AEvaZombieCharacter::EnsurePrototypeDebugLabelInitialized()
{
    if (!TypeLabel)
    {
        UE_LOG(LogAdaptiveHorror, Warning,
            TEXT("[EnemyVisual] LabelInitMissing Actor=%s Class=%s EnemyType=%s"),
            *GetName(),
            *GetClass()->GetName(),
            *UEnum::GetValueAsString(EvolutionType));
        return;
    }

    if (!TypeLabel->IsRegistered() && GetWorld())
    {
        TypeLabel->RegisterComponent();
    }
    if (TypeLabel->GetAttachParent() != GetCapsuleComponent())
    {
        TypeLabel->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
    }

    TypeLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    TypeLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 165.0f));
    TypeLabel->SetHorizontalAlignment(EHTA_Center);
    TypeLabel->SetVerticalAlignment(EVRTA_TextCenter);
    TypeLabel->SetVisibility(bDisplayOverheadVisuals, true);
    TypeLabel->SetHiddenInGame(!bDisplayOverheadVisuals, true);
    TypeLabel->SetOwnerNoSee(false);
    TypeLabel->SetOnlyOwnerSee(false);

    if (HealthBarLabel)
    {
        if (!HealthBarLabel->IsRegistered() && GetWorld())
        {
            HealthBarLabel->RegisterComponent();
        }
        if (HealthBarLabel->GetAttachParent() != GetCapsuleComponent())
        {
            HealthBarLabel->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
        }
        HealthBarLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        HealthBarLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 139.0f));
        HealthBarLabel->SetHorizontalAlignment(EHTA_Center);
        HealthBarLabel->SetVerticalAlignment(EVRTA_TextCenter);
        HealthBarLabel->SetOwnerNoSee(false);
        HealthBarLabel->SetOnlyOwnerSee(false);
    }

    if (HealthValueLabel)
    {
        if (!HealthValueLabel->IsRegistered() && GetWorld())
        {
            HealthValueLabel->RegisterComponent();
        }
        if (HealthValueLabel->GetAttachParent() != GetCapsuleComponent())
        {
            HealthValueLabel->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
        }
        HealthValueLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        HealthValueLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 119.0f));
        HealthValueLabel->SetHorizontalAlignment(EHTA_Center);
        HealthValueLabel->SetVerticalAlignment(EVRTA_TextCenter);
        HealthValueLabel->SetOwnerNoSee(false);
        HealthValueLabel->SetOnlyOwnerSee(false);
    }

    if (DebugIntentLabel)
    {
        if (!DebugIntentLabel->IsRegistered() && GetWorld())
        {
            DebugIntentLabel->RegisterComponent();
        }
        if (DebugIntentLabel->GetAttachParent() != GetCapsuleComponent())
        {
            DebugIntentLabel->AttachToComponent(GetCapsuleComponent(), FAttachmentTransformRules::KeepRelativeTransform);
        }
        DebugIntentLabel->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        DebugIntentLabel->SetRelativeLocation(FVector(0.0f, 0.0f, 193.0f));
        DebugIntentLabel->SetHorizontalAlignment(EHTA_Center);
        DebugIntentLabel->SetVerticalAlignment(EVRTA_TextCenter);
        DebugIntentLabel->SetTextRenderColor(FColor::Cyan);
        DebugIntentLabel->SetWorldSize(22.0f);
        DebugIntentLabel->SetOwnerNoSee(false);
        DebugIntentLabel->SetOnlyOwnerSee(false);
        const bool bShouldShow = ShouldShowDebugIntentLabel();
        DebugIntentLabel->SetVisibility(bShouldShow, true);
        DebugIntentLabel->SetHiddenInGame(!bShouldShow, true);
    }

    UpdatePrototypeHealthBar();
}

void AEvaZombieCharacter::LogPrototypeDebugLabelState(const FString& Context) const
{
    const APlayerCameraManager* CameraManager = GetWorld() ? UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0) : nullptr;
    const bool bCameraAcquired = CameraManager != nullptr;
    float CameraDistance = -1.0f;
    bool bDistanceHidden = false;
    if (TypeLabel && CameraManager)
    {
        CameraDistance = FVector::Dist(TypeLabel->GetComponentLocation(), CameraManager->GetCameraLocation());
        bDistanceHidden = CameraDistance > DebugLabelMaxVisibleDistance;
    }

    UE_LOG(LogAdaptiveHorror, Log,
        TEXT("[EnemyVisual] %s Actor=%s Class=%s EnemyType=%s DebugLabelComponent=%s LabelText=%s Visible=%s HiddenInGame=%s OwnerNoSee=%s OnlyOwnerSee=%s DistanceHideCondition=%s MaxDistance=%.1f CameraAcquired=%s CameraDistance=%.1f CapsuleScale=%s BodyScale=%s HeadScale=%s ActorScale=%s AIControllerClass=%s AutoPossessAI=%d Controller=%s"),
        *Context,
        *GetName(),
        *GetClass()->GetName(),
        *UEnum::GetValueAsString(EvolutionType),
        TypeLabel ? TEXT("true") : TEXT("false"),
        TypeLabel ? *TypeLabel->Text.ToString() : TEXT("None"),
        TypeLabel && TypeLabel->IsVisible() ? TEXT("true") : TEXT("false"),
        TypeLabel && TypeLabel->bHiddenInGame ? TEXT("true") : TEXT("false"),
        TypeLabel && TypeLabel->bOwnerNoSee ? TEXT("true") : TEXT("false"),
        TypeLabel && TypeLabel->bOnlyOwnerSee ? TEXT("true") : TEXT("false"),
        bDistanceHidden ? TEXT("true") : TEXT("false"),
        DebugLabelMaxVisibleDistance,
        bCameraAcquired ? TEXT("true") : TEXT("false"),
        CameraDistance,
        *GetCapsuleComponent()->GetRelativeScale3D().ToCompactString(),
        BodyVisual ? *BodyVisual->GetRelativeScale3D().ToCompactString() : TEXT("None"),
        HeadVisual ? *HeadVisual->GetRelativeScale3D().ToCompactString() : TEXT("None"),
        *GetActorScale3D().ToCompactString(),
        AIControllerClass ? *AIControllerClass->GetName() : TEXT("None"),
        static_cast<int32>(AutoPossessAI),
        GetController() ? *GetController()->GetClass()->GetName() : TEXT("None"));
}

void AEvaZombieCharacter::UpdatePrototypeDebugLabelFacing()
{
    if (!TypeLabel || !GetWorld())
    {
        return;
    }

    if (!bDisplayOverheadVisuals)
    {
        TypeLabel->SetVisibility(false, true);
        TypeLabel->SetHiddenInGame(true, true);
        if (HealthBarLabel)
        {
            HealthBarLabel->SetVisibility(false, true);
            HealthBarLabel->SetHiddenInGame(true, true);
        }
        if (HealthValueLabel)
        {
            HealthValueLabel->SetVisibility(false, true);
            HealthValueLabel->SetHiddenInGame(true, true);
        }
        if (DebugIntentLabel)
        {
            DebugIntentLabel->SetVisibility(false, true);
            DebugIntentLabel->SetHiddenInGame(true, true);
        }
        return;
    }

    if (HealthComponent && HealthComponent->IsDead())
    {
        TypeLabel->SetVisibility(false, true);
        if (HealthBarLabel)
        {
            HealthBarLabel->SetVisibility(false, true);
        }
        if (HealthValueLabel)
        {
            HealthValueLabel->SetVisibility(false, true);
        }
        if (DebugIntentLabel)
        {
            DebugIntentLabel->SetVisibility(false, true);
            DebugIntentLabel->SetHiddenInGame(true, true);
        }
        return;
    }

    UpdatePrototypeHealthBar();

    APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
    if (!CameraManager)
    {
        TypeLabel->SetVisibility(true, true);
        TypeLabel->SetHiddenInGame(false, true);
        if (DebugIntentLabel)
        {
            const bool bShowIntent = ShouldShowDebugIntentLabel();
            DebugIntentLabel->SetVisibility(bShowIntent, true);
            DebugIntentLabel->SetHiddenInGame(!bShowIntent, true);
        }
        return;
    }

    const FVector LabelLocation = TypeLabel->GetComponentLocation();
    const FVector ToCamera = CameraManager->GetCameraLocation() - LabelLocation;
    const float DistanceSq = ToCamera.SizeSquared();
    const bool bShouldShow = DistanceSq <= FMath::Square(DebugLabelMaxVisibleDistance);
    TypeLabel->SetVisibility(bShouldShow, true);
    TypeLabel->SetHiddenInGame(!bShouldShow, true);
    if (HealthBarLabel)
    {
        HealthBarLabel->SetVisibility(bShouldShow && bDisplayOverheadHealthBar, true);
        HealthBarLabel->SetHiddenInGame(!(bShouldShow && bDisplayOverheadHealthBar), true);
    }
    if (HealthValueLabel)
    {
        const bool bShowHealthValue = bShouldShow && bDisplayOverheadHealthBar && ShouldShowDebugHealthNumbers();
        HealthValueLabel->SetVisibility(bShowHealthValue, true);
        HealthValueLabel->SetHiddenInGame(!bShowHealthValue, true);
    }
    if (DebugIntentLabel)
    {
        const bool bShowIntent = bShouldShow && ShouldShowDebugIntentLabel();
        DebugIntentLabel->SetVisibility(bShowIntent, true);
        DebugIntentLabel->SetHiddenInGame(!bShowIntent, true);
    }
    if (!bShouldShow || ToCamera.IsNearlyZero())
    {
        return;
    }

    // TextRender faces along its local +X axis. Use yaw-only billboarding so the label stays readable
    // from normal FPS angles and never inherits the enemy's left/right rotation.
    const FRotator FacingRotation(0.0f, ToCamera.Rotation().Yaw, 0.0f);
    TypeLabel->SetWorldRotation(FacingRotation);
    if (HealthBarLabel)
    {
        HealthBarLabel->SetWorldRotation(FacingRotation);
    }
    if (HealthValueLabel)
    {
        HealthValueLabel->SetWorldRotation(FacingRotation);
    }
    if (DebugIntentLabel)
    {
        DebugIntentLabel->SetWorldRotation(FacingRotation);
    }
}

void AEvaZombieCharacter::UpdatePrototypeHealthBar()
{
    const bool bAlive = HealthComponent && !HealthComponent->IsDead();
    const bool bShowBar = bDisplayOverheadVisuals && bDisplayOverheadHealthBar && bAlive;
    if (HealthBarLabel)
    {
        const float HealthPercent = HealthComponent ? FMath::Clamp(HealthComponent->GetHealthPercent(), 0.0f, 1.0f) : 0.0f;
        const int32 FilledSegments = FMath::Clamp(FMath::RoundToInt(HealthPercent * 10.0f), 0, 10);
        FString BarText(TEXT("["));
        for (int32 Index = 0; Index < 10; ++Index)
        {
            BarText += Index < FilledSegments ? TEXT("#") : TEXT("-");
        }
        BarText += TEXT("]");

        HealthBarLabel->SetText(FText::FromString(BarText));
        HealthBarLabel->SetTextRenderColor(HealthPercent > 0.5f ? FColor::Green :
            (HealthPercent > 0.25f ? FColor::Yellow : FColor::Red));
        HealthBarLabel->SetVisibility(bShowBar, true);
        HealthBarLabel->SetHiddenInGame(!bShowBar, true);
    }

    if (HealthValueLabel)
    {
        const bool bShowValue = bShowBar && ShouldShowDebugHealthNumbers();
        HealthValueLabel->SetText(FText::FromString(HealthComponent ?
            FString::Printf(TEXT("%.0f / %.0f"), HealthComponent->GetCurrentHealth(), HealthComponent->GetMaxHealth()) :
            FString(TEXT("0 / 0"))));
        HealthValueLabel->SetVisibility(bShowValue, true);
        HealthValueLabel->SetHiddenInGame(!bShowValue, true);
    }
}

bool AEvaZombieCharacter::ShouldShowDebugHealthNumbers() const
{
#if !UE_BUILD_SHIPPING
    return bDebugHealthNumbersVisible || CVarEvaDebugEnemyHealthNumbers.GetValueOnGameThread() != 0;
#else
    return false;
#endif
}

bool AEvaZombieCharacter::ShouldShowDebugIntentLabel() const
{
#if !UE_BUILD_SHIPPING
    if (CurrentDebugIntentText.IsEmpty() || !bDisplayOverheadVisuals || (HealthComponent && HealthComponent->IsDead()))
    {
        return false;
    }

    const AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr;
    return GameMode && GameMode->IsGameplayActive() && GameMode->IsDebugHUDVisible();
#else
    return false;
#endif
}

void AEvaZombieCharacter::ApplyEvolutionToController()
{
    if (AEvaZombieAIController* ZombieController = Cast<AEvaZombieAIController>(GetController()))
    {
        ZombieController->ConfigureCombat(CurrentAttackRange, CurrentAttackDamage, 1.5f);
        ZombieController->ApplyCurrentGameplayAdaptation(true);
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
    UEvaAudioFunctionLibrary::PlayPrototypeToneAtLocation(this, GetActorLocation(), 98.0f, 0.24f, 0.34f);

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
    if (LeftArmVisual)
    {
        LeftArmVisual->SetVisibility(false, true);
    }
    if (RightArmVisual)
    {
        RightArmVisual->SetVisibility(false, true);
    }
    if (LeftLegVisual)
    {
        LeftLegVisual->SetVisibility(false, true);
    }
    if (RightLegVisual)
    {
        RightLegVisual->SetVisibility(false, true);
    }
    if (LeftShoulderVisual)
    {
        LeftShoulderVisual->SetVisibility(false, true);
    }
    if (RightShoulderVisual)
    {
        RightShoulderVisual->SetVisibility(false, true);
    }
    if (TypeLabel)
    {
        TypeLabel->SetVisibility(false, true);
    }
    if (HealthBarLabel)
    {
        HealthBarLabel->SetVisibility(false, true);
    }
    if (HealthValueLabel)
    {
        HealthValueLabel->SetVisibility(false, true);
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
