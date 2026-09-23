// Fill out your copyright notice in the Description page of Project Settings.


#include "FWHealManaActorComponent.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "FWNetworkSubsystem.h"

// Sets default values for this component's properties
UFWHealManaActorComponent::UFWHealManaActorComponent()
{
	/** Tick을 사용하지 않음 **/
	PrimaryComponentTick.bCanEverTick = false;
}

// Called when the game starts
void UFWHealManaActorComponent::BeginPlay()
{
	Super::BeginPlay();

	/** 초기 HP 및 MP 설정 **/
	CurrentHealth = MaxHealth;
	CurrentMana = MaxMana;

	if (UFWNetworkSubsystem* Net = GetNetwork())
	{
		Net->OnHealthResult.AddDynamic(this, &UFWHealManaActorComponent::HandleHealthResult);
		Net->OnManaResult.AddDynamic(this, &UFWHealManaActorComponent::HandleManaResult);
		Net->SendUpdateHealth(CurrentHealth);
		Net->SendUpdateMana(CurrentMana);
	}

	if (ManaRegenPerSecond > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			ManaRegenTimerHandle,
			this, &UFWHealManaActorComponent::TickManaRegen,
			ManaRegenTickInterval, /*bLoop=*/true);
	}
}

bool UFWHealManaActorComponent::ConsumeMana(float Amount)
{
    if (Amount < 0.f) return false;
    if (CurrentMana < Amount) return false;    // 마나 부족

    CurrentMana -= Amount;
    OnManaChanged.Broadcast(this, CurrentMana, MaxMana, -Amount);
    if (UFWNetworkSubsystem* Net = GetNetwork())
    {
        Net->SendUpdateMana(CurrentMana);
    }
    return true;
}

void UFWHealManaActorComponent::RegenerateMana(float Amount)
{
    if (Amount <= 0.f || !IsAlive()) return;
    if (CurrentMana >= MaxMana) return;

    const float OldMana = CurrentMana;
    CurrentMana = FMath::Clamp(CurrentMana + Amount, 0.f, MaxMana);
    const float Delta = CurrentMana - OldMana;

    if (Delta > 0.f)
    {
        OnManaChanged.Broadcast(this, CurrentMana, MaxMana, Delta);
        if (UFWNetworkSubsystem* Net = GetNetwork())
        {
            Net->SendUpdateMana(CurrentMana);
        }
    }
}

void UFWHealManaActorComponent::ApplyDamage(float Amount)
{
    if (Amount <= 0.f || !IsAlive()) return;

    const float OldHealth = CurrentHealth;
    CurrentHealth = FMath::Clamp(CurrentHealth - Amount, 0.f, MaxHealth);
    const float Delta = CurrentHealth - OldHealth;

    if (Delta != 0.f)
    {
        OnHealthChanged.Broadcast(this, CurrentHealth, MaxHealth, Delta);
        if (UFWNetworkSubsystem* Net = GetNetwork())
        {
            Net->SendUpdateHealth(CurrentHealth);
        }
    }
}

void UFWHealManaActorComponent::Heal(float Amount)
{
    if (Amount <= 0.f || !IsAlive()) return;
    if (CurrentHealth >= MaxHealth) return;

    const float OldHealth = CurrentHealth;
    CurrentHealth = FMath::Clamp(CurrentHealth + Amount, 0.f, MaxHealth);
    const float Delta = CurrentHealth - OldHealth;

    if (Delta > 0.f)
    {
        OnHealthChanged.Broadcast(this, CurrentHealth, MaxHealth, Delta);
        if (UFWNetworkSubsystem* Net = GetNetwork())
        {
            Net->SendUpdateHealth(CurrentHealth);
        }
    }
}

void UFWHealManaActorComponent::TickManaRegen()
{
    RegenerateMana(ManaRegenPerSecond * ManaRegenTickInterval);
}

UFWNetworkSubsystem* UFWHealManaActorComponent::GetNetwork() const
{
    UWorld* World = GetWorld();
    UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
    return GI ? GI->GetSubsystem<UFWNetworkSubsystem>() : nullptr;
}

void UFWHealManaActorComponent::HandleHealthResult(float ServerHealth)
{
    // Most calls here are just the server echoing back a value we ourselves just sent
    // (ConsumeMana/RegenerateMana/ApplyDamage/Heal already applied it locally and
    // broadcast the change) - only re-broadcast if this actually changes anything,
    // e.g. a future case where the server value disagrees with our local one.
    const float Delta = ServerHealth - CurrentHealth;
    if (Delta == 0.f) return;

    CurrentHealth = ServerHealth;
    OnHealthChanged.Broadcast(this, CurrentHealth, MaxHealth, Delta);
}

void UFWHealManaActorComponent::HandleManaResult(float ServerMana)
{
    const float Delta = ServerMana - CurrentMana;
    if (Delta == 0.f) return;

    CurrentMana = ServerMana;
    OnManaChanged.Broadcast(this, CurrentMana, MaxMana, Delta);
}

/** 주어진 액터에서 UFWHealManaActorComponent를 찾음 **/
UFWHealManaActorComponent* UFWHealManaActorComponent::GetAttributes(AActor* FromActor)
{
    return FromActor ? FromActor->FindComponentByClass<UFWHealManaActorComponent>() : nullptr;
}


