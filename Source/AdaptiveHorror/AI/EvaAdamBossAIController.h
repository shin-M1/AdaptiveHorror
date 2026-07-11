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

protected:
    bool TryChargeAttack();
    bool TryRoarSummon();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Adam")
    float RoarCooldown = 14.0f;

private:
    float LastChargeTime = -1000.0f;
    float LastRoarTime = -1000.0f;
};
