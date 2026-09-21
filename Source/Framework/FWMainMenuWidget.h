#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "FWMainMenuWidget.generated.h"

class UButton;
class UTextBlock;
class UBorder;

/** Code-built UMG menu; no background texture or Blueprint asset required. */
UCLASS()
class FRAMEWORK_API UFWMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	TSharedPtr<SWidget> GetInitialFocusWidget() const;
	void ShowStatus(const FText& Message);
	void SetStartingGame();

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildMenu();
	UFUNCTION()
	void OnStartClicked();
	UFUNCTION()
	void OnControlsClicked();
	UFUNCTION()
	void OnQuitClicked();

	UPROPERTY(Transient)
	TObjectPtr<UButton> StartButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> ControlsButton;
	UPROPERTY(Transient)
	TObjectPtr<UButton> QuitButton;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StartLabel;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> ControlsLabel;
	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> StatusLabel;
	UPROPERTY(Transient)
	TObjectPtr<UBorder> ControlsPanel;
};
