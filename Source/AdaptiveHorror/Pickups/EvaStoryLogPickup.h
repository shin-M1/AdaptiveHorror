#pragma once

#include "CoreMinimal.h"
#include "Pickups/EvaPickupBase.h"
#include "EvaStoryLogPickup.generated.h"

class AEvaResearchFacilityDirector;

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API AEvaStoryLogPickup : public AEvaPickupBase
{
    GENERATED_BODY()

public:
    AEvaStoryLogPickup();

    UFUNCTION(BlueprintCallable, Category = "EVA|Story")
    void ConfigureStoryLog(FName NewLogId, const FString& NewTitle, const FString& NewBody,
        AEvaResearchFacilityDirector* NewDirector = nullptr);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Story")
    FName LogId = TEXT("EVA_LOG");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Story", meta = (MultiLine = "true"))
    FString Title = TEXT("EVA Log");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Story", meta = (MultiLine = "true"))
    FString Body = TEXT("Recovered research note.");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Story")
    TObjectPtr<AEvaResearchFacilityDirector> Director;

protected:
    virtual bool ApplyPickup(AEvaPlayerCharacter* Player) override;
};
