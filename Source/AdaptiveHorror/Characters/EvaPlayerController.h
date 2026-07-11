#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "EvaPlayerController.generated.h"

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API AEvaPlayerController : public APlayerController
{
    GENERATED_BODY()

protected:
    virtual void BeginPlay() override;
};

