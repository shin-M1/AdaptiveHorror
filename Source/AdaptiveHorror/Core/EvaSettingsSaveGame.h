#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "EvaSettingsSaveGame.generated.h"

UCLASS(BlueprintType)
class ADAPTIVEHORROR_API UEvaSettingsSaveGame : public USaveGame
{
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MasterVolume = 0.8f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BGMVolume = 0.45f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Settings", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SFXVolume = 0.85f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Settings", meta = (ClampMin = "0.1", ClampMax = "5.0"))
    float MouseSensitivity = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Settings")
    bool bInvertMouseY = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Settings")
    bool bFullscreen = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Settings")
    FString Resolution = TEXT("1280x720");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Settings", meta = (ClampMin = "0", ClampMax = "3"))
    int32 GraphicsQuality = 1;
};
