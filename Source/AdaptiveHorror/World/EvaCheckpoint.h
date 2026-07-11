#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EvaCheckpoint.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPrimitiveComponent;

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API AEvaCheckpoint : public AActor
{
    GENERATED_BODY()

public:
    AEvaCheckpoint();

    UFUNCTION(BlueprintPure, Category = "EVA|Checkpoint")
    bool IsActivated() const { return bActivated; }

protected:
    UFUNCTION()
    void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep,
        const FHitResult& SweepResult);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Checkpoint")
    TObjectPtr<USphereComponent> Trigger;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Checkpoint")
    TObjectPtr<UStaticMeshComponent> Visual;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Checkpoint")
    bool bActivated = false;
};
