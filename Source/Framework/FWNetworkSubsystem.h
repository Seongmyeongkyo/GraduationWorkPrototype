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
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FFWOnPlayerAttack, int32, PlayerId, float, DirX, float, DirY);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FFWOnPlayerSkill, int32, PlayerId, int32, SkillIndex, float, DirX, float, DirY);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FFWOnPlayerHit, int32, AttackerId, int32, TargetId, int32, Damage);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFWOnExpResult, int32, Amount);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFWOnItemResult, int32, ItemId);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FFWOnManaUpdate, float, CurrentMana, float, MaxMana);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FFWOnSkillFailed, int32, SkillIndex);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FFWOnPlayerVisibility, int32, PlayerId, bool, bVisible, FVector, Location);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FFWOnChatReceived, int32, SenderId, int32, Channel, const FString&, SenderName, const FString&, Message, bool, bIsTeammate);
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

	/**
	 * True while the server owns server-managed values (mana, ...). When false - server not running,
	 * still connecting, or connection lost - gameplay code must run the client-side fallback using
	 * the same rules (FWNet constants in NetworkProtocol.h). Every server-managed value branches on this.
	 */
	UFUNCTION(BlueprintCallable, Category = "Network")
	bool IsServerAuthoritative() const;

	/** Queues the login packet; sent as soon as the connection finishes if not connected yet. */
	UFUNCTION(BlueprintCallable, Category = "Network")
	void SendLogin(const FString& Username);

	UFUNCTION(BlueprintCallable, Category = "Network")
	void SendMove(const FVector& Destination);

	// Combat/progression relays - rough placeholders, see NetworkProtocol.h.
	UFUNCTION(BlueprintCallable, Category = "Network")
	void SendAttack(float DirX, float DirY);

	UFUNCTION(BlueprintCallable, Category = "Network")
	void SendSkill(int32 SkillIndex, float DirX, float DirY);

	UFUNCTION(BlueprintCallable, Category = "Network")
	void SendHit(int32 TargetPlayerId, int32 Damage);

	UFUNCTION(BlueprintCallable, Category = "Network")
	void SendGetExp(int32 Amount);

	UFUNCTION(BlueprintCallable, Category = "Network")
	void SendGetItem(int32 ItemId);

	// Stat sync - rough placeholders, see NetworkProtocol.h.
	UFUNCTION(BlueprintCallable, Category = "Network")
	void SendUpdateHealth(float CurrentHealth);

	/** Channel is an FWNet::CHAT_* value. Text longer than CHAT_MAX_BYTES (UTF-8) is cut at a character boundary. */
	UFUNCTION(BlueprintCallable, Category = "Network")
	void SendChat(int32 Channel, const FString& Message);

	/** 0 = not in a bush. */
	UFUNCTION(BlueprintCallable, Category = "Network")
	void SendBushState(int32 BushId);

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

	UPROPERTY(BlueprintAssignable, Category = "Network")
	FFWOnPlayerAttack OnPlayerAttack;

	UPROPERTY(BlueprintAssignable, Category = "Network")
	FFWOnPlayerSkill OnPlayerSkill;

	UPROPERTY(BlueprintAssignable, Category = "Network")
	FFWOnPlayerHit OnPlayerHit;

	/** Personal - only fires for the player who gained the exp/item, never broadcast. */
	UPROPERTY(BlueprintAssignable, Category = "Network")
	FFWOnExpResult OnExpResult;

	UPROPERTY(BlueprintAssignable, Category = "Network")
	FFWOnItemResult OnItemResult;

	/** Mana is owned by the server; these only ever fire for the local player. */
	UPROPERTY(BlueprintAssignable, Category = "Network")
	FFWOnManaUpdate OnManaUpdate;

	UPROPERTY(BlueprintAssignable, Category = "Network")
	FFWOnSkillFailed OnSkillFailed;

	/** Every chat line from the server, including our own messages echoed back and system notices (SenderId = -1). */
	UPROPERTY(BlueprintAssignable, Category = "Network")
	FFWOnChatReceived OnChatReceived;

	/** Another player became hidden (entered a bush we can't see into) or visible again. */
	UPROPERTY(BlueprintAssignable, Category = "Network")
	FFWOnPlayerVisibility OnPlayerVisibility;

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
