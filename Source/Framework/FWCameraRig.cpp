// Fill out your copyright notice in the Description page of Project Settings.


#include "FWCameraRig.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"

// Sets default values
AFWCameraRig::AFWCameraRig()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Root ---> 카메라 리그의 초점 위치를 잡는 역할
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	// SpringArm ---> 카메라의 위치를 잡는 역할
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(SceneRoot);
	CameraBoom->TargetArmLength = CameraArmLength;
	CameraBoom->SetRelativeRotation(FRotator(CameraPitch, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false;

	// Camera ---> 실제 카메라
	TopDownCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCamera->bUsePawnControlRotation = false;
	
	DesiredArmLength = CameraArmLength;
}

// Called when the game starts or when spawned
void AFWCameraRig::BeginPlay()
{
	Super::BeginPlay();
	
}

// Called every frame
void AFWCameraRig::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Zoom 보간
	if (CameraBoom && !FMath::IsNearlyEqual(CameraBoom->TargetArmLength, DesiredArmLength)) {
		CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, DesiredArmLength, DeltaTime, ZoomInterSpeed);
	}

	// Pan 보간
	const FVector TargetVelocity = InputPanDirection * PanMaxSpeed;
	const bool bAccelerating = !InputPanDirection.IsNearlyZero();
	const float InterpSpeed = bAccelerating ? PanAccelInterpSpeed : PanDecelInterpSpeed;

	CurrentPanVelocity = FMath::VInterpTo(CurrentPanVelocity, TargetVelocity, DeltaTime, InterpSpeed);
	
	if (!CurrentPanVelocity.IsNearlyZero()) {
		AddActorWorldOffset(CurrentPanVelocity * DeltaTime);
	}
}

void AFWCameraRig::SnapTo(const FVector& NewLocation)
{
	SetActorLocation(NewLocation);

	// SnapTo 시점에서 잔여 속도 초기화
	CurrentPanVelocity = FVector::ZeroVector;
	InputPanDirection = FVector::ZeroVector;
}

void AFWCameraRig::SetPanDirection(const FVector& WorldDirection)
{
	InputPanDirection = WorldDirection;
}

void AFWCameraRig::AddZoom(float ScrollDelta)
{
	DesiredArmLength = FMath::Clamp(
		DesiredArmLength - ScrollDelta * ZoomStep,
		MinArmLength,
		MaxArmLength
	);
}

