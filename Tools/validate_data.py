#!/usr/bin/env python3
"""FPG 밸런스 데이터 검증 — ValidateAll()의 선행 구현.

docs/17 §17.14의 검증 규칙 10종 중 **데이터만으로 판정 가능한 것**을 구현합니다.
엔진 프로젝트가 생기기 전부터 CI에서 돌려 CSV 오류를 즉시 잡는 것이 목적입니다.

엔진이 들어오면 이 규칙들을 C++ `UFPGDataRegistry::ValidateAll()`로 옮기고,
에셋·문자열 테이블이 필요한 V3·V4·V7을 추가하십시오.
그때까지 이 스크립트가 유일한 안전망입니다.

사용법:
    python Tools/validate_data.py                 # 저장소 루트에서
    python Tools/validate_data.py --dir <경로>     # CSV 폴더 직접 지정

종료 코드: 0 = 전부 통과, 1 = 하나 이상 실패
"""

from __future__ import annotations

import argparse
import csv
import io
import sys
from pathlib import Path

# ── docs/17 §17.2 열거형 ──────────────────────────────────────
ITEM_GRADE = {"Common", "Rare", "Epic", "Legendary"}
ITEM_CATEGORY = {"Gun", "Attack", "Defense", "Utility", "RaceOnly"}
MODE_MASK = {"All", "BattleOnly", "SpeedOnly", "SingleOnly"}
POI_TYPE = {"AircraftStore", "WeaponShop", "RepairBay"}
GAME_MODE = {
    "SingleEndurance", "SingleTimeAttack",
    "SpeedSolo", "SpeedTeam", "BattleFFA", "BattleTeam",
}

# 기체 스탯 5종 — 합계가 반드시 100 (P8: P2W 방지)
AIRCRAFT_STATS = ("Speed", "Accel", "Turn", "Durability", "BoostPower")

EXPECTED_TABLES = [
    "DT_Aircraft", "DT_Item", "DT_Module", "DT_Poi", "DT_Map", "DT_Mode",
    "DT_Economy", "DT_Progression", "DT_LevelCurve", "DT_ItemDropRate",
    "DT_Enemy", "DT_DifficultyScaling", "DT_Flight",
]

# DT_Flight가 반드시 담고 있어야 하는 키. SimulateMove()가 전부 참조하므로
# 하나라도 빠지면 비행이 기본값으로 조용히 돌아갑니다 (P5 위반).
REQUIRED_FLIGHT_KEYS = {
    "CRUISE_SPEED", "ACCEL_THRUST", "DECEL_BRAKE", "THROTTLE_RATE",
    "ROLL_MAX_ANGLE", "BANK_TURN_RATE_AT_30DEG", "BANK_TURN_RATE_AT_75DEG",
    "ATTITUDE_INERTIA_SEC", "SPEED_LOSS_ROLL_PCT", "SPEED_LOSS_VECTOR_PCT",
    "SPEED_LOSS_MAX_PCT", "DIVE_ACCEL_BONUS", "DIVE_ACCEL_REF_PITCH",
    "STALL_SPEED", "STALL_ENTER_SEC", "STALL_RECOVER_SPEED",
    "STALL_INPUT_SCALE", "STALL_NOSE_DOWN_RATE",
    "ALTITUDE_CEILING_M", "ALTITUDE_CEILING_POWER_MULT",
}


class Report:
    """규칙별 결과를 모아 한 번에 출력합니다."""

    def __init__(self) -> None:
        self.failures: list[tuple[str, str]] = []
        self.skipped: list[tuple[str, str]] = []
        self.passed: list[str] = []

    def fail(self, rule: str, message: str) -> None:
        self.failures.append((rule, message))

    def skip(self, rule: str, reason: str) -> None:
        self.skipped.append((rule, reason))

    def ok(self, rule: str) -> None:
        self.passed.append(rule)

    def finish(self, rule: str) -> None:
        """해당 규칙에서 실패가 하나도 안 나왔으면 통과로 기록합니다."""
        if not any(r == rule for r, _ in self.failures):
            self.ok(rule)


def load_csv(path: Path, report: Report) -> list[dict[str, str]]:
    """CSV를 읽고 파일 수준 규약을 검사합니다."""
    raw = path.read_bytes()

    # F1. UTF-8 BOM 금지.
    #     git은 BOM을 정규화해 주지 않고, BOM이 있으면 첫 열 이름이
    #     "﻿Id"가 되어 언리얼 임포트와 이 스크립트가 모두 깨집니다.
    #     Excel에서 "CSV UTF-8"로 저장하면 BOM이 붙으므로 실제로 자주 발생합니다.
    if raw.startswith(b"\xef\xbb\xbf"):
        report.fail("F1", f"{path.name}: UTF-8 BOM이 있습니다. BOM 없이 저장하십시오")
        raw = raw[3:]

    text = raw.decode("utf-8")

    # C1. '#' 주석 행 금지.
    #     docs/17 §17.1은 '#' 주석을 규약으로 두지만, 언리얼 내장 DataTable
    #     임포터는 첫 행을 무조건 헤더로 읽습니다. 선행 주석이 있으면
    #     에디터에서만 깨지고 CI는 통과하는 최악의 조합이 되므로 여기서 막습니다.
    #     (커스텀 로더를 도입하기로 결정하면 이 검사를 제거하십시오)
    for i, line in enumerate(text.splitlines(), start=1):
        if line.startswith("#"):
            report.fail(
                "C1",
                f"{path.name}:{i}: '#' 주석 행은 언리얼 내장 임포터가 헤더로 읽어 깨집니다",
            )

    rows = list(csv.DictReader(io.StringIO(text)))

    # 줄바꿈(CRLF)은 검사하지 않습니다. .gitattributes의 `* text=auto eol=lf`가
    # 커밋 시 정규화하므로 작업 트리 상태로 실패시키면 거짓 양성이 됩니다.
    return rows


def to_int(value: str, default: int | None = None) -> int | None:
    try:
        return int(value)
    except (TypeError, ValueError):
        return default


def to_float(value: str, default: float | None = None) -> float | None:
    try:
        return float(value)
    except (TypeError, ValueError):
        return default


# ── docs/17 §17.14 규칙 ───────────────────────────────────────

def rule_v1_unique_ids(tables: dict[str, list[dict]], report: Report) -> None:
    """V1. 모든 Id가 표 안에서 유일한가."""
    for name, rows in tables.items():
        if not rows:
            continue
        key = "Id" if "Id" in rows[0] else ("Context" if "Context" in rows[0]
                                            else ("Level" if "Level" in rows[0] else None))
        if key is None:
            report.fail("V1", f"{name}: 기본 키 열(Id/Context/Level)을 찾을 수 없습니다")
            continue
        seen: dict[str, int] = {}
        for i, row in enumerate(rows, start=2):  # 2 = 헤더 다음 행
            v = row.get(key, "")
            if v in seen:
                report.fail("V1", f"{name}:{i}: {key}='{v}' 중복 (최초 {seen[v]}행)")
            else:
                seen[v] = i
    report.finish("V1")


def rule_v2_stat_sum(tables: dict[str, list[dict]], report: Report) -> None:
    """V2. DT_Aircraft의 스탯 5개 합이 정확히 100인가.

    P8(P2W 금지)의 기계적 방어선입니다. 이 검사가 없으면 상위호환 기체가
    조용히 들어오고, 멀티 게임에서 그것은 리뷰를 파괴합니다.
    """
    for i, row in enumerate(tables.get("DT_Aircraft", []), start=2):
        vals = [to_int(row.get(s, "")) for s in AIRCRAFT_STATS]
        if any(v is None for v in vals):
            report.fail("V2", f"DT_Aircraft:{i}: {row.get('Id')} 스탯에 숫자가 아닌 값")
            continue
        total = sum(vals)  # type: ignore[arg-type]
        if total != 100:
            detail = ", ".join(f"{s}={v}" for s, v in zip(AIRCRAFT_STATS, vals))
            report.fail("V2", f"DT_Aircraft:{i}: {row.get('Id')} 스탯 합 {total} (기대 100) — {detail}")
    report.finish("V2")


def rule_v5_supported_modes(tables: dict[str, list[dict]], report: Report) -> None:
    """V5. DT_Map.SupportedModes의 모드가 DT_Mode에 존재하는가."""
    known = {r.get("Id", "") for r in tables.get("DT_Mode", [])}
    if not known:
        report.skip("V5", "DT_Mode가 비어 있어 검사 불가")
        return
    for i, row in enumerate(tables.get("DT_Map", []), start=2):
        for mode in filter(None, row.get("SupportedModes", "").split("|")):
            if mode not in known:
                report.fail("V5", f"DT_Map:{i}: {row.get('Id')} 가 존재하지 않는 모드 '{mode}' 참조")
    report.finish("V5")


def rule_v6_drop_rate(tables: dict[str, list[dict]], report: Report) -> None:
    """V6. DT_ItemDropRate 각 행의 합이 100인가."""
    cols = ("Common", "Rare", "Epic", "Legendary")
    for i, row in enumerate(tables.get("DT_ItemDropRate", []), start=2):
        vals = [to_int(row.get(c, "")) for c in cols]
        if any(v is None for v in vals):
            report.fail("V6", f"DT_ItemDropRate:{i}: {row.get('Context')} 에 숫자가 아닌 값")
            continue
        total = sum(vals)  # type: ignore[arg-type]
        if total != 100:
            report.fail("V6", f"DT_ItemDropRate:{i}: {row.get('Context')} 합 {total} (기대 100)")
    report.finish("V6")


def rule_v8_unlock_level(tables: dict[str, list[dict]], report: Report) -> None:
    """V8. UnlockLevel이 DT_LevelCurve 범위 안인가.

    UnlockLevel=0은 '기본 제공'을 뜻하므로 곡선에 없는 것이 정상입니다(D-14).
    """
    levels = {r.get("Level", "") for r in tables.get("DT_LevelCurve", [])}
    if not levels:
        report.skip("V8", "DT_LevelCurve가 비어 있어 검사 불가")
        return
    for name in ("DT_Aircraft", "DT_Module"):
        for i, row in enumerate(tables.get(name, []), start=2):
            ul = row.get("UnlockLevel", "")
            if ul == "0":
                continue  # 기본 제공
            if ul not in levels:
                report.fail("V8", f"{name}:{i}: {row.get('Id')} UnlockLevel={ul} 이 DT_LevelCurve에 없음")
    report.finish("V8")


def rule_v9_speed_order(tables: dict[str, list[dict]], report: Report) -> None:
    """V9. MinSpeed < MaxSpeed < BoostSpeed 인가."""
    for i, row in enumerate(tables.get("DT_Aircraft", []), start=2):
        mn, mx, bs = (to_float(row.get(k, "")) for k in ("MinSpeed", "MaxSpeed", "BoostSpeed"))
        if None in (mn, mx, bs):
            report.fail("V9", f"DT_Aircraft:{i}: {row.get('Id')} 속도 값에 숫자가 아닌 값")
            continue
        if not (mn < mx < bs):  # type: ignore[operator]
            report.fail(
                "V9",
                f"DT_Aircraft:{i}: {row.get('Id')} 속도 순서 위반 "
                f"(Min={mn}, Max={mx}, Boost={bs})",
            )
    report.finish("V9")


def rule_v10_mode_mask(tables: dict[str, list[dict]], report: Report) -> None:
    """V10. ModeMask가 SpeedOnly인 아이템이 전투 드롭에 섞이지 않는가 — **대리 검사**.

    원 규칙은 '전투 드롭 테이블'을 대상으로 하지만, 현재 DT_ItemDropRate는
    아이템별이 아니라 (Context x Grade) 확률표라 직접 검사할 대상이 없습니다.
    드롭 로직이 구현되면 원 규칙으로 교체하십시오.

    대신 지금 확인 가능한 정합성을 봅니다: Category=RaceOnly 인 아이템은
    ModeMask도 SpeedOnly 여야 합니다. 둘이 어긋나면 레이스 전용 아이템이
    전투에 등장하거나 그 반대가 됩니다.
    """
    for i, row in enumerate(tables.get("DT_Item", []), start=2):
        cat, mask = row.get("Category", ""), row.get("ModeMask", "")
        if cat == "RaceOnly" and mask != "SpeedOnly":
            report.fail("V10", f"DT_Item:{i}: {row.get('Id')} Category=RaceOnly 인데 ModeMask={mask}")
        if mask == "SpeedOnly" and cat != "RaceOnly":
            report.fail("V10", f"DT_Item:{i}: {row.get('Id')} ModeMask=SpeedOnly 인데 Category={cat}")
    report.finish("V10")


# ── 추가 검사 (docs/17에 없지만 데이터만으로 가능한 것) ────────

def extra_enums(tables: dict[str, list[dict]], report: Report) -> None:
    """E1. 열거형 열의 값이 docs/17 §17.2에 정의된 것인가.

    열거형을 문자열로 저장하기로 한 결정(§17.2)의 대가는 오타가 컴파일에
    잡히지 않는다는 것입니다. 그 구멍을 여기서 막습니다.
    """
    checks = [
        ("DT_Item", "Category", ITEM_CATEGORY),
        ("DT_Item", "Grade", ITEM_GRADE),
        ("DT_Item", "ModeMask", MODE_MASK),
        ("DT_Poi", "Type", POI_TYPE),
    ]
    for table, col, allowed in checks:
        for i, row in enumerate(tables.get(table, []), start=2):
            v = row.get(col, "")
            if v not in allowed:
                report.fail("E1", f"{table}:{i}: {row.get('Id')} {col}='{v}' 는 정의되지 않은 값")

    for i, row in enumerate(tables.get("DT_Mode", []), start=2):
        v = row.get("Id", "")
        if v not in GAME_MODE:
            report.fail("E1", f"DT_Mode:{i}: Id='{v}' 가 EGameMode에 없음")
    report.finish("E1")


def extra_level_curve(tables: dict[str, list[dict]], report: Report) -> None:
    """E2. 레벨 곡선의 내부 정합성 (D-14).

    docs/17의 최초 표본은 RequiredXP와 CumulativeXP가 서로 모순이었습니다.
    같은 일이 재발하면 언락 시점이 조용히 어긋나므로 기계적으로 막습니다.
    """
    rows = tables.get("DT_LevelCurve", [])
    if not rows:
        report.skip("E2", "DT_LevelCurve가 비어 있어 검사 불가")
        return

    running = 0
    prev_level = None
    prev_required = None
    for i, row in enumerate(rows, start=2):
        level = to_int(row.get("Level", ""))
        required = to_int(row.get("RequiredXP", ""))
        cumulative = to_int(row.get("CumulativeXP", ""))
        if None in (level, required, cumulative):
            report.fail("E2", f"DT_LevelCurve:{i}: 숫자가 아닌 값")
            continue

        # 레벨 번호가 1부터 빠짐없이 이어지는가
        if prev_level is not None and level != prev_level + 1:
            report.fail("E2", f"DT_LevelCurve:{i}: 레벨 {prev_level} 다음이 {level} — 중간이 비었습니다")
        prev_level = level

        # 곡선이 역행하지 않는가
        if prev_required is not None and required < prev_required:  # type: ignore[operator]
            report.fail("E2", f"DT_LevelCurve:{i}: L{level} 요구 XP {required} 가 이전 {prev_required} 보다 작음")
        prev_required = required

        # 누적이 실제 합과 일치하는가
        running += required  # type: ignore[operator]
        if running != cumulative:
            report.fail("E2", f"DT_LevelCurve:{i}: L{level} 누적 {cumulative} (계산값 {running})")

    if rows and to_int(rows[0].get("Level", "")) != 1:
        report.fail("E2", "DT_LevelCurve: 첫 행이 Level=1 이 아닙니다")
    report.finish("E2")


def extra_flight_keys(tables: dict[str, list[dict]], report: Report) -> None:
    """E4. DT_Flight에 SimulateMove()가 참조하는 키가 전부 있는가.

    빠진 키가 있으면 FFPGFlightParams의 하드코딩 기본값이 조용히 쓰입니다.
    그러면 CSV를 고쳐도 비행이 안 변하는, 원인 찾기 어려운 상황이 됩니다. (P5)
    """
    rows = tables.get("DT_Flight", [])
    if not rows:
        report.skip("E4", "DT_Flight가 비어 있어 검사 불가")
        return

    present = {r.get("Id", "") for r in rows}
    for key in sorted(REQUIRED_FLIGHT_KEYS - present):
        report.fail("E4", f"DT_Flight: '{key}' 누락 — SimulateMove()가 참조하는 값입니다")
    for key in sorted(present - REQUIRED_FLIGHT_KEYS):
        report.fail("E4", f"DT_Flight: '{key}' 는 코드가 읽지 않는 값입니다 (오타 또는 미사용)")

    for i, row in enumerate(rows, start=2):
        if to_float(row.get("Value", "")) is None:
            report.fail("E4", f"DT_Flight:{i}: {row.get('Id')} Value가 숫자가 아님")
    report.finish("E4")


def extra_numeric_sanity(tables: dict[str, list[dict]], report: Report) -> None:
    """E3. 음수가 들어오면 안 되는 열에 음수가 있는가.

    Damage와 Ammo는 예외입니다. Damage 음수 = 회복(§17.4 규약),
    Ammo -1 = 무한.
    """
    non_negative = [
        ("DT_Item", ("Cooldown", "Range", "ProjectileSpeed", "Duration", "Radius",
                     "ShopPrice", "WarnBeforeFire")),
        ("DT_Aircraft", ("BaseHP", "BoostDuration", "BoostCooldown", "ExtraItemSlots",
                         "UnlockLevel", "StorePrice_Same", "StorePrice_Upgrade")),
        ("DT_Enemy", ("HP", "Damage", "FireRate", "DetectRange", "AggroTime",
                      "SpawnFromKm", "CreditReward")),
        ("DT_Poi", ("DockSpeedLimit", "DockTimeSec", "BaseRepairCost",
                    "RepairCostMultiplier", "BoostRefillCost", "AmmoRefillCost")),
    ]
    for table, cols in non_negative:
        for i, row in enumerate(tables.get(table, []), start=2):
            for col in cols:
                v = to_float(row.get(col, ""))
                if v is not None and v < 0:
                    report.fail("E3", f"{table}:{i}: {row.get('Id')} {col}={v} 가 음수")

    for i, row in enumerate(tables.get("DT_Item", []), start=2):
        ammo = to_int(row.get("Ammo", ""))
        if ammo is not None and ammo < -1:
            report.fail("E3", f"DT_Item:{i}: {row.get('Id')} Ammo={ammo} (무한은 -1)")
    report.finish("E3")


def main() -> int:
    parser = argparse.ArgumentParser(description="FPG 밸런스 데이터 검증")
    parser.add_argument("--dir", type=Path, default=None, help="CSV 폴더 (기본: Config/DataTables)")
    args = parser.parse_args()

    root = Path(__file__).resolve().parent.parent
    data_dir = args.dir or (root / "Config" / "DataTables")

    if not data_dir.is_dir():
        print(f"[오류] 데이터 폴더를 찾을 수 없습니다: {data_dir}", file=sys.stderr)
        return 1

    report = Report()

    tables: dict[str, list[dict]] = {}
    for name in EXPECTED_TABLES:
        path = data_dir / f"{name}.csv"
        if not path.is_file():
            report.fail("T0", f"{name}.csv 가 없습니다 ({data_dir})")
            tables[name] = []
            continue
        tables[name] = load_csv(path, report)
    report.finish("T0")
    report.finish("F1")
    report.finish("C1")

    for rule in (
        rule_v1_unique_ids, rule_v2_stat_sum, rule_v5_supported_modes,
        rule_v6_drop_rate, rule_v8_unlock_level, rule_v9_speed_order,
        rule_v10_mode_mask, extra_enums, extra_level_curve, extra_flight_keys,
        extra_numeric_sanity,
    ):
        rule(tables, report)

    # 엔진·에셋이 있어야 하는 규칙은 건너뛴 사실을 명시적으로 남깁니다.
    report.skip("V3", "EffectClass/AbilityClass/BehaviorTree — Content/ 필요 (M2)")
    report.skip("V4", "MeshPath/IconPath/PreviewPath — 에셋 필요 (M2)")
    report.skip("V7", "NameKey 4개 언어 — Localization/ 필요 (M2)")

    # ── 출력 ──
    print(f"FPG 데이터 검증 — {data_dir}")
    print(f"표 {len([t for t in tables.values() if t])}/{len(EXPECTED_TABLES)}종 로드\n")

    for rule in sorted(set(report.passed)):
        print(f"  [통과] {rule}")
    for rule, reason in report.skipped:
        print(f"  [보류] {rule} — {reason}")

    if report.failures:
        print(f"\n실패 {len(report.failures)}건:")
        for rule, message in report.failures:
            print(f"  [{rule}] {message}")
        print("\n검증 실패 — 빌드를 통과시키지 않습니다.")
        return 1

    print("\n전 규칙 통과.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
