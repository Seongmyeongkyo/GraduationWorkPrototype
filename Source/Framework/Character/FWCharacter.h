// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "FWCharacter.generated.h"

class UFWHealManaActorComponent;

UCLASS()
class FRAMEWORK_API AFWCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	AFWCharacter();

	/** HP/MP 등 스탯 저장소, 서버 권한 + 클라이언트 자동 복제. **/
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FW|Stats")
	TObjectPtr<UFWHealManaActorComponent> AttributeComp;

	virtual void Tick(float DeltaSeconds) override;

	/** true면 다른 플레이어를 나타내는 아바타로, 로컬 입력 대신 네트워크 좌표를 따라 이동합니다. */
	UPROPERTY(BlueprintReadOnly, Category = "Network")
	bool bIsRemoteAvatar = false;

	/** 서버가 부여한 플레이어 ID (원격 아바타 식별용). */
	UPROPERTY(BlueprintReadOnly, Category = "Network")
	int32 NetPlayerId = INDEX_NONE;

	/** 원격 아바타가 걸어갈 목표 지점을 설정합니다(로컬로 조작되는 폰에는 사용하지 않음). */
	void SetRemoteDestination(const FVector& Destination);

	/** 원격 아바타의 이동 목표를 지우고 즉시 정지 (순간이동 직후 사용) **/
	void StopRemoteMovement();

private:
	/** 원격 아바타를 목표 지점으로 이동시킴 (Tick 에서 호출, bIsRemoteAvatar 일 때만) */
	void MoveTowardRemoteDestination(float DeltaSeconds);

	/** LoL-style soft collision: while moving and overlapping another character, sidestep around it (every character, local or remote). */
	void ApplySoftSeparation(float DeltaSeconds);

	FVector RemoteDestination = FVector::ZeroVector;
	bool bHasRemoteDestination = false;

	/** NavMesh route to RemoteDestination, computed when a destination arrives. Empty = walk straight (no NavMesh / no path). */
	TArray<FVector> RemotePathPoints;
	int32 RemotePathIndex = 0;
};
