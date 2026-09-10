// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FWPlayerController.generated.h"

class AFWCameraRig;
class UInputMappingContext;
class UInputAction;
struct FInputActionValue;
class UFWNetworkSubsystem;
class AFWCharacter;

UCLASS()
class FRAMEWORK_API AFWPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFWPlayerController();

	/** MiniMap 클릭 등 외부에서 특정 월드 지점으로 이동 요청 **/
	UFUNCTION(BlueprintCallable, Category = "Movement")
	void MoveToWorldLocation(const FVector& WorldLocation);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TSubclassOf<class UFWMiniMapWidget> MiniMapWidgetClass;
	
private:
	UPROPERTY()
	TObjectPtr<class UFWMiniMapWidget> MiniMapWidgetInstance;

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	// ---- Enhanced Input ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputMappingContext> DefaultMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> MoveAction;      // 우클릭 이동

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SkillQAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SkillWAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SkillEAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> SkillRAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> ZoomAction;      // 마우스 휠

	// ---- Camera Rig ----
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> RecenterCameraAction; // 스페이스바

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	TObjectPtr<UInputAction> CameraLockToggleAction; // 카메라 고정 토글 Y

	// ---- 이동 파라미터 ----
	/** 목적지에 이 거리 안으로 들어오면 도착으로 간주하고 멈춤(작을수록 정확) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	float DestinationAcceptanceRadius = 10.f;

	/** 커서 지점 판별에 쓸 트레이스 채널 (기본: Visibility) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Movement")
	
	// Editor에서 CollisionChannel Default를 Visibility로 설정하면, Editor에서 지정한 CollisionProfile에 따라 Trace가 동작한다.
	// Floor처리 할 때는 Visibility로 설정하고, Floor에 CollisionProfile을 Block으로 설정
	TEnumAsByte<ECollisionChannel> MoveTraceChannel = ECC_GameTraceChannel1;

	// ---- 클릭 위치 마커 ----
	/** 이동 목표 지점에 마커를 그릴지 여부 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Marker")
	bool bShowMoveMarker = true;

	/** 마커 링 반지름 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Marker")
	float MarkerRadius = 40.f;

	/** 마커 색상 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Marker")
	FColor MarkerColor = FColor::Green;

	/** 이동 경로 표시 **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Marker")
	bool bShowMovePath = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Marker")
	FColor PathColor = FColor::Yellow;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Marker")
	float PathThickness = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Marker")
	float PathZOffset = 5.f;	// 마커가 땅에 묻히지 않도록 Z오프셋 올리기

	// 카메라 리그 & 엣지 팬 튜닝 //
	/** 스폰할 카메라 리그 클래스 (미지정 시 AFWCameraRig 기본 사용) **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	TSubclassOf<AFWCameraRig> CameraRigClass;

	/** 스크린 엣지에서 팬 시작 **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	float EdgePanThreshold = 20.f;

	/** true = 리그가 캐릭터를 매 프레임 따라감 false = 자유 팬(엣지 이동) **/
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Camera")
	bool bCameraLockedToPawn = false;

	// ---- 입력 콜백 ----
	void OnMoveTriggered(const FInputActionValue& Value);
	void OnZoom(const FInputActionValue& Value);
	void OnSkillQ(const FInputActionValue& Value);
	void OnSkillW(const FInputActionValue& Value);
	void OnSkillE(const FInputActionValue& Value);
	void OnSkillR(const FInputActionValue& Value);
	void OnRecenterCamera(const FInputActionValue& Value);	// 리센터 스페이스바 (기존 Started)
	void OnRecenterCameraReleased(const FInputActionValue& Value); // 리센터 스페이스바 (holded)
	void OnCameraLockToggle(const FInputActionValue& Value);    // 카메라 고정 토글

	virtual void ActivateSkill(int32 SkillIndex);

	// ---- 네트워크 ----
	UPROPERTY(EditAnywhere, Category = "Network")
	FString ServerIP = TEXT("127.0.0.1");

	UPROPERTY(EditAnywhere, Category = "Network")
	int32 ServerPort = 3500;

	UPROPERTY(EditAnywhere, Category = "Network")
	FString PlayerUsername = TEXT("Player");

	UFUNCTION()
	void HandleLoginResult(bool bSuccess, const FString& Message);

	UFUNCTION()
	void HandleAvatarInfo(int32 PlayerId, FVector Location);

	UFUNCTION()
	void HandlePlayerAdded(int32 PlayerId, const FString& Username, FVector Location);

	UFUNCTION()
	void HandlePlayerRemoved(int32 PlayerId);

	UFUNCTION()
	void HandlePlayerMoved(int32 PlayerId, FVector Destination);

	UFUNCTION()
	void HandleConnectionFailed(const FString& Reason);

private:
	/** 커서 아래 지점으로 이동 목표를 갱신 */
	void UpdateDestinationFromCursor();

	/** 이동 목표 지점을 설정하고(커서 우클릭, 미니맵 클릭 공용) 서버로 좌표를 전송 */
	void SetMoveDestination(const FVector& Destination);

	UFWNetworkSubsystem* GetNetwork() const;

	/** 현재 목표를 향해 캐릭터를 이동시킴 (PlayerTick 에서 호출) */
	void MoveTowardDestination();

	/** 이동 목표 지점에 마커를 그림 (PlayerTick 에서 호출) */
	void DrawMoveMarker();

	/** 엣지 팬 카메라 업데이트 **/
	void UpdateEdgePanCamera(float DeltaTime);

	FVector CachedDestination = FVector::ZeroVector;
	bool bHasMoveDestination = false;

	// 리센터 홀드 중 체크
	bool bRecenterHeld = false;

	void DrawMovePath();

	/** 컨트롤러가 소유하는 카메라 리그 **/
	UPROPERTY()
	TObjectPtr<AFWCameraRig> CameraRig;

	/** 서버가 부여한 내 플레이어 ID. 다른 플레이어의 add/move 이벤트와 구분하는 데 사용. */
	int32 LocalPlayerId = INDEX_NONE;

	/** 다른 플레이어를 나타내는, 이 클라이언트가 직접 스폰한 아바타들. */
	TMap<int32, TWeakObjectPtr<AFWCharacter>> RemoteAvatars;
};
