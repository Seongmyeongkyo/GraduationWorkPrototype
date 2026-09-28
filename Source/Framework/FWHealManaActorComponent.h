// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FWHealManaActorComponent.generated.h"

class UFWNetworkSubsystem;

/** UI 및 게임 로직이 구독하여 값 변화 획득 **/
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnAttributeChanged,
	UFWHealManaActorComponent*, AttrComp,
	float, NewValue,
	float, MaxValue,
	float, Delta);

UCLASS( ClassGroup=(FW), meta=(BlueprintSpawnableComponent) )
class FRAMEWORK_API UFWHealManaActorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UFWHealManaActorComponent();

	/** 델리게이트 (UI 구독용) **/
	UPROPERTY(BlueprintAssignable, Category = "FW|Attribute")
	FOnAttributeChanged OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category = "FW|Attribute")
	FOnAttributeChanged OnManaChanged;

	
	/** 상태 변경 API. 로컬로 즉시 적용하고, 로컬 플레이어가 조종 중인 캐릭터라면 새 값을 서버에 보고한다(응답 없음). **/
	/** 마나 소모. true = 소모 성공. false = 부족하거나 죽어있음. **/
	UFUNCTION(BlueprintCallable, Category = "FW|Attribute")
	bool ConsumeMana(float Amount);

	/** 마나 회복 (수동/자동 재생 공용). **/
	UFUNCTION(BlueprintCallable, Category = "FW|Attribute")
	void RegenerateMana(float Amount);

	/** 피격 등으로 체력 감소. **/
	UFUNCTION(BlueprintCallable, Category = "FW|Attribute")
	void ApplyDamage(float Amount);

	/** 회복 아이템/스킬 등으로 체력 증가. **/
	UFUNCTION(BlueprintCallable, Category = "FW|Attribute")
	void Heal(float Amount);

	/** Getter **/
	UFUNCTION(BlueprintPure, Category = "FW|Attribute")
	float GetCurrentHealth() const { return CurrentHealth; }

	UFUNCTION(BlueprintPure, Category = "FW|Attribute")
	float GetMaxHealth() const { return MaxHealth; }

	UFUNCTION(BlueprintPure, Category = "FW|Attribute")
	float GetCurrentMana() const { return CurrentMana; }

	UFUNCTION(BlueprintPure, Category = "FW|Attribute")
	float GetMaxMana() const { return MaxMana; }

	UFUNCTION(BlueprintPure, Category = "FW|Attribute")
	float GetHealthPercent() const { return MaxHealth > 0.f ? CurrentHealth / MaxHealth : 0.f; }

	UFUNCTION(BlueprintPure, Category = "FW|Attribute")
	float GetManaPercent() const { return MaxMana > 0.f ? CurrentMana / MaxMana : 0.f; }

	UFUNCTION(BlueprintPure, Category = "FW|Attribute")
	bool IsAlive() const { return CurrentHealth > 0.f; }

	/** 임의 액터에서 FWHealManaActorComponent 찾기 (편의 함수) **/
	UFUNCTION(BlueprintPure, Category = "FW|Attribute", meta = (DefaultToSelf = "FromActor"))
	static UFWHealManaActorComponent* GetAttributes(AActor* FromActor);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

	/** 스탯. 이 클라이언트가 로컬로 소유/갱신한다. 서버로는 보고만 하고 받지는 않는다. **/
	UPROPERTY(EditAnywhere, Category = "FW|Attribute|Health")
	float MaxHealth = 100.f;

	UPROPERTY(VisibleAnywhere, Category = "FW|Attribute|Health")
	float CurrentHealth = 100.f;

	UPROPERTY(EditAnywhere, Category = "FW|Attribute|Mana")
	float MaxMana = 100.f;

	UPROPERTY(VisibleAnywhere, Category = "FW|Attribute|Mana")
	float CurrentMana = 100.f;

	/** 자동 마나 재생 **/
	/** 초당 마나 회복량. 0으로 두면 자동 재생 없음. **/
	UPROPERTY(EditAnywhere, Category = "FW|Attribute|Mana")
	float ManaRegenPerSecond = 5.f;

	/** 회복 틱 간격. 짧을수록 부드럽지만 네트워크 부하 증가. **/
	UPROPERTY(EditAnywhere, Category = "FW|Attribute|Mana")
	float ManaRegenTickInterval = 0.5f;

	FTimerHandle ManaRegenTimerHandle;
	void TickManaRegen();

	/**
	 * 이 컴포넌트의 소유자가 로컬 플레이어가 조종 중인 폰일 때만 FWNetworkSubsystem을 반환, 아니면 nullptr.
	 * 원격 아바타도 같은 AFWCharacter라 이 컴포넌트를 갖고 있으므로, 이 검사가 없으면
	 * 원격 아바타의 값이 "내 값"으로 서버에 보고된다.
	 */
	UFWNetworkSubsystem* GetLocalPlayerNetwork() const;

};
