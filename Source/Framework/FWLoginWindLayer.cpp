#include "FWLoginWindLayer.h"
#include "UI/FWLoginWindModel.h"
#include "Engine/Texture2D.h"
#include "Framework/Application/SlateApplication.h"
#include "ImageCore.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SLeafWidget.h"

namespace
{
	double Luminance(const FColor& C) { return .2126 * C.R + .7152 * C.G + .0722 * C.B; }

	// Canvas와 같이 sRGB 값에서 알파를 곱해 보간하고 다시 나눕니다.
	// 투명 PNG 테두리의 검은색이 잎 색상 보정에 섞이지 않게 합니다.
	void ResizeForCanvas(const FImage& Source, FImage& Result)
	{
		const int32 W = FWLoginWind::SpriteWidth;
		const int32 H = FMath::RoundToInt(double(W) * Source.SizeY / Source.SizeX);
		Result.Init(W, H, ERawImageFormat::BGRA8, EGammaSpace::sRGB);
		const auto Src = Source.AsBGRA8();
		auto Dst = Result.AsBGRA8();
		for (int32 Y = 0; Y < H; ++Y)
		{
			const double SY = (Y + .5) * Source.SizeY / H - .5;
			const int32 Y0 = FMath::FloorToInt(SY);
			for (int32 X = 0; X < W; ++X)
			{
				const double SX = (X + .5) * Source.SizeX / W - .5;
				const int32 X0 = FMath::FloorToInt(SX);
				double R = 0, G = 0, B = 0, Alpha = 0;
				for (int32 J = 0; J < 2; ++J)
				{
					for (int32 I = 0; I < 2; ++I)
					{
						const FColor P = Src[FMath::Clamp(Y0 + J, 0, Source.SizeY - 1) * Source.SizeX + FMath::Clamp(X0 + I, 0, Source.SizeX - 1)];
						const double Weight = (I ? SX - X0 : 1 - (SX - X0)) * (J ? SY - Y0 : 1 - (SY - Y0));
						const double A = P.A * Weight;
						Alpha += A; R += P.R * A; G += P.G * A; B += P.B * A;
					}
				}
				Dst[Y * W + X] = Alpha > 0 ? FColor(FMath::RoundToInt(R / Alpha), FMath::RoundToInt(G / Alpha), FMath::RoundToInt(B / Alpha), FMath::RoundToInt(Alpha)) : FColor(0,0,0,0);
			}
		}
	}

	// Canvas의 초기 색상 보정에 대응. 원본 PNG는 수정하지 않고 메모리 안에서만 보정합니다.
	void MatchPalette(FImage& Sprite, const FImage& Background, const FWLoginWind::FCluster& C)
	{
		TArray<FColor> Colors;
		const double CX = C.X + C.Flip * C.Width * .45, CY = C.Y - C.Width * .18;
		const auto BG = Background.AsBGRA8();
		for (int32 Y = FMath::Max(0, FMath::FloorToInt(CY - 42)); Y < FMath::Min(double(Background.SizeY), CY + 42); Y += 2)
		{
			for (int32 X = FMath::Max(0, FMath::FloorToInt(CX - 55)); X < FMath::Min(double(Background.SizeX), CX + 55); X += 2)
			{
				const FColor P = BG[Y * Background.SizeX + X];
				if (P.G > P.B * 1.35 && P.G > P.R * .77 && P.G > 24 && P.R < P.G * 1.35) Colors.Add(P);
			}
		}
		Colors.StableSort([](const FColor& A, const FColor& B) { return Luminance(A) < Luminance(B); });
		FColor Palette[] = { FColor(29,48,12), FColor(86,104,25), FColor(188,179,48) };
		if (Colors.Num() > 8)
		{
			Palette[0] = Colors[FMath::FloorToInt((Colors.Num() - 1) * .18)];
			Palette[1] = Colors[FMath::FloorToInt((Colors.Num() - 1) * .52)];
			Palette[2] = Colors[FMath::FloorToInt((Colors.Num() - 1) * .86)];
		}
		for (FColor& P : Sprite.AsBGRA8())
		{
			if (!P.A) continue;
			const double V = FMath::Clamp((Luminance(P) - 25.0) / 195.0, 0.0, 1.0);
			const FColor A = Palette[V < .5 ? 0 : 1], B = Palette[V < .5 ? 1 : 2];
			const double F = V < .5 ? V * 2.0 : (V - .5) * 2.0;
			P.R = uint8(FMath::RoundToInt(A.R + (B.R - A.R) * F));
			P.G = uint8(FMath::RoundToInt(A.G + (B.G - A.G) * F));
			P.B = uint8(FMath::RoundToInt(A.B + (B.B - A.B) * F));
		}
	}
}

class SFWLoginWindLayer : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SFWLoginWindLayer) {}
		SLATE_ARGUMENT(TArray<FSlateBrush>, Brushes)
	SLATE_END_ARGS()
	void Construct(const FArguments& Args)
	{
		Brushes = Args._Brushes;
		SetCanTick(true);
	}
	void SetParameters(float Wind, float Distant, int32 Leaves, bool bBranches)
	{
		WindStrength = Wind; Emphasis = Distant; LeafCount = Leaves; bShowBranches = bBranches;
		ForceVolatile(Wind > 0.f && (Leaves > 0 || (bBranches && !Brushes.IsEmpty())));
		Invalidate(EInvalidateWidgetReason::Paint);
	}
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(FWLoginWind::Width, FWLoginWind::Height); }
	virtual void Tick(const FGeometry&, double, float DeltaTime) override
	{
		Time = FWLoginWind::Advance(Time, DeltaTime, WindStrength);
	}
	double GetTime() const { return Time; }

	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
		FSlateWindowElementList& Out, int32 Layer, const FWidgetStyle& Style, bool) const override
	{
		using namespace FWLoginWind;
		const FVector2f Size(Geometry.GetLocalSize());
		if (Size.X <= 0.f || Size.Y <= 0.f) return Layer;
		// 고정 배경과 동일한 중앙 크롭. 지면에는 어떤 변형도 적용하지 않습니다.
		const float Cover = FMath::Max(Size.X / Width, Size.Y / Height);
		const FVector2f Origin = (Size - FVector2f(Width, Height) * Cover) * .5f;
		const auto Transform = Geometry.GetAccumulatedRenderTransform();
		const FLinearColor Tint = Style.GetColorAndOpacityTint();
		FSlateRenderer* Renderer = FSlateApplication::Get().GetRenderer();
		auto Vertex = [&](const FVector2f& Point, const FVector2f& UV, FColor Color)
		{
			return FSlateVertex::Make<ESlateVertexRounding::Disabled>(Transform, Origin + Point * Cover, UV, Color);
		};
		if (bShowBranches)
		{
			for (int32 Index = 0; Index < Brushes.Num(); ++Index)
			{
				const FCluster& C = Clusters[Index];
				const FSlateBrush& Brush = Brushes[Index];
				const float H = C.Width * Brush.ImageSize.Y / Brush.ImageSize.X;
				const double Angle = FMath::DegreesToRadians(double(C.Angle)) + Rocking(C, Time, WindStrength, Emphasis);
				const float Cos = float(FMath::Cos(Angle)), Sin = float(FMath::Sin(Angle));
				auto Position = [&](float X, float Y)
				{
					X *= C.Flip;
					return FVector2f(C.X + X * Cos - Y * Sin, C.Y + X * Sin + Y * Cos);
				};
				TArray<FSlateVertex> Vertices;
				TArray<SlateIndex> Indices;
				Vertices.Reserve(StripCount * 4); Indices.Reserve(StripCount * 6);
				const FColor Color = (FLinearColor(1, 1, 1, C.Opacity) * Tint).ToFColor(true);
				for (int32 I = 0; I < StripCount; ++I)
				{
					const double U = double(I) / StripCount;
					const float X = float((U - .035) * C.Width);
					const float Y = -H * .71f + float(Bend(C, U, Time, WindStrength, Emphasis));
					const float W = C.Width / StripCount + .6f;
					const float SW = Brush.ImageSize.X / StripCount;
					const float UEnd = FMath::Min(1.f, (I * SW + SW + FMath::Min(1.f, Brush.ImageSize.X - (I + 1) * SW)) / Brush.ImageSize.X);
					const SlateIndex Base = Vertices.Num();
					Vertices.Add(Vertex(Position(X, Y), FVector2f(U, 0), Color));
					Vertices.Add(Vertex(Position(X + W, Y), FVector2f(UEnd, 0), Color));
					Vertices.Add(Vertex(Position(X, Y + H), FVector2f(U, 1), Color));
					Vertices.Add(Vertex(Position(X + W, Y + H), FVector2f(UEnd, 1), Color));
					Indices.Append({Base, SlateIndex(Base + 1), SlateIndex(Base + 2), SlateIndex(Base + 1), SlateIndex(Base + 3), SlateIndex(Base + 2)});
				}
				FSlateDrawElement::MakeCustomVerts(Out, Layer + Index, Renderer->GetResourceHandle(Brush), Vertices, Indices, nullptr, 0, 0);
			}
		}
		const FSlateBrush* White = FCoreStyle::Get().GetBrush("WhiteBrush");
		const FColor LeafColors[] = { FColor(174,183,85), FColor(199,166,83), FColor(112,139,66), FColor(161,173,86) };
		const int32 LeafLayer = Layer + UE_ARRAY_COUNT(Clusters);
		for (int32 I = 0; I < LeafCount; ++I)
		{
			const double Seed = FMath::Fmod(I * .61803398875, 1.0);
			const double P = FMath::Fmod(Time / (22 + (I % 5) * 3) + Seed, 1.0);
			const FVector2f Center(-80 + P * 1832, 100 + ((I * 113) % 410) + P * 180 + 24 * FMath::Sin(P * 8 + I));
			const float LeafSize = 7 + (I % 4) * 2;
			const double Angle = I + Time * (.3 + (I % 3) * .08);
			const float Cos = FMath::Cos(Angle), Sin = FMath::Sin(Angle);
			const float Flip = FMath::Cos(Time * 1.3 + I) * .65 + .2;
			auto Point = [&](FVector2f V)
			{
				V.X *= Flip;
				return Center + FVector2f(V.X * Cos - V.Y * Sin, V.X * Sin + V.Y * Cos);
			};
			const float Alpha = .7 * FMath::Min(1.0, FMath::Min(P * 12, (1.0 - P) * 12));
			FLinearColor Fill = FLinearColor::FromSRGBColor(LeafColors[I % 4]); Fill.A = Alpha;
			TArray<FSlateVertex> Vertices;
			TArray<SlateIndex> Indices;
			Vertices.Add(Vertex(Center, FVector2f(.5f, .5f), (Fill * Tint).ToFColor(true)));
			// HTML의 두 베지어 곡선을 작은 삼각형으로 표시합니다.
			for (int32 Curve = 0; Curve < 2; ++Curve)
			{
				const FVector2f A = Curve == 0 ? FVector2f(-1, 0) : FVector2f(1, 0);
				const FVector2f B = Curve == 0 ? FVector2f(-.2f, -.7f) : FVector2f(.35f, .6f);
				const FVector2f C = Curve == 0 ? FVector2f(.65f, -.45f) : FVector2f(-.5f, .5f);
				const FVector2f D = -A;
				for (int32 J = 0; J < 12; ++J)
				{
					const float T = J / 12.f, S = 1.f - T;
					const FVector2f V = (A * S*S*S + B * 3*S*S*T + C * 3*S*T*T + D * T*T*T) * LeafSize;
					Vertices.Add(Vertex(Point(V), FVector2f(.5f, .5f), (Fill * Tint).ToFColor(true)));
				}
			}
			for (int32 J = 0; J < 24; ++J) Indices.Append({0, SlateIndex(J + 1), SlateIndex((J + 1) % 24 + 1)});
			FSlateDrawElement::MakeCustomVerts(Out, LeafLayer + I * 2, Renderer->GetResourceHandle(*White), Vertices, Indices, nullptr, 0, 0);
			FLinearColor Vein = FLinearColor::FromSRGBColor(FColor(115,116,68)); Vein.A = Alpha;
			TArray<FVector2f> Line = {Origin + Point(FVector2f(-LeafSize, 0)) * Cover, Origin + Point(FVector2f(LeafSize, 0)) * Cover};
			FSlateDrawElement::MakeLines(Out, LeafLayer + I * 2 + 1, Geometry.ToPaintGeometry(), Line, ESlateDrawEffect::None, Vein * Tint, true, .6f * Cover);
		}
		return LeafLayer + LeafCount * 2;
	}
private:
	TArray<FSlateBrush> Brushes;
	double Time = 0;
	float WindStrength = 2.9f, Emphasis = 3.f;
	int32 LeafCount = 12;
	bool bShowBranches = true;
};

void UFWLoginWindLayer::PrepareSprites()
{
	if (bAttemptedPrepare) return;
	bAttemptedPrepare = true;
	const FString Root = FPaths::ProjectDir() / TEXT("Art/UI/LoginScreen");
	FImage Background;
	if (!FImageUtils::LoadImage(*(Root / TEXT("CloseIN_LoginBackground_Forest_v3.png")), Background))
	{
		UE_LOG(LogTemp, Warning, TEXT("[FWLoginWind] Palette source PNG unavailable; retaining the fixed background."));
		return;
	}
	Background.ChangeFormat(ERawImageFormat::BGRA8, EGammaSpace::sRGB);
	if (Background.SizeX != 1672 || Background.SizeY != 941)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FWLoginWind] Expected Forest_v3 at 1672x941; skipping unmatched branch layers."));
		return;
	}
	FImage Sprites[3];
	const TCHAR* Names[] = {TEXT("FoliageSprig.png"), TEXT("FoliageCluster.png"), TEXT("FoliageDistant.png")};
	for (int32 I = 0; I < 3; ++I)
	{
		FImage Source;
		if (!FImageUtils::LoadImage(*(Root / TEXT("WindPreview") / Names[I]), Source))
		{
			UE_LOG(LogTemp, Warning, TEXT("[FWLoginWind] Missing layer PNG: %s"), Names[I]);
			return;
		}
		// HTML의 768px 캐시와 같은 크기. 색상 보정은 크기 변경 후 한 번만 수행합니다.
		Source.ChangeFormat(ERawImageFormat::BGRA8, EGammaSpace::sRGB);
		ResizeForCanvas(Source, Sprites[I]);
	}
	for (const FWLoginWind::FCluster& Cluster : FWLoginWind::Clusters)
	{
		FImage Matched;
		Sprites[Cluster.Variant].CopyTo(Matched, ERawImageFormat::BGRA8, EGammaSpace::sRGB);
		MatchPalette(Matched, Background, Cluster);
		UTexture2D* Texture = FImageUtils::CreateTexture2DFromImage(Matched);
		if (!Texture)
		{
			BranchTextures.Reset(); BranchBrushes.Reset();
			return;
		}
		Texture->SRGB = true;
		Texture->Filter = TF_Bilinear;
		Texture->AddressX = TA_Clamp; Texture->AddressY = TA_Clamp;
		Texture->NeverStream = true;
		Texture->UpdateResource();
		BranchTextures.Add(Texture);
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = FVector2D(Matched.SizeX, Matched.SizeY);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		BranchBrushes.Add(Brush);
	}
}

void UFWLoginWindLayer::SetWindParameters(float InStrength, float InDistantEmphasis, int32 InLeafCount, bool bInShowBranches)
{
	WindStrength = FMath::IsFinite(InStrength) ? FMath::Clamp(InStrength, 0.f, 6.f) : 2.9f;
	DistantEmphasis = FMath::IsFinite(InDistantEmphasis) ? FMath::Clamp(InDistantEmphasis, 1.f, 4.f) : 3.f;
	FloatingLeafCount = FMath::Clamp(InLeafCount, 0, 18);
	bShowBranches = bInShowBranches;
	if (WindSlate) WindSlate->SetParameters(WindStrength, DistantEmphasis, FloatingLeafCount, bShowBranches);
}

void UFWLoginWindLayer::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	SetWindParameters(WindStrength, DistantEmphasis, FloatingLeafCount, bShowBranches);
}

TSharedRef<SWidget> UFWLoginWindLayer::RebuildWidget()
{
	PrepareSprites();
	SAssignNew(WindSlate, SFWLoginWindLayer).Brushes(BranchBrushes);
	SetWindParameters(WindStrength, DistantEmphasis, FloatingLeafCount, bShowBranches);
	return WindSlate.ToSharedRef();
}

void UFWLoginWindLayer::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	WindSlate.Reset();
}

double UFWLoginWindLayer::GetAnimationTime() const { return WindSlate ? WindSlate->GetTime() : 0.0; }
