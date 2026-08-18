// FPG — DataRegistry 로딩·검증 테스트.
//
// docs/16 AR4 — "DataTable 참조 오류가 런타임 크래시로" 이어지는 것을 막습니다.
//
// 이 테스트가 지키는 것은 P5입니다. CSV의 값이 실제로 FFPGFlightParams까지
// 도달하는지 확인하지 않으면, 열 이름 하나만 어긋나도 값이 조용히 C++
// 기본값으로 떨어지고 "CSV를 고쳤는데 게임이 안 변한다"가 됩니다.
//
// 실행: UnrealEditor-Cmd.exe <uproject> -ExecCmds="Automation RunTests FPG.Data" -unattended -nullrhi

#include "Misc/AutomationTest.h"
#include "Core/Services/FPGDataRegistry.h"
#include "Engine/GameInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace FPGDataTest
{
	/**
	 * 테스트용 레지스트리를 만듭니다.
	 *
	 * ⚠️ `NewObject<UFPGDataRegistry>()` 를 그냥 호출하면 안 됩니다.
	 *    UGameInstanceSubsystem 은 ClassWithin 이 UGameInstance 라서
	 *    Outer 가 GameInstance 가 아니면 ensure 가 터집니다.
	 *    실제 게임에서는 서브시스템 시스템이 GameInstance 를 Outer 로 만들어 주므로
	 *    이건 테스트 하네스에서만 신경 쓰면 되는 부분입니다.
	 *
	 *    여기서 만드는 GameInstance 는 Init() 을 거치지 않은 껍데기지만,
	 *    레지스트리가 검사하는 기능(CSV 로딩·조회·검증)은 GameInstance 상태를
	 *    건드리지 않으므로 문제되지 않습니다.
	 */
	UFPGDataRegistry* MakeRegistry()
	{
		UGameInstance* OuterGameInstance = NewObject<UGameInstance>(GetTransientPackage());
		return NewObject<UFPGDataRegistry>(OuterGameInstance);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPGDataRegistryLoadTest,
	"FPG.Data.Registry.Load",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FFPGDataRegistryLoadTest::RunTest(const FString&)
{
	UFPGDataRegistry* Registry = FPGDataTest::MakeRegistry();

	TArray<FString> Problems;
	const bool bLoaded = Registry->LoadFromDirectory(UFPGDataRegistry::GetDefaultDataDirectory(), Problems);

	for (const FString& Problem : Problems)
	{
		AddError(FString::Printf(TEXT("로딩 문제: %s"), *Problem));
	}
	TestTrue(TEXT("CSV를 문제 없이 읽어야 합니다"), bLoaded);

	// 열 이름이 어긋나면 행은 생기지만 값이 전부 기본값이 됩니다.
	// 그래서 "행이 있는가"가 아니라 "값이 맞는가"를 봅니다.
	const FFPGAircraftRow* Falcon = Registry->FindAircraft(TEXT("AIRCRAFT_FALCON"));
	if (!Falcon)
	{
		AddError(TEXT("AIRCRAFT_FALCON 행을 찾지 못했습니다"));
		return false;
	}

	TestEqual(TEXT("Falcon 스탯 합은 100이어야 합니다 (P8)"),
		Falcon->Speed + Falcon->Accel + Falcon->Turn + Falcon->Durability + Falcon->BoostPower, 100);
	TestTrue(TEXT("Falcon MaxSpeed가 CSV에서 실제로 들어와야 합니다"), Falcon->MaxSpeed > 0.f);
	TestTrue(TEXT("Falcon 속도 순서 Min < Max < Boost"),
		Falcon->MinSpeed < Falcon->MaxSpeed && Falcon->MaxSpeed < Falcon->BoostSpeed);

	// 열거형이 문자열에서 제대로 파싱됐는지 — 실패하면 전부 0번 값이 됩니다.
	const FFPGItemRow* Vulcan = Registry->FindItem(TEXT("ITEM_VULCAN"));
	if (Vulcan)
	{
		TestTrue(TEXT("ITEM_VULCAN 의 Category 는 Gun 이어야 합니다 (열거형 파싱)"),
			Vulcan->Category == EItemCategory::Gun);
		TestEqual(TEXT("ITEM_VULCAN 은 무한 탄약(-1) 이어야 합니다"), Vulcan->Ammo, -1);
	}
	else
	{
		AddError(TEXT("ITEM_VULCAN 행을 찾지 못했습니다"));
	}

	// 음수 = 회복 규약 (docs/17 §17.4)
	if (const FFPGItemRow* Repair = Registry->FindItem(TEXT("ITEM_REPAIR_PACK")))
	{
		TestTrue(TEXT("ITEM_REPAIR_PACK 의 Damage 는 음수(=회복)여야 합니다"), Repair->Damage < 0.f);
	}

	if (const FFPGModuleRow* Module = Registry->FindModule(TEXT("MOD_LIGHT_FRAME")))
	{
		TestFalse(TEXT("MOD_LIGHT_FRAME 의 StatModifiers 가 비어 있으면 안 됩니다"),
			Module->StatModifiers.IsEmpty());
	}
	else
	{
		AddError(TEXT("MOD_LIGHT_FRAME 행을 찾지 못했습니다"));
	}

	return true;
}

// ─────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPGDataRegistryFlightParamsTest,
	"FPG.Data.Registry.FlightParams",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FFPGDataRegistryFlightParamsTest::RunTest(const FString&)
{
	UFPGDataRegistry* Registry = FPGDataTest::MakeRegistry();
	TArray<FString> Problems;
	Registry->LoadFromDirectory(UFPGDataRegistry::GetDefaultDataDirectory(), Problems);

	// P5의 핵심 검사: CSV 값이 실제로 시뮬레이션 파라미터까지 도달하는가.
	//
	// 기본값과 같은 값을 비교하면 "안 들어왔는데 통과"하는 헛통과가 생깁니다.
	// 그래서 일부러 기본값을 엉뚱하게 흐트러뜨린 뒤, CSV 값으로 덮였는지 봅니다.
	FFPGFlightParams Params;
	Params.CruiseSpeed = -12345.f;
	Params.MaxSpeed = -12345.f;
	Params.StallSpeed = -12345.f;
	Params.BankTurnRateAt30 = -12345.f;

	const bool bBuilt = Registry->BuildFlightParams(TEXT("AIRCRAFT_FALCON"), Params);
	TestTrue(TEXT("Falcon 파라미터를 만들 수 있어야 합니다"), bBuilt);

	TestNotEqual(TEXT("CruiseSpeed 가 DT_Flight 에서 덮여야 합니다"), Params.CruiseSpeed, -12345.f);
	TestNotEqual(TEXT("MaxSpeed 가 DT_Aircraft 에서 덮여야 합니다"), Params.MaxSpeed, -12345.f);
	TestNotEqual(TEXT("StallSpeed 가 DT_Flight 에서 덮여야 합니다"), Params.StallSpeed, -12345.f);
	TestNotEqual(TEXT("BankTurnRateAt30 이 DT_Flight 에서 덮여야 합니다"), Params.BankTurnRateAt30, -12345.f);

	// D-16 단위 정정이 유지되는지 — 순항 속도가 도보 속도로 되돌아가면 잡힙니다.
	// 1 UU = 1cm 이므로 18,000 UU/s = 648km/h. 1,000 미만이면 100배 오류 재발입니다.
	TestTrue(
		TEXT("D-16: 순항 속도가 100배 오류 스케일로 되돌아가면 안 됩니다 (>1000 UU/s)"),
		Params.CruiseSpeed > 1000.f);

	// 조합이 실제로 두 표에서 왔는지 교차 확인
	const FFPGAircraftRow* Falcon = Registry->FindAircraft(TEXT("AIRCRAFT_FALCON"));
	if (Falcon)
	{
		TestEqual(TEXT("MaxSpeed 는 DT_Aircraft 값과 같아야 합니다"), Params.MaxSpeed, Falcon->MaxSpeed);
	}
	TestEqual(TEXT("CruiseSpeed 는 DT_Flight 값과 같아야 합니다"),
		Params.CruiseSpeed, Registry->GetFlightValue(TEXT("CRUISE_SPEED"), -1.f));

	// 없는 기체를 물으면 실패해야 하고, Out 을 건드리면 안 됩니다.
	FFPGFlightParams Untouched;
	Untouched.MaxSpeed = 4242.f;
	TestFalse(TEXT("없는 기체 ID는 false 를 돌려줘야 합니다"),
		Registry->BuildFlightParams(TEXT("AIRCRAFT_DOES_NOT_EXIST"), Untouched));
	TestEqual(TEXT("실패 시 Out 은 손대지 않아야 합니다"), Untouched.MaxSpeed, 4242.f);

	return true;
}

// ─────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPGDataRegistryValidateTest,
	"FPG.Data.Registry.ValidateAll",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FFPGDataRegistryValidateTest::RunTest(const FString&)
{
	UFPGDataRegistry* Registry = FPGDataTest::MakeRegistry();
	TArray<FString> Problems;
	Registry->LoadFromDirectory(UFPGDataRegistry::GetDefaultDataDirectory(), Problems);

	TArray<FString> Errors;
	const bool bValid = Registry->ValidateAll(Errors);

	for (const FString& Error : Errors)
	{
		AddError(FString::Printf(TEXT("ValidateAll: %s"), *Error));
	}
	TestTrue(TEXT("현재 데이터는 ValidateAll 을 통과해야 합니다"), bValid);

	// 검증기가 실제로 잡는지 — 통과만 하는 검증기는 쓸모가 없습니다.
	// 존재하지 않는 디렉터리를 읽히면 표가 비므로 반드시 실패해야 합니다.
	UFPGDataRegistry* Empty = FPGDataTest::MakeRegistry();
	TArray<FString> IgnoredProblems;
	Empty->LoadFromDirectory(TEXT("/이런/경로는/없습니다"), IgnoredProblems);

	TArray<FString> EmptyErrors;
	TestFalse(TEXT("표가 비면 ValidateAll 이 실패해야 합니다"), Empty->ValidateAll(EmptyErrors));
	TestTrue(TEXT("실패 사유가 보고되어야 합니다"), EmptyErrors.Num() > 0);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
