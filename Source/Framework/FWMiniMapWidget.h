// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FWMiniMapWidget.generated.h"

class UImage;
class UMaterialInterface;

/**
 * 
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class FRAMEWORK_API UFWMiniMapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual bool Initialize() override;

	virtual FReply NativeOnMouseButtonDown(
		const FGeometry& InGeometry,
		const FPointerEvent& InMouseEvent) override;

	/** BP가 반드시 이 이름의 Image Widget을 참조해야함 **/
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> MiniMapImage;

	/** RT_MiniMap을 샘플링하는 Material. Class Defaults 에서 M_MiniMap 지정 **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap")
	TObjectPtr<UMaterialInterface> MiniMapMaterial;

	/** MiniMap이 담당하는 월드 중심 좌표 **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap")
	FVector2D MiniMapCenter = FVector2D::ZeroVector;

	/** MiniMap이 담당하는 월드 크기 = SceneCapture의 Ortho Width **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap")
	float MiniMapWorldSize = 8000.f;

	/** MiniMap UI 우클릭 이벤트 (캐릭터 이동) **/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "MiniMap")
	bool bEnableClickToMove = true;
	
};
