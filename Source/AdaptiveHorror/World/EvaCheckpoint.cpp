#include "World/EvaCheckpoint.h"
#include "Characters/EvaPlayerCharacter.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Core/EvaPrototypeGameMode.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "UObject/ConstructorHelpers.h"

AEvaCheckpoint::AEvaCheckpoint()
{
    PrimaryActorTick.bCanEverTick = false;

    Trigger = CreateDefaultSubobject<USphereComponent>(TEXT("Trigger"));
    SetRootComponent(Trigger);
    Trigger->SetSphereRadius(120.0f);
    Trigger->SetCollisionProfileName(TEXT("Trigger"));
    Trigger->OnComponentBeginOverlap.AddDynamic(this, &AEvaCheckpoint::HandleOverlap);

    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
    Visual->SetupAttachment(Trigger);
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Visual->SetRelativeScale3D(FVector(0.6f, 0.6f, 0.1f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (CylinderMesh.Succeeded())
    {
        Visual->SetStaticMesh(CylinderMesh.Object);
    }
}

void AEvaCheckpoint::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    AEvaPlayerCharacter* Player = Cast<AEvaPlayerCharacter>(OtherActor);
    if (!Player)
    {
        return;
    }

    bActivated = true;
    if (AEvaPrototypeGameMode* GameMode = GetWorld() ? GetWorld()->GetAuthGameMode<AEvaPrototypeGameMode>() : nullptr)
    {
        FTransform RespawnTransform = GetActorTransform();
        RespawnTransform.SetLocation(GetActorLocation() + FVector(0.0f, 0.0f, 150.0f));
        GameMode->ActivateCheckpoint(RespawnTransform);
    }
}
