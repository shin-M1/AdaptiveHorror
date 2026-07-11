#pragma once

#include "CoreMinimal.h"
#include "Weapons/EvaWeaponBase.h"
#include "EvaHitscanWeapon.generated.h"

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API AEvaHitscanWeapon : public AEvaWeaponBase
{
    GENERATED_BODY()

public:
    AEvaHitscanWeapon();

protected:
    virtual bool PerformFire() override;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Weapon", meta = (ClampMin = "1.0"))
    float TraceRange = 5000.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Weapon", meta = (ClampMin = "1.0"))
    float HeadshotMultiplier = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Debug")
    bool bDrawDebugTrace = true;
};

