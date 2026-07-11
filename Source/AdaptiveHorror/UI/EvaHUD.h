#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "EvaHUD.generated.h"

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API AEvaHUD : public AHUD
{
    GENERATED_BODY()

public:
    virtual void DrawHUD() override;
};

