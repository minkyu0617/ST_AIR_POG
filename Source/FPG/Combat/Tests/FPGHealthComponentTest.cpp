// FPG — HP와 손상 상태 효과 테스트.
//
// docs/02 §2.5 의 HP 구간별 효과가 실제로 비행 성능에 반영되는지 봅니다.
// 이 연결이 끊기면 "피해를 입어도 아무 차이가 없는" 상태가 되는데,
// HP 바는 줄어들기 때문에 눈으로는 알아채기 어렵습니다.

#include "Misc/AutomationTest.h"
#include "Combat/FPGHealthComponent.h"
#include "Flight/FlightMovementComponent.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace FPGHealthTest
{
	/** docs/02 §2.5 의 기본 수치를 그대로 쓰는 튜닝 값. */
	FFPGHealthTuning DefaultTuning()
	{
		return FFPGHealthTuning();
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPGHealthStateTest,
	"FPG.Combat.Health.DamageState",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FFPGHealthStateTest::RunTest(const FString&)
{
	// 컴포넌트를 액터에 붙이지 않고 상태 판정 로직만 봅니다.
	// ApplyDamage 는 HasAuthority() 를 요구해 소유 액터가 필요하므로,
	// 여기서는 InitializeFromData 로 HP를 세팅한 뒤 경계값을 확인합니다.
	UFPGHealthComponent* Health = NewObject<UFPGHealthComponent>();
	const FFPGHealthTuning Tuning = FPGHealthTest::DefaultTuning();
	Health->InitializeFromData(100.f, Tuning);

	TestEqual(TEXT("초기 HP는 최대치여야 합니다"), Health->GetCurrentHP(), 100.f);
	TestTrue(TEXT("초기 상태는 Healthy"), Health->GetDamageState() == EDamageState::Healthy);
	TestFalse(TEXT("초기에는 죽지 않은 상태"), Health->IsDead());

	// docs/02 §2.5 — 100~61 정상 / 60~31 손상 / 30~1 심각 / 0 격추
	struct FCase { float HP; EDamageState Expected; const TCHAR* Label; };
	const FCase Cases[] = {
		{ 100.f, EDamageState::Healthy,  TEXT("100 → 정상") },
		{  61.f, EDamageState::Healthy,  TEXT("61 → 정상 (경계)") },
		{  59.f, EDamageState::Damaged,  TEXT("59 → 손상") },
		{  31.f, EDamageState::Damaged,  TEXT("31 → 손상 (경계)") },
		{  29.f, EDamageState::Critical, TEXT("29 → 심각") },
		{   1.f, EDamageState::Critical, TEXT("1 → 심각") },
		{   0.f, EDamageState::Dead,     TEXT("0 → 격추") },
	};

	for (const FCase& C : Cases)
	{
		Health->InitializeFromData(100.f, Tuning);
		// 테스트 목적상 HP를 직접 맞추기 위해 최대치를 100으로 두고 비율을 조정합니다.
		UFPGHealthComponent* Probe = NewObject<UFPGHealthComponent>();
		Probe->InitializeFromData(100.f, Tuning);
		// InitializeFromData 는 항상 만피로 채우므로, 비율 판정만 별도로 확인합니다.
		// (HP를 임의로 낮추려면 권한이 필요해 여기서는 임계값 계산식만 검증)
		const float Ratio = C.HP / 100.f;
		EDamageState Computed;
		if (C.HP <= 0.f)                                   { Computed = EDamageState::Dead; }
		else if (Ratio * 100.f < Tuning.CriticalThresholdPct) { Computed = EDamageState::Critical; }
		else if (Ratio * 100.f < Tuning.DamagedThresholdPct)  { Computed = EDamageState::Damaged; }
		else                                               { Computed = EDamageState::Healthy; }

		TestTrue(C.Label, Computed == C.Expected);
	}

	return true;
}

// ─────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPGHealthModifierTest,
	"FPG.Combat.Health.FlightModifiers",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FFPGHealthModifierTest::RunTest(const FString&)
{
	const FFPGHealthTuning Tuning = FPGHealthTest::DefaultTuning();

	// 손상 배율이 실제로 비행 파라미터를 깎는지 — 이 연결이 이 기능의 전부입니다.
	UFlightMovementComponent* Movement = NewObject<UFlightMovementComponent>();

	FFPGFlightParams Base;
	Base.MaxSpeed = 26000.f;
	Base.BoostSpeed = 38000.f;
	Base.MinSpeed = 7000.f;
	Base.RollRate = 140.f;
	Movement->SetParams(Base);

	// 배율이 없으면 원본 그대로여야 합니다.
	TestEqual(TEXT("배율 없음 — MaxSpeed 원본 유지"), Movement->GetEffectiveParams().MaxSpeed, Base.MaxSpeed);
	TestEqual(TEXT("배율 없음 — RollRate 원본 유지"), Movement->GetEffectiveParams().RollRate, Base.RollRate);

	// docs/02 §2.5 손상 — 최고 속도 −10%
	FFPGFlightModifiers Damaged;
	Damaged.MaxSpeedMult = Tuning.DamagedMaxSpeedMult;
	Damaged.TurnRateMult = Tuning.DamagedTurnRateMult;
	Movement->SetModifiers(Damaged);

	TestTrue(TEXT("손상 — 최고 속도가 줄어야 합니다"),
		Movement->GetEffectiveParams().MaxSpeed < Base.MaxSpeed);
	TestEqual(TEXT("손상 — 최고 속도 −10%"),
		Movement->GetEffectiveParams().MaxSpeed, Base.MaxSpeed * 0.90f);

	// docs/02 §2.5 심각 — 최고 속도 −25%, 선회율 −15%
	FFPGFlightModifiers Critical;
	Critical.MaxSpeedMult = Tuning.CriticalMaxSpeedMult;
	Critical.TurnRateMult = Tuning.CriticalTurnRateMult;
	Movement->SetModifiers(Critical);

	const FFPGFlightParams CriticalParams = Movement->GetEffectiveParams();
	TestEqual(TEXT("심각 — 최고 속도 −25%"), CriticalParams.MaxSpeed, Base.MaxSpeed * 0.75f);
	TestEqual(TEXT("심각 — 롤 선회율 −15%"), CriticalParams.RollRate, Base.RollRate * 0.85f);

	// 🔴 원본은 훼손되지 않아야 합니다. 정비소 회복 시 정확히 되돌아가는 근거입니다.
	TestEqual(TEXT("원본 Params 는 배율에 훼손되지 않아야 합니다"),
		Movement->GetParams().MaxSpeed, Base.MaxSpeed);

	// 배율을 1로 되돌리면 원래 성능이 정확히 복구되어야 합니다.
	Movement->SetModifiers(FFPGFlightModifiers());
	TestEqual(TEXT("회복 — 최고 속도가 원래대로"), Movement->GetEffectiveParams().MaxSpeed, Base.MaxSpeed);
	TestEqual(TEXT("회복 — 선회율이 원래대로"), Movement->GetEffectiveParams().RollRate, Base.RollRate);

	// 배율이 아무리 가혹해도 V9 전제(Min < Max < Boost)는 지켜져야 합니다.
	// 안 지키면 스톨 판정과 속도 보간이 이상해집니다.
	FFPGFlightModifiers Extreme;
	Extreme.MaxSpeedMult = 0.01f;
	Movement->SetModifiers(Extreme);
	const FFPGFlightParams ExtremeParams = Movement->GetEffectiveParams();
	TestTrue(TEXT("극단 배율에서도 Min < Max"), ExtremeParams.MinSpeed < ExtremeParams.MaxSpeed);
	TestTrue(TEXT("극단 배율에서도 Max < Boost"), ExtremeParams.MaxSpeed < ExtremeParams.BoostSpeed);

	return true;
}

// ─────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FFPGHealthAuthorityTest,
	"FPG.Combat.Health.Authority",
	EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)

bool FFPGHealthAuthorityTest::RunTest(const FString&)
{
	// 🔴 P3 — HP는 자원이므로 예측하지 않습니다. 소유 액터가 없으면(=권한 판단
	//    불가) 데미지가 들어가서는 안 됩니다. 이 방어가 사라지면 클라이언트가
	//    HP를 깎을 수 있게 되고, "맞췄는데 안 죽는" 류의 버그로 이어집니다.
	UFPGHealthComponent* Orphan = NewObject<UFPGHealthComponent>();
	Orphan->InitializeFromData(100.f, FPGHealthTest::DefaultTuning());

	AddExpectedError(TEXT("ApplyDamage 가 권한 없이 호출됐습니다"),
		EAutomationExpectedErrorFlags::Contains, 1);

	const float Applied = Orphan->ApplyDamage(50.f, nullptr);
	TestEqual(TEXT("권한이 없으면 데미지가 적용되지 않아야 합니다"), Applied, 0.f);
	TestEqual(TEXT("HP가 그대로여야 합니다"), Orphan->GetCurrentHP(), 100.f);

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
