#include "Weapons/EvaHitscanWeapon.h"
#include "AI/EvaZombieCharacter.h"
#include "Audio/EvaAudioFunctionLibrary.h"
#include "Characters/EvaPlayerCharacter.h"
#include "Components/EvaHealthComponent.h"
#include "Components/EvaPlayerTelemetryComponent.h"
#include "Components/PrimitiveComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Hearing.h"

AEvaHitscanWeapon::AEvaHitscanWeapon()
{
    WeaponName = TEXT("Handgun");
    BaseDamage = 25.0f;
    HeadshotMultiplier = 2.0f;
    MagazineSize = 12;
    StartingReserveAmmo = 60;
    ReloadDuration = 1.5f;
    TraceRange = 5000.0f;
}

bool AEvaHitscanWeapon::PerformFire()
{
    APawn* InstigatorPawn = GetInstigator();
    APlayerController* PlayerController = InstigatorPawn ? Cast<APlayerController>(InstigatorPawn->GetController()) : nullptr;
    const AEvaPlayerCharacter* OwnerPlayer = Cast<AEvaPlayerCharacter>(GetOwner());
    if (!PlayerController || !GetWorld() || (OwnerPlayer && OwnerPlayer->IsDead()))
    {
        return false;
    }

    FVector ViewLocation;
    FRotator ViewRotation;
    PlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
    const FVector TraceEnd = ViewLocation + ViewRotation.Vector() * TraceRange;

    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(EvaHitscan), true, GetOwner());
    QueryParams.AddIgnoredActor(this);
    QueryParams.AddIgnoredActor(GetOwner());

    FHitResult Hit;
    const bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, TraceEnd, ECC_Visibility, QueryParams);
    const FVector FinalTraceEnd = bHit ? Hit.ImpactPoint : TraceEnd;

    if (bDrawDebugTrace)
    {
        DrawDebugLine(GetWorld(), ViewLocation, FinalTraceEnd, bHit ? FColor::Red : FColor::Green, false, 1.0f, 0, 1.0f);
    }

    UAISense_Hearing::ReportNoiseEvent(GetWorld(), ViewLocation, 1.0f, InstigatorPawn, 1000.0f, TEXT("Weapon.Fire"));
    UEvaAudioFunctionLibrary::PlayPrototypeToneAtLocation(this, ViewLocation, 880.0f, 0.045f, 0.48f);

    AActor* HitActor = Hit.GetActor();
    if (!bHit || !HitActor || !HitActor->FindComponentByClass<UEvaHealthComponent>())
    {
        return true;
    }

    const bool bHeadshot = Hit.BoneName.ToString().Contains(TEXT("head"), ESearchCase::IgnoreCase) ||
        (Hit.GetComponent() && Hit.GetComponent()->ComponentHasTag(TEXT("Head")));
    const float AppliedDamage = BaseDamage * (bHeadshot ? HeadshotMultiplier : 1.0f);

    UGameplayStatics::ApplyPointDamage(HitActor, AppliedDamage, ViewRotation.Vector(), Hit,
        PlayerController, this, UDamageType::StaticClass());

    if (const AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(GetOwner()))
    {
        if (UEvaPlayerTelemetryComponent* Telemetry = Player->GetTelemetryComponent())
        {
            Telemetry->RecordHit(bHeadshot, FVector::Distance(ViewLocation, Hit.ImpactPoint));
        }
    }

#if !UE_BUILD_SHIPPING
    if (GEngine)
    {
        GEngine->AddOnScreenDebugMessage(-1, 1.5f, bHeadshot ? FColor::Yellow : FColor::Green,
            FString::Printf(TEXT("Hit %s for %.0f%s"), *HitActor->GetName(), AppliedDamage,
                bHeadshot ? TEXT(" [HEADSHOT]") : TEXT("")));
    }
#endif

    if (AEvaZombieCharacter* Zombie = Cast<AEvaZombieCharacter>(HitActor))
    {
        Zombie->AlertToPlayer(InstigatorPawn);
    }

    return true;
}
