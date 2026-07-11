#pragma once

#include "CoreMinimal.h"
#include "AI/EvaTelemetryTypes.h"
#include "GameFramework/Actor.h"
#include "EvaFacilityZoneTrigger.generated.h"

class AEvaResearchFacilityDirector;
class UBoxComponent;
class UStaticMeshComponent;

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API AEvaFacilityZoneTrigger : public AActor
{
    GENERATED_BODY()

public:
    AEvaFacilityZoneTrigger();

    UFUNCTION(BlueprintCallable, Category = "EVA|Facility")
    void ConfigureZone(EEvaFacilityZone NewZone, AEvaResearchFacilityDirector* NewDirector,
        const FVector& BoxExtent);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Facility")
    EEvaFacilityZone Zone = EEvaFacilityZone::EntryLobby;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Facility")
    TObjectPtr<AEvaResearchFacilityDirector> Director;

protected:
    UFUNCTION()
    void HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
        UPrimitiveComponent* OtherComponent, int32 OtherBodyIndex, bool bFromSweep,
        const FHitResult& SweepResult);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Facility")
    TObjectPtr<UBoxComponent> Trigger;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Facility")
    TObjectPtr<UStaticMeshComponent> Visual;
};
