#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EvaPickupBase.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPrimitiveComponent;
class AEvaPlayerCharacter;

UCLASS(Abstract, Blueprintable)
class ADAPTIVEHORROR_API AEvaPickupBase : public AActor
{
    GENERATED_BODY()

public:
    AEvaPickupBase();

protected:
    UFUNCTION()
    void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep,
        const FHitResult& SweepResult);

    virtual bool ApplyPickup(AEvaPlayerCharacter* Player);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Pickup")
    TObjectPtr<USphereComponent> Trigger;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Pickup")
    TObjectPtr<UStaticMeshComponent> Visual;
};
