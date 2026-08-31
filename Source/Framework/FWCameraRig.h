// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FWCameraRig.generated.h"

class USpringArmComponent;
class UCameraComponent;

UCLASS()
class FRAMEWORK_API AFWCameraRig : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AFWCameraRig();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float CameraPitch = -50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	float CameraArmLength = 1400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | Zoom")
	float MinArmLength = 600.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | Zoom")
	float MaxArmLength = 2400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | Zoom")
	float ZoomStep = 150.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | Zoom")
	float ZoomInterSpeed = 10.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | Pan")
	float PanMaxSpeed = 2500.f;	// 최대 속도

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | Pan")
	float PanAccelInterpSpeed = 10.f; // 가속 보간 속도

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera | Pan")
	float PanDecelInterpSpeed = 6.f; // 감속 보간 속도

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	// LockMode
	void SnapTo(const FVector& NewLocation);

	// 매 프레임 방향만 전달
	void SetPanDirection(const FVector& WorldDirection);

	// ZoomMode
	void AddZoom(float ScrollDelta);

protected:
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USceneComponent> SceneRoot;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	TObjectPtr<UCameraComponent> TopDownCamera;

private:
	float DesiredArmLength = 1400.f;

	// Pan 속도 상태
	FVector CurrentPanVelocity = FVector::ZeroVector;
	FVector InputPanDirection = FVector::ZeroVector;

};
