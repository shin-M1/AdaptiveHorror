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

    UFUNCTION(BlueprintPure, Category = "EVA|Hunter")
    EEvaHunterCounterType GetLockedCounterType() const { return LockedCounterType; }

protected:
    void ObserveTarget();
    void InitializeCounterProfile();
    void ExecuteCounterBehavior(EEvaCombatStyle ObservedStyle);
    EEvaCombatStyle CounterTypeToCombatStyle(EEvaHunterCounterType CounterType) const;
    FName ResolveNearbyTaggedActorId(FName Tag, const FVector& FromLocation, float MaxDistance) const;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Hunter")
    float ObservationInterval = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Hunter")
    float PreferredBerserkerCounterRange = 850.0f;

private:
    float LastObservationTime = -1000.0f;
    float LastCounterMoveTime = -1000.0f;
    bool bCounterProfileLocked = false;
    EEvaHunterCounterType LockedCounterType = EEvaHunterCounterType::None;
};
