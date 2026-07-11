#include "World/EvaFacilityZoneTrigger.h"
#include "Characters/EvaPlayerCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "UObject/ConstructorHelpers.h"
#include "World/EvaResearchFacilityDirector.h"

AEvaFacilityZoneTrigger::AEvaFacilityZoneTrigger()
{
    PrimaryActorTick.bCanEverTick = false;

    Trigger = CreateDefaultSubobject<UBoxComponent>(TEXT("Trigger"));
    SetRootComponent(Trigger);
    Trigger->SetBoxExtent(FVector(300.0f, 550.0f, 180.0f));
    Trigger->SetCollisionProfileName(TEXT("Trigger"));
    Trigger->OnComponentBeginOverlap.AddDynamic(this, &AEvaFacilityZoneTrigger::HandleOverlap);

    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
    Visual->SetupAttachment(Trigger);
    Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Visual->SetRelativeScale3D(FVector(0.08f, 6.0f, 0.05f));

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        Visual->SetStaticMesh(CubeMesh.Object);
    }

    Tags.Add(TEXT("EvaZoneTrigger"));
}

void AEvaFacilityZoneTrigger::ConfigureZone(const EEvaFacilityZone NewZone,
    AEvaResearchFacilityDirector* NewDirector, const FVector& BoxExtent)
{
    Zone = NewZone;
    Director = NewDirector;
    if (Trigger)
    {
        Trigger->SetBoxExtent(BoxExtent);
    }
}

void AEvaFacilityZoneTrigger::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
    UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
    if (Director && Cast<AEvaPlayerCharacter>(OtherActor))
    {
        Director->NotifyZoneEntered(Zone);
    }
}
