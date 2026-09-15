// The pure flight step. Client and server run this exact function (docs/16 §16.5, FPG.md P4).
#pragma once

#include "CoreMinimal.h"
#include "Flight/FPGFlightTypes.h"

/**
 * Deterministic flight integration.
 *
 * Contract - do not break these, M3 prediction depends on them:
 *   - No world access, no actor access, no RNG, no wall-clock or frame-count reads.
 *   - Output depends only on (In, Move, Config).
 *   - Same inputs always produce bit-identical output.
 *
 * Collision is deliberately NOT handled here; the movement component sweeps the result.
 */
namespace FPGFlightSimulation
{
	FPG_API FFPGMoveState Step(const FFPGMoveState& In, const FFPGMove& Move, const FFPGFlightConfig& Config);

	/** Largest delta time a single step will integrate. Longer frames are clamped. */
	FPG_API float GetMaxStepDeltaTime();
}
