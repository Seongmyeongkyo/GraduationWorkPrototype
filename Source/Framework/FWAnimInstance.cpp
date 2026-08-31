// Fill out your copyright notice in the Description page of Project Settings.


#include "FWAnimInstance.h"
#include "FWCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "KismetAnimationLibrary.h"

void UFWAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	OwningCharacter = Cast<AFWCharacter>(TryGetPawnOwner());
}

void UFWAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	// 초기화 시점 폰 없을 경우 방지용
	if (!OwningCharacter) {
		OwningCharacter = Cast<AFWCharacter>(TryGetPawnOwner());
		if (!OwningCharacter) {
			return;
		}
	}

	const FVector Velocity = OwningCharacter->GetVelocity();

	// XY 평면 속도 크기
	GroundSpeed = Velocity.Size2D();

	// XY 평면 속도 방향
	Direction = UKismetAnimationLibrary::CalculateDirection(Velocity, OwningCharacter->GetActorRotation());

	// 이동 여부 (Idle 판정)
	bIsMoving = GroundSpeed > 3.f;

	// 캐릭터 공중 상태
	if (UCharacterMovementComponent* Move = OwningCharacter->GetCharacterMovement()) {
		bIsFalling = Move->IsFalling();
	}
}


