// Copyright Epic Games, Inc. All Rights Reserved.

#include "FWPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputActionValue.h"
#include "GameFramework/Pawn.h"
#include "Engine/LocalPlayer.h"
#include "Engine/Engine.h"
#include "DrawDebugHelpers.h"
#include "FWCharacter.h"
#include "FWCameraRig.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "UObject/ConstructorHelpers.h"
#include "FWMiniMapWidget.h"
#include "FWStatusBarWidget.h"
#include "FWHealManaActorComponent.h"
#include "FWChatWidget.h"
#include "GameFramework/PlayerState.h"      // PlayerState->GetPlayerName()
#include "Engine/GameViewportClient.h"      // 게임 뷰포트 포커스 복구
#include "Widgets/SViewport.h"
#include "TimerManager.h"                   // SetTimerForNextTick
#include "InputCoreTypes.h"                 // EKeys
#include "FWNetworkSubsystem.h"

AFWPlayerController::AFWPlayerController()
{
	// 마우스 커서 온오프
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	// Keep the independent top-down rig as the view target after pawn possession/travel.
	bAutoManageActiveCameraTarget = false;

	/** MiniMap Widget Class 설정 **/
	static ConstructorHelpers::FClassFinder<UFWMiniMapWidget> MiniMapClassFinder(TEXT("/Game/UI/WBP_MiniMap"));
	if (MiniMapClassFinder.Succeeded()) {
		MiniMapWidgetClass = MiniMapClassFinder.Class;
	}
	else {
		UE_LOG(LogTemp, Error, TEXT("[FW] WBP_MiniMap 에셋을 못 찾음. 경로 확인: /Game/UI/WBP_MiniMap"));
	}

	/** StatusBar Widget Class 로드 **/
	static ConstructorHelpers::FClassFinder<UFWStatusBarWidget>
		StatusBarClassFinder(TEXT("/Game/UI/WBP_StatusBarWidget"));
	if (StatusBarClassFinder.Succeeded()) {
		StatusBarWidgetClass = StatusBarClassFinder.Class;
	}
	else {
		UE_LOG(LogTemp, Error, TEXT("[FW] WBP_StatusBarWidget 에셋을 못 찾음: /Game/UI/WBP_StatusBarWidget"));
	}

	/** Chat Widget Class 로드 **/
	static ConstructorHelpers::FClassFinder<UFWChatWidget>
		ChatClassFinder(TEXT("/Game/UI/WBP_Chat"));
	if (ChatClassFinder.Succeeded()) {
		ChatWidgetClass = ChatClassFinder.Class;
	}
	else {
		UE_LOG(LogTemp, Error, TEXT("[FW] WBP_Chat 에셋을 못 찾음: /Game/UI/WBP_Chat"));
	}

}

void AFWPlayerController::MoveToWorldLocation(const FVector& WorldLocation)
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys) {
		return;
	}

	// 클릭한 월드 지점을 navMesh 위의 가장 가까운 지점으로 스냅
	FNavLocation ProjectedLocation;
	if (NavSys->ProjectPointToNavigation(WorldLocation, ProjectedLocation, FVector(1000.f, 1000.f, 1000.f))) {
		SetMoveDestination(ProjectedLocation.Location);
	}

	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[FW] 미니맵 클릭 지점이 NavMesh 밖: %s"), *WorldLocation.ToString());
	}
}

void AFWPlayerController::BeginPlay()
{
	Super::BeginPlay();

	UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (Subsystem && DefaultMappingContext)
	{
		Subsystem->AddMappingContext(DefaultMappingContext, /*Priority=*/0);
	}

	// 카메라 리그 스폰 & ViewTarget 설정
	if (!CameraRigClass) {
		CameraRigClass = AFWCameraRig::StaticClass();
	}

	if (GetWorld() && CameraRigClass) {
		const FVector StartLocation = GetPawn() ? GetPawn()->GetActorLocation() : FVector::ZeroVector;
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		Params.Owner = this;

		CameraRig = GetWorld()->SpawnActor<AFWCameraRig>(CameraRigClass, StartLocation, FRotator::ZeroRotator, Params);

		if (CameraRig) {
			// 컨트롤러의 시점을 리그로 전환
			SetViewTargetWithBlend(CameraRig, /*BlendTime=*/0.f);
			UE_LOG(LogTemp, Log, TEXT("[FW] 카메라 리그 스폰 완료"));
		}
		else {
			UE_LOG(LogTemp, Error, TEXT("[FW] 카메라 리그 스폰 실패"));
		}
	}

	// 마우스를 뷰포트에 고정 (Edge Pan 시 마우스가 화면 밖으로 나가는 것을 방지)
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	InputMode.SetHideCursorDuringCapture(false);	// 마우스 커서 항상 표시
	SetInputMode(InputMode);
	bShowMouseCursor = true;	// 오류 방지용 마우스 커서 항상 표시

	// MiniMap Widget 생성 & Viewport에 추가
	if (MiniMapWidgetClass) {
		MiniMapWidgetInstance = CreateWidget<UFWMiniMapWidget>(this, MiniMapWidgetClass);
		if (MiniMapWidgetInstance) {
			MiniMapWidgetInstance->AddToViewport(100);
			UE_LOG(LogTemp, Log, TEXT("[FW] 미니맵 위젯 생성 완료"));
		}
	}

	else {
		UE_LOG(LogTemp, Error, TEXT("[FW-Diag] MiniMapWidgetClass 가 NULL"));
	}

	/** StatusBar Widget 생성(로컬 컨트롤러만 - 서버가 클라이언트 UI 만들지 않도록) **/
	if (StatusBarWidgetClass && IsLocalController())	// 로컬 컨트롤러만 UI 생성
	{
		StatusBarWidgetInstance = CreateWidget<UFWStatusBarWidget>(this, StatusBarWidgetClass);
		if (StatusBarWidgetInstance) {
			StatusBarWidgetInstance->AddToViewport(50);  // ZOrder: 미니맵(100)보다 아래
			UE_LOG(LogTemp, Log, TEXT("[FW] StatusBar 위젯 생성 완료"));
		}
	}

	/** Chat Widget 생성 (로컬 컨트롤러만) **/
	if (IsLocalController()) {
		if (ChatWidgetClass) {
			ChatWidgetInstance = CreateWidget<UFWChatWidget>(this, ChatWidgetClass);
			if (ChatWidgetInstance) {
				ChatWidgetInstance->AddToViewport(60);   // StatusBar(50) 위, MiniMap(100) 아래
				ChatWidgetInstance->OnMessageSubmitted.AddDynamic(this, &AFWPlayerController::HandleChatMessageSubmitted);
				ChatWidgetInstance->OnChatClosed.AddDynamic(this, &AFWPlayerController::HandleChatClosed);
				UE_LOG(LogTemp, Log, TEXT("[FW] 채팅 위젯 생성 완료"));
			}
			else {
				UE_LOG(LogTemp, Error, TEXT("[FW] 채팅 위젯 생성 실패"));
			}
		}
		else {
			UE_LOG(LogTemp, Error, TEXT("[FW] ChatWidgetClass 가 NULL — WBP_Chat 경로/부모 클래스 확인"));
		}
	}

	if (UFWNetworkSubsystem* Net = GetNetwork())
	{
		Net->OnLoginResult.AddDynamic(this, &AFWPlayerController::HandleLoginResult);
		Net->OnAvatarInfo.AddDynamic(this, &AFWPlayerController::HandleAvatarInfo);
		Net->OnPlayerAdded.AddDynamic(this, &AFWPlayerController::HandlePlayerAdded);
		Net->OnPlayerRemoved.AddDynamic(this, &AFWPlayerController::HandlePlayerRemoved);
		Net->OnPlayerMoved.AddDynamic(this, &AFWPlayerController::HandlePlayerMoved);
		Net->OnPlayerHit.AddDynamic(this, &AFWPlayerController::HandlePlayerHit);
		Net->OnPlayerSkill.AddDynamic(this, &AFWPlayerController::HandlePlayerSkill);
		Net->OnSkillFailed.AddDynamic(this, &AFWPlayerController::HandleSkillFailed);
		Net->OnManaUpdate.AddDynamic(this, &AFWPlayerController::HandleManaUpdate);
		Net->OnConnectionFailed.AddDynamic(this, &AFWPlayerController::HandleConnectionFailed);

		if (Net->ConnectToServer(ServerIP, ServerPort))
		{
			Net->SendLogin(PlayerUsername);
		}
	}
}

void AFWPlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);
	if (CameraRig && InPawn)
	{
		CameraRig->SnapTo(InPawn->GetActorLocation());
		SetViewTarget(CameraRig);
	}
}

UFWNetworkSubsystem* AFWPlayerController::GetNetwork() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UFWNetworkSubsystem>() : nullptr;
}

void AFWPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	// Play 시점에 /Game/Input 의 에셋을 새로 로드 (에디터에서 지정 안 했을 때만).
	// LoadObject 는 현재 저장된 에셋을 그대로 돌려주므로 매핑이 항상 최신이다.
	if (!DefaultMappingContext) { DefaultMappingContext = LoadObject<UInputMappingContext>(nullptr, TEXT("/Game/Input/IMC_Default.IMC_Default")); }
	if (!MoveAction)   { MoveAction   = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_Move.IA_Move")); }
	if (!ZoomAction)   { ZoomAction   = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_Zoom.IA_Zoom")); }
	if (!SkillQAction) { SkillQAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_SkillQ.IA_SkillQ")); }
	if (!SkillWAction) { SkillWAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_SkillW.IA_SkillW")); }
	if (!SkillEAction) { SkillEAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_SkillE.IA_SkillE")); }
	if (!SkillRAction) { SkillRAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_SkillR.IA_SkillR")); }
	
	// 카메라 리센터 액션 로드
	if (!RecenterCameraAction) {
		RecenterCameraAction = LoadObject<UInputAction>(
			nullptr, TEXT("/Game/Input/IA_RecenterCamera.IA_RecenterCamera")
		);
	}

	// 카메라 토글 액션 로드
	if (!CameraLockToggleAction) {
		CameraLockToggleAction = LoadObject<UInputAction>(
			nullptr, TEXT("/Game/Input/IA_CameraLockToggle.IA_CameraLockToggle"));
	}

	// 채팅 열기 액션 로드 (Enter)
	if (!OpenChatAction) {
		OpenChatAction = LoadObject<UInputAction>(nullptr, TEXT("/Game/Input/IA_OpenChat.IA_OpenChat"));
	}

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC)
	{
		UE_LOG(LogTemp, Error, TEXT("[FW] InputComponent 가 EnhancedInputComponent 가 아닙니다. "
			"Project Settings > Input 의 Default Input Component Class 를 확인하세요."));
		return;
	}

	// 에셋이 지정되지 않았으면(로드 실패 등) 바인딩을 건너뛰어 크래시를 방지한다.
	if (MoveAction)   { EIC->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AFWPlayerController::OnMoveTriggered); }  // 우클릭 이동
	if (ZoomAction)   { EIC->BindAction(ZoomAction, ETriggerEvent::Triggered, this, &AFWPlayerController::OnZoom); }           // 마우스 휠 줌
	if (SkillQAction) { EIC->BindAction(SkillQAction, ETriggerEvent::Started, this, &AFWPlayerController::OnSkillQ); }
	if (SkillWAction) { EIC->BindAction(SkillWAction, ETriggerEvent::Started, this, &AFWPlayerController::OnSkillW); }
	if (SkillEAction) { EIC->BindAction(SkillEAction, ETriggerEvent::Started, this, &AFWPlayerController::OnSkillE); }
	if (SkillRAction) { EIC->BindAction(SkillRAction, ETriggerEvent::Started, this, &AFWPlayerController::OnSkillR); }

	// 리센터 바인딩
	if (RecenterCameraAction) {
		EIC->BindAction(RecenterCameraAction, ETriggerEvent::Started, this, &AFWPlayerController::OnRecenterCamera);
		EIC->BindAction(RecenterCameraAction, ETriggerEvent::Completed, this, &AFWPlayerController::OnRecenterCameraReleased);
	}

	if (CameraLockToggleAction) {
		EIC->BindAction(CameraLockToggleAction, ETriggerEvent::Started, this, &AFWPlayerController::OnCameraLockToggle);
	}

	// 채팅창 바인딩
	if (OpenChatAction) {
		EIC->BindAction(OpenChatAction, ETriggerEvent::Started, this, &AFWPlayerController::OnOpenChat);
	}
}

void AFWPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	FlushPendingMoveSend();
	MoveTowardDestination();
	DrawMoveMarker();
	DrawMovePath();
	UpdateEdgePanCamera(DeltaTime);	// 카메라 갱신
}

// ----------------------------------------------------------------------------
// 이동
// ----------------------------------------------------------------------------
void AFWPlayerController::OnMoveTriggered(const FInputActionValue& /*Value*/)
{
	// 채팅 입력 중 이동 허용 여부
	if (IsChatInputActive() && !bAllowMouseMoveWhileChatting) {
		return;
	}

	UpdateDestinationFromCursor();
}

void AFWPlayerController::UpdateDestinationFromCursor()
{
	FHitResult Hit;
	if (!GetHitResultUnderCursor(MoveTraceChannel, /*bTraceComplex=*/false, Hit) || !Hit.bBlockingHit)
	{
		return;
	}

	FVector Destination = Hit.ImpactPoint;

	// 우클릭 위치를 NavMesh 위 가장 가까운 위치로 보정
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	if (NavSys) {
		FNavLocation ProjectedLocation;
		const FVector SearchExtent(500.f, 500.f, 500.f);	// 5m 반경 안에서 가장 가까운 navi 지점 검색

		if (NavSys->ProjectPointToNavigation(Destination, ProjectedLocation, SearchExtent)) {
			Destination = ProjectedLocation.Location;
		}

		else {
			return;	// NavMesh 위에 없으면 이동 취소
		}
	}
	SetMoveDestination(Destination);
}

void AFWPlayerController::SetMoveDestination(const FVector& Destination)
{
	CachedDestination = Destination;
	bHasMoveDestination = true;

	bMoveSendPending = true;
	FlushPendingMoveSend();
}

void AFWPlayerController::FlushPendingMoveSend()
{
	if (!bMoveSendPending)
	{
		return;
	}

	const double Now = GetWorld()->GetTimeSeconds();
	if (bHasSentMove)
	{
		if (Now - LastMoveSendTime < MoveSendInterval)
		{
			return; // 예약 유지 - 다음 PlayerTick에서 재시도
		}
		if (FVector::DistSquared(CachedDestination, LastSentDestination) < FMath::Square(MinMoveSendDistance))
		{
			bMoveSendPending = false; // 지난번에 보낸 곳과 사실상 같은 지점 - 보낼 필요 없음
			return;
		}
	}

	if (UFWNetworkSubsystem* Net = GetNetwork())
	{
		Net->SendMove(CachedDestination);
	}
	LastSentDestination = CachedDestination;
	LastMoveSendTime = Now;
	bHasSentMove = true;
	bMoveSendPending = false;
}

void AFWPlayerController::MoveTowardDestination()
{
	if (!bHasMoveDestination)
	{
		return;
	}

	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	// 평면 거리만 판단 (현재는)
	const FVector ToTarget = CachedDestination - ControlledPawn->GetActorLocation();

	if (FVector(ToTarget.X, ToTarget.Y, 0.f).SizeSquared() <= FMath::Square(DestinationAcceptanceRadius))
	{
		bHasMoveDestination = false; // 도착 → 정지
		return;
	}

	// NavMesh 경로탐색이 필요해지면(장애물 우회 등) 이 직접 이동을
	// UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, CachedDestination) 으로 교체


	// NavMesh 위에 있으면 경로 이동, 아니면 직선 이동 폴백 (안전장치)
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	ANavigationData* NavData = NavSys ? NavSys->GetDefaultNavDataInstance(FNavigationSystem::DontCreate) : nullptr;

	FNavLocation ProjectedLoc;
	const bool bCharOnNavMesh = NavData && NavSys->ProjectPointToNavigation(
		ControlledPawn->GetActorLocation(),
		ProjectedLoc,
		FVector(1000.f, 1000.f, 1000.f),
		NavData);

	if (bCharOnNavMesh)
	{
		UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, CachedDestination);
	}
	else
	{
		// NavMesh 밖으로 나갔거나 아직 준비 안 됨 - 직선 이동으로 계속
		ControlledPawn->AddMovementInput(ToTarget.GetSafeNormal2D(), 1.f);
	}
}

void AFWPlayerController::DrawMoveMarker()
{
	if (!bShowMoveMarker || !bHasMoveDestination)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 우클릭 위치 표시
	const FVector Center = CachedDestination + FVector(0.f, 0.f, 2.f); // z-fighting 방지
	DrawDebugCircle(World, Center, MarkerRadius, 32, MarkerColor, false, -1.f, 0, 3.f,
		FVector(1, 0, 0), FVector(0, 1, 0), false);
	DrawDebugLine(World, CachedDestination, CachedDestination + FVector(0.f, 0.f, 120.f),
		MarkerColor, false, -1.f, 0, 3.f);
}

// 엣지 팬 카메라 갱신
void AFWPlayerController::UpdateEdgePanCamera(float DeltaTime)
{
	if (!CameraRig) { return; }

	const bool bFollowPawn = bCameraLockedToPawn || bRecenterHeld;

	if (bFollowPawn) {
		CameraRig->SetPanDirection(FVector::ZeroVector);
		if (APawn* MyPawn = GetPawn()) {
			CameraRig->SnapTo(MyPawn->GetActorLocation());
		}
		return;   // 스페이스바 or 고정 시엔 엣지 팬 무시
	}

	// 뷰포트 크기
	int32 ViewX = 0, ViewY = 0;
	GetViewportSize(ViewX, ViewY);
	if (ViewX <= 0 || ViewY <= 0) {
		return;
	}

	// 마우스 위치 (좌상단 원점, 픽셀)
	float MouseX = 0.f, MouseY = 0.f;
	const bool bHasMouse = GetMousePosition(MouseX, MouseY);

	// 엣지 팬 여부 판단
	FVector2D PanInput = FVector2D::ZeroVector;
	if (bHasMouse) {
		if (MouseX <= EdgePanThreshold) {
			PanInput.X = -1.f;
		}
		else if (MouseX >= ViewX - EdgePanThreshold) {
			PanInput.X = 1.f;
		}

		if (MouseY <= EdgePanThreshold) {
			PanInput.Y = -1.f;
		}
		else if (MouseY >= ViewY - EdgePanThreshold) {
			PanInput.Y = 1.f;
		}
	}

	// EdgeMode
	// 방향만 전달, 실제 이동/감속은 CameraRig에서 관리
	FVector WorldDirection = FVector(-PanInput.Y, PanInput.X, 0.f);

	// 대각선 이동 시 속도 보정
	if (WorldDirection.SizeSquared() > 1.f) {
		WorldDirection.Normalize();
	}

	CameraRig->SetPanDirection(WorldDirection);
}

void AFWPlayerController::DrawMovePath()
{
	if (!bShowMovePath || !bHasMoveDestination) {
		return;
	}

	APawn* MyPawn = GetPawn();
	UWorld* World = GetWorld();
	if (!MyPawn || !World) {
		return;
	}

	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavSys) {
		return;
	}

	const FVector ZOffset(0.f, 0.f, PathZOffset);
	const FVector Start = MyPawn->GetActorLocation();
	const FVector End = CachedDestination;

	// NavMesh 경로 쿼리 (장애물 우회 경로)
	UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(this, Start, End, MyPawn);

	if (NavPath && NavPath->IsValid() && NavPath->PathPoints.Num() >= 2) {
		// 세그먼트별로 라인 그리기
		for (int32 i = 0; i < NavPath->PathPoints.Num() - 1; ++i) {
			const FVector From = NavPath->PathPoints[i] + ZOffset;
			const FVector To = NavPath->PathPoints[i + 1] + ZOffset;
			DrawDebugLine(World, From, To, PathColor, false, -1.f, 0, PathThickness);
		}
	}
	else {
		// NavMesh 경로 실패 시 직선 폴백
		DrawDebugLine(World, Start + ZOffset, End + ZOffset, PathColor, false, -1.f, 0, PathThickness);
	}
}

// ----------------------------------------------------------------------------
// 카메라 줌 ---> 변경점 (리그로 위임)
// ----------------------------------------------------------------------------
void AFWPlayerController::OnZoom(const FInputActionValue& Value)
{
	// 채팅이 열려 있는 동안 휠은 로그 스크롤 전용 (카메라 휠 차단)
	if (IsChatInputActive()) { return; }

	const float ScrollDelta = Value.Get<float>();
	if (FMath::IsNearlyZero(ScrollDelta))
	{
		return;
	}

	if (CameraRig)
	{
		CameraRig->AddZoom(ScrollDelta);
	}
}

// ----------------------------------------------------------------------------
// 스킬
// ----------------------------------------------------------------------------
void AFWPlayerController::OnSkillQ(const FInputActionValue& /*Value*/) { RequestSkill(0); }
void AFWPlayerController::OnSkillW(const FInputActionValue& /*Value*/) { RequestSkill(1); }
void AFWPlayerController::OnSkillE(const FInputActionValue& /*Value*/) { RequestSkill(2); }
void AFWPlayerController::OnSkillR(const FInputActionValue& /*Value*/) { RequestSkill(3); }

void AFWPlayerController::RequestSkill(int32 SkillIndex)
{
	if (IsChatInputActive()) { return; }   // ← 추가: 채팅 입력 중에는 스킬 차단

	// 캐릭터 유효성 체크
	APawn* MyPawn = GetPawn();
	if (!MyPawn)
	{
		return;
	}

	UFWNetworkSubsystem* Net = GetNetwork();
	if (!Net || !Net->IsServerAuthoritative())
	{
		// Offline fallback: the server's mana rule, resolved locally and immediately.
		AFWCharacter* MyChar = Cast<AFWCharacter>(MyPawn);
		if (!MyChar || !MyChar->AttributeComp)
		{
			return;
		}
		if (MyChar->AttributeComp->TryConsumeManaOffline(FWNet::SKILL_MANA_COST))
		{
			ActivateSkill(SkillIndex);
		}
		else
		{
			HandleSkillFailed(SkillIndex); // same reaction as a server rejection
		}
		return;
	}

	// Aim toward the cursor; fall back to the pawn's facing if there's nothing under it.
	FVector Dir = MyPawn->GetActorForwardVector();
	FHitResult Hit;
	if (GetHitResultUnderCursor(MoveTraceChannel, /*bTraceComplex=*/false, Hit) && Hit.bBlockingHit)
	{
		const FVector ToCursor = (Hit.ImpactPoint - MyPawn->GetActorLocation()).GetSafeNormal2D();
		if (!ToCursor.IsNearlyZero())
		{
			Dir = ToCursor;
		}
	}

	Net->SendSkill(SkillIndex, Dir.X, Dir.Y);
}

void AFWPlayerController::OnRecenterCamera(const FInputActionValue& Value)
{
	if (IsChatInputActive()) { return; }   // 채팅 중 Space = 띄어쓰기
	
	UE_LOG(LogTemp, Log, TEXT("[FW] 리센터 스페이스바 눌림"));
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::White,
			FString::Printf(TEXT(">> 스페이스바 눌림")));
	}

	bRecenterHeld = true;    // 홀드 상태 true
	if (CameraRig) {
		if (APawn* MyPawn = GetPawn()) {
			CameraRig->SnapTo(MyPawn->GetActorLocation());
		}
	}
}

void AFWPlayerController::OnRecenterCameraReleased(const FInputActionValue& Value)
{
	UE_LOG(LogTemp, Log, TEXT("[FW] 리센터 스페이스바 뗌"));
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::White,
			FString::Printf(TEXT(">> 스페이스바 뗌")));
	}

	bRecenterHeld = false;
}

void AFWPlayerController::OnCameraLockToggle(const FInputActionValue& Value)
{
	if (IsChatInputActive()) { return; }   // 채팅 중 Y = 글자

	bCameraLockedToPawn = !bCameraLockedToPawn;

	UE_LOG(LogTemp, Log, TEXT("[FW] 카메라 고정 토글: %s"),
		bCameraLockedToPawn ? TEXT("ON") : TEXT("OFF"));

	if (GEngine) {
		GEngine->AddOnScreenDebugMessage(-1, 4.0f, FColor::White,
			FString::Printf(TEXT(">> 카메라 토글 %s"),
				bCameraLockedToPawn ? TEXT("ON") : TEXT("OFF")));
	}

	if (bCameraLockedToPawn && CameraRig) {
		if (APawn* MyPawn = GetPawn()) {
			CameraRig->SnapTo(MyPawn->GetActorLocation());
		}
	}

}

void AFWPlayerController::ActivateSkill(int32 SkillIndex)
{
	static const TCHAR* SkillNames[] = { TEXT("Q"), TEXT("W"), TEXT("E"), TEXT("R") };
	static const FColor SkillColors[] = { FColor::Red, FColor::Green, FColor::Blue, FColor::Yellow };

	const bool bValid = (SkillIndex >= 0 && SkillIndex < 4);
	const TCHAR* Name = bValid ? SkillNames[SkillIndex] : TEXT("?");
	const FColor Color = bValid ? SkillColors[SkillIndex] : FColor::White;

	UE_LOG(LogTemp, Log, TEXT("[FW] 스킬 발동: %s (index=%d)"), Name, SkillIndex);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 4.0f, Color,
			FString::Printf(TEXT(">> [%s] 스킬"), Name));
	}
}

// ----------------------------------------------------------------------------
// 채팅
// ----------------------------------------------------------------------------
void AFWPlayerController::OnOpenChat(const FInputActionValue& /*Value*/)
{
	if (!ChatWidgetInstance) {
		return;
	}

	// 이미 열려 있는데 Enter가 게임까지 왔다
	// 입력창이 잠깐 포커스를 잃은 상태(마우스를 누르고 있는 중 등) → 지금 내용 전송
	if (ChatWidgetInstance->IsChatOpen()) {
		ChatWidgetInstance->SubmitInput();
		return;
	}

	// Shift+Enter = 전체, Enter = 팀
	const bool bShiftDown = IsInputKeyDown(EKeys::LeftShift) || IsInputKeyDown(EKeys::RightShift);
	const EFWChatChannel OpenChannel = bShiftDown ? EFWChatChannel::All : EFWChatChannel::Team;

	ApplyGameAndUIInputMode(false);   // 마우스는 계속 게임이 받음
	ChatWidgetInstance->OpenChat(OpenChannel);
}

void AFWPlayerController::HandleChatMessageSubmitted(EFWChatChannel Channel, const FString& MessageText)
{
	// PlayerController가 구독중이므로 로컬 loopback 블록 사용 (서버 연결 시 서버에서 수정 필요)
	FString MyName = TEXT("Me");
	if (PlayerState && !PlayerState->GetPlayerName().IsEmpty()) {
		MyName = PlayerState->GetPlayerName();
	}

	if (ChatWidgetInstance) {
		ChatWidgetInstance->AddIncomingMessage(Channel, MyName, MessageText);
	}

	// HandleChatClosed가 전송·취소 공통으로 처리
}

void AFWPlayerController::HandleChatClosed()
{
	ApplyGameAndUIInputMode(false);

	// 입력창의 "Clear Keyboard Focus on Commit"이 이 콜백이 끝난 뒤에 실행됨
	// 입력창이 닫힐 때 포커스를 주면 바로 지워지므로 다음 틱에 게임 뷰포트로 복구
	GetWorldTimerManager().SetTimerForNextTick(this, &AFWPlayerController::RestoreGameViewportFocus);
}

bool AFWPlayerController::IsChatInputActive() const
{
	return ChatWidgetInstance && ChatWidgetInstance->IsChatOpen();
}

void AFWPlayerController::ApplyGameAndUIInputMode(bool bFocusGameViewport)
{
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::LockAlways);
	InputMode.SetHideCursorDuringCapture(false);

	if (bFocusGameViewport) {
		UGameViewportClient* GVC = GetWorld() ? GetWorld()->GetGameViewport() : nullptr;
		if (GVC) {
			TSharedPtr<SViewport> ViewportWidget = GVC->GetGameViewportWidget();
			if (ViewportWidget.IsValid()) {
				InputMode.SetWidgetToFocus(ViewportWidget);
			}
		}
	}

	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void AFWPlayerController::RestoreGameViewportFocus()
{
	// 그 사이 채팅이 다시 열렸으면 건드리지 않음
	if (IsChatInputActive()) {
		return;
	}

	ApplyGameAndUIInputMode(true);
}

// ----------------------------------------------------------------------------
// 네트워크 (Server 이벤트 처리)
// ----------------------------------------------------------------------------
void AFWPlayerController::HandleLoginResult(bool bSuccess, const FString& Message)
{
	UE_LOG(LogTemp, Log, TEXT("[FWNet] Login %s: %s"), bSuccess ? TEXT("success") : TEXT("failed"), *Message);
}

void AFWPlayerController::HandleConnectionFailed(const FString& Reason)
{
	UE_LOG(LogTemp, Warning, TEXT("[FWNet] Connection failed: %s"), *Reason);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 8.0f, FColor::Red,
			FString::Printf(TEXT("서버에 연결할 수 없습니다 (%s). Server.exe가 실행 중인지 확인하세요."), *Reason));
	}
}

void AFWPlayerController::HandleAvatarInfo(int32 PlayerId, FVector Location)
{
	// 서버가 로그인 직후 나에게만 보내는 패킷. 다른 플레이어의 add/move 이벤트와
	// 내 것을 구분하기 위해 ID만 기억해 둔다 (다시 스폰하지 않음).
	LocalPlayerId = PlayerId;
}

void AFWPlayerController::HandlePlayerAdded(int32 PlayerId, const FString& Username, FVector Location)
{
	if (PlayerId == LocalPlayerId)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AFWCharacter* Avatar = World->SpawnActor<AFWCharacter>(AFWCharacter::StaticClass(), Location, FRotator::ZeroRotator, Params);
	if (Avatar)
	{
		Avatar->bIsRemoteAvatar = true;
		Avatar->NetPlayerId = PlayerId;
		RemoteAvatars.Add(PlayerId, Avatar);
	}
}

void AFWPlayerController::HandlePlayerRemoved(int32 PlayerId)
{
	if (TWeakObjectPtr<AFWCharacter>* Found = RemoteAvatars.Find(PlayerId))
	{
		if (Found->IsValid())
		{
			(*Found)->Destroy();
		}
		RemoteAvatars.Remove(PlayerId);
	}
}

void AFWPlayerController::HandlePlayerMoved(int32 PlayerId, FVector Destination)
{
	if (PlayerId == LocalPlayerId)
	{
		return; // relay echoes our own move back to us; we're already moving locally.
	}

	if (TWeakObjectPtr<AFWCharacter>* Found = RemoteAvatars.Find(PlayerId))
	{
		if (Found->IsValid())
		{
			(*Found)->SetRemoteDestination(Destination);
		}
	}
}

void AFWPlayerController::HandlePlayerHit(int32 /*AttackerId*/, int32 TargetId, int32 Damage)
{
	// No enemy health bars yet - only react when we're the one who got hit.
	if (TargetId != LocalPlayerId)
	{
		return;
	}

	if (AFWCharacter* MyChar = Cast<AFWCharacter>(GetPawn()))
	{
		if (MyChar->AttributeComp)
		{
			MyChar->AttributeComp->ApplyDamage(static_cast<float>(Damage));
		}
	}
}

void AFWPlayerController::HandlePlayerSkill(int32 PlayerId, int32 SkillIndex, float /*DirX*/, float /*DirY*/)
{
	// The server already spent the mana. No skill visuals for other players yet - only fire our own.
	if (PlayerId == LocalPlayerId)
	{
		ActivateSkill(SkillIndex);
	}
}

void AFWPlayerController::HandleSkillFailed(int32 /*SkillIndex*/)
{
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f, FColor::Cyan, TEXT("마나가 부족합니다"));
	}
}

void AFWPlayerController::HandleManaUpdate(float CurrentMana, float MaxMana)
{
	if (AFWCharacter* MyChar = Cast<AFWCharacter>(GetPawn()))
	{
		if (MyChar->AttributeComp)
		{
			MyChar->AttributeComp->ApplyServerMana(CurrentMana, MaxMana);
		}
	}
}
