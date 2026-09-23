// Wire format shared with the external Server process (Server/Protocol.h).
// Field-for-field mirror - keep the two in sync by hand.
#pragma once
#include <cstdint>

namespace FWNet
{
	constexpr uint16_t PORT = 3500;
	constexpr int32_t MAX_NAME_LEN = 20;
	constexpr int32_t RECV_CAPACITY = 4096;
	constexpr int32_t SKILL_COUNT = 3;

	enum PACKET_TYPE : uint8_t
	{
		C2S_LOGIN = 0, C2S_MOVE,
		S2C_LOGIN_RESULT, S2C_AVATAR_INFO, S2C_ADD_PLAYER, S2C_REMOVE_PLAYER, S2C_MOVE_PLAYER,
		C2S_ATTACK, C2S_SKILL, C2S_HIT, C2S_GET_EXP, C2S_GET_ITEM,
		S2C_PLAYER_ATTACK, S2C_PLAYER_SKILL, S2C_PLAYER_HIT, S2C_EXP_RESULT, S2C_ITEM_RESULT,
		C2S_UPDATE_HEALTH, C2S_UPDATE_MANA,
		S2C_HEALTH_RESULT, S2C_MANA_RESULT
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

	// ---- Combat / progression (rough placeholders, mirrors Server/Protocol.h - subject to change) ----

	// No target/hitbox info - server just relays so other clients can play the animation/VFX.
	struct C2S_Attack
	{
		uint8_t size;
		PACKET_TYPE type;
		float dirX, dirY;
	};

	struct C2S_Skill
	{
		uint8_t size;
		PACKET_TYPE type;
		uint8_t skillIndex; // 0..SKILL_COUNT-1
		float dirX, dirY;
	};

	// Client-authoritative hit report (same trust model as C2S_Move): this client
	// decides it landed a hit and how much damage - the server does not validate it.
	struct C2S_Hit
	{
		uint8_t size;
		PACKET_TYPE type;
		int32_t targetPlayerId;
		int32_t damage;
	};

	struct C2S_GetExp
	{
		uint8_t size;
		PACKET_TYPE type;
		int32_t amount;
	};

	struct C2S_GetItem
	{
		uint8_t size;
		PACKET_TYPE type;
		int32_t itemId;
	};

	struct S2C_PlayerAttack
	{
		uint8_t size;
		PACKET_TYPE type;
		int32_t playerId;
		float dirX, dirY;
	};

	struct S2C_PlayerSkill
	{
		uint8_t size;
		PACKET_TYPE type;
		int32_t playerId;
		uint8_t skillIndex;
		float dirX, dirY;
	};

	// Broadcast to everyone, not just the target - lets the victim (targetId ==
	// self) apply damage and bystanders play hit VFX from the same packet.
	struct S2C_PlayerHit
	{
		uint8_t size;
		PACKET_TYPE type;
		int32_t attackerId;
		int32_t targetId;
		int32_t damage;
	};

	// Echoed only to the player who gained it, not broadcast.
	struct S2C_ExpResult
	{
		uint8_t size;
		PACKET_TYPE type;
		int32_t amount;
	};

	// Echoed only to the player who picked it up, not broadcast.
	struct S2C_ItemResult
	{
		uint8_t size;
		PACKET_TYPE type;
		int32_t itemId;
	};

	// ---- Health / mana sync (rough placeholders, mirrors Server/Protocol.h) ----

	struct C2S_UpdateHealth
	{
		uint8_t size;
		PACKET_TYPE type;
		float currentHealth;
	};

	struct C2S_UpdateMana
	{
		uint8_t size;
		PACKET_TYPE type;
		float currentMana;
	};

	// Echoed only to the player who reported it, not broadcast.
	struct S2C_HealthResult
	{
		uint8_t size;
		PACKET_TYPE type;
		float currentHealth;
	};

	// Echoed only to the player who reported it, not broadcast.
	struct S2C_ManaResult
	{
		uint8_t size;
		PACKET_TYPE type;
		float currentMana;
	};
#pragma pack(pop)
}
