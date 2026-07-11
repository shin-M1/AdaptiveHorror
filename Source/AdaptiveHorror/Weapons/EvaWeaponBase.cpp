#include "Weapons/EvaWeaponBase.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/EvaPlayerTelemetryComponent.h"
#include "Characters/EvaPlayerCharacter.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

AEvaWeaponBase::AEvaWeaponBase()
{
    PrimaryActorTick.bCanEverTick = false;

    WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponRoot"));
    SetRootComponent(WeaponRoot);

    WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
    WeaponMesh->SetupAttachment(WeaponRoot);
    WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    WeaponMesh->SetRelativeScale3D(FVector(0.35f, 0.12f, 0.18f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        WeaponMesh->SetStaticMesh(CubeMesh.Object);
    }
}

void AEvaWeaponBase::BeginPlay()
{
    Super::BeginPlay();
    AmmoInMagazine = MagazineSize;
    ReserveAmmo = StartingReserveAmmo;
}

void AEvaWeaponBase::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ReloadTimer);
    }
    Super::EndPlay(EndPlayReason);
}

bool AEvaWeaponBase::TryFire()
{
    if (bReloading || AmmoInMagazine <= 0 || !GetWorld())
    {
        if (AmmoInMagazine <= 0)
        {
            StartReload();
        }
        return false;
    }

    if (const AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(GetOwner()))
    {
        if (Player->IsDead())
        {
            return false;
        }
    }

    const float Now = GetWorld()->GetTimeSeconds();
    if (Now - LastFireTime < SecondsBetweenShots)
    {
        return false;
    }

    LastFireTime = Now;
    --AmmoInMagazine;

    if (const AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(GetOwner()))
    {
        if (UEvaPlayerTelemetryComponent* Telemetry = Player->GetTelemetryComponent())
        {
            Telemetry->RecordShot(WeaponName);
        }
    }

    return PerformFire();
}

bool AEvaWeaponBase::StartReload()
{
    if (bReloading || AmmoInMagazine >= MagazineSize || ReserveAmmo <= 0 || !GetWorld())
    {
        return false;
    }

    bReloading = true;
    GetWorldTimerManager().SetTimer(ReloadTimer, this, &AEvaWeaponBase::FinishReload, ReloadDuration, false);
    return true;
}

void AEvaWeaponBase::AddReserveAmmo(const int32 Amount)
{
    ReserveAmmo = FMath::Max(0, ReserveAmmo + Amount);
}

void AEvaWeaponBase::ResetForRespawn()
{
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(ReloadTimer);
    }
    bReloading = false;
    AmmoInMagazine = MagazineSize;
    ReserveAmmo = FMath::Max(ReserveAmmo, StartingReserveAmmo);
}

bool AEvaWeaponBase::PerformFire()
{
    return true;
}

void AEvaWeaponBase::FinishReload()
{
    const int32 Needed = MagazineSize - AmmoInMagazine;
    const int32 Loaded = FMath::Min(Needed, ReserveAmmo);
    AmmoInMagazine += Loaded;
    ReserveAmmo -= Loaded;
    bReloading = false;
}
