#include "Characters/EvaPlayerCharacter.h"
#include "AI/EvaLearningSubsystem.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/EvaHealthComponent.h"
#include "Components/EvaPlayerTelemetryComponent.h"
#include "Components/SceneComponent.h"
#include "Core/EvaPrototypeGameMode.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "InputCoreTypes.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"
#include "Perception/AISense_Hearing.h"
#include "Perception/AISense_Sight.h"
#include "Weapons/EvaHitscanWeapon.h"
#include "Weapons/EvaWeaponBase.h"

AEvaPlayerCharacter::AEvaPlayerCharacter()
{
    PrimaryActorTick.bCanEverTick = false;

    GetCapsuleComponent()->InitCapsuleSize(42.0f, 92.0f);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

    FirstPersonCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
    FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
    FirstPersonCamera->bUsePawnControlRotation = true;

    bUseControllerRotationPitch = false;
    bUseControllerRotationRoll = false;
    bUseControllerRotationYaw = true;

    HealthComponent = CreateDefaultSubobject<UEvaHealthComponent>(TEXT("HealthComponent"));
    TelemetryComponent = CreateDefaultSubobject<UEvaPlayerTelemetryComponent>(TEXT("TelemetryComponent"));

    StimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("StimuliSource"));
    StimuliSource->RegisterForSense(UAISense_Sight::StaticClass());
    StimuliSource->RegisterForSense(UAISense_Hearing::StaticClass());

    GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
    GetCharacterMovement()->bOrientRotationToMovement = false;
    GetCharacterMovement()->GetNavAgentPropertiesRef().bCanCrouch = true;

    StarterWeaponClass = AEvaHitscanWeapon::StaticClass();
}

void AEvaPlayerCharacter::PostInitializeComponents()
{
    Super::PostInitializeComponents();
    BuildRuntimeInputMapping();
}

void AEvaPlayerCharacter::BeginPlay()
{
    Super::BeginPlay();
    if (HealthComponent)
    {
        HealthComponent->OnDeath.AddDynamic(this, &AEvaPlayerCharacter::HandleDeath);
    }
    if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
    {
        MovementComponent->MaxWalkSpeed = WalkSpeed;
    }
    if (StimuliSource)
    {
        StimuliSource->RegisterWithPerceptionSystem();
    }
    AddRuntimeInputMapping();
    SpawnStarterWeapon();
}

void AEvaPlayerCharacter::PawnClientRestart()
{
    Super::PawnClientRestart();
    AddRuntimeInputMapping();
}

void AEvaPlayerCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
        {
            if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
                ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
            {
                if (RuntimeMappingContext)
                {
                    InputSubsystem->RemoveMappingContext(RuntimeMappingContext);
                }
            }
        }
    }
    Super::EndPlay(EndPlayReason);
}

void AEvaPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    if (UEnhancedInputComponent* EnhancedInput = Cast<UEnhancedInputComponent>(PlayerInputComponent))
    {
        if (MoveAction)
        {
            EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AEvaPlayerCharacter::Move);
        }
        if (LookAction)
        {
            EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &AEvaPlayerCharacter::Look);
        }
        if (JumpAction)
        {
            EnhancedInput->BindAction(JumpAction, ETriggerEvent::Started, this, &AEvaPlayerCharacter::StartJump);
            EnhancedInput->BindAction(JumpAction, ETriggerEvent::Completed, this, &AEvaPlayerCharacter::StopJump);
        }
        if (SprintAction)
        {
            EnhancedInput->BindAction(SprintAction, ETriggerEvent::Started, this, &AEvaPlayerCharacter::StartSprint);
            EnhancedInput->BindAction(SprintAction, ETriggerEvent::Completed, this, &AEvaPlayerCharacter::StopSprint);
        }
        if (FireAction)
        {
            EnhancedInput->BindAction(FireAction, ETriggerEvent::Started, this, &AEvaPlayerCharacter::FireWeapon);
        }
        if (ReloadAction)
        {
            EnhancedInput->BindAction(ReloadAction, ETriggerEvent::Started, this, &AEvaPlayerCharacter::ReloadWeapon);
        }
#if !UE_BUILD_SHIPPING
        if (DebugIncreaseAnalysisAction)
        {
            EnhancedInput->BindAction(DebugIncreaseAnalysisAction, ETriggerEvent::Started, this,
                &AEvaPlayerCharacter::DebugIncreaseEvaAnalysis);
        }
        if (DebugForceHunterAction)
        {
            EnhancedInput->BindAction(DebugForceHunterAction, ETriggerEvent::Started, this,
                &AEvaPlayerCharacter::DebugForceHunterSpawn);
        }
        if (DebugForceZombieWaveAction)
        {
            EnhancedInput->BindAction(DebugForceZombieWaveAction, ETriggerEvent::Started, this,
                &AEvaPlayerCharacter::DebugForceZombieWave);
        }
        if (DebugWarpAdamAction)
        {
            EnhancedInput->BindAction(DebugWarpAdamAction, ETriggerEvent::Started, this,
                &AEvaPlayerCharacter::DebugWarpToAdamArena);
        }
        if (DebugRestoreAction)
        {
            EnhancedInput->BindAction(DebugRestoreAction, ETriggerEvent::Started, this,
                &AEvaPlayerCharacter::DebugRestorePlayer);
        }
        if (DebugForceClearAction)
        {
            EnhancedInput->BindAction(DebugForceClearAction, ETriggerEvent::Started, this,
                &AEvaPlayerCharacter::DebugForceStageClear);
        }
        if (DebugTelemetrySnapshotAction)
        {
            EnhancedInput->BindAction(DebugTelemetrySnapshotAction, ETriggerEvent::Started, this,
                &AEvaPlayerCharacter::DebugPrintTelemetrySnapshot);
        }
        if (DebugNavigationVisualizationAction)
        {
            EnhancedInput->BindAction(DebugNavigationVisualizationAction, ETriggerEvent::Started, this,
                &AEvaPlayerCharacter::DebugToggleNavigationVisualization);
        }
#endif
    }
}

float AEvaPlayerCharacter::TakeDamage(const float DamageAmount, const FDamageEvent& DamageEvent,
    AController* EventInstigator, AActor* DamageCauser)
{
    if (IsStageClearActive())
    {
        return 0.0f;
    }

    if (IsDead())
    {
        return 0.0f;
    }

    if (DamageCauser)
    {
        LastDamageCause = DamageCauser->ActorHasTag(TEXT("Hunter")) ? TEXT("Hunter") :
            DamageCauser->ActorHasTag(TEXT("Zombie")) ? TEXT("Zombie") : DamageCauser->GetFName();
    }
    if (TelemetryComponent)
    {
        TelemetryComponent->RecordDamageTaken(DamageAmount, LastDamageCause);
    }
    return Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
}

bool AEvaPlayerCharacter::IsDead() const
{
    return HealthComponent && HealthComponent->IsDead();
}

void AEvaPlayerCharacter::ResetForCheckpoint(const FTransform& CheckpointTransform)
{
    SetActorTransform(CheckpointTransform, false, nullptr, ETeleportType::TeleportPhysics);
    if (HealthComponent)
    {
        HealthComponent->ResetToFullHealth();
    }
    if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
    {
        MovementComponent->SetMovementMode(MOVE_Walking);
        MovementComponent->StopMovementImmediately();
    }
    SetActorEnableCollision(true);
    LastDamageCause = TEXT("Unknown");
    if (CurrentWeapon)
    {
        CurrentWeapon->ResetForRespawn();
    }
}

void AEvaPlayerCharacter::BuildRuntimeInputMapping()
{
    if (RuntimeMappingContext)
    {
        return;
    }

    RuntimeMappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_EvaRuntime"));
    MoveAction = NewObject<UInputAction>(this, TEXT("IA_Move"));
    LookAction = NewObject<UInputAction>(this, TEXT("IA_Look"));
    JumpAction = NewObject<UInputAction>(this, TEXT("IA_Jump"));
    SprintAction = NewObject<UInputAction>(this, TEXT("IA_Sprint"));
    FireAction = NewObject<UInputAction>(this, TEXT("IA_Fire"));
    ReloadAction = NewObject<UInputAction>(this, TEXT("IA_Reload"));
#if !UE_BUILD_SHIPPING
    DebugIncreaseAnalysisAction = NewObject<UInputAction>(this, TEXT("IA_Debug_EVAAnalysis"));
    DebugForceHunterAction = NewObject<UInputAction>(this, TEXT("IA_Debug_Hunter"));
    DebugForceZombieWaveAction = NewObject<UInputAction>(this, TEXT("IA_Debug_ZombieWave"));
    DebugWarpAdamAction = NewObject<UInputAction>(this, TEXT("IA_Debug_WarpAdam"));
    DebugRestoreAction = NewObject<UInputAction>(this, TEXT("IA_Debug_Restore"));
    DebugForceClearAction = NewObject<UInputAction>(this, TEXT("IA_Debug_StageClear"));
    DebugTelemetrySnapshotAction = NewObject<UInputAction>(this, TEXT("IA_Debug_TelemetrySnapshot"));
    DebugNavigationVisualizationAction = NewObject<UInputAction>(this, TEXT("IA_Debug_NavigationVisualization"));
#endif

    MoveAction->ValueType = EInputActionValueType::Axis2D;
    LookAction->ValueType = EInputActionValueType::Axis2D;
    JumpAction->ValueType = EInputActionValueType::Boolean;
    SprintAction->ValueType = EInputActionValueType::Boolean;
    FireAction->ValueType = EInputActionValueType::Boolean;
    ReloadAction->ValueType = EInputActionValueType::Boolean;
#if !UE_BUILD_SHIPPING
    DebugIncreaseAnalysisAction->ValueType = EInputActionValueType::Boolean;
    DebugForceHunterAction->ValueType = EInputActionValueType::Boolean;
    DebugForceZombieWaveAction->ValueType = EInputActionValueType::Boolean;
    DebugWarpAdamAction->ValueType = EInputActionValueType::Boolean;
    DebugRestoreAction->ValueType = EInputActionValueType::Boolean;
    DebugForceClearAction->ValueType = EInputActionValueType::Boolean;
    DebugTelemetrySnapshotAction->ValueType = EInputActionValueType::Boolean;
    DebugNavigationVisualizationAction->ValueType = EInputActionValueType::Boolean;
#endif

    FEnhancedActionKeyMapping& Forward = RuntimeMappingContext->MapKey(MoveAction, EKeys::W);
    UInputModifierSwizzleAxis* ForwardSwizzle = NewObject<UInputModifierSwizzleAxis>(RuntimeMappingContext);
    ForwardSwizzle->Order = EInputAxisSwizzle::YXZ;
    Forward.Modifiers.Add(ForwardSwizzle);

    FEnhancedActionKeyMapping& Back = RuntimeMappingContext->MapKey(MoveAction, EKeys::S);
    UInputModifierSwizzleAxis* BackSwizzle = NewObject<UInputModifierSwizzleAxis>(RuntimeMappingContext);
    BackSwizzle->Order = EInputAxisSwizzle::YXZ;
    Back.Modifiers.Add(BackSwizzle);
    Back.Modifiers.Add(NewObject<UInputModifierNegate>(RuntimeMappingContext));

    RuntimeMappingContext->MapKey(MoveAction, EKeys::D);
    FEnhancedActionKeyMapping& Left = RuntimeMappingContext->MapKey(MoveAction, EKeys::A);
    Left.Modifiers.Add(NewObject<UInputModifierNegate>(RuntimeMappingContext));

    RuntimeMappingContext->MapKey(LookAction, EKeys::MouseX);
    FEnhancedActionKeyMapping& MouseY = RuntimeMappingContext->MapKey(LookAction, EKeys::MouseY);
    UInputModifierSwizzleAxis* LookSwizzle = NewObject<UInputModifierSwizzleAxis>(RuntimeMappingContext);
    LookSwizzle->Order = EInputAxisSwizzle::YXZ;
    MouseY.Modifiers.Add(LookSwizzle);

    RuntimeMappingContext->MapKey(JumpAction, EKeys::SpaceBar);
    RuntimeMappingContext->MapKey(SprintAction, EKeys::LeftShift);
    RuntimeMappingContext->MapKey(FireAction, EKeys::LeftMouseButton);
    RuntimeMappingContext->MapKey(ReloadAction, EKeys::R);
#if !UE_BUILD_SHIPPING
    RuntimeMappingContext->MapKey(DebugIncreaseAnalysisAction, EKeys::F1);
    RuntimeMappingContext->MapKey(DebugForceHunterAction, EKeys::F2);
    RuntimeMappingContext->MapKey(DebugForceZombieWaveAction, EKeys::F3);
    RuntimeMappingContext->MapKey(DebugWarpAdamAction, EKeys::F4);
    RuntimeMappingContext->MapKey(DebugRestoreAction, EKeys::F5);
    RuntimeMappingContext->MapKey(DebugForceClearAction, EKeys::F6);
    RuntimeMappingContext->MapKey(DebugTelemetrySnapshotAction, EKeys::F7);
    RuntimeMappingContext->MapKey(DebugNavigationVisualizationAction, EKeys::F9);
    RuntimeMappingContext->MapKey(DebugNavigationVisualizationAction, EKeys::N);
#endif
}

void AEvaPlayerCharacter::AddRuntimeInputMapping()
{
    if (!RuntimeMappingContext)
    {
        return;
    }

    if (const APlayerController* PlayerController = Cast<APlayerController>(GetController()))
    {
        if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
        {
            if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
                ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(LocalPlayer))
            {
                InputSubsystem->RemoveMappingContext(RuntimeMappingContext);
                InputSubsystem->AddMappingContext(RuntimeMappingContext, 0);
            }
        }
    }
}

void AEvaPlayerCharacter::Move(const FInputActionValue& Value)
{
    if (IsDead() || IsStageClearActive())
    {
        return;
    }
    const FVector2D Movement = Value.Get<FVector2D>();
    const FRotator YawRotation(0.0f, GetControlRotation().Yaw, 0.0f);
    AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Movement.Y);
    AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Movement.X);
}

void AEvaPlayerCharacter::Look(const FInputActionValue& Value)
{
    if (!IsDead())
    {
        const FVector2D LookValue = Value.Get<FVector2D>();
        const float PitchInput = bInvertMouseY ? LookValue.Y : -LookValue.Y;
        AddControllerYawInput(LookValue.X);
        AddControllerPitchInput(PitchInput);
    }
}

void AEvaPlayerCharacter::StartSprint()
{
    if (!IsDead() && !IsStageClearActive())
    {
        if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
        {
            MovementComponent->MaxWalkSpeed = SprintSpeed;
        }
    }
}

void AEvaPlayerCharacter::StopSprint()
{
    if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
    {
        MovementComponent->MaxWalkSpeed = WalkSpeed;
    }
}

void AEvaPlayerCharacter::StartJump()
{
    if (!IsDead() && !IsStageClearActive())
    {
        Jump();
    }
}

void AEvaPlayerCharacter::StopJump()
{
    StopJumping();
}

void AEvaPlayerCharacter::FireWeapon()
{
    if (!IsDead() && !IsStageClearActive() && CurrentWeapon)
    {
        CurrentWeapon->TryFire();
    }
}

void AEvaPlayerCharacter::ReloadWeapon()
{
    if (!IsDead() && !IsStageClearActive() && CurrentWeapon)
    {
        CurrentWeapon->StartReload();
    }
}

void AEvaPlayerCharacter::SpawnStarterWeapon()
{
    if (CurrentWeapon || !StarterWeaponClass || !GetWorld())
    {
        return;
    }

    FActorSpawnParameters SpawnParameters;
    SpawnParameters.Owner = this;
    SpawnParameters.Instigator = this;
    CurrentWeapon = GetWorld()->SpawnActor<AEvaWeaponBase>(StarterWeaponClass, FVector::ZeroVector,
        FRotator::ZeroRotator, SpawnParameters);
    if (CurrentWeapon)
    {
        USceneComponent* AttachTarget = FirstPersonCamera ? FirstPersonCamera.Get() : GetRootComponent();
        if (AttachTarget)
        {
            CurrentWeapon->AttachToComponent(AttachTarget, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
        }
        CurrentWeapon->SetActorRelativeLocation(FVector(45.0f, 18.0f, -18.0f));
        CurrentWeapon->SetActorRelativeRotation(FRotator::ZeroRotator);
    }
}

void AEvaPlayerCharacter::HandleDeath(AActor* DeadActor)
{
    if (IsStageClearActive())
    {
        return;
    }

    if (UCharacterMovementComponent* MovementComponent = GetCharacterMovement())
    {
        MovementComponent->DisableMovement();
        MovementComponent->StopMovementImmediately();
    }
    if (TelemetryComponent)
    {
        TelemetryComponent->RecordDeathCause(LastDamageCause);
    }

    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->HandlePlayerDeath(this);
    }
}

bool AEvaPlayerCharacter::IsStageClearActive() const
{
    const AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr;
    return GameMode && GameMode->IsStageClear();
}

void AEvaPlayerCharacter::DebugIncreaseEvaAnalysis()
{
#if !UE_BUILD_SHIPPING
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->DebugIncreaseEvaAnalysis(20.0f);
    }
#endif
}

void AEvaPlayerCharacter::DebugForceHunterSpawn()
{
#if !UE_BUILD_SHIPPING
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->DebugForceHunterSpawn();
    }
#endif
}

void AEvaPlayerCharacter::DebugForceZombieWave()
{
#if !UE_BUILD_SHIPPING
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->DebugForceZombieWave();
    }
#endif
}

void AEvaPlayerCharacter::DebugWarpToAdamArena()
{
#if !UE_BUILD_SHIPPING
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->DebugWarpPlayerToAdamArena();
    }
#endif
}

void AEvaPlayerCharacter::DebugRestorePlayer()
{
#if !UE_BUILD_SHIPPING
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->DebugRestorePlayer();
    }
#endif
}

void AEvaPlayerCharacter::DebugForceStageClear()
{
#if !UE_BUILD_SHIPPING
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->DebugForceStageClear();
    }
#endif
}

void AEvaPlayerCharacter::DebugPrintTelemetrySnapshot()
{
#if !UE_BUILD_SHIPPING
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->DebugPrintTelemetrySnapshot();
    }
#endif
}

void AEvaPlayerCharacter::DebugToggleNavigationVisualization()
{
#if !UE_BUILD_SHIPPING
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        GameMode->DebugToggleNavigationVisualization();
    }
#endif
}
