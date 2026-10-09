#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FWLoginPlayerController.generated.h"

class UFWLoginWidget;

UCLASS(Config = Game)
class FRAMEWORK_API AFWLoginPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFWLoginPlayerController();

	/** Separate entry point so server admission can precede travel later. */
	UFUNCTION(BlueprintCallable, Category = "Menu")
	void StartGame();

	UFUNCTION(BlueprintCallable, Category = "Menu")
	void QuitGame();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** Also include the destination in ProjectPackagingSettings.MapsToCook. */
	UPROPERTY(Config, EditDefaultsOnly, Category = "Menu")
	TSoftObjectPtr<UWorld> GameplayLevel;

private:
	UPROPERTY(Transient)
	TObjectPtr<UFWLoginWidget> MenuWidget;

	bool bStartingGame = false;
};
