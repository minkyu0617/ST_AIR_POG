# 17. 데이터 스키마

> [16번 문서](16_architecture.md)가 **"코드를 어떻게 나눌 것인가"** 라면, 이 문서는 **"데이터를 어떤 모양으로 담을 것인가"** 입니다.
> 여기 정의된 CSV는 그대로 `Config/DataTables/`에 넣어 개발을 시작할 수 있습니다.

---

## 17.1 공통 규약

| 항목 | 규칙 |
|---|---|
| 파일 위치 | `Config/DataTables/*.csv` |
| 인코딩 | UTF-8 (BOM 없음) |
| ID 형식 | `대문자_스네이크` (예: `AIRCRAFT_FALCON`, `ITEM_GUIDED_MISSILE`) |
| 참조 무결성 | 다른 표의 ID를 참조하는 열은 `ValidateAll()`이 검사 → [16 §16.10](16_architecture.md) |
| 표시 문자열 | **CSV에 넣지 않는다.** ID만 두고 실제 문구는 문자열 테이블에서 (다국어 대응) |
| 주석 | `#`으로 시작하는 행은 무시 |
| 시간 단위 | 초(sec), 거리 단위 | 언리얼 유닛(1 UU = 1 cm) |

> **표시 문자열 분리가 중요합니다.** `Name` 열에 "유도 미사일"을 직접 쓰면 영어판을 만들 때 표를 통째로 복제해야 합니다. `NameKey=ITEM_GUIDED_MISSILE_NAME`만 두고 번역은 별도 관리합니다.

---

## 17.2 열거형 (Enum)

C++와 CSV가 공유하는 값입니다. **문자열로 저장**하고 파싱 시 변환합니다(숫자는 나중에 순서가 바뀌면 데이터가 전부 어긋납니다).

```cpp
enum class EItemGrade   : uint8 { Common, Rare, Epic, Legendary };
enum class EItemCategory: uint8 { Gun, Attack, Defense, Utility, RaceOnly };
enum class EModeMask    : uint8 { All, BattleOnly, SpeedOnly, SingleOnly };
enum class EPoiType     : uint8 { AircraftStore, WeaponShop, RepairBay };
enum class EGameMode    : uint8 { SingleEndurance, SingleTimeAttack,
                                  SpeedSolo, SpeedTeam, BattleFFA, BattleTeam };
enum class EMatchPhase  : uint8 { WaitingForPlayers, Boarding, Countdown,
                                  InProgress, SuddenDeath, PostMatch };
enum class ERoomLockState:uint8 { Open, Locked, Starting, InGame };
enum class EFlightState : uint8 { Normal, Vector, Stall, Destroyed };
enum class EDamageState : uint8 { Healthy, Damaged, Critical, Dead };
enum class EAltitudeLayer:uint8 { Canyon, CloudSea, Skyport, Stratos };
```

---

## 17.3 DT_Aircraft — 기체

스탯 총합은 **100 고정**입니다([07 §7.2](07_progression_economy.md)). 상위호환 기체를 만들지 않기 위한 규칙이며, `ValidateAll()`이 합계를 검사해 어기면 빌드를 실패시킵니다.

| 열 | 타입 | 설명 |
|---|---|---|
| `Id` | Name | 기본 키 |
| `NameKey` / `DescKey` | Name | 문자열 테이블 키 |
| `Speed` / `Accel` / `Turn` / `Durability` / `BoostPower` | int | **합계 = 100** |
| `BaseHP` | float | 60~140. `Durability`에서 파생되지만 명시적으로 둠 |
| `MaxSpeed` / `MinSpeed` / `BoostSpeed` | float | u/s → [02 §2.2](02_core_gameplay.md) |
| `RollRate` / `PitchRate` / `YawRate` | float | °/s |
| `BoostDuration` / `BoostCooldown` | float | 초 |
| `ExtraItemSlots` | int | Hauler = 1, 나머지 0 |
| `AbilityClass` | SoftClassPath | 고유기. 없으면 비움 |
| `MeshPath` / `IconPath` | SoftObjectPath | |
| `UnlockLevel` | int | 0 = 기본 제공 |
| `StorePrice_Same` / `StorePrice_Upgrade` | int | 경기 중 교체 비용 |

```csv
# DT_Aircraft.csv
Id,NameKey,Speed,Accel,Turn,Durability,BoostPower,BaseHP,MaxSpeed,MinSpeed,BoostSpeed,RollRate,PitchRate,YawRate,BoostDuration,BoostCooldown,ExtraItemSlots,AbilityClass,UnlockLevel,StorePrice_Same,StorePrice_Upgrade
AIRCRAFT_FALCON,AIRCRAFT_FALCON_NAME,20,20,20,20,20,100,260,70,380,140,50,65,3.0,8.0,0,,0,150,300
AIRCRAFT_DART,AIRCRAFT_DART_NAME,32,24,14,12,18,70,310,85,430,120,44,52,3.0,8.5,0,/Game/Abilities/BP_AfterburnerDash.BP_AfterburnerDash_C,0,150,300
AIRCRAFT_BASTION,AIRCRAFT_BASTION_NAME,14,14,16,38,18,140,220,60,330,110,42,55,3.0,8.0,0,/Game/Abilities/BP_TempArmor.BP_TempArmor_C,5,150,350
AIRCRAFT_WHISPER,AIRCRAFT_WHISPER_NAME,18,18,36,14,14,75,245,62,355,190,72,95,2.5,9.0,0,/Game/Abilities/BP_SnapTurn.BP_SnapTurn_C,8,150,400
AIRCRAFT_HAULER,AIRCRAFT_HAULER_NAME,16,16,18,28,22,120,230,65,345,115,46,58,3.5,7.5,1,,12,150,450
AIRCRAFT_COMET,AIRCRAFT_COMET_NAME,24,18,16,12,30,70,275,72,410,125,46,60,4.0,4.0,0,,16,150,500
AIRCRAFT_VEX,AIRCRAFT_VEX_NAME,20,20,22,16,22,85,260,70,385,145,52,68,3.0,8.0,0,/Game/Abilities/BP_GunBoost.BP_GunBoost_C,22,150,550
AIRCRAFT_ZENITH,AIRCRAFT_ZENITH_NAME,26,16,20,16,22,85,285,68,395,130,48,62,3.0,8.0,0,/Game/Abilities/BP_HighAltitude.BP_HighAltitude_C,30,150,600
```

---

## 17.4 DT_Item — 무기 · 아이템

수치 근거는 [06번 문서](06_combat_items.md)입니다.

| 열 | 타입 | 설명 |
|---|---|---|
| `Id` | Name | |
| `Category` | Enum | Gun / Attack / Defense / Utility / RaceOnly |
| `Grade` | Enum | Common / Rare / Epic / Legendary |
| `EffectClass` | SoftClassPath | **필수.** 실제 동작 → [16 §16.8](16_architecture.md) |
| `Damage` | float | 없으면 0 |
| `Ammo` | int | −1 = 무한 |
| `Cooldown` | float | 연사 간격 |
| `Range` / `ProjectileSpeed` | float | 사거리 / 탄속. 0 = 히트스캔 |
| `RequiresLockOn` | bool | |
| `Duration` | float | 지속형 효과 |
| `Radius` | float | 범위형 효과 |
| `ModeMask` | Enum | 등장 모드 제한 |
| `ShopPrice` | int | 무기점 가격 |
| `WarnBeforeFire` | float | 상대에게 경고가 뜨는 선행 시간 (밸런싱 원칙 1) |

```csv
# DT_Item.csv
Id,Category,Grade,EffectClass,Damage,Ammo,Cooldown,Range,ProjectileSpeed,RequiresLockOn,Duration,Radius,ModeMask,ShopPrice,WarnBeforeFire
ITEM_VULCAN,Gun,Common,/Game/Effects/BP_HitscanGun.BP_HitscanGun_C,8,-1,0.083,90000,220000,false,0,0,All,0,0
ITEM_GUIDED_MISSILE,Attack,Common,/Game/Effects/BP_Projectile.BP_Projectile_C,30,3,2.0,150000,45000,true,0,0,All,40,0.3
ITEM_ROCKET_POD,Attack,Common,/Game/Effects/BP_Projectile.BP_Projectile_C,9,2,3.0,60000,60000,false,0,0,All,40,0
ITEM_LASER_LANCE,Attack,Rare,/Game/Effects/BP_ChargeBeam.BP_ChargeBeam_C,55,2,6.0,200000,0,false,1.5,0,All,90,1.5
ITEM_SKY_MINE,Attack,Rare,/Game/Effects/BP_DeployMine.BP_DeployMine_C,45,3,2.0,0,0,false,15.0,3000,All,90,0
ITEM_EMP_BURST,Attack,Epic,/Game/Effects/BP_EmpBurst.BP_EmpBurst_C,0,1,0,40000,0,false,3.0,40000,All,180,0.4
ITEM_DRONE_SWARM,Attack,Epic,/Game/Effects/BP_DroneSwarm.BP_DroneSwarm_C,60,1,0,80000,30000,false,12.0,0,All,180,0.4
ITEM_RAILGUN,Attack,Legendary,/Game/Effects/BP_Hitscan.BP_Hitscan_C,85,1,0,300000,0,false,0,0,All,400,0.4
ITEM_FLARE,Defense,Common,/Game/Effects/BP_Flare.BP_Flare_C,0,3,1.0,0,0,false,4.0,0,All,40,0
ITEM_REPAIR_PACK,Defense,Common,/Game/Effects/BP_HealOverTime.BP_HealOverTime_C,-35,2,0,0,0,false,3.0,0,All,40,0
ITEM_SHIELD_FIELD,Defense,Rare,/Game/Effects/BP_Invuln.BP_Invuln_C,0,1,0,0,0,false,2.5,0,All,90,0
ITEM_NITRO,Utility,Rare,/Game/Effects/BP_SpeedBuff.BP_SpeedBuff_C,0,1,0,0,0,false,8.0,0,All,90,0
ITEM_SMOKE_TRAIL,Utility,Common,/Game/Effects/BP_SmokeTrail.BP_SmokeTrail_C,0,2,1.0,0,0,false,15.0,5000,All,40,0
ITEM_WIND_DRAFT,RaceOnly,Rare,/Game/Effects/BP_SpeedBuff.BP_SpeedBuff_C,0,1,0,0,0,false,8.0,0,SpeedOnly,90,0
ITEM_TELEPORT_MARK,RaceOnly,Rare,/Game/Effects/BP_Rewind.BP_Rewind_C,0,1,0,0,0,false,4.0,0,SpeedOnly,90,0
ITEM_RANK_STEAL,RaceOnly,Epic,/Game/Effects/BP_RankSwap.BP_RankSwap_C,0,1,0,0,0,false,0,0,SpeedOnly,180,1.0
```

> `ITEM_REPAIR_PACK`의 `Damage=-35`는 **음수 = 회복** 규약입니다. 회복 전용 열을 따로 두지 않아 `ItemEffect`가 하나로 처리됩니다.

---

## 17.5 DT_Module — 부품

```csv
# DT_Module.csv — StatModifiers는 "스탯:배율" 세미콜론 구분
Id,NameKey,UnlockLevel,CoinPrice,InGamePrice,StatModifiers
MOD_LIGHT_FRAME,MOD_LIGHT_FRAME_NAME,3,800,200,Accel:1.08;Durability:0.88
MOD_ARMOR_PLATE,MOD_ARMOR_PLATE_NAME,3,800,200,Durability:1.15;Turn:0.92
MOD_WIDE_INTAKE,MOD_WIDE_INTAKE_NAME,6,1400,200,MaxSpeed:1.07;BoostDuration:0.80
MOD_PRECISION_SIGHT,MOD_PRECISION_SIGHT_NAME,9,1800,200,GunSpread:0.75;ItemDamage:0.90
MOD_FIRE_SUPPRESSOR,MOD_FIRE_SUPPRESSOR_NAME,12,2200,200,ImmuneBurning:1;MaxSpeed:0.95
MOD_EXT_MAGAZINE,MOD_EXT_MAGAZINE_NAME,15,2500,200,ItemAmmo:1;Accel:0.94
```

> 배율 방식(곱셈)을 쓰면 기체가 무엇이든 **비례해서** 적용됩니다. 덧셈 방식은 약한 기체에 과도하게 유리해져 밸런스가 깨집니다.

---

## 17.6 DT_Poi / DT_Map / DT_Mode

```csv
# DT_Poi.csv
Id,Type,NameKey,IconPath,DockSpeedLimit,DockTimeSec,BaseRepairCost,RepairCostMultiplier,BoostRefillCost,AmmoRefillCost
POI_AIRCRAFT_STORE,AircraftStore,POI_STORE_NAME,/Game/UI/Icons/T_Poi_Store,20000,3.0,0,1.0,0,0
POI_WEAPON_SHOP,WeaponShop,POI_WEAPON_NAME,/Game/UI/Icons/T_Poi_Weapon,20000,3.0,0,1.0,30,25
POI_REPAIR_BAY,RepairBay,POI_REPAIR_NAME,/Game/UI/Icons/T_Poi_Repair,20000,3.0,60,1.6,30,0
```

```csv
# DT_Map.csv
Id,NameKey,LevelPath,SupportedModes,LengthMeters,PoiCount,MinPlayers,MaxPlayers,PreviewPath,MusicId
MAP_CANYON_CORRIDOR,MAP_CANYON_NAME,/Game/Maps/M1_Canyon,SpeedSolo|SpeedTeam,14200,4,2,10,/Game/UI/Preview/T_M1,MUSIC_RACE_A
MAP_FLOATING_CITY,MAP_CITY_NAME,/Game/Maps/M2_City,SpeedSolo|SpeedTeam,16800,5,2,10,/Game/UI/Preview/T_M2,MUSIC_RACE_B
MAP_VOLCANO,MAP_VOLCANO_NAME,/Game/Maps/M3_Volcano,SpeedSolo|SpeedTeam,13500,3,2,10,/Game/UI/Preview/T_M3,MUSIC_RACE_A
MAP_ARCTIC,MAP_ARCTIC_NAME,/Game/Maps/M4_Arctic,SpeedSolo|SpeedTeam,15400,4,2,10,/Game/UI/Preview/T_M4,MUSIC_RACE_C
MAP_CLOUD_ARENA,MAP_ARENA_NAME,/Game/Maps/M5_Arena,BattleFFA|BattleTeam,0,4,2,10,/Game/UI/Preview/T_M5,MUSIC_BATTLE_A
MAP_STORM_EYE,MAP_STORM_NAME,/Game/Maps/M6_Storm,BattleFFA|BattleTeam,0,3,2,10,/Game/UI/Preview/T_M6,MUSIC_BATTLE_B
MAP_AIRBASE,MAP_AIRBASE_NAME,/Game/Maps/M7_Airbase,BattleFFA|BattleTeam,0,4,2,10,/Game/UI/Preview/T_M7,MUSIC_BATTLE_A
MAP_ENDLESS,MAP_ENDLESS_NAME,/Game/Maps/E1_Endless,SingleEndurance,0,0,1,1,/Game/UI/Preview/T_E1,MUSIC_SINGLE
```

```csv
# DT_Mode.csv — 모드 규칙 자체를 데이터화
Id,GameModeClass,Lives,RespawnDelay,InvulnDuration,TimeLimitSec,SuddenDeathAtSec,HardCutSec,TeamCountMin,TeamCountMax,AllowFriendlyFire,ScoreTable
SingleEndurance,/Script/SAPGame.SingleEnduranceGameMode,1,0,0,0,0,0,0,0,false,
SpeedSolo,/Script/SAPGame.SpeedRaceGameMode,-1,5.0,3.0,480,0,480,0,0,false,
SpeedTeam,/Script/SAPGame.SpeedRaceGameMode,-1,5.0,3.0,480,0,480,2,5,false,10;8;6;5;4;3;2;1
BattleFFA,/Script/SAPGame.BattleGameMode,3,8.0,3.0,360,360,480,0,0,false,
BattleTeam,/Script/SAPGame.BattleGameMode,-1,8.0,3.0,360,360,480,2,5,false,
```

> **모드 규칙을 데이터로 뺀 이유**: "부활 시간을 8초에서 6초로" 같은 조정이 출시 후 수십 번 발생합니다. `Lives=-1`은 무제한(팀 공유 풀 사용)을 뜻합니다.

---

## 17.7 DT_Economy / DT_Progression

```csv
# DT_Economy.csv — 세션 크레딧 획득량 (07번 문서)
Id,Value
CREDIT_PER_KM,15
CREDIT_PER_RING,10
CREDIT_PER_KILL,40
CREDIT_PER_ASSIST,16
CREDIT_RANK_BONUS_MAX,20
CREDIT_RANK_BONUS_MIN,5
CREDIT_RANK_BONUS_INTERVAL,30
```

```csv
# DT_Progression.csv — XP 및 레벨 곡선
Id,Value
XP_MATCH_COMPLETE,100
XP_WIN,150
XP_PER_KILL,25
XP_PER_KM_SINGLE,6
XP_NEW_RECORD,200
XP_DAILY_FIRST_WIN,300
```

```csv
# DT_LevelCurve.csv — 레벨 1~30, 누적 40시간 목표
Level,RequiredXP,CumulativeXP
1,0,0
2,400,400
3,500,900
5,750,2400
10,1600,10500
16,2900,29800
22,4300,60900
30,6500,128000
```

```csv
# DT_ItemDropRate.csv — 역전 보정 (06 §6.4)
Context,Common,Rare,Epic,Legendary
DEFAULT,50,33,14,3
RACE_RANK_TOP,70,25,5,0
RACE_RANK_MID,50,33,14,3
RACE_RANK_BOTTOM,30,35,27,8
```

---

## 17.8 DT_Enemy — 싱글 적기

```csv
# DT_Enemy.csv
Id,Tier,HP,Damage,FireRate,DetectRange,AggroTime,BehaviorTree,SpawnFromKm,CreditReward
ENEMY_INTERCEPTOR_T1,1,60,5,0.5,120000,8,/Game/AI/BT_Interceptor,5,40
ENEMY_INTERCEPTOR_T2,2,110,8,0.7,160000,12,/Game/AI/BT_Interceptor,15,60
ENEMY_INTERCEPTOR_T3,3,180,12,1.0,200000,15,/Game/AI/BT_Ace,30,90
ENEMY_SAM_SITE,2,250,20,0.3,180000,20,/Game/AI/BT_Static,30,70
```

```csv
# DT_DifficultyScaling.csv — 50km 이후 무한 스케일링 (04번 문서)
Id,Value
SCALE_START_KM,50
SCALE_STEP_KM,5
SCALE_HP_PER_STEP,0.08
SCALE_DENSITY_PER_STEP,0.05
SCALE_MAX_MULTIPLIER,3.0
```

---

## 17.9 세이브 데이터

### 로컬 세이브 (`SAPSaveGame`)
```cpp
USTRUCT()
struct FSAPSaveData
{
    int32   SaveVersion;              // 마이그레이션용. 반드시 첫 필드
    int32   PlayerLevel;
    int64   CurrentXP;
    int64   SkyCoin;
    TArray<FName> UnlockedAircraft;
    TArray<FName> UnlockedModules;
    TArray<FName> OwnedCosmetics;
    TMap<FName, FSAPLoadout>  Loadouts;      // 기체별 부품 구성
    TMap<FName, FSAPMapRecord> MapRecords;   // 맵별 최고 기록
    FSAPSettings Settings;                    // 키 바인딩, 그래픽, 접근성
    FDateTime LastDailyReset;
};
```

| 항목 | 정책 |
|---|---|
| 저장 시점 | 경기 종료 시, 설정 변경 시, 상점 구매 시 |
| 클라우드 | Steam Cloud 동기화 (설정 + 진행도) |
| **버전 관리** | `SaveVersion` 불일치 시 마이그레이션 함수 순차 적용 |
| 손상 대응 | 파싱 실패 시 백업 슬롯 → 실패 시 초기화 후 알림 |

> **`SaveVersion`을 처음부터 넣으십시오.** 나중에 필드를 추가할 때 이게 없으면 기존 유저의 세이브가 전부 깨지고, 출시 후에 그 일이 벌어지면 되돌릴 방법이 없습니다.

### 리더보드 제출 (`FSAPRecord`)
```cpp
struct FSAPRecord
{
    int64   DistanceCm;        // 정렬 키
    FName   MapId;
    int32   Seed;              // 주간 리더보드 검증용
    FName   AircraftId;
    int32   Kills;
    float   PlayTimeSec;
    TArray<uint8> InputStream; // 압축된 입력 기록 (<50KB) — 재시뮬 검증용
};
```

---

## 17.10 네트워크 메시지

### 복제 상태 (Replicated Properties)

| 클래스 | 속성 | 조건 |
|---|---|---|
| `GameState` | `MatchPhase`, `ServerStartTimeUtc`, `RemainingSec`, `AirspaceRadius`, `AirspaceCenter` | 전원 |
| `PlayerState` | `Nickname`, `TeamId`, `Kills`, `Deaths`, `Assists`, `Rank`, `SessionCredit`, `LivesLeft`, `AircraftId` | 전원 |
| `AircraftPawn` | `MoveState`, `HP`, `DamageState`, `SelectedSlot` | 전원 (거리별 빈도 차등) |
| `WeaponSlot` | `Slots[]` | **소유자만** (적에게 내 아이템을 노출하지 않음) |

### RPC 목록

| 방향 | 이름 | 신뢰성 | 설명 |
|---|---|---|---|
| C→S | `ServerSendMove(FSAPMove)` | Unreliable | 60Hz. 손실 대비 3프레임 중복 |
| S→C | `ClientAckMove(FSAPMoveState, Timestamp)` | Unreliable | 화해용 |
| C→S | `ServerFire(SlotIndex, AimDir, ClientTime)` | Reliable | 발사 요청 |
| S→All | `MulticastFireFX(PawnId, ItemId, Origin, Dir)` | Unreliable | 이펙트 |
| S→All | `MulticastKill(KillerId, VictimId, ItemId)` | Reliable | 킬피드 |
| C→S | `ServerPlacePing(WorldPos, PingType)` | Reliable | 핑 (2초 쿨다운 서버 검증) |
| S→Team | `ClientReceivePing(PlayerId, Pos, Type)` | Reliable | 팀원에게만 |
| C→S | `ServerBuyItem(PoiId, ItemId, SettingsVersion)` | Reliable | 상점 구매 |
| C→S | `ServerSetReady(bool, SettingsVersion)` | Reliable | 레디 토글 |
| C→S | `ServerChangeRoomSetting(FRoomSettingChange)` | Reliable | 방장 전용 |
| S→All | `ClientRoomStateUpdate(FRoomState)` | Reliable | 방 상태 |
| S→All | `ClientToast(FToastEvent)` | Reliable | 토스트 |

### 방 상태 구조체
```cpp
USTRUCT()
struct FSAPRoomState
{
    FString  RoomName;
    FString  JoinCode;            // 6자리
    int32    HostPlayerId;
    EGameMode Mode;
    FName    MapId;
    int32    TeamCount;
    ERoomLockState LockState;
    uint32   SettingsVersion;     // 경합 방지 → 16 §16.11
    TArray<FSAPRoomPlayer> Players;
};

USTRUCT()
struct FSAPRoomPlayer
{
    int32   PlayerId;
    FString Nickname;
    int32   TeamId;
    bool    bIsReady;
    int32   PingMs;
    FName   AircraftId;
};
```

### 토스트 이벤트
```cpp
USTRUCT()
struct FSAPToastEvent
{
    EToastType Type;      // MODE_CHANGED, MAP_CHANGED, PLAYER_JOIN ...
    FName   BeforeKey;    // 변경 전 (문자열 키)
    FName   AfterKey;     // 변경 후
    FString PlayerName;   // 입장/퇴장/방장 위임용
};
```
→ 표시 규격은 [09 §9.3](09_ui_ux_flow.md)

---

## 17.11 이동 동기화 구조체

```cpp
// 클라이언트 → 서버 (입력)
USTRUCT()
struct FSAPMove
{
    float   ClientTimestamp;
    uint8   InputFlags;      // 비트: W,A,S,D,Space,Ctrl,Fire
    FVector2D AimDelta;      // 마우스 델타
    uint8   SelectedSlot;
};

// 서버 → 클라이언트 (권위 상태)
USTRUCT()
struct FSAPMoveState
{
    float     Timestamp;     // 어느 Move까지 처리했는지
    FVector_NetQuantize100 Location;
    FQuat     Rotation;
    FVector_NetQuantize100 Velocity;
    float     Throttle;
    float     BoostRemaining;
    EFlightState State;
};
```

- `InputFlags`를 **비트 플래그로 압축**해 60Hz 전송의 대역폭을 줄입니다.
- `FVector_NetQuantize100`은 cm 단위 정밀도로 압축 전송하는 Unreal 내장 타입입니다.
- **`FSAPMove`에 위치를 넣지 않습니다.** 위치를 클라이언트가 보내면 텔레포트핵이 가능해집니다. **입력만 보내고 위치는 서버가 계산**합니다.

---

## 17.12 텔레메트리 이벤트

밸런싱의 유일한 객관적 근거입니다([10 §10.6](10_tech_stack.md)). **개인 식별 정보를 담지 않으며**, 옵트아웃을 제공합니다.

| 이벤트 | 필드 | 답하려는 질문 |
|---|---|---|
| `match_start` | mode, map, playerCount, aircraftId | 어떤 모드/기체가 실제로 선택되는가 |
| `match_end` | mode, map, durationSec, rank, kills, deaths | 세션 길이가 목표와 맞는가 |
| `poi_visit` | poiType, mapId, atSec, creditSpent | **POI 딜레마 가설이 작동하는가** ← 최우선 |
| `poi_skip` | poiType, distanceFromPath | 왜 안 들르는가 |
| `player_death` | cause, killerItemId, atSec, positionLayer | 어디서 죽는가 |
| `item_use` | itemId, hit(bool), targetDistance | 어떤 아이템이 안 쓰이는가 |
| `session_quit` | phase, atSec, mapProgress | **어디서 이탈하는가** ← 온보딩 개선 근거 |
| `tutorial_step` | stepId, completed, retries | 어느 조작에서 막히는가 |
| `settings_changed` | key, value | 어떤 옵션이 실제로 필요한가 |
| `fps_sample` | avgFps, gpuTier, resolution | 최적화 우선순위 |

> **`poi_visit` / `poi_skip` / `session_quit` 세 가지는 v1.0부터 반드시 수집**하십시오. 각각 이 게임의 핵심 설계 가설, 밸런스 문제, 온보딩 실패 지점을 직접 측정합니다.

---

## 17.13 문자열 테이블

```csv
# Localization/StringTable_KO.csv
Key,Text
AIRCRAFT_FALCON_NAME,팰컨
ITEM_GUIDED_MISSILE_NAME,유도 미사일
POI_REPAIR_NAME,정비소
TOAST_MODE_CHANGED,게임 모드 변경
TOAST_SETTINGS_RESET,설정이 변경되어 준비 상태가 해제되었습니다.
HUD_ALTITUDE_FORMAT,{0}m
LOBBY_LOCKED_TOOLTIP,전원 준비 완료 — 설정을 변경하려면 준비를 해제하세요
```

| 규칙 | 내용 |
|---|---|
| 언어 | `_KO` `_EN` `_ZH_CN` `_JA` |
| 서식 인자 | `{0}` `{1}` 순번 방식 (언어마다 어순이 다르므로 이름 있는 인자 권장) |
| 누락 검사 | CI에서 언어 간 키 집합 비교 → 누락 시 **빌드 실패** |
| 길이 여유 | UI 버튼은 한국어 기준 폭의 **1.3배**를 확보 → [09 §9.8](09_ui_ux_flow.md) |

---

## 17.14 검증 규칙 (`ValidateAll()`)

CI와 에디터 시작 시 자동 실행되며, 하나라도 실패하면 **빌드를 통과시키지 않습니다**.

| # | 규칙 |
|---|---|
| V1 | 모든 `Id`가 표 안에서 유일한가 |
| V2 | `DT_Aircraft`의 스탯 5개 합이 정확히 100인가 |
| V3 | 모든 `EffectClass` / `AbilityClass` / `BehaviorTree` 경로가 실제로 존재하는가 |
| V4 | `MeshPath` / `IconPath` / `PreviewPath` 에셋이 존재하는가 |
| V5 | `DT_Map.SupportedModes`의 모드가 `DT_Mode`에 존재하는가 |
| V6 | `DT_ItemDropRate` 각 행의 합이 100인가 |
| V7 | 모든 `NameKey`가 4개 언어 문자열 테이블에 존재하는가 |
| V8 | `UnlockLevel`이 `DT_LevelCurve` 범위 안인가 |
| V9 | `MinSpeed < MaxSpeed < BoostSpeed` 인가 |
| V10 | `ModeMask`가 `SpeedOnly`인 아이템이 전투 드롭 테이블에 없는가 |

> 이 10개 규칙이 **런타임 크래시의 상당 부분을 빌드 단계로 앞당깁니다.** 특히 V3·V4·V7은 "출시 빌드에서만 크래시"라는 최악의 상황을 막습니다.
