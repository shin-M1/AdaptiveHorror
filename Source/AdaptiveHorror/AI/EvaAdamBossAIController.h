#pragma once

#include "CoreMinimal.h"
#include "AI/EvaZombieAIController.h"
#include "EvaAdamBossAIController.generated.h"

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API AEvaAdamBossAIController : public AEvaZombieAIController
{
    GENERATED_BODY()

public:
    AEvaAdamBossAIController();
    virtual void Tick(float DeltaSeconds) override;

    UFUNCTION(BlueprintPure, Category = "EVA|Adam Debug")
    FString GetCurrentAdamState() const { return CurrentAdamState; }

    UFUNCTION(BlueprintPure, Category = "EVA|Adam Debug")
    FString GetLastAdamEvent() const { return LastAdamEvent; }

    UFUNCTION(BlueprintPure, Category = "EVA|Adam Debug")
    float GetCurrentTargetDistance() const { return CurrentTargetDistance; }

    UFUNCTION(BlueprintPure, Category = "EVA|Adam Debug")
    int32 GetLastSummonCount() const { return LastSummonCount; }

protected:
    virtual void TryAttackTarget() override;
    bool TryChargeAttack();
    bool TryRoarSummon();
    void SetAdamDebugState(const FString& NewState, const FString& NewEvent = FString(), int32 NewSummonCount = INDEX_NONE);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Adam")
    float RoarCooldown = 14.0f;

private:
    float LastChargeTime = -1000.0f;
    float LastRoarTime = -1000.0f;
    float CurrentTargetDistance = -1.0f;
    int32 LastSummonCount = 0;

    UPROPERTY(VisibleAnywhere, Category = "EVA|Adam Debug")
    FString CurrentAdamState = TEXT("Inactive");

    UPROPERTY(VisibleAnywhere, Category = "EVA|Adam Debug")
    FString LastAdamEvent = TEXT("None");
};
