#include "Pickups/EvaHealthPickup.h"
#include "Characters/EvaPlayerCharacter.h"
#include "Components/EvaHealthComponent.h"

bool AEvaHealthPickup::ApplyPickup(AEvaPlayerCharacter* Player)
{
    if (!Player || !Player->GetHealthComponent() || Player->GetHealthComponent()->IsDead() ||
        Player->GetHealthComponent()->GetCurrentHealth() >= Player->GetHealthComponent()->GetMaxHealth())
    {
        return false;
    }
    Player->GetHealthComponent()->Heal(HealAmount);
    return true;
}

