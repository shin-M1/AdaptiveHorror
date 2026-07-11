#pragma once

#include "CoreMinimal.h"
#include "AI/EvaZombieCharacter.h"
#include "EvaHunterCharacter.generated.h"

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API AEvaHunterCharacter : public AEvaZombieCharacter
{
    GENERATED_BODY()

public:
    AEvaHunterCharacter();

    UFUNCTION(BlueprintCallable, Category = "EVA|Hunter")
    void OnHunterDefeated();

    UFUNCTION(BlueprintCallable, Category = "EVA|Hunter")
    void InitializeHunterTier(int32 NewHunterTier);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Hunter", meta = (ClampMin = "1.0"))
    float HunterHP = 250.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Hunter", meta = (ClampMin = "0.0", ClampMax = "100.0"))
    float AnalysisRate = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Hunter", meta = (ClampMin = "0.0"))
    float LearningSpeedMultiplier = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Hunter", meta = (ClampMin = "1"))
    int32 HunterTier = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Hunter")
    TSubclassOf<class AEvaAnalysisCorePickup> AnalysisCorePickupClass;

protected:
    virtual void BeginPlay() override;
    virtual void OnDefeated() override;

private:
    void DropAnalysisCore();

    bool bHunterDefeated = false;
};
