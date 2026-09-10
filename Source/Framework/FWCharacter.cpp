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
	// 원격 아바타(다른 플레이어)의 이동 추종에 사용. 카메라 줌 등은 여전히 카메라 리그에서 관리.
	PrimaryActorTick.bCanEverTick = true;

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

		// Controller 없는(원격 아바타) 상태에서도 실제로 걷도록 허용.
		// 기본값(false)이면 CharacterMovementComponent 가 Controller 없는 캐릭터의
		// velocity/acceleration 을 매 틱 0으로 되돌려 AddMovementInput 이 무시된다.
		Move->bRunPhysicsWithNoController = true;
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

void AFWCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bIsRemoteAvatar)
	{
		MoveTowardRemoteDestination();
	}
}

void AFWCharacter::SetRemoteDestination(const FVector& Destination)
{
	RemoteDestination = Destination;
	bHasRemoteDestination = true;
}

void AFWCharacter::MoveTowardRemoteDestination()
{
	if (!bHasRemoteDestination)
	{
		return;
	}

	const FVector ToTarget = RemoteDestination - GetActorLocation();

	constexpr float AcceptanceRadius = 10.f;
	if (FVector(ToTarget.X, ToTarget.Y, 0.f).SizeSquared() <= FMath::Square(AcceptanceRadius))
	{
		bHasRemoteDestination = false; // 도착 -> 정지
		return;
	}

	// 로컬 플레이어처럼 NavMesh 경로를 따라가지 않고 직선으로 이동한다 (단순화).
	// 장애물 회피가 필요해지면 AIController 를 붙여 SimpleMoveToLocation 으로 교체.
	AddMovementInput(ToTarget.GetSafeNormal2D(), 1.f);
}
