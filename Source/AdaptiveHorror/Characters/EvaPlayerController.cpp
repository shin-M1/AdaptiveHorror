#include "Characters/EvaPlayerController.h"

void AEvaPlayerController::BeginPlay()
{
    Super::BeginPlay();
    bShowMouseCursor = false;
    FInputModeGameOnly InputMode;
    SetInputMode(InputMode);
    SetIgnoreMoveInput(false);
    SetIgnoreLookInput(false);
}

