// Fill out your copyright notice in the Description page of Project Settings.


#include "FWMiniMapWidget.h"
#include "Components/Image.h"
#include "FWPlayerController.h"
#include "Materials/MaterialInterface.h"

bool UFWMiniMapWidget::Initialize()
{
	if (!Super::Initialize()) {
		return false;
	}
	
    // MiniMap Material을 브러시로 세팅
    if (MiniMapImage && MiniMapMaterial) {
        MiniMapImage->SetBrushFromMaterial(MiniMapMaterial);
    }

    // MiniMapImage를 클릭 가능하게 설정
    if (MiniMapImage) {
        MiniMapImage->SetVisibility(ESlateVisibility::Visible);
    }

    // Widget 자체 표시
    SetVisibility(ESlateVisibility::Visible);
    return true;

}

FReply UFWMiniMapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    if (!bEnableClickToMove) {
        return FReply::Unhandled();
    }

    if (InMouseEvent.GetEffectingButton() != EKeys::RightMouseButton) {
        return FReply::Unhandled();
    }

    if (!MiniMapImage) {
        return FReply::Unhandled();
    }

    //실제 MiniMapImage 의 지오메트리 사용 (위젯 전체가 아니라)
    const FGeometry ImageGeometry = MiniMapImage->GetCachedGeometry();
    const FVector2D ScreenPosition = InMouseEvent.GetScreenSpacePosition();
    const FVector2D LocalPosition = ImageGeometry.AbsoluteToLocal(ScreenPosition);
    const FVector2D Size = ImageGeometry.GetLocalSize();

    if (Size.X <= 0.f || Size.Y <= 0.f) {
        return FReply::Unhandled();
    }

    //클릭이 실제 이미지 영역 밖이면 무시 (게임으로 이벤트 전달)
    if (LocalPosition.X < 0.f || LocalPosition.X > Size.X ||
        LocalPosition.Y < 0.f || LocalPosition.Y > Size.Y)
    {
        return FReply::Unhandled();
    }

    // 정규화
    const float u = LocalPosition.X / Size.X;
    const float v = LocalPosition.Y / Size.Y;

    // 월드 좌표 매핑
    const float WorldY = MiniMapCenter.Y + (u - 0.5f) * MiniMapWorldSize;
    const float WorldX = MiniMapCenter.X - (v - 0.5f) * MiniMapWorldSize;
    const FVector WorldPosition(WorldX, WorldY, 0.f);

    if (AFWPlayerController* PC = Cast<AFWPlayerController>(GetOwningPlayer())) {
        PC->MoveToWorldLocation(WorldPosition);
    }

    return FReply::Handled();

}


