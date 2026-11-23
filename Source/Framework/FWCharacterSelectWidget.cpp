#include "FWCharacterSelectWidget.h"
#include "FWCharacterSelectButton.h"
#include "FWSkillButtonWidget.h"
#include "FWTeammateSelectSlot.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "FWCharacterSelect"

void UFWCharacterSelectWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (Btn_Confirm)
	{
		Btn_Confirm->OnClicked.AddDynamic(this, &UFWCharacterSelectWidget::HandleConfirmClicked);
	}

	SkillButtons = { SkillButton_Q, SkillButton_W, SkillButton_E, SkillButton_R };
}

const FFWCharacterSelectData* UFWCharacterSelectWidget::FindCharacterData(FName CharacterId) const
{
	if (!CharacterDataTable || CharacterId.IsNone())
	{
		return nullptr;
	}
	return CharacterDataTable->FindRow<FFWCharacterSelectData>(CharacterId, TEXT("FWCharacterSelect"), false);
}

bool UFWCharacterSelectWidget::IsSelectableCharacter(FName CharacterId) const
{
	const FFWCharacterSelectData* Data = FindCharacterData(CharacterId);
	return Data && Data->bSelectable;
}

FName UFWCharacterSelectWidget::FindFirstSelectableCharacterId() const
{
	for (const FName& CharacterId : RosterOrder)
	{
		if (IsSelectableCharacter(CharacterId))
		{
			return CharacterId;
		}
	}
	return NAME_None;
}

void UFWCharacterSelectWidget::InitializeScreen(const FString& Nickname)
{
	bSelectionLocked = false;
	bRequestPending = false;
	SelectedCharacterId = NAME_None;

	LocalView = FFWTeamSelectPlayerView();
	LocalView.Nickname = Nickname;
	LocalView.bIsLocalPlayer = true;
	LocalView.bOccupied = true;

	if (!CharacterDataTable)
	{
		UE_LOG(LogTemp, Error, TEXT("[FWCharacterSelect] CharacterDataTable is not set on %s."), *GetClass()->GetName());
	}

	BuildCharacterButtons();

	// 연결되지 않은 슬롯을 실제 팀원처럼 표시하지 않음
	if (Slot_TeamA)
	{
		Slot_TeamA->SetEmpty();
	}
	if (Slot_TeamB)
	{
		Slot_TeamB->SetEmpty();
	}
	RefreshLocalSlot();

	ClearRemainingTime();
	ClearStatusMessage();
	RefreshInputLock();
}

void UFWCharacterSelectWidget::BuildCharacterButtons()
{
	if (!UniformGrid_Characters)
	{
		return;
	}

	UniformGrid_Characters->ClearChildren();
	CharacterButtonRefs.Reset();

	if (!CharacterButtonClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[FWCharacterSelect] CharacterButtonClass is not set on %s."), *GetClass()->GetName());
		return;
	}

	const int32 Columns = FMath::Max(1, RosterColumns);
	for (int32 Index = 0; Index < RosterOrder.Num(); ++Index)
	{
		const FName CharacterId = RosterOrder[Index];
		const FFWCharacterSelectData* Data = FindCharacterData(CharacterId);
		if (!Data)
		{
			// 칸은 비워 두고 다음 캐릭터는 원래 위치에 배치
			UE_LOG(LogTemp, Warning, TEXT("[FWCharacterSelect] Row '%s' is missing in CharacterDataTable."), *CharacterId.ToString());
			continue;
		}

		UFWCharacterSelectButton* Button = CreateWidget<UFWCharacterSelectButton>(this, CharacterButtonClass);
		if (!Button)
		{
			continue;
		}

		Button->SetCharacterData(CharacterId, *Data);
		Button->OnCharacterClicked.AddDynamic(this, &UFWCharacterSelectWidget::HandleCharacterButtonClicked);

		if (UUniformGridSlot* GridSlot = UniformGrid_Characters->AddChildToUniformGrid(Button, Index / Columns, Index % Columns))
		{
			GridSlot->SetHorizontalAlignment(HAlign_Fill);
			GridSlot->SetVerticalAlignment(VAlign_Fill);
		}
		CharacterButtonRefs.Add(Button);
	}
}

void UFWCharacterSelectWidget::HandleCharacterButtonClicked(FName CharacterId)
{
	PreviewCharacter(CharacterId);
}

bool UFWCharacterSelectWidget::PreviewCharacter(FName CharacterId)
{
	// 1. 잠금/대기 중이면 종료
	if (bSelectionLocked || bRequestPending)
	{
		return false;
	}

	// 2~3. 행이 없거나 선택 불가면 안내 후 종료
	const FFWCharacterSelectData* Data = FindCharacterData(CharacterId);
	if (!Data)
	{
		ShowSelectionError(LOCTEXT("UnknownCharacter", "선택할 수 없는 캐릭터입니다."));
		return false;
	}
	if (!Data->bSelectable)
	{
		ShowSelectionError(FText::Format(LOCTEXT("NotSelectable", "{0}은(는) 아직 선택할 수 없습니다."), Data->DisplayName));
		return false;
	}

	ClearStatusMessage();

	// 4~10. 표시 갱신 (확정 상태는 저장하지 않음)
	return ApplyCharacterDisplay(CharacterId);
}

bool UFWCharacterSelectWidget::ApplyCharacterDisplay(FName CharacterId)
{
	const FFWCharacterSelectData* Data = FindCharacterData(CharacterId);
	if (!Data)
	{
		return false;
	}

	SelectedCharacterId = CharacterId;

	ApplyCharacterArt(*Data);

	if (Txt_CharacterName)
	{
		Txt_CharacterName->SetText(Data->DisplayName);
	}
	if (Txt_RoleName)
	{
		Txt_RoleName->SetText(Data->RoleName.IsEmpty() ? LOCTEXT("RolePending", "준비 중") : Data->RoleName);
	}
	if (Img_RoleBadge)
	{
		if (Data->RoleBadge)
		{
			Img_RoleBadge->SetBrushFromTexture(Data->RoleBadge, false);
			Img_RoleBadge->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			Img_RoleBadge->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	ApplySkills(*Data);
	RefreshButtonSelection();
	RefreshLocalSlot();
	return true;
}

void UFWCharacterSelectWidget::ApplyCharacterArt(const FFWCharacterSelectData& Data)
{
	if (!Img_CharacterArt)
	{
		return;
	}
	if (!Data.FullBody)
	{
		Img_CharacterArt->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	Img_CharacterArt->SetBrushFromTexture(Data.FullBody, false);
	Img_CharacterArt->SetVisibility(ESlateVisibility::HitTestInvisible);

	// Img_CharacterArt는 Canvas_Design의 직접 자식이어야 크기/위치 보정이 적용됨
	UCanvasPanelSlot* ArtSlot = Cast<UCanvasPanelSlot>(Img_CharacterArt->Slot);
	if (!ArtSlot)
	{
		return;
	}

	FVector2D Size = Data.ArtSize;
	if (Size.X <= 0.0 || Size.Y <= 0.0)
	{
		// 원본 비율 유지: 높이를 ArtDefaultHeight에 맞추고 가로를 계산
		const float TextureWidth = static_cast<float>(Data.FullBody->GetSizeX());
		const float TextureHeight = static_cast<float>(Data.FullBody->GetSizeY());
		Size = TextureHeight > 0.f
			? FVector2D(TextureWidth * ArtDefaultHeight / TextureHeight, ArtDefaultHeight)
			: FVector2D(ArtDefaultHeight, ArtDefaultHeight);
	}

	ArtSlot->SetAnchors(FAnchors(0.f, 0.f));
	ArtSlot->SetAlignment(FVector2D::ZeroVector);
	ArtSlot->SetSize(Size);
	ArtSlot->SetPosition(FVector2D(ArtCenterX - Size.X * 0.5f, ArtBottomY - Size.Y));
}

void UFWCharacterSelectWidget::ApplySkills(const FFWCharacterSelectData& Data)
{
	static const FText FallbackKeys[] =
	{
		FText::FromString(TEXT("Q")), FText::FromString(TEXT("W")), FText::FromString(TEXT("E")), FText::FromString(TEXT("R"))
	};

	for (int32 Index = 0; Index < SkillButtons.Num(); ++Index)
	{
		UFWSkillButtonWidget* SkillButton = SkillButtons[Index];
		if (!SkillButton)
		{
			continue;
		}

		const bool bHasData = Data.Skills.IsValidIndex(Index) && Data.Skills[Index].HasDisplayData();
		SkillButton->SetSkillData(bHasData ? Data.Skills[Index] : FFWCharacterSkillData(), FallbackKeys[Index], bHasData);
	}
}

void UFWCharacterSelectWidget::RefreshButtonSelection()
{
	for (UFWCharacterSelectButton* Button : CharacterButtonRefs)
	{
		if (Button)
		{
			Button->SetSelected(Button->GetCharacterId() == SelectedCharacterId);
		}
	}
}

void UFWCharacterSelectWidget::RefreshInputLock()
{
	const bool bLocked = bSelectionLocked || bRequestPending;

	for (UFWCharacterSelectButton* Button : CharacterButtonRefs)
	{
		if (Button)
		{
			Button->SetSelectionLocked(bLocked);
		}
	}

	if (Btn_Confirm)
	{
		Btn_Confirm->SetIsEnabled(!bLocked);
	}

	if (Txt_ConfirmLabel)
	{
		if (bRequestPending)
		{
			Txt_ConfirmLabel->SetText(LOCTEXT("ConfirmPending", "확인 중..."));
		}
		else if (LocalView.bConfirmed)
		{
			Txt_ConfirmLabel->SetText(LOCTEXT("ConfirmDone", "확정 완료"));
		}
		else if (bSelectionLocked)
		{
			Txt_ConfirmLabel->SetText(LOCTEXT("ConfirmUnavailable", "선택 불가"));
		}
		else
		{
			Txt_ConfirmLabel->SetText(LOCTEXT("Confirm", "선택 확정"));
		}
	}
}

void UFWCharacterSelectWidget::RefreshLocalSlot()
{
	if (!Slot_Local)
	{
		return;
	}
	LocalView.CharacterId = SelectedCharacterId;
	Slot_Local->ApplyView(LocalView, FindPortrait(SelectedCharacterId));
}

UTexture2D* UFWCharacterSelectWidget::FindPortrait(FName CharacterId) const
{
	const FFWCharacterSelectData* Data = FindCharacterData(CharacterId);
	return Data ? Data->Portrait.Get() : nullptr;
}

void UFWCharacterSelectWidget::HandleConfirmClicked()
{
	// 캐릭터 유효성은 컨트롤러의 RequestConfirmSelection에서 검사
	if (bSelectionLocked || bRequestPending)
	{
		return;
	}
	OnConfirmRequested.Broadcast(SelectedCharacterId);
}

void UFWCharacterSelectWidget::SetRemainingSeconds(int32 Seconds)
{
	if (!Txt_RemainingTime)
	{
		return;
	}
	const int32 Clamped = FMath::Max(0, Seconds);
	Txt_RemainingTime->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"), Clamped / 60, Clamped % 60)));
}

void UFWCharacterSelectWidget::ClearRemainingTime()
{
	if (Txt_RemainingTime)
	{
		Txt_RemainingTime->SetText(FText::FromString(TEXT("--:--")));
	}
}

void UFWCharacterSelectWidget::SetRequestPending(bool bIsPending)
{
	bRequestPending = bIsPending;
	if (bRequestPending)
	{
		ShowStatusMessage(LOCTEXT("Pending", "선택 확인 중..."));
	}
	else
	{
		ClearStatusMessage();
	}
	RefreshInputLock();
}

void UFWCharacterSelectWidget::SetConfirmedState()
{
	bRequestPending = false;
	bSelectionLocked = true;
	LocalView.bConfirmed = true;
	ClearStatusMessage();
	RefreshInputLock();
	RefreshLocalSlot();
}

void UFWCharacterSelectWidget::SetFailedState(const FText& Message)
{
	bRequestPending = false;
	bSelectionLocked = true;
	RefreshInputLock();
	ShowSelectionError(Message);
}

void UFWCharacterSelectWidget::ShowSelectionError(const FText& Message)
{
	UE_LOG(LogTemp, Warning, TEXT("[FWCharacterSelect] %s"), *Message.ToString());
	if (Txt_StatusMessage)
	{
		Txt_StatusMessage->SetText(Message);
		Txt_StatusMessage->SetColorAndOpacity(FSlateColor(ErrorMessageColor));
		Txt_StatusMessage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UFWCharacterSelectWidget::ShowStatusMessage(const FText& Message)
{
	if (Txt_StatusMessage)
	{
		Txt_StatusMessage->SetText(Message);
		Txt_StatusMessage->SetColorAndOpacity(FSlateColor(StatusMessageColor));
		Txt_StatusMessage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

void UFWCharacterSelectWidget::ClearStatusMessage()
{
	if (Txt_StatusMessage)
	{
		Txt_StatusMessage->SetText(FText::GetEmpty());
		Txt_StatusMessage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UFWCharacterSelectWidget::ApplyTeamSlots(const TArray<FFWTeamSelectPlayerView>& PlayerViews)
{
	TArray<FFWTeamSelectPlayerView> Others;
	for (const FFWTeamSelectPlayerView& View : PlayerViews)
	{
		if (!View.bOccupied)
		{
			continue;
		}
		if (View.bIsLocalPlayer)
		{
			// 본인 슬롯의 캐릭터는 현재 미리보기/최종 표시 ID를 그대로 사용
			if (!View.Nickname.IsEmpty())
			{
				LocalView.Nickname = View.Nickname;
			}
			// 본인 확정 표시는 ApplyConfirmedSelection → SetConfirmedState 경로로만 바꿈
			continue;
		}
		Others.Add(View);
	}

	// 목록 순서가 바뀌어도 같은 사람이 같은 칸에 오도록 PlayerId 기준 정렬
	Others.Sort([](const FFWTeamSelectPlayerView& A, const FFWTeamSelectPlayerView& B)
		{
			return A.PlayerId < B.PlayerId;
		});

	UFWTeammateSelectSlot* OtherSlots[] = { Slot_TeamA, Slot_TeamB };
	const int32 SlotCount = static_cast<int32>(UE_ARRAY_COUNT(OtherSlots));
	for (int32 Index = 0; Index < SlotCount; ++Index)
	{
		if (!OtherSlots[Index])
		{
			continue;
		}
		if (Others.IsValidIndex(Index))
		{
			OtherSlots[Index]->ApplyView(Others[Index], FindPortrait(Others[Index].CharacterId));
		}
		else
		{
			OtherSlots[Index]->SetEmpty();
		}
	}

	if (Others.Num() > SlotCount)
	{
		UE_LOG(LogTemp, Warning, TEXT("[FWCharacterSelect] %d teammates received, only %d slots are shown."), Others.Num(), SlotCount);
	}

	RefreshLocalSlot();
}

void UFWCharacterSelectWidget::SetTransitionOpacity(float Opacity)
{
	if (!Img_FadeBlack)
	{
		return;
	}
	const float Alpha = FMath::Clamp(Opacity, 0.f, 1.f);
	Img_FadeBlack->SetColorAndOpacity(FLinearColor(0.f, 0.f, 0.f, Alpha));
	// 검정 화면이 보이는 동안에는 클릭을 막음
	Img_FadeBlack->SetVisibility(Alpha > 0.f ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
}

#undef LOCTEXT_NAMESPACE