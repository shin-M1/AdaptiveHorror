#include "Components/EvaHealthComponent.h"
#include "GameFramework/Actor.h"

UEvaHealthComponent::UEvaHealthComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UEvaHealthComponent::BeginPlay()
{
    Super::BeginPlay();
    CurrentHealth = MaxHealth;
    if (AActor* Owner = GetOwner())
    {
        Owner->OnTakeAnyDamage.AddDynamic(this, &UEvaHealthComponent::HandleAnyDamage);
    }
}

float UEvaHealthComponent::GetHealthPercent() const
{
    return MaxHealth > 0.0f ? CurrentHealth / MaxHealth : 0.0f;
}

void UEvaHealthComponent::Heal(const float Amount)
{
    if (bDead || Amount <= 0.0f)
    {
        return;
    }

    const float PreviousHealth = CurrentHealth;
    CurrentHealth = FMath::Clamp(CurrentHealth + Amount, 0.0f, MaxHealth);
    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, CurrentHealth - PreviousHealth);
}

void UEvaHealthComponent::ResetToFullHealth()
{
    const float Delta = MaxHealth - CurrentHealth;
    CurrentHealth = MaxHealth;
    bDead = false;
    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, Delta);
}

void UEvaHealthComponent::SetMaxHealth(const float NewMaxHealth, const bool bFillHealth)
{
    MaxHealth = FMath::Max(1.0f, NewMaxHealth);
    CurrentHealth = bFillHealth ? MaxHealth : FMath::Min(CurrentHealth, MaxHealth);
}

void UEvaHealthComponent::HandleAnyDamage(AActor* DamagedActor, const float Damage,
    const UDamageType* DamageType, AController* InstigatedBy, AActor* DamageCauser)
{
    if (bDead || Damage <= 0.0f)
    {
        return;
    }

    const float PreviousHealth = CurrentHealth;
    CurrentHealth = FMath::Clamp(CurrentHealth - Damage, 0.0f, MaxHealth);
    OnHealthChanged.Broadcast(CurrentHealth, MaxHealth, CurrentHealth - PreviousHealth);

    if (CurrentHealth <= 0.0f)
    {
        bDead = true;
        OnDeath.Broadcast(DamagedActor);
    }
}

