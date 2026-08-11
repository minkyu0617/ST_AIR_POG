# Config/DataTables

밸런스 수치 원본입니다. **정본 스키마는 [`docs/17_data_schema.md`](../../docs/17_data_schema.md)** 이고, 이 폴더는 그것을 실제 파일로 배치한 것입니다.

> 🔴 **P5 — 밸런스 수치는 코드에 하드코딩하지 않습니다.** 새 수치가 필요하면 C++ 상수가 아니라 이 폴더에 열을 추가하십시오.
> 밸런싱은 출시 후 수백 번 반복되며, 매번 컴파일과 재패키징이 필요하면 밸런스 패치 주기가 주 단위가 되고 그동안 유저는 떠납니다.

## 파일 목록

| 파일 | 내용 | 근거 문서 |
|---|---|---|
| `DT_Aircraft.csv` | 기체 8종 | [07 §7.2](../../docs/07_progression_economy.md), [02 §2.2](../../docs/02_core_gameplay.md) |
| `DT_Item.csv` | 무기·아이템 16종 | [06](../../docs/06_combat_items.md) |
| `DT_Module.csv` | 부품 6종 | [07](../../docs/07_progression_economy.md) |
| `DT_Poi.csv` | 상점 POI 3종 | [05](../../docs/05_map_poi_minimap.md) |
| `DT_Map.csv` | 맵 8종 | [05](../../docs/05_map_poi_minimap.md) |
| `DT_Mode.csv` | 모드 규칙 5종 | [04](../../docs/04_game_modes.md) |
| `DT_Economy.csv` | 세션 크레딧 획득량 | [07](../../docs/07_progression_economy.md) |
| `DT_Progression.csv` | XP 획득량 | [07](../../docs/07_progression_economy.md) |
| `DT_LevelCurve.csv` | 레벨 곡선 30행 (총 88,340 XP) | [07](../../docs/07_progression_economy.md), [14 D-14](../../docs/14_open_questions.md) |
| `DT_ItemDropRate.csv` | 역전 보정 드롭률 | [06 §6.4](../../docs/06_combat_items.md) |
| `DT_Enemy.csv` | 싱글 적기 4종 | [04](../../docs/04_game_modes.md) |
| `DT_DifficultyScaling.csv` | 50km 이후 무한 스케일링 | [04](../../docs/04_game_modes.md) |

## 파일 규약

| 항목 | 규칙 |
|---|---|
| 인코딩 | **UTF-8, BOM 없음** |
| 줄바꿈 | **LF** (`.gitattributes`가 강제) |
| 첫 행 | **반드시 헤더.** 주석 행을 앞에 두지 마십시오 — 아래 참조 |
| ID 형식 | 대문자 스네이크 (`AIRCRAFT_FALCON`) |
| 표시 문자열 | **넣지 않습니다.** `NameKey`만 두고 실제 문구는 문자열 테이블에서 |
| LFS | **넣지 않습니다.** diff를 읽어야 리뷰가 가능합니다 |

### ⚠️ 주석 행에 대하여

`docs/17 §17.1`은 "`#`으로 시작하는 행은 무시"를 규약으로 정하고 있지만, **이 폴더의 CSV에는 `#` 주석 행을 넣지 않았습니다.** 언리얼 내장 DataTable CSV 임포터는 첫 행을 무조건 헤더로 읽기 때문에, 선행 `#` 행이 있으면 임포트가 깨집니다.

`docs/17`의 코드 블록 첫 줄에 있는 `# DT_Aircraft.csv` 같은 표기는 **문서에서 파일 이름을 알려주는 라벨**이지 파일 내용이 아닙니다.

→ `#` 주석을 실제로 지원하려면 커스텀 로더가 필요합니다. `ValidateAll()` 구현 시 결정하십시오.

## ✅ 해결된 공백

### `DT_LevelCurve.csv` — 30행 완성 (2026-08-11, D-14)

배치 시점에는 8개 표본만 있었고 `RequiredXP`와 `CumulativeXP`가 **서로 모순**이었습니다. `RequiredXP` 앵커 7개를 고정하고 나머지를 보간한 뒤 누적을 전량 재계산했습니다. → [14 D-14](../../docs/14_open_questions.md)

| 구간 | 평균 XP/레벨 | 총량 대비 |
|---|---|---|
| 초반 L2~L10 | 941 | 9.6% |
| 중반 L11~L20 | 2,775 | 31.4% |
| 후반 L21~L30 | 5,212 | 59.0% |

총 **88,340 XP** ≈ 40~44시간(추정). **M1 텔레메트리로 교정 필요** — 교정 시 곡선이 아니라 `DT_Progression`의 획득량을 조정하십시오.

`UnlockLevel=0`은 "기본 제공"을 뜻하므로 곡선에 없는 것이 정상입니다 — **V8 구현 시 0을 예외 처리**하십시오.

## 🔴 채워야 할 공백 (M0 작업)

배치하면서 확인된 것입니다. **코드 작성 전에 메우십시오.**

### 1. `DT_Aircraft.csv` — 에셋 경로 열이 비어 있음

`docs/17 §17.3`의 열 정의에는 `DescKey` / `MeshPath` / `IconPath`가 있으나 예시 CSV에는 빠져 있었습니다. **열은 추가하고 값은 비워 뒀습니다** — 에셋이 아직 없기 때문입니다(M0 시점 코드 0줄).

→ **V4 검증은 "값이 비어 있지 않으면 존재해야 한다"로 구현하십시오.** "무조건 존재해야 한다"로 만들면 M1 내내 빌드가 실패합니다. 에셋이 들어오는 M2에 규칙을 강화하십시오.

### 2. `DT_Mode.csv` — `SingleEndurance`의 `RespawnDelay=0`

`Lives=1`이라 부활이 없어 무의미한 값입니다. D-06(레이스 격추 시 5초 후 체크포인트 부활)이 확정되면 `SpeedSolo`/`SpeedTeam`의 `RespawnDelay=5.0`과 정합성을 확인하십시오.

## 검증

`ValidateAll()`이 CI와 에디터 시작 시 자동 실행되며, 하나라도 실패하면 **빌드를 통과시키지 않습니다.** 규칙 10종은 [`docs/17 §17.14`](../../docs/17_data_schema.md)에 있습니다.

배치 시점에 수동 확인한 것:

| 규칙 | 결과 |
|---|---|
| **V1** 표 안에서 `Id` 유일 | ✅ 전 파일 통과 |
| **V2** 기체 스탯 5개 합 = 100 | ✅ 8종 전부 100 (P8 — P2W 방지의 핵심) |
| **V6** 드롭률 각 행 합 = 100 | ✅ 4행 전부 100 |
| **V9** `MinSpeed < MaxSpeed < BoostSpeed` | ✅ 8종 전부 통과 |
| **V8** `UnlockLevel`이 곡선 범위 안 | ✅ 통과 (기체 8종 + 부품 6종, `UnlockLevel=0` 예외 처리) |
| 곡선 단조 증가 · 누적 정합성 | ✅ 30행 전부, 불일치 0건 |
| V3·V4·V5·V7·V10 | ⏸ 에셋·문자열 테이블·코드가 없어 검증 불가 |
