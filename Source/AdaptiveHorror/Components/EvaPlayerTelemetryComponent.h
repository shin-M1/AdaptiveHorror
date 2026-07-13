#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AI/EvaTelemetryTypes.h"
#include "EvaPlayerTelemetryComponent.generated.h"

UCLASS(ClassGroup = (EVA), BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent))
class ADAPTIVEHORROR_API UEvaPlayerTelemetryComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UEvaPlayerTelemetryComponent();

    UFUNCTION(BlueprintCallable, Category = "EVA|Telemetry")
    void RecordShot(FName WeaponName);

    UFUNCTION(BlueprintCallable, Category = "EVA|Telemetry")
    void RecordHit(bool bHeadshot, float CombatDistance);

    UFUNCTION(BlueprintCallable, Category = "EVA|Telemetry")
    void RecordKill();

    UFUNCTION(BlueprintCallable, Category = "EVA|Telemetry")
    void RecordDeathCause(FName Cause);

    UFUNCTION(BlueprintCallable, Category = "EVA|Telemetry")
    void RecordEscapeRoute(FName RouteId);

    UFUNCTION(BlueprintCallable, Category = "EVA|Telemetry")
    void RecordHideSpot(FName HideSpotId);

    UFUNCTION(BlueprintCallable, Category = "EVA|Telemetry")
    void RecordDamageTaken(float DamageAmount, FName DamageSource);

    UFUNCTION(BlueprintCallable, Category = "EVA|Telemetry")
    void ResetTelemetry();

    UFUNCTION(BlueprintPure, Category = "EVA|Telemetry")
    float GetHeadshotRate() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Telemetry")
    float GetAccuracy() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Telemetry")
    FName GetDominantWeapon() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Telemetry")
    float GetAverageCombatDistance() const { return Telemetry.AverageCombatDistance; }

    UFUNCTION(BlueprintPure, Category = "EVA|Telemetry")
    EEvaCombatStyle ClassifyCombatStyle() const;

    UFUNCTION(BlueprintPure, Category = "EVA|Telemetry")
    FEvaTelemetrySnapshot GetTelemetry() const { return Telemetry; }

private:
    class UEvaLearningSubsystem* GetLearningSubsystem() const;

    UPROPERTY(VisibleAnywhere, Category = "EVA|Telemetry")
    FEvaTelemetrySnapshot Telemetry;
};
