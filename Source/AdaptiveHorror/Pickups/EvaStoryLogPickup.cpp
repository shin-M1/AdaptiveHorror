#include "Pickups/EvaStoryLogPickup.h"
#include "Characters/EvaPlayerCharacter.h"
#include "Kismet/GameplayStatics.h"
#include "World/EvaResearchFacilityDirector.h"

AEvaStoryLogPickup::AEvaStoryLogPickup()
{
    Tags.Add(TEXT("EvaStoryLog"));
    if (Visual)
    {
        Visual->SetRelativeScale3D(FVector(0.14f, 0.14f, 0.35f));
    }
}

void AEvaStoryLogPickup::ConfigureStoryLog(const FName NewLogId, const FString& NewTitle,
    const FString& NewBody, AEvaResearchFacilityDirector* NewDirector)
{
    LogId = NewLogId;
    Title = NewTitle;
    Body = NewBody;
    Director = NewDirector;
}

bool AEvaStoryLogPickup::ApplyPickup(AEvaPlayerCharacter* Player)
{
    if (!Player)
    {
        return false;
    }

    AEvaResearchFacilityDirector* TargetDirector = Director;
    if (!TargetDirector && GetWorld())
    {
        TArray<AActor*> Directors;
        UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEvaResearchFacilityDirector::StaticClass(), Directors);
        TargetDirector = Directors.Num() > 0 ? Cast<AEvaResearchFacilityDirector>(Directors[0]) : nullptr;
    }

    if (TargetDirector)
    {
        TargetDirector->NotifyStoryLogCollected(LogId, Title, Body);
    }
    return true;
}
