#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "FWMainMenuPlayerController.generated.h"

class UFWMainMenuWidget;

UCLASS(Config = Game)
class FRAMEWORK_API AFWMainMenuPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AFWMainMenuPlayerController();

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
	TObjectPtr<UFWMainMenuWidget> MenuWidget;

	bool bStartingGame = false;
};
