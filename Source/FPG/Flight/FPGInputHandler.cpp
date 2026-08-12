#include "Flight/FPGInputHandler.h"
#include "Flight/FlightTypes.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** 디지털 키와 아날로그 축(트리거·스틱)을 한 가지로 취급합니다. */
	bool IsDown(const APlayerController& PC, const FKey& Key)
	{
		if (!Key.IsValid())
		{
			return false;
		}
		if (Key.IsAnalog())
		{
			return FMath::Abs(PC.GetInputAnalogKeyState(Key)) >= UFPGInputHandler::AnalogThreshold;
		}
		return PC.IsInputKeyDown(Key);
	}
}

uint8 UFPGInputHandler::BuildInputFlags(const APlayerController& PC, const FFPGKeyBindings& Keys)
{
	uint8 Flags = 0;

	auto Apply = [&](EFPGInputFlags Flag, const FKey& Primary, const FKey& Pad)
	{
		if (IsDown(PC, Primary) || IsDown(PC, Pad))
		{
			Flags |= static_cast<uint8>(Flag);
		}
	};

	Apply(EFPGInputFlags::Thrust, Keys.Thrust, Keys.PadThrust);
	Apply(EFPGInputFlags::Brake,  Keys.Brake,  Keys.PadBrake);
	Apply(EFPGInputFlags::Vector, Keys.Vector, Keys.PadVector);
	Apply(EFPGInputFlags::Boost,  Keys.Boost,  Keys.PadBoost);
	Apply(EFPGInputFlags::Fire,   Keys.Fire,   Keys.PadFire);

	// 롤은 게임패드에서 왼쪽 스틱 좌우로 들어옵니다. (docs/03 §3.4)
	const float PadRoll = PC.GetInputAnalogKeyState(EKeys::Gamepad_LeftX);
	if (IsDown(PC, Keys.RollLeft)  || PadRoll <= -AnalogThreshold) { Flags |= static_cast<uint8>(EFPGInputFlags::RollLeft); }
	if (IsDown(PC, Keys.RollRight) || PadRoll >=  AnalogThreshold) { Flags |= static_cast<uint8>(EFPGInputFlags::RollRight); }

	// 좌우를 동시에 누르면 상쇄합니다. 한쪽만 살리면 키보드 롤오버에 따라
	// 결과가 달라져 재현이 어려운 버그가 됩니다.
	constexpr uint8 BothRoll = static_cast<uint8>(EFPGInputFlags::RollLeft) | static_cast<uint8>(EFPGInputFlags::RollRight);
	if ((Flags & BothRoll) == BothRoll)
	{
		Flags &= ~BothRoll;
	}

	// W/S 동시 입력도 같은 이유로 상쇄합니다.
	constexpr uint8 BothThrottle = static_cast<uint8>(EFPGInputFlags::Thrust) | static_cast<uint8>(EFPGInputFlags::Brake);
	if ((Flags & BothThrottle) == BothThrottle)
	{
		Flags &= ~BothThrottle;
	}

	return Flags;
}
