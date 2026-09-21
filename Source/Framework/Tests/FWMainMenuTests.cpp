// Run in a standalone development game starting at MainMenu:
// -ExecCmds="Automation RunTests Framework.Menu.StartAndTravel" -TestExit="Automation Test Queue Empty"
#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/Button.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "../FWCharacter.h"
#include "../FWCameraRig.h"
#include "../FWGameMode.h"
#include "../FWMainMenuPlayerController.h"
#include "../FWMainMenuGameMode.h"
#include "../FWMainMenuWidget.h"
#include "../FWMiniMapWidget.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

namespace
{
UWorld* FindGameWorld()
{
	for (const FWorldContext& Context : GEngine->GetWorldContexts())
	{
		if (Context.WorldType == EWorldType::Game && Context.World())
		{
			return Context.World();
		}
	}
	return nullptr;
}

class FMenuTravelCheck : public IAutomationLatentCommand
{
public:
	explicit FMenuTravelCheck(FAutomationTestBase* InTest) : Test(InTest) {}

	virtual bool Update() override
	{
		const double Now = FPlatformTime::Seconds();
		if (Now - StartedAt > 90.0)
		{
			Test->AddError(TEXT("Timed out waiting for menu/gameplay travel."));
			return true;
		}
		if (Now < NextActionAt)
		{
			return false;
		}
		UWorld* World = FindGameWorld();
		if (!World)
		{
			return false;
		}
		if (Phase == 0)
		{
			AFWMainMenuPlayerController* PC = Cast<AFWMainMenuPlayerController>(World->GetFirstPlayerController());
			if (!PC)
			{
				return false;
			}
			Test->TestTrue(TEXT("Menu uses its own game mode"), World->GetAuthGameMode()->IsA<AFWMainMenuGameMode>());
			Test->TestNull(TEXT("Menu does not spawn a gameplay pawn"), PC->GetPawn());
			TArray<UUserWidget*> Widgets;
			UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Widgets, UFWMainMenuWidget::StaticClass());
			if (!Test->TestEqual(TEXT("One menu is on screen"), Widgets.Num(), 1))
			{
				return true;
			}
			Menu = Cast<UFWMainMenuWidget>(Widgets[0]);
			UButton* Start = Cast<UButton>(Menu->GetWidgetFromName(TEXT("StartGame")));
			UButton* Controls = Cast<UButton>(Menu->GetWidgetFromName(TEXT("Controls")));
			UButton* Quit = Cast<UButton>(Menu->GetWidgetFromName(TEXT("QuitGame")));
			if (!Start || !Controls || !Quit)
			{
				Test->AddError(TEXT("Missing a menu button."));
				return true;
			}
			Test->TestTrue(TEXT("Quit button is bound"), Quit->OnClicked.IsBound());
			UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Widgets, UFWMiniMapWidget::StaticClass());
			Test->TestEqual(TEXT("Menu has no gameplay minimap"), Widgets.Num(), 0);
			NextActionAt = Now + 2.0;
			Phase = 1;
			return false;
		}
		if (Phase == 1)
		{
			FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/Menu/MainMenu.png"), true, false);
			Phase = 2;
			NextActionAt = Now + 2.0;
			return false;
		}
		if (Phase == 2)
		{
			CastChecked<UButton>(Menu->GetWidgetFromName(TEXT("Controls")))->OnClicked.Broadcast();
			Test->TestTrue(TEXT("Controls panel opens"), Menu->GetWidgetFromName(TEXT("ControlsPanel"))->GetVisibility() == ESlateVisibility::Visible);
			Phase = 3;
			NextActionAt = Now + 1.0;
			return false;
		}
		if (Phase == 3)
		{
			FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/Menu/Controls.png"), true, false);
			Phase = 4;
			NextActionAt = Now + 2.0;
			return false;
		}
		if (Phase == 4)
		{
			CastChecked<UButton>(Menu->GetWidgetFromName(TEXT("Controls")))->OnClicked.Broadcast();
			Test->TestTrue(TEXT("Controls panel closes"), Menu->GetWidgetFromName(TEXT("ControlsPanel"))->GetVisibility() == ESlateVisibility::Collapsed);
			CastChecked<UButton>(Menu->GetWidgetFromName(TEXT("StartGame")))->OnClicked.Broadcast();
			Test->TestFalse(TEXT("Start is disabled while travelling"), Menu->GetWidgetFromName(TEXT("StartGame"))->GetIsEnabled());
			Phase = 5;
			NextActionAt = Now + 3.0;
			return false;
		}
		if (Phase == 5)
		{
			if (!World->GetMapName().EndsWith(TEXT("DefaultMap")))
			{
				return false;
			}
			Test->TestTrue(TEXT("Start loads gameplay game mode"), World->GetAuthGameMode()->IsA<AFWGameMode>());
			Test->TestNotNull(TEXT("Gameplay character spawns"), Cast<AFWCharacter>(World->GetFirstPlayerController()->GetPawn()));
			Test->TestNotNull(TEXT("Gameplay keeps the top-down camera after possession"), Cast<AFWCameraRig>(World->GetFirstPlayerController()->GetViewTarget()));
			TArray<UUserWidget*> Widgets;
			UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Widgets, UFWMainMenuWidget::StaticClass());
			Test->TestEqual(TEXT("Menu removed after travel"), Widgets.Num(), 0);
			UWidgetBlueprintLibrary::GetAllWidgetsOfClass(World, Widgets, UFWMiniMapWidget::StaticClass());
			Test->TestEqual(TEXT("Gameplay minimap appears"), Widgets.Num(), 1);
			FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir() / TEXT("Screenshots/Menu/Gameplay.png"), true, false);
			Phase = 6;
			NextActionAt = Now + 2.0;
			return false;
		}
		return true;
	}

private:
	FAutomationTestBase* Test;
	TWeakObjectPtr<UFWMainMenuWidget> Menu;
	int32 Phase = 0;
	double StartedAt = FPlatformTime::Seconds();
	double NextActionAt = 0.0;
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFWMenuTravelTest, "Framework.Menu.StartAndTravel", EAutomationTestFlags::ClientContext | EAutomationTestFlags::EngineFilter)

bool FFWMenuTravelTest::RunTest(const FString& Parameters)
{
	ADD_LATENT_AUTOMATION_COMMAND(FMenuTravelCheck(this));
	return true;
}

#endif
