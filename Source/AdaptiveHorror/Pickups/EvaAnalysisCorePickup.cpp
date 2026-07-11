#include "Pickups/EvaAnalysisCorePickup.h"
#include "AI/EvaLearningSubsystem.h"
#include "Characters/EvaPlayerCharacter.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

AEvaAnalysisCorePickup::AEvaAnalysisCorePickup()
{
    Tags.Add(TEXT("AnalysisCore"));
    if (Visual)
    {
        Visual->SetRelativeScale3D(FVector(0.18f, 0.18f, 0.45f));
    }
}

bool AEvaAnalysisCorePickup::ApplyPickup(AEvaPlayerCharacter* Player)
{
    if (!Player || !GetWorld())
    {
        return false;
    }

    if (UGameInstance* GameInstance = GetWorld()->GetGameInstance())
    {
        if (UEvaLearningSubsystem* Learning = GameInstance->GetSubsystem<UEvaLearningSubsystem>())
        {
            Learning->RecordAnalysisCoreRecovered(SourceHunterTier);
        }
    }

    // TODO: Route this pickup into a research/crafting inventory once the lab hub exists.
    return true;
}
