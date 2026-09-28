// Fill out your copyright notice in the Description page of Project Settings.


#include "FWHealManaActorComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/Pawn.h"
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

	/** 초기 HP/MP 설정 (서버에 접속하면 마나는 서버가 보낸 값으로 덮어씀) **/
	CurrentHealth = MaxHealth;
	MaxMana = FWNet::MAX_MANA;
	CurrentMana = MaxMana;

	GetWorld()->GetTimerManager().SetTimer(
		OfflineManaRegenTimer, this, &UFWHealManaActorComponent::TickOfflineManaRegen, 1.f, /*bLoop=*/true);
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
        if (UFWNetworkSubsystem* Net = GetLocalPlayerNetwork())
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
        if (UFWNetworkSubsystem* Net = GetLocalPlayerNetwork())
        {
            Net->SendUpdateHealth(CurrentHealth);
        }
    }
}

void UFWHealManaActorComponent::ApplyServerMana(float NewCurrent, float NewMax)
{
    const float Delta = NewCurrent - CurrentMana;
    if (Delta == 0.f && NewMax == MaxMana) return;

    CurrentMana = NewCurrent;
    MaxMana = NewMax;
    OnManaChanged.Broadcast(this, CurrentMana, MaxMana, Delta);
}

// Offline fallback below mirrors the server's rules in Session.cpp (C2S_SKILL / regen_mana) - keep them identical.
bool UFWHealManaActorComponent::TryConsumeManaOffline(float Amount)
{
    if (CurrentMana < Amount) return false;

    CurrentMana -= Amount;
    OnManaChanged.Broadcast(this, CurrentMana, MaxMana, -Amount);
    return true;
}

void UFWHealManaActorComponent::TickOfflineManaRegen()
{
    if (!UsesOfflineFallback() || CurrentMana >= MaxMana) return;

    const float OldMana = CurrentMana;
    CurrentMana = FMath::Min(CurrentMana + FWNet::MANA_REGEN_PER_SECOND, MaxMana);
    OnManaChanged.Broadcast(this, CurrentMana, MaxMana, CurrentMana - OldMana);
}

bool UFWHealManaActorComponent::UsesOfflineFallback() const
{
    const APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
    {
        return false;
    }
    const UFWNetworkSubsystem* Net = GetLocalPlayerNetwork();
    return !Net || !Net->IsServerAuthoritative();
}

UFWNetworkSubsystem* UFWHealManaActorComponent::GetLocalPlayerNetwork() const
{
    // 원격 아바타는 Controller 없이 스폰되므로 여기서 걸러진다. 로컬 폰도 빙의 전(BeginPlay 직후)에는
    // 걸러지지만, 그 시점엔 값이 초기값이고 어차피 아직 서버 연결 전이라 보고할 것이 없다.
    const APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn || !OwnerPawn->IsLocallyControlled())
    {
        return nullptr;
    }

    UWorld* World = GetWorld();
    UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
    return GI ? GI->GetSubsystem<UFWNetworkSubsystem>() : nullptr;
}

/** 주어진 액터에서 UFWHealManaActorComponent를 찾음 **/
UFWHealManaActorComponent* UFWHealManaActorComponent::GetAttributes(AActor* FromActor)
{
    return FromActor ? FromActor->FindComponentByClass<UFWHealManaActorComponent>() : nullptr;
}


