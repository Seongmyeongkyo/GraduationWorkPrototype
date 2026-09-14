// Fill out your copyright notice in the Description page of Project Settings.


#include "FWStatusBarWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "FWHealManaActorComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"

void UFWStatusBarWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 위젯이 뜨는 시점에 이미 폰이 있으면 자동 바인딩
    if (APlayerController* PC = GetOwningPlayer())
    {
        if (APawn* Pawn = PC->GetPawn())
        {
            BindToActor(Pawn);
        }
    }
    // 만약 이 시점에 폰이 없다면 PlayerController::OnPossessedPawnChanged 콜백으로 처리 가능
    // (지금 프로젝트는 캐릭터가 GameMode에서 스폰되므로 여기서 이미 있음)
}

void UFWStatusBarWidget::NativeDestruct()
{
    if (TrackedAttrComp)
    {
        TrackedAttrComp->OnHealthChanged.RemoveDynamic(this, &UFWStatusBarWidget::HandleHealthChanged);
        TrackedAttrComp->OnManaChanged.RemoveDynamic(this, &UFWStatusBarWidget::HandleManaChanged);
        TrackedAttrComp = nullptr;
    }
    Super::NativeDestruct();
}

void UFWStatusBarWidget::BindToActor(AActor* Actor)
{
    // 이전 바인딩 해제 (예: 캐릭터가 리스폰되면 새 캐릭터로 다시 바인딩)
    if (TrackedAttrComp)
    {
        TrackedAttrComp->OnHealthChanged.RemoveDynamic(this, &UFWStatusBarWidget::HandleHealthChanged);
        TrackedAttrComp->OnManaChanged.RemoveDynamic(this, &UFWStatusBarWidget::HandleManaChanged);
        TrackedAttrComp = nullptr;
    }

    if (!Actor) return;

    TrackedAttrComp = UFWHealManaActorComponent::GetAttributes(Actor);
    if (!TrackedAttrComp) return;

    TrackedAttrComp->OnHealthChanged.AddDynamic(this, &UFWStatusBarWidget::HandleHealthChanged);
    TrackedAttrComp->OnManaChanged.AddDynamic(this, &UFWStatusBarWidget::HandleManaChanged);

    // 초기 UI 상태 반영 (델리게이트는 앞으로의 변화만 알림)
    RefreshHealthUI(TrackedAttrComp->GetCurrentHealth(), TrackedAttrComp->GetMaxHealth());
    RefreshManaUI(TrackedAttrComp->GetCurrentMana(), TrackedAttrComp->GetMaxMana());
}

void UFWStatusBarWidget::HandleHealthChanged(UFWHealManaActorComponent*, float NewValue, float MaxValue, float /*Delta*/)
{
    RefreshHealthUI(NewValue, MaxValue);
}

void UFWStatusBarWidget::HandleManaChanged(UFWHealManaActorComponent*, float NewValue, float MaxValue, float /*Delta*/)
{
    RefreshManaUI(NewValue, MaxValue);
}

void UFWStatusBarWidget::RefreshHealthUI(float Current, float Max)
{
    if (HealthBar)
    {
        HealthBar->SetPercent(Max > 0.f ? Current / Max : 0.f);
    }
    if (HealthText)
    {
        HealthText->SetText(FText::FromString(
            FString::Printf(TEXT("%d / %d"),
                FMath::RoundToInt(Current), FMath::RoundToInt(Max))));
    }
}

void UFWStatusBarWidget::RefreshManaUI(float Current, float Max)
{
    if (ManaBar)
    {
        ManaBar->SetPercent(Max > 0.f ? Current / Max : 0.f);
    }
    if (ManaText)
    {
        ManaText->SetText(FText::FromString(
            FString::Printf(TEXT("%d / %d"),
                FMath::RoundToInt(Current), FMath::RoundToInt(Max))));
    }
}
