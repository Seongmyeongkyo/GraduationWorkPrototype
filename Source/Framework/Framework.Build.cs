// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Framework : ModuleRules
{
	public Framework(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "AIModule", "NavigationSystem", "AnimGraphRuntime", "UMG", "Slate", "SlateCore" });

		PrivateDependencyModuleNames.AddRange(new string[] { "Sockets", "Networking", "ImageCore" });

		// 승인된 HTML과 동일한 PNG를 사용. 패키징에서도 색상 보정/잎 표시가 가능하도록 포함합니다.
		// 웹 브라우저나 로컬 HTTP 서버는 게임 실행에 필요하지 않습니다.
		if (Target.Type != TargetType.Server)
		{
			RuntimeDependencies.Add("$(ProjectDir)/Art/UI/LoginScreen/CloseIN_LoginBackground_Forest_v3.png", StagedFileType.NonUFS);
			foreach (string Sprite in new string[] { "FoliageSprig.png", "FoliageCluster.png", "FoliageDistant.png" })
			{
				RuntimeDependencies.Add("$(ProjectDir)/Art/UI/LoginScreen/WindPreview/" + Sprite, StagedFileType.NonUFS);
			}
		}

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
