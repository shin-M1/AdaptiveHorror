#pragma once

#include "CoreMinimal.h"
#include "Pickups/EvaPickupBase.h"
#include "EvaHealthPickup.generated.h"

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API AEvaHealthPickup : public AEvaPickupBase
{
    GENERATED_BODY()

protected:
    virtual bool ApplyPickup(AEvaPlayerCharacter* Player) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Pickup", meta = (ClampMin = "1.0"))
    float HealAmount = 35.0f;
};

