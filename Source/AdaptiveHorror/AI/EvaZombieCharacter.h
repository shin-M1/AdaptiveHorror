#pragma once

#include "CoreMinimal.h"
#include "AI/EvaTelemetryTypes.h"
#include "GameFramework/Character.h"
#include "EvaZombieCharacter.generated.h"

class UBoxComponent;
class UEvaHealthComponent;
class USphereComponent;
class UStaticMeshComponent;
class UTextRenderComponent;

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API AEvaZombieCharacter : public ACharacter
{
    GENERATED_BODY()

public:
    AEvaZombieCharacter();
    virtual void Tick(float DeltaSeconds) override;

    virtual float TakeDamage(float DamageAmount, struct FDamageEvent const& DamageEvent,
        class AController* EventInstigator, AActor* DamageCauser) override;

    UFUNCTION(BlueprintCallable, Category = "EVA|AI")
    void AlertToPlayer(APawn* PlayerPawn);

    UFUNCTION(BlueprintPure, Category = "EVA|AI")
    UEvaHealthComponent* GetHealthComponent() const { return HealthComponent; }

    UFUNCTION(BlueprintCallable, Category = "EVA|Evolution")
    virtual void ConfigureEvolution(EEvaEvolutionType NewEvolutionType);

    UFUNCTION(BlueprintPure, Category = "EVA|Evolution")
    EEvaEvolutionType GetEvolutionType() const { return EvolutionType; }

    UFUNCTION(BlueprintCallable, Category = "EVA|Evolution")
    void ApplyEvolutionToController();

    UFUNCTION(BlueprintPure, Category = "EVA|Visual")
    UTextRenderComponent* GetTypeLabelComponent() const { return TypeLabel; }

    UFUNCTION(BlueprintCallable, Category = "EVA|Visual")
    void EnsurePrototypeDebugLabelInitialized();

    UFUNCTION(BlueprintPure, Category = "EVA|Visual")
    UStaticMeshComponent* GetBodyVisualComponent() const { return BodyVisual; }

    UFUNCTION(BlueprintPure, Category = "EVA|Visual")
    UStaticMeshComponent* GetLeftArmVisualComponent() const { return LeftArmVisual; }

    UFUNCTION(BlueprintPure, Category = "EVA|Visual")
    UStaticMeshComponent* GetRightArmVisualComponent() const { return RightArmVisual; }

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void HandleDeath(AActor* DeadActor);

    virtual void OnDefeated();
    void SetPrototypeDebugLabel(const FString& Label, const FColor& Color, float WorldSize = 42.0f);
    void UpdatePrototypeDebugLabelFacing();
    void LogPrototypeDebugLabelState(const FString& Context) const;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|AI")
    TObjectPtr<UEvaHealthComponent> HealthComponent;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Visual")
    TObjectPtr<UStaticMeshComponent> BodyVisual;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Visual")
    TObjectPtr<UStaticMeshComponent> HeadVisual;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Visual")
    TObjectPtr<UStaticMeshComponent> LeftArmVisual;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Visual")
    TObjectPtr<UStaticMeshComponent> RightArmVisual;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Visual")
    TObjectPtr<UTextRenderComponent> TypeLabel;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Visual")
    float DebugLabelMaxVisibleDistance = 12000.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Combat")
    TObjectPtr<UBoxComponent> TorsoHitbox;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Combat")
    TObjectPtr<USphereComponent> HeadHitbox;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|AI")
    float MovementSpeed = 260.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|AI")
    float BaseMovementSpeed = 260.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|AI")
    float BaseHealth = 100.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|AI")
    float BaseAttackRange = 150.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|AI")
    float BaseAttackDamage = 10.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Evolution")
    EEvaEvolutionType EvolutionType = EEvaEvolutionType::None;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Evolution")
    float HeadDamageMultiplier = 1.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Evolution")
    float CurrentAttackRange = 150.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Evolution")
    float CurrentAttackDamage = 10.0f;

    TWeakObjectPtr<AController> LastDamageInstigator;

    bool bDefeatHandled = false;
};
