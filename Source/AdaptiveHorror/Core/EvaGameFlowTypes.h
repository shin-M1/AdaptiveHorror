#pragma once

#include "CoreMinimal.h"
#include "EvaGameFlowTypes.generated.h"

UENUM(BlueprintType)
enum class EEvaGameFlowState : uint8
{
    Title UMETA(DisplayName = "Title"),
    Playing UMETA(DisplayName = "Playing"),
    Paused UMETA(DisplayName = "Paused"),
    PlayerDead UMETA(DisplayName = "Player Dead"),
    StageCleared UMETA(DisplayName = "Stage Cleared"),
    Loading UMETA(DisplayName = "Loading")
};

UENUM(BlueprintType)
enum class EEvaSettingsReturnTarget : uint8
{
    Title UMETA(DisplayName = "Title"),
    Pause UMETA(DisplayName = "Pause")
};
