#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "FWLoginWindLayer.generated.h"

class SFWLoginWindLayer;
class UTexture2D;

/** 승인된 HTML의 투명 잎 레이어만 표시. 고정 배경 UImage와 완전히 분리합니다. */
UCLASS(Config=Game)
class FRAMEWORK_API UFWLoginWindLayer : public UWidget
{
	GENERATED_BODY()
public:
	/** 바람 세기 0~6. 0이면 가지와 낙엽 모두 정지, 기본값 2.9. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Login|Wind", meta=(ClampMin="0", ClampMax="6"))
	float WindStrength = 2.9f;
	/** 뒤쪽 나무 강조 1~4. 원경 유효 세기는 5에서 제한, 기본값 3.0. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Login|Wind", meta=(ClampMin="1", ClampMax="4"))
	float DistantEmphasis = 3.f;
	/** 흩날리는 낙엽 개수 0~18. 기본값 12. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Login|Wind", meta=(ClampMin="0", ClampMax="18"))
	int32 FloatingLeafCount = 12;
	/** 나무에 덧붙인 잎가지 9개 표시 여부. 낙엽 설정과 독립적입니다. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category="Login|Wind")
	bool bShowBranches = true;

	/** 실행 중 설정 변경용. 초기값은 DefaultGame.ini에서 수정합니다. */
	UFUNCTION(BlueprintCallable, Category="Login|Wind")
	void SetWindParameters(float InStrength, float InDistantEmphasis, int32 InLeafCount, bool bInShowBranches);
	virtual void SynchronizeProperties() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;
	int32 GetPreparedBranchCount() const { return BranchTextures.Num(); }
	double GetAnimationTime() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void PrepareSprites();
	bool bAttemptedPrepare = false;
	UPROPERTY(Transient)
	TArray<TObjectPtr<UTexture2D>> BranchTextures;
	UPROPERTY(Transient)
	TArray<FSlateBrush> BranchBrushes;
	TSharedPtr<SFWLoginWindLayer> WindSlate;
};
