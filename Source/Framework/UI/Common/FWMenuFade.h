#pragma once

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateColorBrush.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"

namespace FWMenuFade
{
	// 카메라 페이드로 가려지지 않는 UMG까지 화면 전체를 검게 덮습니다.
	inline void SetOpacity(UWidgetTree* Tree, TObjectPtr<UBorder>& Mask, float Opacity)
	{
		const float Alpha = FMath::Clamp(Opacity, 0.f, 1.f);
		if (!Tree) return;
		if (!Mask && Alpha > 0.f)
		{
			UOverlay* Root = Cast<UOverlay>(Tree->RootWidget);
			if (!Root) return;
			Mask = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("LevelTransitionFade"));
			Mask->SetBrush(FSlateColorBrush(FLinearColor::White));
			Mask->SetPadding(FMargin(0));
			UOverlaySlot* FadeOverlaySlot = Root->AddChildToOverlay(Mask);
			FadeOverlaySlot->SetHorizontalAlignment(HAlign_Fill);
			FadeOverlaySlot->SetVerticalAlignment(VAlign_Fill);
		}
		if (Mask)
		{
			Mask->SetBrushColor(FLinearColor(0,0,0,Alpha));
			Mask->SetVisibility(Alpha > 0.f ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	}
	inline float Duration(float Seconds)
	{
		return FMath::IsFinite(Seconds) ? FMath::Clamp(Seconds, 0.f, 2.f) : .25f;
	}
}
