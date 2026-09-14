// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FWStatusBarWidget.generated.h"

class UProgressBar;
class UTextBlock;
class UFWHealManaActorComponent;

UCLASS()
class FRAMEWORK_API UFWStatusBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;

    /** 이 위젯이 표시할 대상 액터의 AttributeComponent에 바인딩 */
    UFUNCTION(BlueprintCallable, Category = "FW|Status")
    void BindToActor(AActor* Actor);

protected:
    // BP에서 이 이름의 위젯을 만들어야 함 (필수)
    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBar> HealthBar;

    UPROPERTY(meta = (BindWidget))
    TObjectPtr<UProgressBar> ManaBar;

    // 선택: 있으면 "75 / 100" 텍스트 표시
    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> HealthText;

    UPROPERTY(meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> ManaText;

    UPROPERTY()
    TObjectPtr<UFWHealManaActorComponent> TrackedAttrComp;

    UFUNCTION()
    void HandleHealthChanged(UFWHealManaActorComponent* Comp, float NewValue, float MaxValue, float Delta);

    UFUNCTION()
    void HandleManaChanged(UFWHealManaActorComponent* Comp, float NewValue, float MaxValue, float Delta);

    void RefreshHealthUI(float Current, float Max);
    void RefreshManaUI(float Current, float Max);
	
};
