// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Framework : ModuleRules
{
	public Framework(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "AIModule", "NavigationSystem", "AnimGraphRuntime", "UMG", "Slate", "SlateCore" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Sockets", "Networking", "ImageCore", "RenderCore", "RHI" });

		// 승인된 HTML과 동일한 PNG를 사용. 패키징에서도 색상 보정/잎 표시가 가능하도록 포함합니다.
		// 웹 브라우저나 로컬 HTTP 서버는 게임 실행에 필요하지 않습니다.
		if (Target.Type != TargetType.Server)
		{
			RuntimeDependencies.Add("$(ProjectDir)/Art/UI/LoginScreen/CloseIN_LoginBackground_Forest_v3.png", StagedFileType.NonUFS);
			RuntimeDependencies.Add("$(ProjectDir)/Art/UI/Lobby/Backgrounds/CloseIN_Lobby_Background_ForestEntrance_v1.png", StagedFileType.NonUFS);
			RuntimeDependencies.Add("$(ProjectDir)/Art/UI/Matchmaking/CloseIN_MatchComplete_Background_v1_1280.png", StagedFileType.NonUFS);
			RuntimeDependencies.Add("$(ProjectDir)/Art/UI/Matchmaking/CloseIN_MatchComplete_TextOnly_v1_1280.png", StagedFileType.NonUFS);
			foreach (string Sprite in new string[] { "FoliageSprig.png", "FoliageCluster.png", "FoliageDistant.png" })
			{
				RuntimeDependencies.Add("$(ProjectDir)/Art/UI/LoginScreen/WindPreview/" + Sprite, StagedFileType.NonUFS);
			}
			// 로비는 문자 없는 공용 스킨 PNG를 9-slice로 표시하고 문자는 UMG로 따로 렌더링한다.
			foreach (string Skin in new string[] {
				"primary-default", "primary-hover", "primary-pressed",
				"secondary-default", "secondary-hover", "secondary-pressed",
				"quiet-default", "quiet-hover", "quiet-pressed",
				"mode-default", "mode-selected", "mode-selected-disabled", "disabled",
				"lock", "header-shade", "bottom-shade", "leaf-left", "leaf-right"
			})
			{
				RuntimeDependencies.Add("$(ProjectDir)/Art/UI/Lobby/Components/" + Skin + ".png", StagedFileType.NonUFS);
			}
			// 승인된 세로 팀 카드/중립 프로필/중앙 초대창 스킨.
			foreach (string Skin in new string[] { "card-filled", "card-empty", "card-searching", "dialog-surface", "profile-neutral", "leader-mark", "invite-plus", "arrow-back" })
			{
				RuntimeDependencies.Add("$(ProjectDir)/Art/UI/MenuTeamConcept/Components/" + Skin + ".png", StagedFileType.NonUFS);
			}
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
