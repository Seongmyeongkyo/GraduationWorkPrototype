// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "FWAnimInstance.generated.h"

class AFWCharacter;

/**
 * 
 */
UCLASS()
class FRAMEWORK_API UFWAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	/** BlendSpace 의 Speed 축 (XY 평면 속도 크기) **/
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float GroundSpeed;

	/** BlendSpace 의 Direction 축 (XY 평면 속도 방향) **/
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	float Direction = 0.f;

	/** 캐릭터 공중 상태 **/
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsFalling = false;

	/** 이동 여부 (Idle 판정) **/
	UPROPERTY(BlueprintReadOnly, Category = "Movement")
	bool bIsMoving = false;

protected:
	UPROPERTY(BlueprintReadOnly, Transient, Category = "References")
	TObjectPtr<AFWCharacter> OwningCharacter;
	
};
