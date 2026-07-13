#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "EvaAudioFunctionLibrary.generated.h"

UCLASS()
class ADAPTIVEHORROR_API UEvaAudioFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable, Category = "EVA|Audio", meta = (WorldContext = "WorldContextObject"))
    static void PlayPrototypeTone2D(const UObject* WorldContextObject, float Frequency = 440.0f,
        float Duration = 0.08f, float VolumeScale = 0.5f);

    UFUNCTION(BlueprintCallable, Category = "EVA|Audio", meta = (WorldContext = "WorldContextObject"))
    static void PlayPrototypeToneAtLocation(const UObject* WorldContextObject, const FVector& Location,
        float Frequency = 440.0f, float Duration = 0.08f, float VolumeScale = 0.5f);
};
