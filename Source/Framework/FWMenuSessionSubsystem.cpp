#include "FWMenuSessionSubsystem.h"

void UFWMenuSessionSubsystem::SetLocalDisplayName(const FString& Name)
{
	LocalDisplayName = Name.TrimStartAndEnd().Left(20);
	if (LocalDisplayName.IsEmpty()) ResetLocalDisplayName();
}
