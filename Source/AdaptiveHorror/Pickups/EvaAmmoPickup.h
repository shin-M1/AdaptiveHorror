#pragma once

#include "CoreMinimal.h"
#include "Pickups/EvaPickupBase.h"
#include "EvaAmmoPickup.generated.h"

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API AEvaAmmoPickup : public AEvaPickupBase
{
    GENERATED_BODY()

protected:
    virtual bool ApplyPickup(AEvaPlayerCharacter* Player) override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Pickup", meta = (ClampMin = "1"))
    int32 AmmoAmount = 24;
};

