#pragma once

#include "CoreMinimal.h"
#include "AI/EvaZombieCharacter.h"
#include "EvaAdamBossCharacter.generated.h"

UCLASS(Blueprintable)
class ADAPTIVEHORROR_API AEvaAdamBossCharacter : public AEvaZombieCharacter
{
    GENERATED_BODY()

public:
    AEvaAdamBossCharacter();

    UFUNCTION(BlueprintPure, Category = "EVA|Adam")
    bool IsPhaseTwo() const { return bPhaseTwo; }

    UFUNCTION(BlueprintPure, Category = "EVA|Adam")
    bool ShouldEnterPhaseTwo(float CurrentHealth, float MaxHealth) const;

    UFUNCTION(BlueprintCallable, Category = "EVA|Adam")
    void EnterPhaseTwo();

    UFUNCTION(BlueprintCallable, Category = "EVA|Adam")
    void SpawnRoarMinions(int32 Count = 2, bool bForceEvolved = false);

    UFUNCTION(BlueprintPure, Category = "EVA|Adam")
    float GetChargeDamage() const { return ChargeDamage; }

    UFUNCTION(BlueprintPure, Category = "EVA|Adam")
    float GetChargeCooldown() const { return ChargeCooldown; }

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Adam")
    float AdamHP = 2500.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Adam")
    float AdamMovementSpeed = 260.0f * 0.85f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Adam")
    float AdamMeleeDamage = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Adam")
    float AdamAttackInterval = 2.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Adam")
    float ChargeDamage = 35.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Adam")
    float ChargeCooldown = 8.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Adam")
    TSubclassOf<AEvaZombieCharacter> RoarMinionClass;

protected:
    virtual void BeginPlay() override;
    virtual void OnDefeated() override;

    UFUNCTION()
    void HandleAdamHealthChanged(float CurrentHealth, float MaxHealth, float Delta);

private:
    bool bPhaseTwo = false;
    bool bPhaseTwoMinionSpawned = false;
};
