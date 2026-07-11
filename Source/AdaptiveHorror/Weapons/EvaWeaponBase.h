#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "EvaWeaponBase.generated.h"

class USceneComponent;
class UStaticMeshComponent;

UCLASS(Abstract, Blueprintable)
class ADAPTIVEHORROR_API AEvaWeaponBase : public AActor
{
    GENERATED_BODY()

public:
    AEvaWeaponBase();

    UFUNCTION(BlueprintCallable, Category = "EVA|Weapon")
    bool TryFire();

    UFUNCTION(BlueprintCallable, Category = "EVA|Weapon")
    bool StartReload();

    UFUNCTION(BlueprintCallable, Category = "EVA|Weapon")
    void AddReserveAmmo(int32 Amount);

    UFUNCTION(BlueprintCallable, Category = "EVA|Weapon")
    void ResetForRespawn();

    UFUNCTION(BlueprintPure, Category = "EVA|Weapon")
    int32 GetAmmoInMagazine() const { return AmmoInMagazine; }

    UFUNCTION(BlueprintPure, Category = "EVA|Weapon")
    int32 GetReserveAmmo() const { return ReserveAmmo; }

    UFUNCTION(BlueprintPure, Category = "EVA|Weapon")
    int32 GetMagazineSize() const { return MagazineSize; }

    UFUNCTION(BlueprintPure, Category = "EVA|Weapon")
    bool IsReloading() const { return bReloading; }

    UFUNCTION(BlueprintPure, Category = "EVA|Weapon")
    FName GetWeaponName() const { return WeaponName; }

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual bool PerformFire();

    UFUNCTION()
    void FinishReload();

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Weapon")
    TObjectPtr<USceneComponent> WeaponRoot;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Weapon")
    TObjectPtr<UStaticMeshComponent> WeaponMesh;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Weapon")
    FName WeaponName = TEXT("Handgun");

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Weapon", meta = (ClampMin = "0.0"))
    float BaseDamage = 25.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Weapon", meta = (ClampMin = "1"))
    int32 MagazineSize = 12;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Weapon", meta = (ClampMin = "0"))
    int32 StartingReserveAmmo = 60;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Weapon", meta = (ClampMin = "0.0"))
    float ReloadDuration = 1.5f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "EVA|Weapon", meta = (ClampMin = "0.01"))
    float SecondsBetweenShots = 0.2f;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Weapon")
    int32 AmmoInMagazine = 12;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Weapon")
    int32 ReserveAmmo = 60;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "EVA|Weapon")
    bool bReloading = false;

private:
    float LastFireTime = -1000.0f;
    FTimerHandle ReloadTimer;
};
