#include "Pickups/EvaAmmoPickup.h"
#include "Characters/EvaPlayerCharacter.h"
#include "Weapons/EvaWeaponBase.h"

bool AEvaAmmoPickup::ApplyPickup(AEvaPlayerCharacter* Player)
{
    if (!Player || !Player->GetCurrentWeapon())
    {
        return false;
    }
    Player->GetCurrentWeapon()->AddReserveAmmo(AmmoAmount);
    return true;
}

