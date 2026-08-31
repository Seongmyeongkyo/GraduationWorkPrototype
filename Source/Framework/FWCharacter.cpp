// Copyright Epic Games, Inc. All Rights Reserved.

#include "FWCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimInstance.h"
#include "UObject/ConstructorHelpers.h"

AFWCharacter::AFWCharacter()
{
	// 카메라 리그에서 관리
	PrimaryActorTick.bCanEverTick = false;

	// 시점 고정
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	// 캐릭터 무브먼트
	if (UCharacterMovementComponent* Move = GetCharacterMovement())
	{
		Move->bOrientRotationToMovement = true;                 // 이동 방향으로 회전
		Move->RotationRate = FRotator(0.f, 640.f, 0.f);         // 회전 속도
		Move->bConstrainToPlane = true;                         // 평면 이동
		Move->bSnapToPlaneAtStart = true;
		Move->MaxWalkSpeed = 600.f;
		Move->BrakingDecelerationWalking = 2048.f;
		Move->bUseSeparateBrakingFriction = true;
		Move->BrakingFriction = 4.f;
	}

	// 캡슐 크기(언리얼 마네킹 표준 값)
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	if (USkeletalMeshComponent* MeshComp = GetMesh())
	{
		MeshComp->SetRelativeLocation(FVector(0.f, 0.f, -96.f));
		MeshComp->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));

		static ConstructorHelpers::FObjectFinder<USkeletalMesh> MannequinMesh(
			TEXT("/Game/Characters/Mannequins/Meshes/SKM_Manny_Simple.SKM_Manny_Simple"));
		if (MannequinMesh.Succeeded())
		{
			MeshComp->SetSkeletalMeshAsset(MannequinMesh.Object);
		}

		// 애니메이션 블루프린트 연결
		static ConstructorHelpers::FClassFinder<UAnimInstance> AnimBP(
			TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
		if (AnimBP.Succeeded())
		{
			MeshComp->SetAnimInstanceClass(AnimBP.Class);
		}
	}
}
