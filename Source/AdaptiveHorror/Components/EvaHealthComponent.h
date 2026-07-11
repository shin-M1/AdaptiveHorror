#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "EvaHealthComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FEvaHealthChangedSignature, float, CurrentHealth, float, MaxHealth, float, Delta);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FEvaDeathSignature, AActor*, DeadActor);

UCLASS(ClassGroup = (EVA), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class ADAPTIVEHORROR_API UEvaHealthComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEvaHealthComponent();

    UFUNCTION(BlueprintPure, Category = "EVA|Health")
    float GetCurrentHealth() const { return CurrentHealth; }

    UFUNCTION(BlueprintPure, Category = "EVA|Health")
    float GetMaxHealth() const { return MaxHealth; }

    UFUNCTION(BlueprintPure, Category = "EVA|Health")
    float GetHealthPercent() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Health")
    bool IsDead() const { return bDead; }

    UFUNCTION(BlueprintCallable, Category = "EVA|Health")
    void Heal(float Amount);

    UFUNCTION(BlueprintCallable, Category = "EVA|Health")
    void ResetToFullHealth();

    UFUNCTION(BlueprintCallable, Category = "EVA|Health")
    void SetMaxHealth(float NewMaxHealth, bool bFillHealth = true);

    UPROPERTY(BlueprintAssignable, Category = "EVA|Health")
    FEvaHealthChangedSignature OnHealthChanged;

    UPROPERTY(BlueprintAssignable, Category = "EVA|Health")
    FEvaDeathSignature OnDeath;

protected:
    virtual void BeginPlay() override;

    UFUNCTION()
    void HandleAnyDamage(AActor* DamagedActor, float Damage, const UDamageType* DamageType,
        AController* InstigatedBy, AActor* DamageCauser);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Health", meta = (ClampMin = "1.0"))
    float MaxHealth = 100.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Health")
    float CurrentHealth = 100.0f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Health")
    bool bDead = false;
};

