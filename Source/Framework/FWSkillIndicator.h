// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Components/PrimitiveComponent.h"
#include "FWSkillIndicator.generated.h"

class UMaterialInterface;

/** 하나의 삼각형 그리기 / 좌표 : 컴포넌트 로컬 기준(+X : 스킬이 나가는 방향, +Y : 오른쪽) **/
struct FFWIndicatorBatch
{
	TArray<FVector3f> Vertices;
	TArray<FVector2f> UVs;	// U : 진행방향, V : 폭 방향. 머티리얼을 직접 만들 때 사용
	TArray<int32> Indices;	// 3개의 정점으로 이루어진 삼각형을 그리기 위한 인덱스 배열
	FLinearColor Color = FLinearColor::White; // RGB : 밝기까지 곱한 색상, A : 불투명도
};

/** 범위 표시 모양을 그리는 컴포넌트 **/
// 현재 메시 에셋이 없으므로 넘겨받는 삼각형 묶음을 그대로 렌더링
UCLASS()
class FRAMEWORK_API UFWSkillIndicatorMeshComponent : public UPrimitiveComponent
{
	GENERATED_BODY()

public:
	UFWSkillIndicatorMeshComponent();

	/** 넘겨받은 삼각형 묶음 (모양이 바뀔 경우 호출) **/
	void SetBatches(TArray<FFWIndicatorBatch>&& InBatches);

	const TArray<FFWIndicatorBatch>& GetBatches() const { return Batches; }

	/** 머티리얼 지정 (nullptr이면 엔진 기본 머티리얼 사용) **/
	void SetIndicatorMaterial(UMaterialInterface* InMaterial);

	/** 실제로 그릴 때 쓰는 머티리얼 **/
	UMaterialInterface* GetResolvedMaterial() const;

	// UPrimitiveComponent 인터페이스
	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;
	virtual FBoxSphereBounds CalcBounds(const FTransform& LocalToWorld) const override;
	virtual void GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials = false) const override;

private:
	UPROPERTY()
	TObjectPtr<UMaterialInterface> IndicatorMaterial;

	TArray<FFWIndicatorBatch> Batches;

	/** Batches 감싸는 로컬 경계 **/
	FBox LocalBounds = FBox(ForceInit);
};

/** 스킬 범위 표시 액터 **/
// 스킬을 누르고 있으면 공격범위 하나만 표시하고, 위치와 방향만 갱신
UCLASS()
class FRAMEWORK_API AFWSkillIndicator : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFWSkillIndicator();

	/** 직선 모양의 스킬 범위 표시 **/
	// Origin : 스킬이 나가는 시작점, Direction : 스킬이 나가는 방향, Length : 스킬 범위 길이, Width : 스킬 범위 폭
	void ShowLine(const FVector& Origin, const FVector& Direction, float Length, float Width);

	/** 범위 표시 숨김 **/
	void HideIndicator();

#if WITH_EDITOR
	// 에디터에서 속성 변경 시 호출
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	// ---- 모양 ----
	/** 캐릭터 중심에서 떨어진 곳부터 표시 (캐릭터의 발끝을 가리지 않도록) **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Indicator | Shape")
	float StartOffset = 50.f;

	/** 지면에 묻히지 않도록 설정 **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Indicator | Shape")
	float GroundOffset = 3.f;

	/** 양옆 스킬 경계선 두께 **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Indicator | Shape")
	float EdgeThickness = 4.f;

	/** 스킬 폭 대비 비율 **/
	// 중앙의 가장 굵은 부분 폭
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Indicator | Shape")
	float CoreWidthRatio = 0.3f;

	// 스킬 끝 부분 길이
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Indicator | Shape")
	float HeadLengthRatio = 0.8f;

	// 스킬 끝 부분 가운데 두께
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Indicator | Shape")
	float HeadThicknessRatio = 0.45f;

	// ---- 색 ----
	/** 색상 **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Indicator | Color")
	FLinearColor IndicatorColor = FLinearColor(0.f, 0.85f, 1.f, 1.f);

	/** 색상에 곱하는 밝기 (1보다 크면 빛나는 효과) **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Indicator | Color")
	float Brightness = 4.f;

	/** 부위별 불투명도 (0 = 투명, 1 = 불투명) **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Indicator | Color")
	float EdgeOpacity = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Indicator | Color")
	float CoreOpacity = 0.8f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Indicator | Color")
	float HeadOpacity = 0.95f;

	// ---- 머티리얼 ----
	/** 머티리얼 에셋 지정(nullptr이면 엔진 기본 머티리얼 사용) **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Indicator | Color")
	TObjectPtr<UMaterialInterface> IndicatorMaterial;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Indicator")
	TObjectPtr<UFWSkillIndicatorMeshComponent> MeshComponent;

private:
	/** 직선 모양 **/
	void BuildLineShape(float Length, float Width);

	/** 마지막으로 모양을 만든 길이 / 폭 (만일 같은 값이라면 갱신 안함) **/
	float BuiltLength = -1.f;
	float BuiltWidth = -1.f;
};
