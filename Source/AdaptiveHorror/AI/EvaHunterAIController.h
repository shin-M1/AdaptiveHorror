#pragma once

#include "CoreMinimal.h"
#include "AI/EvaZombieAIController.h"
#include "EvaHunterAIController.generated.h"

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API AEvaHunterAIController : public AEvaZombieAIController
{
    GENERATED_BODY()

public:
    AEvaHunterAIController();
    virtual void Tick(float DeltaSeconds) override;

protected:
    void ObserveTarget();
    void ExecuteCounterBehavior(EEvaCombatStyle ObservedStyle);
    FName ResolveNearbyTaggedActorId(FName Tag, const FVector& FromLocation, float MaxDistance) const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Hunter")
    float ObservationInterval = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Hunter")
    float PreferredBerserkerCounterRange = 850.0f;

private:
    float LastObservationTime = -1000.0f;
    float LastCounterMoveTime = -1000.0f;
};
