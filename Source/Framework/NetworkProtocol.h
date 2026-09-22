// Wire format shared with the external Server process (Server/Protocol.h).
// Field-for-field mirror - keep the two in sync by hand.
#pragma once
#include <cstdint>

namespace FWNet
{
	constexpr uint16_t PORT = 3500;
	constexpr int32_t MAX_NAME_LEN = 20;
	constexpr int32_t RECV_CAPACITY = 4096;

	enum PACKET_TYPE : uint8_t
	{
		C2S_LOGIN = 0, C2S_MOVE,
		S2C_LOGIN_RESULT, S2C_AVATAR_INFO, S2C_ADD_PLAYER, S2C_REMOVE_PLAYER, S2C_MOVE_PLAYER
	};

#pragma pack(push, 1)
	struct C2S_Login
	{
		uint8_t size;
		PACKET_TYPE type;
		char username[MAX_NAME_LEN];
	};

	// Destination the player clicked, in Unreal world-space units.
	struct C2S_Move
	{
		uint8_t size;
		PACKET_TYPE type;
		float x, y, z;
	};

	struct S2C_LoginResult
	{
		uint8_t size;
		PACKET_TYPE type;
		bool success;
		char message[50];
	};

	struct S2C_AvatarInfo
	{
		uint8_t size;
		PACKET_TYPE type;
		int32_t playerId;
		float x, y, z;
	};

	struct S2C_AddPlayer
	{
		uint8_t size;
		PACKET_TYPE type;
		int32_t playerId;
		char username[MAX_NAME_LEN];
		float x, y, z;
	};

	struct S2C_RemovePlayer
	{
		uint8_t size;
		PACKET_TYPE type;
		int32_t playerId;
	};

	struct S2C_MovePlayer
	{
		uint8_t size;
		PACKET_TYPE type;
		int32_t playerId;
		float x, y, z;
	};
#pragma pack(pop)
}
