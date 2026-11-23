#include "FWMenuSessionSubsystem.h"

void UFWMenuSessionSubsystem::SetLocalDisplayName(const FString& Name)
{
	LocalDisplayName = Name.TrimStartAndEnd().Left(20);
	if (LocalDisplayName.IsEmpty()) ResetLocalDisplayName();
}

void UFWMenuSessionSubsystem::ConfirmCharacterSelection(FName CharacterId)
{
	if (CharacterId.IsNone())
	{
		return;
	}

	SelectedCharacterId = CharacterId;
	bCharacterSelectionConfirmed = true;
}

void UFWMenuSessionSubsystem::ResetCharacterSelection()
{
	SelectedCharacterId = NAME_None;
	bCharacterSelectionConfirmed = false;
}