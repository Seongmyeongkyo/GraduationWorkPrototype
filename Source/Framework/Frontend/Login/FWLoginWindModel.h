#pragma once

#include "CoreMinimal.h"

// Art/UI/LoginScreen/WindPreview/index.html에서 승인된 배치/수식.
// 배경 이미지에는 적용하지 않습니다. 추가 잎 레이어만 이 좌표를 사용합니다.
namespace FWLoginWind
{
	constexpr float Width = 1672.f; // 원본 이미지 기준 가로 크기
	constexpr float Height = 941.f; // 원본 이미지 기준 세로 크기
	constexpr int32 StripCount = 54; // HTML과 같은 잎가지 세로 분할 수
	constexpr int32 SpriteWidth = 768; // 색상 보정용 잎 텍스처 가로 크기

	struct FCluster
	{
		float X, Y, Width, Angle, Flip, Phase;
		int32 Variant;
		float Opacity, Sway;
	};

	// 고정점 X/Y, 가로 크기, 회전(도), 좌우 반전, 위상, 이미지 종류, 불투명도, 흔들림 배율.
	// HTML의 brightness/contrast/saturation은 최종 코드에서 미사용이므로 적용하지 않습니다.
	inline constexpr FCluster Clusters[] = {
		{45,112,275,14,1,0,0,1,1},
		{258,69,148,-7,1,1,1,.92f,.8f},
		{442,89,122,9,1,2,1,.88f,.65f},
		{657,65,85,-8,1,3,2,.78f,1.15f},
		{981,88,68,13,1,4,2,.72f,1.1f},
		{1118,59,59,-16,1,8,1,.7f,1.05f},
		{1617,57,94,12,-1,5,1,.86f,.85f},
		{1576,117,72,-9,-1,9,2,.8f,.75f},
		{25,454,163,-16,1,7,0,.95f,.85f}
	};

	inline bool IsDistant(const FCluster& C) { return C.Width < 100.f && C.X < 1250.f; }
	inline double Strength(const FCluster& C, double Wind, double Emphasis)
	{
		return IsDistant(C) ? FMath::Min(Wind * Emphasis, 5.0) : Wind;
	}
	inline double Advance(double Time, double Delta, double Wind)
	{
		return Time + FMath::Clamp(Delta, 0.0, .05) * FMath::Min(Wind, .8);
	}
	inline double Rocking(const FCluster& C, double Time, double Wind, double Emphasis)
	{
		return IsDistant(C) ? Strength(C, Wind, Emphasis) * .018 * FMath::Sin(Time * .95 + C.Phase) : 0.0;
	}
	inline double Bend(const FCluster& C, double U, double Time, double Wind, double Emphasis)
	{
		return Strength(C, Wind, Emphasis) * C.Sway * U * U *
			(3.3 * FMath::Sin(Time * .95 + C.Phase) + 1.1 * FMath::Sin(Time * 2.1 + C.Phase + U * 5.0));
	}
}
