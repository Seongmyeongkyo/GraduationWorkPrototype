// Fill out your copyright notice in the Description page of Project Settings.

#include "FWSkillIndicator.h"
#include "Engine/Engine.h"
#include "Engine/CollisionProfile.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialRenderProxy.h"
#include "PrimitiveViewRelevance.h"
#include "PrimitiveSceneProxy.h"
#include "SceneInterface.h"
#include "MeshElementCollector.h"
#include "DynamicMeshBuilder.h"
#include "SceneView.h"

// ----------------------------------------------------------------------------
// 씬 프록시 (렌더 스레드에서 컴포넌트를 대신해 그리는 객체)
// 엔진의 디버그 메시(ULineBatchComponent)와 같은 방식으로, 매 프레임 동적 메시 그림
// ----------------------------------------------------------------------------
class FFWSkillIndicatorSceneProxy final : public FPrimitiveSceneProxy
{
public:
	FFWSkillIndicatorSceneProxy(const UFWSkillIndicatorMeshComponent* InComponent, const UMaterialInterface* InMaterial)
		: FPrimitiveSceneProxy(InComponent)
		, Batches(InComponent->GetBatches())
		, MaterialProxy(InMaterial->GetRenderProxy())
		, MaterialRelevance(InMaterial->GetRelevance_Concurrent(GetScene().GetShaderPlatform()))
	{
	}

	virtual SIZE_T GetTypeHash() const override
	{
		static size_t UniquePointer;
		return reinterpret_cast<size_t>(&UniquePointer);
	}

	virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override
	{
		for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++) {
			if (!(VisibilityMap & (1 << ViewIndex))) {
				continue;
			}

			for (const FFWIndicatorBatch& Batch : Batches) {
				FDynamicMeshBuilder MeshBuilder(Views[ViewIndex]->GetFeatureLevel());

				for (int32 VertexIndex = 0; VertexIndex < Batch.Vertices.Num(); ++VertexIndex) {
					const FVector2f UV = Batch.UVs.IsValidIndex(VertexIndex) ? Batch.UVs[VertexIndex] : FVector2f::ZeroVector;
					MeshBuilder.AddVertex(Batch.Vertices[VertexIndex], UV,
						FVector3f(1.f, 0.f, 0.f), FVector3f(0.f, 1.f, 0.f), FVector3f(0.f, 0.f, 1.f), FColor::White);
				}
				for (int32 Index = 0; Index + 2 < Batch.Indices.Num(); Index += 3) {
					MeshBuilder.AddTriangle(Batch.Indices[Index], Batch.Indices[Index + 1], Batch.Indices[Index + 2]);
				}

				// 머티리얼의 "Color" 파라미터만 이 묶음의 색으로 바꿔서 그림 (한 프레임짜리 프록시, Collector 가 정리)
				FMaterialRenderProxy* ColoredProxy = new FColoredMaterialRenderProxy(MaterialProxy, Batch.Color);
				Collector.RegisterOneFrameMaterialProxy(ColoredProxy);

				MeshBuilder.GetMesh(GetLocalToWorld(), ColoredProxy, SDPG_World, /*bDisableBackfaceCulling=*/true, /*bReceivesDecals=*/false, ViewIndex, Collector);
			}
		}
	}

	virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
	{
		FPrimitiveViewRelevance Result;
		Result.bDrawRelevance = IsShown(View);
		Result.bDynamicRelevance = true;
		// 반투명 여부 등 머티리얼의 성질을 그대로 반영 (어느 렌더 패스에서 그릴지 결정됨)
		MaterialRelevance.SetPrimitiveViewRelevance(Result);
		return Result;
	}

	virtual uint32 GetMemoryFootprint() const override
	{
		return static_cast<uint32>(sizeof(*this) + FPrimitiveSceneProxy::GetAllocatedSize() + Batches.GetAllocatedSize());
	}

private:
	TArray<FFWIndicatorBatch> Batches;
	const FMaterialRenderProxy* MaterialProxy;
	FMaterialRelevance MaterialRelevance;
};

// ----------------------------------------------------------------------------
// 범위 표시 메시 컴포넌트
// ----------------------------------------------------------------------------
UFWSkillIndicatorMeshComponent::UFWSkillIndicatorMeshComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// 보이기만 하는 표시물: 충돌, 내비게이션, 그림자, 데칼에 관여하지 않음
	SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
	SetGenerateOverlapEvents(false);
	SetCanEverAffectNavigation(false);
	CastShadow = false;
	bReceivesDecals = false;
	bAffectDynamicIndirectLighting = false;
	bAffectDistanceFieldLighting = false;

	// 미니맵(SceneCapture)에는 그리지 않음
	bHiddenInSceneCapture = true;
}

void UFWSkillIndicatorMeshComponent::SetBatches(TArray<FFWIndicatorBatch>&& InBatches)
{
	Batches = MoveTemp(InBatches);

	// 컬링에 쓰이는 경계 상자 갱신
	LocalBounds = FBox(ForceInit);
	for (const FFWIndicatorBatch& Batch : Batches) {
		for (const FVector3f& Vertex : Batch.Vertices) {
			LocalBounds += FVector(Vertex);
		}
	}
	if (LocalBounds.IsValid) {
		LocalBounds = LocalBounds.ExpandBy(1.f);   // 납작한 모양
	}

	UpdateBounds();
	MarkRenderStateDirty();   // 바뀐 모양으로 프록시를 다시 만듦
}

void UFWSkillIndicatorMeshComponent::SetIndicatorMaterial(UMaterialInterface* InMaterial)
{
	if (IndicatorMaterial != InMaterial) {
		IndicatorMaterial = InMaterial;
		MarkRenderStateDirty();
	}
}

UMaterialInterface* UFWSkillIndicatorMeshComponent::GetResolvedMaterial() const
{
	if (IndicatorMaterial) {
		return IndicatorMaterial;
	}

	// 지정된 머티리얼이 없으면 엔진 기본 머티리얼 (반투명 + Unlit, "Color" 파라미터로 색 지정)
	if (GEngine) {
		return GEngine->DebugMeshMaterial.Get();
	}
	return nullptr;
}

FPrimitiveSceneProxy* UFWSkillIndicatorMeshComponent::CreateSceneProxy()
{
	const UMaterialInterface* Material = GetResolvedMaterial();
	if (Batches.Num() == 0 || !Material) {
		return nullptr;
	}

	return new FFWSkillIndicatorSceneProxy(this, Material);
}

FBoxSphereBounds UFWSkillIndicatorMeshComponent::CalcBounds(const FTransform& LocalToWorld) const
{
	if (!LocalBounds.IsValid) {
		return FBoxSphereBounds(LocalToWorld.GetLocation(), FVector(1.f), 1.f);
	}

	return FBoxSphereBounds(LocalBounds).TransformBy(LocalToWorld);
}

void UFWSkillIndicatorMeshComponent::GetUsedMaterials(TArray<UMaterialInterface*>& OutMaterials, bool bGetDebugMaterials) const
{
	if (UMaterialInterface* Material = GetResolvedMaterial()) {
		OutMaterials.Add(Material);
	}
}

// ----------------------------------------------------------------------------
// 범위 표시 액터
// ----------------------------------------------------------------------------
AFWSkillIndicator::AFWSkillIndicator()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UFWSkillIndicatorMeshComponent>(TEXT("IndicatorMesh"));
	SetRootComponent(MeshComponent);

	// 조준을 시작하기 전에는 보이지 않음
	SetHidden(true);
}

void AFWSkillIndicator::ShowLine(const FVector& Origin, const FVector& Direction, float Length, float Width)
{
	// 모양(삼각형)은 길이, 폭이 바뀔 때만 다시 만들고, 평소에는 위치와 방향만 옮김
	if (!FMath::IsNearlyEqual(Length, BuiltLength) || !FMath::IsNearlyEqual(Width, BuiltWidth)) {
		BuildLineShape(Length, Width);
	}

	const FRotator Facing(0.f, Direction.Rotation().Yaw, 0.f);   // 수평 방향만 사용
	SetActorLocationAndRotation(Origin + FVector(0.f, 0.f, GroundOffset), Facing);
	SetActorHiddenInGame(false);
}

void AFWSkillIndicator::HideIndicator()
{
	SetActorHiddenInGame(true);
}

#if WITH_EDITOR
void AFWSkillIndicator::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	BuiltLength = -1.f;
	BuiltWidth = -1.f;
}
#endif

void AFWSkillIndicator::BuildLineShape(float Length, float Width)
{
	BuiltLength = Length;
	BuiltWidth = Width;

	if (!MeshComponent) {
		return;
	}

	// 로컬 좌표: +X = 스킬이 나아가는 방향, +Y = 오른쪽, 원점 = 캐릭터 발밑
	const float ShapeLength = FMath::Max(Length, 1.f);
	const float ShapeWidth = FMath::Max(Width, 1.f);
	const float HalfWidth = ShapeWidth * 0.5f;

	const float HeadLength = FMath::Clamp(ShapeWidth * HeadLengthRatio, 0.f, ShapeLength * 0.4f);	// 화살촉 길이
	const float HeadThickness = HeadLength * FMath::Clamp(HeadThicknessRatio, 0.f, 1.f);			// 화살촉 가운데 두께
	const float HornX = ShapeLength - HeadLength;													// 화살촉 양 끝이 놓이는 위치
	const float StartX = FMath::Clamp(StartOffset, 0.f, HornX * 0.5f);								// 표시가 시작되는 위치
	const float EdgeWidth = FMath::Clamp(EdgeThickness, 0.f, HalfWidth * 0.5f);

	// 점 하나 추가 (UV: U = 진행 방향 0 -> 1, V = 폭 방향 0 -> 1)
	auto AddVertex = [ShapeLength, ShapeWidth](FFWIndicatorBatch& Batch, float X, float Y) -> int32 {
		Batch.UVs.Add(FVector2f(X / ShapeLength, Y / ShapeWidth + 0.5f));
		return Batch.Vertices.Add(FVector3f(X, Y, 0.f));
		};

	// 사각형 하나 추가. 뒤(캐릭터 쪽) 변과 앞 변의 X와 왼쪽, 오른쪽 변의 Y 를 받음
	auto AddQuad = [&AddVertex](FFWIndicatorBatch& Batch, float BackX, float FrontX, float LeftY, float RightY) {
		const int32 BackLeft = AddVertex(Batch, BackX, LeftY);
		const int32 BackRight = AddVertex(Batch, BackX, RightY);
		const int32 FrontRight = AddVertex(Batch, FrontX, RightY);
		const int32 FrontLeft = AddVertex(Batch, FrontX, LeftY);

		Batch.Indices.Append({ BackLeft, BackRight, FrontRight, BackLeft, FrontRight, FrontLeft });
		};

	// 화살촉은 두 곡선 사이를 채운 초승달 모양.
	// Side = -1 -> 1 이 왼쪽 끝 -> 오른쪽 끝이고(Y = HalfWidth * Side), 양 끝에서 두 곡선이 만나 뾰족해짐
	//    바깥 곡선 X = ShapeLength - HeadLength * Side^2
	//    안쪽 곡선 X = ShapeLength - HeadThickness - (HeadLength - HeadThickness) * Side^2
	auto HeadOuterX = [ShapeLength, HeadLength](float Side) {
		return ShapeLength - HeadLength * Side * Side;
		};
	auto HeadInnerX = [ShapeLength, HeadLength, HeadThickness](float Side) {
		return ShapeLength - HeadThickness - (HeadLength - HeadThickness) * Side * Side;
		};

	const FLinearColor BrightColor = IndicatorColor * Brightness;

	TArray<FFWIndicatorBatch> NewBatches;
	NewBatches.SetNum(3);

	// 양옆 경계선: 스킬 폭을 알려 주는 가는 선 두 줄 (화살촉 양 끝까지)
	FFWIndicatorBatch& Edges = NewBatches[0];
	Edges.Color = FLinearColor(BrightColor.R, BrightColor.G, BrightColor.B, EdgeOpacity);
	AddQuad(Edges, StartX, HornX, -HalfWidth, -HalfWidth + EdgeWidth);
	AddQuad(Edges, StartX, HornX, HalfWidth - EdgeWidth, HalfWidth);

	// 중앙 띠: 캐릭터 쪽은 가늘고 화살촉 쪽으로 갈수록 굵어짐. 앞쪽 끝은 화살촉 안쪽 곡선에 맞닿음
	FFWIndicatorBatch& Core = NewBatches[1];
	Core.Color = FLinearColor(BrightColor.R, BrightColor.G, BrightColor.B, CoreOpacity);

	const float CoreRatio = FMath::Clamp(CoreWidthRatio, 0.f, 1.f);
	const float CoreBackHalf = FMath::Min(1.5f, HalfWidth * 0.1f);
	const int32 CoreSegments = 6;

	const int32 CoreBackLeft = AddVertex(Core, StartX, -CoreBackHalf);
	const int32 CoreBackRight = AddVertex(Core, StartX, CoreBackHalf);
	int32 PrevFront = INDEX_NONE;
	for (int32 Step = 0; Step <= CoreSegments; ++Step) {
		const float Side = CoreRatio * (-1.f + 2.f * static_cast<float>(Step) / static_cast<float>(CoreSegments));
		const int32 Front = AddVertex(Core, HeadInnerX(Side), HalfWidth * Side);

		// 뒤쪽 왼쪽 점을 꼭짓점으로 삼각형을 부채꼴로 이어 붙임
		if (Step > 0) {
			Core.Indices.Append({ CoreBackLeft, PrevFront, Front });
		}
		PrevFront = Front;
	}
	Core.Indices.Append({ CoreBackLeft, PrevFront, CoreBackRight });

	// 화살촉: 바깥 곡선과 안쪽 곡선 사이를 사각형으로 잘게 나눠 채움
	FFWIndicatorBatch& Head = NewBatches[2];
	Head.Color = FLinearColor(BrightColor.R, BrightColor.G, BrightColor.B, HeadOpacity);

	const int32 HeadSegments = 16;
	for (int32 Step = 0; Step <= HeadSegments; ++Step) {
		const float Side = -1.f + 2.f * static_cast<float>(Step) / static_cast<float>(HeadSegments);
		const int32 Outer = AddVertex(Head, HeadOuterX(Side), HalfWidth * Side);
		const int32 Inner = AddVertex(Head, HeadInnerX(Side), HalfWidth * Side);

		// 직전 단계의 두 점(Outer - 2, Inner - 2)과 이어서 사각형 한 칸을 채움
		if (Step > 0) {
			Head.Indices.Append({ Outer - 2, Inner - 2, Outer, Outer, Inner - 2, Inner });
		}
	}

	MeshComponent->SetIndicatorMaterial(IndicatorMaterial);
	MeshComponent->SetBatches(MoveTemp(NewBatches));
}
