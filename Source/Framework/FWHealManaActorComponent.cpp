// Fill out your copyright notice in the Description page of Project Settings.


#include "FWHealManaActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/Actor.h"
#include "TimerManager.h"
#include "Engine/World.h"

// Sets default values for this component's properties
UFWHealManaActorComponent::UFWHealManaActorComponent()
{
	/** Tick을 사용하지 않음 **/
	PrimaryComponentTick.bCanEverTick = false;

	/** 컴포넌트 복제 켜기 (없으면 클라이언트에 값이 오지 않음) **/
	SetIsReplicatedByDefault(true);
}

// Called when the game starts
void UFWHealManaActorComponent::BeginPlay()
{
	Super::BeginPlay();

	/** 초기 HP 및 MP 설정 **/
	CurrentHealth = MaxHealth;
	CurrentMana = MaxMana;

	/** 서버만 마나 재생 타이머 관리(클라이언트에서 돌면 이중 처리됨) **/
	if (GetOwnerRole() == ROLE_Authority && ManaRegenPerSecond > 0.f)
	{
		GetWorld()->GetTimerManager().SetTimer(
			ManaRegenTimerHandle,
			this, &UFWHealManaActorComponent::TickManaRegen,
			ManaRegenTickInterval, /*bLoop=*/true);
	}
	
}

void UFWHealManaActorComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UFWHealManaActorComponent, CurrentHealth);
    DOREPLIFETIME(UFWHealManaActorComponent, CurrentMana);
}

bool UFWHealManaActorComponent::ConsumeMana(float Amount)
{
    if (GetOwnerRole() != ROLE_Authority) return false;
    if (Amount < 0.f) return false;
    if (CurrentMana < Amount) return false;    // 마나 부족

    CurrentMana -= Amount;
    OnManaChanged.Broadcast(this, CurrentMana, MaxMana, -Amount);
    return true;
}

void UFWHealManaActorComponent::RegenerateMana(float Amount)
{
    if (GetOwnerRole() != ROLE_Authority) return;
    if (Amount <= 0.f || !IsAlive()) return;
    if (CurrentMana >= MaxMana) return;

    const float OldMana = CurrentMana;
    CurrentMana = FMath::Clamp(CurrentMana + Amount, 0.f, MaxMana);
    const float Delta = CurrentMana - OldMana;

    if (Delta > 0.f)
    {
        OnManaChanged.Broadcast(this, CurrentMana, MaxMana, Delta);
    }
}

void UFWHealManaActorComponent::TickManaRegen()
{
    RegenerateMana(ManaRegenPerSecond * ManaRegenTickInterval);
}

void UFWHealManaActorComponent::OnRep_CurrentHealth(float OldValue)
{
    /** 클라이언트에서 서버가 보낸 새 값을 받았을 때 자동 호출 **/
    OnHealthChanged.Broadcast(this, CurrentHealth, MaxHealth, CurrentHealth - OldValue);
}

void UFWHealManaActorComponent::OnRep_CurrentMana(float OldValue)
{
    OnManaChanged.Broadcast(this, CurrentMana, MaxMana, CurrentMana - OldValue);
}

/** 주어진 액터에서 UFWHealManaActorComponent를 찾음 **/
UFWHealManaActorComponent* UFWHealManaActorComponent::GetAttributes(AActor* FromActor)
{
    return FromActor ? FromActor->FindComponentByClass<UFWHealManaActorComponent>() : nullptr;
}


