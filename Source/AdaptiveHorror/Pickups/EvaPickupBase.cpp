#include "Pickups/EvaPickupBase.h"
#include "Characters/EvaPlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"

AEvaPickupBase::AEvaPickupBase()
{
    PrimaryActorTick.bCanEverTick = false;

    Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
    SetRootComponent(Trigger);
    Trigger->SetSphereRadius(60.0f);
    Trigger->SetCollisionProfileName(TEXT("Trigger"));
    Trigger->OnComponentBeginOverlap.AddDynamic(this, &AEvaPickupBase::HandleOverlap);

    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
    Visual->SetupAttachment(Trigger);
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Visual->SetRelativeScale3D(FVector(0.25f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    if (SphereMesh.Succeeded())
    {
        Visual->SetStaticMesh(SphereMesh.Object);
    }
}

void AEvaPickupBase::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(OtherActor))
    {
        if (ApplyPickup(Player))
        {
            Destroy();
        }
    }
}

bool AEvaPickupBase::ApplyPickup(AEvaPlayerCharacter* Player)
{
    return false;
}

