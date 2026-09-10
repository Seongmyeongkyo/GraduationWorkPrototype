// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "NetworkProtocol.h"
#include "FWNetworkSubsystem.generated.h"

class FSocket;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFWOnLoginResult, bool, bSuccess, const FString&, Message);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFWOnAvatarInfo, int32, PlayerId, FVector, Location);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FFWOnPlayerAdded, int32, PlayerId, const FString&, Username, FVector, Location);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFWOnPlayerRemoved, int32, PlayerId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFWOnPlayerMoved, int32, PlayerId, FVector, Destination);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFWOnConnectionFailed, const FString&, Reason);

/**
 * Thin TCP relay client for the external Server process (Server/ in this repo).
 * Polls a non-blocking socket once per frame and turns each S2C packet
 * into a delegate broadcast. Carries no gameplay logic of its own -
 * callers (e.g. AFWPlayerController) decide what a login/move/etc. means.
 */
UCLASS()
class FRAMEWORK_API UFWNetworkSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Deinitialize() override;

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override;

	/** Opens a non-blocking connection. Actual completion is polled in Tick(). */
	UFUNCTION(BlueprintCallable, Category = "Network")
	bool ConnectToServer(const FString& ServerIP, int32 ServerPort);

	UFUNCTION(BlueprintCallable, Category = "Network")
	void Disconnect();

	UFUNCTION(BlueprintCallable, Category = "Network")
	bool IsConnected() const;

	/** Queues the login packet; sent as soon as the connection finishes if not connected yet. */
	UFUNCTION(BlueprintCallable, Category = "Network")
	void SendLogin(const FString& Username);

	UFUNCTION(BlueprintCallable, Category = "Network")
	void SendMove(const FVector& Destination);

	UPROPERTY(BlueprintAssignable, Category = "Network")
	FFWOnLoginResult OnLoginResult;

	UPROPERTY(BlueprintAssignable, Category = "Network")
	FFWOnAvatarInfo OnAvatarInfo;

	UPROPERTY(BlueprintAssignable, Category = "Network")
	FFWOnPlayerAdded OnPlayerAdded;

	UPROPERTY(BlueprintAssignable, Category = "Network")
	FFWOnPlayerRemoved OnPlayerRemoved;

	UPROPERTY(BlueprintAssignable, Category = "Network")
	FFWOnPlayerMoved OnPlayerMoved;

	/** Broadcast whenever a connection attempt fails, immediately or after the async handshake times out/errors. */
	UPROPERTY(BlueprintAssignable, Category = "Network")
	FFWOnConnectionFailed OnConnectionFailed;

private:
	void SendBytes(const void* Data, int32 NumBytes);
	void TrySendPendingLogin();
	void ProcessPacket(const uint8* Packet);

	FSocket* Socket = nullptr;
	uint8 RecvBuffer[FWNet::RECV_CAPACITY];
	int32 PendingBytes = 0;

	bool bHasPendingLogin = false;
	FString PendingLoginUsername;
};
