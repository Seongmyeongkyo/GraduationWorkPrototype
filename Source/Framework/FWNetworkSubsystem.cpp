// Copyright Epic Games, Inc. All Rights Reserved.

#include "FWNetworkSubsystem.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "IPAddress.h"

using namespace FWNet;

void UFWNetworkSubsystem::Deinitialize()
{
	Disconnect();
	Super::Deinitialize();
}

bool UFWNetworkSubsystem::IsTickable() const
{
	return Socket != nullptr;
}

TStatId UFWNetworkSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UFWNetworkSubsystem, STATGROUP_Tickables);
}

bool UFWNetworkSubsystem::ConnectToServer(const FString& ServerIP, int32 ServerPort)
{
	Disconnect();

	ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
	if (!SocketSubsystem)
	{
		OnConnectionFailed.Broadcast(TEXT("Socket subsystem unavailable"));
		return false;
	}

	Socket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("FWNetworkClient"), false);
	if (!Socket)
	{
		OnConnectionFailed.Broadcast(TEXT("Failed to create socket"));
		return false;
	}
	Socket->SetNonBlocking(true);

	bool bIsValid = false;
	TSharedRef<FInternetAddr> Addr = SocketSubsystem->CreateInternetAddr();
	Addr->SetIp(*ServerIP, bIsValid);
	Addr->SetPort(ServerPort);
	if (!bIsValid)
	{
		UE_LOG(LogTemp, Error, TEXT("[FWNet] Invalid server IP: %s"), *ServerIP);
		Disconnect();
		OnConnectionFailed.Broadcast(FString::Printf(TEXT("Invalid server IP: %s"), *ServerIP));
		return false;
	}

	// Non-blocking connect: the handshake finishes asynchronously and is polled in Tick().
	Socket->Connect(*Addr);
	PendingBytes = 0;
	return true;
}

void UFWNetworkSubsystem::Disconnect()
{
	if (Socket)
	{
		Socket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
		Socket = nullptr;
	}
	PendingBytes = 0;
	bHasPendingLogin = false;
}

bool UFWNetworkSubsystem::IsConnected() const
{
	return Socket && Socket->GetConnectionState() == SCS_Connected;
}

void UFWNetworkSubsystem::SendBytes(const void* Data, int32 NumBytes)
{
	if (!IsConnected())
	{
		return;
	}
	int32 BytesSent = 0;
	Socket->Send(static_cast<const uint8*>(Data), NumBytes, BytesSent);
}

void UFWNetworkSubsystem::SendLogin(const FString& Username)
{
	PendingLoginUsername = Username;
	bHasPendingLogin = true;
	TrySendPendingLogin();
}

void UFWNetworkSubsystem::TrySendPendingLogin()
{
	if (!bHasPendingLogin || !IsConnected())
	{
		return;
	}

	C2S_Login Packet{};
	Packet.size = sizeof(C2S_Login);
	Packet.type = C2S_LOGIN;
	FTCHARToUTF8 Utf8Username(*PendingLoginUsername);
	const int32 CopyLen = FMath::Min(Utf8Username.Length(), MAX_NAME_LEN - 1);
	FMemory::Memcpy(Packet.username, Utf8Username.Get(), CopyLen);

	SendBytes(&Packet, Packet.size);
	bHasPendingLogin = false;
}

void UFWNetworkSubsystem::SendMove(const FVector& Destination)
{
	C2S_Move Packet{};
	Packet.size = sizeof(C2S_Move);
	Packet.type = C2S_MOVE;
	Packet.x = Destination.X;
	Packet.y = Destination.Y;
	Packet.z = Destination.Z;
	SendBytes(&Packet, Packet.size);
}

void UFWNetworkSubsystem::SendAttack(float DirX, float DirY)
{
	C2S_Attack Packet{};
	Packet.size = sizeof(C2S_Attack);
	Packet.type = C2S_ATTACK;
	Packet.dirX = DirX;
	Packet.dirY = DirY;
	SendBytes(&Packet, Packet.size);
}

void UFWNetworkSubsystem::SendSkill(int32 SkillIndex, float DirX, float DirY)
{
	C2S_Skill Packet{};
	Packet.size = sizeof(C2S_Skill);
	Packet.type = C2S_SKILL;
	Packet.skillIndex = static_cast<uint8>(SkillIndex);
	Packet.dirX = DirX;
	Packet.dirY = DirY;
	SendBytes(&Packet, Packet.size);
}

void UFWNetworkSubsystem::SendHit(int32 TargetPlayerId, int32 Damage)
{
	C2S_Hit Packet{};
	Packet.size = sizeof(C2S_Hit);
	Packet.type = C2S_HIT;
	Packet.targetPlayerId = TargetPlayerId;
	Packet.damage = Damage;
	SendBytes(&Packet, Packet.size);
}

void UFWNetworkSubsystem::SendGetExp(int32 Amount)
{
	C2S_GetExp Packet{};
	Packet.size = sizeof(C2S_GetExp);
	Packet.type = C2S_GET_EXP;
	Packet.amount = Amount;
	SendBytes(&Packet, Packet.size);
}

void UFWNetworkSubsystem::SendGetItem(int32 ItemId)
{
	C2S_GetItem Packet{};
	Packet.size = sizeof(C2S_GetItem);
	Packet.type = C2S_GET_ITEM;
	Packet.itemId = ItemId;
	SendBytes(&Packet, Packet.size);
}

void UFWNetworkSubsystem::SendUpdateHealth(float CurrentHealth)
{
	C2S_UpdateHealth Packet{};
	Packet.size = sizeof(C2S_UpdateHealth);
	Packet.type = C2S_UPDATE_HEALTH;
	Packet.currentHealth = CurrentHealth;
	SendBytes(&Packet, Packet.size);
}

void UFWNetworkSubsystem::SendUpdateMana(float CurrentMana)
{
	C2S_UpdateMana Packet{};
	Packet.size = sizeof(C2S_UpdateMana);
	Packet.type = C2S_UPDATE_MANA;
	Packet.currentMana = CurrentMana;
	SendBytes(&Packet, Packet.size);
}

void UFWNetworkSubsystem::Tick(float DeltaTime)
{
	if (!Socket)
	{
		return;
	}

	if (Socket->GetConnectionState() == SCS_ConnectionError)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FWNet] Connection to server failed."));
		Disconnect();
		OnConnectionFailed.Broadcast(TEXT("Could not reach the server"));
		return;
	}

	if (!IsConnected())
	{
		return; // handshake still in flight
	}

	TrySendPendingLogin();

	uint32 PendingDataSize = 0;
	while (Socket->HasPendingData(PendingDataSize))
	{
		const int32 SpaceLeft = RECV_CAPACITY - PendingBytes;
		if (SpaceLeft <= 0)
		{
			UE_LOG(LogTemp, Error, TEXT("[FWNet] Recv buffer overflow, disconnecting."));
			Disconnect();
			return;
		}

		int32 BytesRead = 0;
		const bool bOk = Socket->Recv(RecvBuffer + PendingBytes, SpaceLeft, BytesRead, ESocketReceiveFlags::None);
		if (!bOk)
		{
			break;
		}
		if (BytesRead == 0)
		{
			UE_LOG(LogTemp, Log, TEXT("[FWNet] Server closed the connection."));
			Disconnect();
			return;
		}
		PendingBytes += BytesRead;
	}

	int32 Offset = 0;
	while (PendingBytes - Offset > 0)
	{
		const int32 Available = PendingBytes - Offset;
		const uint8 PacketSize = RecvBuffer[Offset];
		if (PacketSize == 0 || PacketSize > Available)
		{
			break;
		}
		ProcessPacket(RecvBuffer + Offset);
		Offset += PacketSize;
	}
	if (Offset > 0 && Offset < PendingBytes)
	{
		FMemory::Memmove(RecvBuffer, RecvBuffer + Offset, PendingBytes - Offset);
	}
	PendingBytes -= Offset;
}

void UFWNetworkSubsystem::ProcessPacket(const uint8* Packet)
{
	const PACKET_TYPE Type = static_cast<PACKET_TYPE>(Packet[1]);
	switch (Type)
	{
	case S2C_LOGIN_RESULT:
	{
		const auto* P = reinterpret_cast<const S2C_LoginResult*>(Packet);
		OnLoginResult.Broadcast(P->success, FString(UTF8_TO_TCHAR(P->message)));
		break;
	}
	case S2C_AVATAR_INFO:
	{
		const auto* P = reinterpret_cast<const S2C_AvatarInfo*>(Packet);
		OnAvatarInfo.Broadcast(P->playerId, FVector(P->x, P->y, P->z));
		break;
	}
	case S2C_ADD_PLAYER:
	{
		const auto* P = reinterpret_cast<const S2C_AddPlayer*>(Packet);
		OnPlayerAdded.Broadcast(P->playerId, FString(UTF8_TO_TCHAR(P->username)), FVector(P->x, P->y, P->z));
		break;
	}
	case S2C_REMOVE_PLAYER:
	{
		const auto* P = reinterpret_cast<const S2C_RemovePlayer*>(Packet);
		OnPlayerRemoved.Broadcast(P->playerId);
		break;
	}
	case S2C_MOVE_PLAYER:
	{
		const auto* P = reinterpret_cast<const S2C_MovePlayer*>(Packet);
		OnPlayerMoved.Broadcast(P->playerId, FVector(P->x, P->y, P->z));
		break;
	}
	case S2C_PLAYER_ATTACK:
	{
		const auto* P = reinterpret_cast<const S2C_PlayerAttack*>(Packet);
		OnPlayerAttack.Broadcast(P->playerId, P->dirX, P->dirY);
		break;
	}
	case S2C_PLAYER_SKILL:
	{
		const auto* P = reinterpret_cast<const S2C_PlayerSkill*>(Packet);
		OnPlayerSkill.Broadcast(P->playerId, P->skillIndex, P->dirX, P->dirY);
		break;
	}
	case S2C_PLAYER_HIT:
	{
		const auto* P = reinterpret_cast<const S2C_PlayerHit*>(Packet);
		OnPlayerHit.Broadcast(P->attackerId, P->targetId, P->damage);
		break;
	}
	case S2C_EXP_RESULT:
	{
		const auto* P = reinterpret_cast<const S2C_ExpResult*>(Packet);
		OnExpResult.Broadcast(P->amount);
		break;
	}
	case S2C_ITEM_RESULT:
	{
		const auto* P = reinterpret_cast<const S2C_ItemResult*>(Packet);
		OnItemResult.Broadcast(P->itemId);
		break;
	}
	case S2C_HEALTH_RESULT:
	{
		const auto* P = reinterpret_cast<const S2C_HealthResult*>(Packet);
		OnHealthResult.Broadcast(P->currentHealth);
		break;
	}
	case S2C_MANA_RESULT:
	{
		const auto* P = reinterpret_cast<const S2C_ManaResult*>(Packet);
		OnManaResult.Broadcast(P->currentMana);
		break;
	}
	default:
		break;
	}
}
