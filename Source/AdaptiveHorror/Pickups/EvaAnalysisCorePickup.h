#pragma once

#include "CoreMinimal.h"
#include "Pickups/EvaPickupBase.h"
#include "EvaAnalysisCorePickup.generated.h"

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API AEvaAnalysisCorePickup : public AEvaPickupBase
{
    GENERATED_BODY()

public:
    AEvaAnalysisCorePickup();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Pickup", meta = (ClampMin = "1"))
    int32 SourceHunterTier = 1;

protected:
    virtual bool ApplyPickup(AEvaPlayerCharacter* Player) override;
};
