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
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Core/EvaPrototypeGameMode.h"
#include "Engine/Engine.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"

#if !UE_BUILD_SHIPPING
static TAutoConsoleVariable<int32> CVarEvaDebugEnemyHealthNumbers(
    TEXT("Eva.DebugEnemyHealthNumbers"),
    0,
    TEXT("Shows CurrentHP / MaxHP under normal enemy overhead HP bars when set to 1."));
#endif

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

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (CubeMesh.Succeeded())
    {
        BodyVisual->SetStaticMesh(CubeMesh.Object);
        LeftArmVisual->SetStaticMesh(CubeMesh.Object);
        RightArmVisual->SetStaticMesh(CubeMesh.Object);
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
    FString DebugLabel(TEXT("ZOMBIE"));
    FColor DebugLabelColor = FColor::Green;

    const bool bFast = NewEvolutionType == EEvaEvolutionType::Fast || NewEvolutionType == EEvaEvolutionType::Composite;
    const bool bArmored = NewEvolutionType == EEvaEvolutionType::Armored || NewEvolutionType == EEvaEvolutionType::Composite;
    const bool bLongArm = NewEvolutionType == EEvaEvolutionType::LongArm || NewEvolutionType == EEvaEvolutionType::Composite;

    if (bFast)
    {
        MovementSpeed *= 1.25f;
        BodyScale = FVector(0.42f, 0.32f, 1.12f);
        HeadScale = FVector(0.30f);
        ArmScale = FVector(0.12f, 0.12f, 0.70f);
        LeftArmLocation = FVector(0.0f, -36.0f, 48.0f);
        RightArmLocation = FVector(0.0f, 36.0f, 48.0f);
        DebugLabel = TEXT("FAST");
        DebugLabelColor = FColor::Cyan;
    }
    if (bArmored)
    {
        NewMaxHealth *= 1.30f;
        HeadDamageMultiplier = 0.5f;
        BodyScale = FVector(FMath::Max(BodyScale.X, 0.82f), FMath::Max(BodyScale.Y, 0.66f), FMath::Max(BodyScale.Z, 1.10f));
        HeadScale = FVector(FMath::Max(HeadScale.X, 0.48f));
        ArmScale = FVector(FMath::Max(ArmScale.X, 0.28f), FMath::Max(ArmScale.Y, 0.24f), FMath::Max(ArmScale.Z, 0.72f));
        LeftArmLocation = FVector(0.0f, -58.0f, 46.0f);
        RightArmLocation = FVector(0.0f, 58.0f, 46.0f);
        DebugLabel = TEXT("ARMORED");
        DebugLabelColor = FColor::Yellow;
    }
    if (bLongArm)
    {
        CurrentAttackRange += 250.0f;
        CurrentAttackDamage *= 1.15f;
        BodyScale = FVector(FMath::Max(BodyScale.X, 0.58f), FMath::Max(BodyScale.Y, 0.42f), FMath::Max(BodyScale.Z, 1.04f));
        ArmScale = FVector(FMath::Max(ArmScale.X, 0.18f), FMath::Max(ArmScale.Y, 0.18f), 1.24f);
        LeftArmLocation = FVector(0.0f, -56.0f, 34.0f);
        RightArmLocation = FVector(0.0f, 56.0f, 34.0f);
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

void AEvaZombieCharacter::SetOverheadDisplayEnabled(const bool bEnabled)
{
    bDisplayOverheadVisuals = bEnabled;
    if (TypeLabel)
    {
        TypeLabel->SetVisibility(bEnabled, true);
        TypeLabel->SetHiddenInGame(!bEnabled, true);
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
        return;
    }

    UpdatePrototypeHealthBar();

    APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
    if (!CameraManager)
    {
        TypeLabel->SetVisibility(true, true);
        TypeLabel->SetHiddenInGame(false, true);
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
    if (LeftArmVisual)
    {
        LeftArmVisual->SetVisibility(false, true);
    }
    if (RightArmVisual)
    {
        RightArmVisual->SetVisibility(false, true);
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
