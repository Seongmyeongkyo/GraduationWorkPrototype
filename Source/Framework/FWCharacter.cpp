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
#include "FWHealManaActorComponent.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "EngineUtils.h"

AFWCharacter::AFWCharacter()
{
	// 원격 아바타(다른 플레이어)의 이동 추종에 사용. 카메라 줌 등은 여전히 카메라 리그에서 관리.
	PrimaryActorTick.bCanEverTick = true;

	/** 캐릭터 액터 자체 복제 켜기 (다른 클라이언트에 보이기 위해) **/
	bReplicates = true;
	SetReplicateMovement(true);

	/** 스탯 컴포넌트 부착 (컴포넌트 자체 복제는 Component 생성자에 포함)**/
	AttributeComp = CreateDefaultSubobject<UFWHealManaActorComponent>(TEXT("AttributeComp"));

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

	// LoL-style unit collision: characters don't hard-block each other (terrain/walls still block);
	// a moving character sidesteps others instead (ApplySoftSeparation).
	// Each client simulated these collisions separately, so one screen rubbed against an avatar that
	// another screen walked straight past - the positions then drifted apart for good.
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

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
		MoveTowardRemoteDestination(DeltaSeconds);
	}

	ApplySoftSeparation(DeltaSeconds);
}

void AFWCharacter::ApplySoftSeparation(float DeltaSeconds)
{
	// Standing characters are never pushed - only the one walking into someone sidesteps.
	const FVector Velocity2D(GetVelocity().X, GetVelocity().Y, 0.f);
	if (Velocity2D.SizeSquared() < 1.f || !GetWorld())
	{
		return;
	}

	// Push only sideways (perpendicular to where we're heading). Forward motion stays with the
	// path/destination, so the two never fight each other - that fight is what made RVO orbit.
	const FVector Forward = Velocity2D.GetSafeNormal();
	const FVector Right = FVector::CrossProduct(FVector::UpVector, Forward);
	const float MinDistance = GetCapsuleComponent()->GetScaledCapsuleRadius() * 2.f;

	FVector Push = FVector::ZeroVector;
	for (TActorIterator<AFWCharacter> It(GetWorld()); It; ++It)
	{
		const AFWCharacter* Other = *It;
		if (Other == this || Other->IsHidden()) // hidden = in a bush we can't see into
		{
			continue;
		}
		const FVector ToOther(Other->GetActorLocation().X - GetActorLocation().X, Other->GetActorLocation().Y - GetActorLocation().Y, 0.f);
		const float Distance = ToOther.Size();
		if (Distance >= MinDistance || FVector::DotProduct(ToOther, Forward) < 0.f) // not overlapping, or already behind us
		{
			continue;
		}
		// Other on our right -> step left, and vice versa (dead ahead -> step right, so every client agrees).
		const float Side = FVector::DotProduct(ToOther, Right) > 0.f ? -1.f : 1.f;
		Push += Right * Side * (MinDistance - Distance);
	}

	if (!Push.IsNearlyZero())
	{
		constexpr float SidestepSpeed = 600.f; // cm/s, same as walking
		AddActorWorldOffset(Push.GetClampedToMaxSize(SidestepSpeed * DeltaSeconds), /*bSweep=*/true);
	}
}

void AFWCharacter::SetRemoteDestination(const FVector& Destination)
{
	RemoteDestination = Destination;
	bHasRemoteDestination = true;

	// Take the same NavMesh route the owner's pawn takes (it moves with SimpleMoveToLocation),
	// so the avatar goes around obstacles instead of getting stuck and drifting from the real position.
	// No AIController on purpose: possessing the avatar would make IsLocallyControlled() true in
	// standalone, and remote avatars would then report bushes/HP as if they were the local player.
	RemotePathPoints.Reset();
	RemotePathIndex = 0;
	if (UNavigationPath* Path = UNavigationSystemV1::FindPathToLocationSynchronously(this, GetActorLocation(), Destination, this))
	{
		if (Path->IsValid() && Path->PathPoints.Num() >= 2)
		{
			RemotePathPoints = Path->PathPoints;
			RemotePathIndex = 1; // [0] is where we are now
		}
	}
}

void AFWCharacter::MoveTowardRemoteDestination(float DeltaSeconds)
{
	if (!bHasRemoteDestination)
	{
		return;
	}

	const bool bFollowingPath = RemotePathPoints.IsValidIndex(RemotePathIndex);
	const bool bLastPoint = !bFollowingPath || RemotePathIndex == RemotePathPoints.Num() - 1;
	const FVector Target = bFollowingPath ? RemotePathPoints[RemotePathIndex] : RemoteDestination;
	const FVector ToTarget = Target - GetActorLocation();

	// Intermediate corners use a wider radius so the avatar doesn't brake at every turn.
	constexpr float AcceptanceRadius = 10.f;
	constexpr float CornerRadius = 50.f;
	// Never smaller than one frame's step: at 600cm/s and a low frame rate the avatar moves more than the
	// radius per frame, so it could hop over the circle every frame and jitter around the point forever.
	const float StepThisFrame = GetCharacterMovement()->GetMaxSpeed() * DeltaSeconds;
	const float ReachDistance = FMath::Max(bLastPoint ? AcceptanceRadius : CornerRadius, StepThisFrame);
	if (FVector(ToTarget.X, ToTarget.Y, 0.f).SizeSquared() <= FMath::Square(ReachDistance))
	{
		if (!bLastPoint)
		{
			++RemotePathIndex;
			return;
		}
		// 도착 -> 정확히 목적지에 세우고 남은 속도를 없앰 (브레이크 관성으로 지나쳐 미끄러지지 않게)
		SetActorLocation(FVector(Target.X, Target.Y, GetActorLocation().Z), /*bSweep=*/true);
		GetCharacterMovement()->StopMovementImmediately();
		bHasRemoteDestination = false;
		RemotePathPoints.Reset();
		return;
	}

	// NavMesh 경로가 있으면 다음 꺾이는 지점으로, 없으면(NavMesh 밖 등) 목적지로 직선 이동
	AddMovementInput(ToTarget.GetSafeNormal2D(), 1.f);
}
