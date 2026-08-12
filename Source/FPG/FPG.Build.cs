using UnrealBuildTool;

public class FPG : ModuleRules
{
	public FPG(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// 모듈 루트를 include 기준점으로 삼습니다.
		// docs/16 §16.12는 폴더를 계층으로 쓰므로(Flight/ Combat/ UI/ ...)
		// 폴더 간 include가 계속 생깁니다. 이렇게 해두면 상대 경로(../../) 없이
		//   #include "Flight/FlightMovementComponent.h"
		// 처럼 계층이 드러나는 형태로 쓸 수 있고, 파일을 옮겨도 안 깨집니다.
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });

		PrivateDependencyModuleNames.AddRange(new string[] { });

		// UI 작업(HUD·미니맵·로비)을 시작할 때 주석을 해제하십시오. → docs/09
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore", "UMG", "CommonUI" });

		// 멀티플레이는 M3입니다. M1은 싱글 전용이므로 아직 켜지 않습니다. → docs/16 §16.17
		// 켤 때는 FPG.uproject의 Plugins에 OnlineSubsystemSteam도 함께 추가해야 합니다.
		// PrivateDependencyModuleNames.AddRange(new string[] { "OnlineSubsystem", "OnlineSubsystemUtils" });
	}
}
