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

};
