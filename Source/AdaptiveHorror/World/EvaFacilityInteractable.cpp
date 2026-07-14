#include "World/EvaFacilityInteractable.h"
#include "AdaptiveHorror.h"
#include "Characters/EvaPlayerCharacter.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "UObject/ConstructorHelpers.h"
#include "World/EvaResearchFacilityDirector.h"

AEvaFacilityInteractable::AEvaFacilityInteractable()
{
    PrimaryActorTick.bCanEverTick = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    Visual = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Visual"));
    Visual->SetupAttachment(SceneRoot);
    Visual->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    Visual->SetCollisionResponseToAllChannels(ECR_Ignore);
    Visual->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    Visual->SetCanEverAffectNavigation(false);

    Label = CreateDefaultSubobject<UTextRenderComponent>(TEXT("Label"));
    Label->SetupAttachment(SceneRoot);
    Label->SetHorizontalAlignment(EHTA_Center);
    Label->SetVerticalAlignment(EVRTA_TextCenter);
    Label->SetWorldSize(42.0f);
    Label->SetRelativeLocation(FVector(0.0f, 0.0f, 95.0f));
    Label->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
    Label->SetTextRenderColor(FColor(120, 235, 255));
    Label->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh.Succeeded())
    {
        Visual->SetStaticMesh(CubeMesh.Object);
    }

    Tags.Add(TEXT("EvaInteractable"));
}

void AEvaFacilityInteractable::BeginPlay()
{
    Super::BeginPlay();
    InitialLocation = GetActorLocation();
    RefreshFromDirector();
}

void AEvaFacilityInteractable::ConfigureInteractable(const EEvaFacilityInteractableType NewType,
    AEvaResearchFacilityDirector* NewDirector, const FString& NewDisplayName, const FName NewLogId,
    const FString& NewLogTitle, const FString& NewLogBody)
{
    InteractableType = NewType;
    Director = NewDirector;
    DisplayName = NewDisplayName;
    LogId = NewLogId;
    LogTitle = NewLogTitle;
    LogBody = NewLogBody;

    Tags.Add(FName(*FString::Printf(TEXT("EvaInteractable_%s"), *DisplayName.Replace(TEXT(" "), TEXT("_")))));
    RefreshFromDirector();
}

FString AEvaFacilityInteractable::GetInteractionPrompt(const AEvaPlayerCharacter* Player) const
{
    const AEvaResearchFacilityDirector* TargetDirector = ResolveDirector();
    if (InteractableType == EEvaFacilityInteractableType::LockedDoor && TargetDirector &&
        !TargetDirector->HasSecurityKeycard() && !TargetDirector->IsObservationDoorOpen())
    {
        return TEXT("E - KEYCARD REQUIRED");
    }

    if (!CanInteract(Player))
    {
        return TEXT("");
    }

    return TypeDefaultPrompt();
}

bool AEvaFacilityInteractable::CanInteract(const AEvaPlayerCharacter* Player) const
{
    const AEvaResearchFacilityDirector* TargetDirector = ResolveDirector();
    if (!Player || !TargetDirector || TargetDirector->IsStageClear())
    {
        return false;
    }

    switch (InteractableType)
    {
    case EEvaFacilityInteractableType::Keycard:
        return !TargetDirector->HasSecurityKeycard();
    case EEvaFacilityInteractableType::LockedDoor:
        return !TargetDirector->IsObservationDoorOpen();
    case EEvaFacilityInteractableType::PowerConsole:
        return !TargetDirector->IsFacilityPowerOnline();
    case EEvaFacilityInteractableType::ResearchLog:
        return true;
    case EEvaFacilityInteractableType::DataCoreConsole:
        return !TargetDirector->IsDataCoreAccessed();
    default:
        return false;
    }
}

bool AEvaFacilityInteractable::Interact(AEvaPlayerCharacter* Player)
{
    AEvaResearchFacilityDirector* TargetDirector = ResolveDirector();
    if (!Player || !TargetDirector)
    {
        return false;
    }

    bool bHandled = false;
    switch (InteractableType)
    {
    case EEvaFacilityInteractableType::Keycard:
        bHandled = TargetDirector->TryAcquireSecurityKeycard();
        break;
    case EEvaFacilityInteractableType::LockedDoor:
        bHandled = TargetDirector->TryOpenObservationDoor();
        break;
    case EEvaFacilityInteractableType::PowerConsole:
        bHandled = TargetDirector->TryRestoreFacilityPower();
        break;
    case EEvaFacilityInteractableType::ResearchLog:
        bHandled = TargetDirector->TryReadResearchLog(LogId.IsNone() ? FName(*DisplayName) : LogId,
            LogTitle.IsEmpty() ? DisplayName : LogTitle,
            LogBody.IsEmpty() ? FString(TEXT("Recovered research note.")) : LogBody);
        break;
    case EEvaFacilityInteractableType::DataCoreConsole:
        bHandled = TargetDirector->TryAccessDataCore();
        break;
    default:
        break;
    }

    RefreshFromDirector();
    return bHandled;
}

void AEvaFacilityInteractable::RefreshFromDirector()
{
    const AEvaResearchFacilityDirector* TargetDirector = ResolveDirector();
    if (!TargetDirector)
    {
        return;
    }

    if (Label)
    {
        Label->SetText(FText::FromString(DisplayName));
    }

    switch (InteractableType)
    {
    case EEvaFacilityInteractableType::Keycard:
        ApplyVisualState(!TargetDirector->HasSecurityKeycard());
        if (Visual)
        {
            Visual->SetRelativeScale3D(FVector(0.55f, 0.08f, 0.32f));
        }
        break;
    case EEvaFacilityInteractableType::LockedDoor:
        ApplyDoorState(TargetDirector->IsObservationDoorOpen());
        break;
    case EEvaFacilityInteractableType::PowerConsole:
        ApplyVisualState(true);
        if (Visual)
        {
            Visual->SetRelativeScale3D(FVector(0.45f, 0.20f, 0.60f));
        }
        if (Label)
        {
            Label->SetText(FText::FromString(TargetDirector->IsFacilityPowerOnline() ?
                FString(TEXT("POWER ONLINE")) : DisplayName));
        }
        break;
    case EEvaFacilityInteractableType::ResearchLog:
        ApplyVisualState(true);
        if (Visual)
        {
            Visual->SetRelativeScale3D(FVector(0.28f, 0.12f, 0.55f));
            Visual->SetRelativeLocation(FVector(0.0f, 0.0f, 8.0f));
        }
        if (Label)
        {
            Label->SetWorldSize(54.0f);
            Label->SetRelativeLocation(FVector(0.0f, 0.0f, 125.0f));
            Label->SetTextRenderColor(FColor(255, 210, 80));
            Label->SetText(FText::FromString(DisplayName));
        }
        break;
    case EEvaFacilityInteractableType::DataCoreConsole:
        ApplyVisualState(true);
        if (Visual)
        {
            Visual->SetRelativeScale3D(FVector(0.70f, 0.22f, 0.55f));
        }
        if (Label)
        {
            Label->SetText(FText::FromString(TargetDirector->IsDataCoreAccessed() ?
                FString(TEXT("DATA CORE COMPLETE")) : DisplayName));
        }
        break;
    default:
        break;
    }
}

void AEvaFacilityInteractable::ApplyVisualState(const bool bActive)
{
    SetActorHiddenInGame(!bActive);
    SetActorEnableCollision(bActive);
    if (Visual)
    {
        Visual->SetVisibility(bActive, true);
        Visual->SetHiddenInGame(!bActive, true);
        Visual->SetCollisionEnabled(bActive ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
        Visual->SetCollisionResponseToAllChannels(ECR_Ignore);
        Visual->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
        Visual->SetCanEverAffectNavigation(false);
    }
    if (Label)
    {
        Label->SetVisibility(bActive, true);
        Label->SetHiddenInGame(!bActive, true);
    }
}

void AEvaFacilityInteractable::ApplyDoorState(const bool bOpen)
{
    SetActorHiddenInGame(false);
    SetActorEnableCollision(!bOpen);
    if (Visual)
    {
        Visual->SetRelativeScale3D(FVector(0.26f, 13.0f, 3.40f));
        Visual->SetVisibility(true, true);
        Visual->SetHiddenInGame(false, true);
        Visual->SetCollisionObjectType(ECC_WorldStatic);
        if (bOpen)
        {
            SetActorLocation(InitialLocation + FVector(0.0f, 0.0f, 360.0f), false, nullptr,
                ETeleportType::TeleportPhysics);
            Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Visual->SetCollisionResponseToAllChannels(ECR_Ignore);
            Visual->SetCanEverAffectNavigation(false);
        }
        else
        {
            SetActorLocation(InitialLocation, false, nullptr, ETeleportType::TeleportPhysics);
            Visual->SetCanEverAffectNavigation(true);
            Visual->SetCollisionProfileName(TEXT("BlockAll"));
            Visual->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Visual->SetCollisionResponseToAllChannels(ECR_Block);
            Visual->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
            Visual->SetCollisionResponseToChannel(ECC_Camera, ECR_Block);
            Visual->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
        }
        Visual->UpdateBounds();
        UNavigationSystemV1::UpdateComponentInNavOctree(*Visual);
        if (UNavigationSystemV1* NavigationSystem = UNavigationSystemV1::GetCurrent(GetWorld()))
        {
            NavigationSystem->AddDirtyArea(Visual->Bounds.GetBox().ExpandBy(120.0f),
                ENavigationDirtyFlag::All, FName(TEXT("EvaObservationDoor")));
        }
    }
    if (Label)
    {
        Label->SetText(FText::FromString(bOpen ? FString(TEXT("OBSERVATION LAB OPEN")) : DisplayName));
        Label->SetVisibility(!bOpen, true);
        Label->SetHiddenInGame(bOpen, true);
    }
}

AEvaResearchFacilityDirector* AEvaFacilityInteractable::ResolveDirector() const
{
    if (Director)
    {
        return Director;
    }
    if (!GetWorld())
    {
        return nullptr;
    }

    TArray<AActor*> Directors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AEvaResearchFacilityDirector::StaticClass(), Directors);
    return Directors.Num() > 0 ? Cast<AEvaResearchFacilityDirector>(Directors[0]) : nullptr;
}

FString AEvaFacilityInteractable::TypeDefaultPrompt() const
{
    switch (InteractableType)
    {
    case EEvaFacilityInteractableType::Keycard:
        return TEXT("E - PICK UP KEYCARD");
    case EEvaFacilityInteractableType::LockedDoor:
        return TEXT("E - UNLOCK OBSERVATION LAB");
    case EEvaFacilityInteractableType::PowerConsole:
        return TEXT("E - RESTORE POWER");
    case EEvaFacilityInteractableType::ResearchLog:
        return TEXT("E - READ LOG");
    case EEvaFacilityInteractableType::DataCoreConsole:
        return TEXT("E - ACCESS DATA CORE");
    default:
        return TEXT("E - INTERACT");
    }
}
