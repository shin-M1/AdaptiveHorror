#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EvaFacilityInteractable.generated.h"

class AEvaPlayerCharacter;
class AEvaResearchFacilityDirector;
class UBoxComponent;
class UMaterialInstanceDynamic;
class UPrimitiveComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UENUM(BlueprintType)
enum class EEvaFacilityInteractableType : uint8
{
    Keycard,
    LockedDoor,
    PowerConsole,
    ResearchLog,
    DataCoreConsole
};

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API AEvaFacilityInteractable : public AActor
{
    GENERATED_BODY()

public:
    AEvaFacilityInteractable();

    virtual void BeginPlay() override;

    UFUNCTION(BlueprintCallable, Category = "EVA|Interaction")
    void ConfigureInteractable(EEvaFacilityInteractableType NewType, AEvaResearchFacilityDirector* NewDirector,
        const FString& NewDisplayName, const FName NewLogId = NAME_None,
        const FString& NewLogTitle = TEXT(""), const FString& NewLogBody = TEXT(""));

    UFUNCTION(BlueprintPure, Category = "EVA|Interaction")
    FString GetInteractionPrompt(const AEvaPlayerCharacter* Player) const;

    UFUNCTION(BlueprintPure, Category = "EVA|Interaction")
    bool CanInteract(const AEvaPlayerCharacter* Player) const;

    UFUNCTION(BlueprintCallable, Category = "EVA|Interaction")
    bool Interact(AEvaPlayerCharacter* Player);

    UFUNCTION(BlueprintCallable, Category = "EVA|Interaction")
    void RefreshFromDirector();

    UFUNCTION(BlueprintPure, Category = "EVA|Interaction")
    EEvaFacilityInteractableType GetInteractableType() const { return InteractableType; }

    UFUNCTION(BlueprintPure, Category = "EVA|Interaction")
    FString GetDisplayName() const { return DisplayName; }

    UFUNCTION(BlueprintPure, Category = "EVA|Interaction|Debug")
    UStaticMeshComponent* GetVisualComponentForDebug() const { return Visual; }

    UFUNCTION(BlueprintPure, Category = "EVA|Interaction|Debug")
    UPrimitiveComponent* GetInteractionCollisionComponentForDebug() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Interaction|Debug")
    bool IsInteractionCollisionEnabledForDebug() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Interaction|Debug")
    bool IsMeshVisibleForDebug() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Interaction|Debug")
    FVector GetInteractionTraceLocation() const;

protected:
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Interaction")
    TObjectPtr<USceneComponent> SceneRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Interaction")
    TObjectPtr<UStaticMeshComponent> Visual;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Interaction")
    TObjectPtr<UBoxComponent> InteractionCollision;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Interaction")
    TObjectPtr<UTextRenderComponent> Label;

    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> VisualMaterial;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Interaction")
    EEvaFacilityInteractableType InteractableType = EEvaFacilityInteractableType::Keycard;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Interaction")
    TObjectPtr<AEvaResearchFacilityDirector> Director;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Interaction")
    FString DisplayName = TEXT("Interactable");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Interaction")
    FName LogId = NAME_None;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Interaction")
    FString LogTitle;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Interaction", meta = (MultiLine = "true"))
    FString LogBody;

private:
    void ApplyVisualState(bool bActive);
    void ApplyDoorState(bool bOpen);
    void ConfigureInteractionCollision(const FVector& RelativeLocation, const FVector& Extent, bool bActive);
    void ApplyVisualColor(const FLinearColor& Color);
    AEvaResearchFacilityDirector* ResolveDirector() const;
    FString TypeDefaultPrompt() const;

    FVector InitialLocation = FVector::ZeroVector;
};
