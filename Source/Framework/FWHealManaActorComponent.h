// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "FWHealManaActorComponent.generated.h"

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

	
	/** 서버 전용 상태 변경 API **/
	/** 마나 소모. true = 소모 성공. false = 부족하거나 죽어있음. 서버에서만 유효. **/
	UFUNCTION(BlueprintCallable, Category = "FW|Attribute")
	bool ConsumeMana(float Amount);

	/** 마나 회복 (수동). 서버에서만 유효. **/
	UFUNCTION(BlueprintCallable, Category = "FW|Attribute")
	void RegenerateMana(float Amount);

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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 스탯(서버 권한, 자동 복제) **/
	UPROPERTY(EditAnywhere, Category = "FW|Attribute|Health")
	float MaxHealth = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentHealth, VisibleAnywhere, Category = "FW|Attribute|Health")
	float CurrentHealth = 100.f;

	UPROPERTY(EditAnywhere, Category = "FW|Attribute|Mana")
	float MaxMana = 100.f;

	UPROPERTY(ReplicatedUsing = OnRep_CurrentMana, VisibleAnywhere, Category = "FW|Attribute|Mana")
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

	/** OnRep(클라이언트에서 복제 감지 시 델리게이트 발동) **/
	UFUNCTION()
	void OnRep_CurrentHealth(float OldValue);

	UFUNCTION()
	void OnRep_CurrentMana(float OldValue);

public:	
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

		
};
