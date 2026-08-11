# 10. 기술 스택

## 10.1 엔진 선정

### ✅ 확정: **Unreal Engine 5.6** (D-01, 2026-08-10)

| 평가 축 | Unreal 5.6 | Unity 6 | Godot 4 |
|---|---|---|---|
| 하늘/구름 표현 | ✅ **Volumetric Cloud + Sky Atmosphere 내장** | 에셋 구매 필요 | 직접 구현 |
| 네트워크 리플리케이션 | ✅ **내장, 검증된 예측/보간** | Netcode for GO (성숙도 중) | 미흡 |
| Steam 연동 | ✅ Online Subsystem Steam (SDR 포함) | Steamworks.NET (서드파티) | GodotSteam |
| 비주얼 수준 (1인 개발 기준) | ✅ 기본값이 이미 예쁨 | 세팅 필요 | 낮음 |
| 로열티 | 100만 달러 초과분 5% | 무료(매출 20만$ 미만) | 무료 |
| 학습 곡선 | 중~상 (C++/Blueprint) | 중 | 하 |
| 패키지 용량 | 큼 (4~8 GB) | 작음 | 매우 작음 |

**결정 근거**: 이 게임의 핵심 비주얼 자산은 **"구름 위 하늘"** 이고, Unreal은 그것을 **엔진 기본 기능으로 무료 제공**합니다. Unity에서 동등한 볼류메트릭 구름을 만들려면 에셋 구매 + 수 주의 셰이더 작업이 필요합니다. 여기에 멀티플레이 리플리케이션까지 더하면 격차는 수개월 규모가 됩니다.
로열티 5%는 **제품 누적 총매출 100만 달러(약 ₩14억) 초과분에만** 적용되므로, [01번 문서](01_product_overview.md)의 목표 매출 구간에서는 실질 부담이 0입니다.

### 라이선스 비용 비교 (D-01 판단 근거)

| | Unreal 5.6 | Unity 6 |
|---|---|---|
| 기본 | 무료 | Personal 무료 (매출·펀딩 연 $200K 미만) |
| 유료 전환 시점 | 누적 총매출 **$100만(≈₩14억) 초과분 5% 로열티** | $200K(≈₩2.8억) 초과 시 **Pro 필수** (약 $2,200/석/년) |
| 런타임 피 | 없음 | 폐지됨 (2024-09 Unity 철회) |
| [01](01_product_overview.md) 기본 시나리오 실부담 | **₩0** (순매출 ₩3.6억 ≈ 총매출 ₩6.5억 < ₩14억) | Pro 구독 발생 — 14~20개월 ≈ **₩380~510만** |
| 낙관 시나리오 실부담 | 약 ₩2,900만 (총매출 ₩19.8억 가정) | Pro 구독 ₩380~510만 |

**기본 시나리오에서는 Unity가 오히려 더 비쌉니다.** 로열티 트리거가 손익분기(12,000장)의 한참 위에 있기 때문입니다.

> ⚠️ **엔진 라이선스 요금제는 수시로 바뀝니다.** 위 수치는 **일정·손익 산정용 기준값**이며, 실제 계약 시점에 Epic 공식 약관을 직접 확인해야 합니다. → [FPG.md §9](../FPG.md)

> **폐기한 대안 (Unity 6)**: "팀이 이미 Unity C# 숙련자" 전제가 성립하지 않아 유일한 근거가 사라졌습니다. Unity를 택했다면 볼류메트릭 구름을 포기하고 **스타일라이즈드(만화풍) 하늘**로 아트 방향을 전환해야 했고, 이는 USP 1번(구름 위 상점 POI)의 비주얼 근거를 약화시킵니다. → [14번 문서 D-01](14_open_questions.md)

## 10.2 기술 구성

| 영역 | 선택 |
|---|---|
| 엔진 | Unreal Engine 5.6 |
| 언어 | C++ (게임플레이 코어) + Blueprint (UI·연출·튜닝) |
| 렌더링 | Lumen **비활성** / 기본 조명 + 라이트맵. Nanite 지형만 선택 적용 |
| 네트워크 | UE Replication + Online Subsystem Steam + Steam Datagram Relay |
| UI | UMG (+ Common UI 플러그인, 게임패드 내비게이션) |
| 물리 | Chaos (기체는 커스텀 Movement Component, 강체 시뮬 미사용) |
| 오디오 | MetaSounds + FMOD(선택) |
| 데이터 | DataTable (CSV 임포트) — 밸런스 수치 전량 |
| 저장 | SaveGame (로컬) + Steam Cloud + Steam Stats/Leaderboards |
| 버전관리 | Git + Git LFS (바이너리 에셋) |
| CI | GitHub Actions (린트/빌드), 주 1회 야간 패키징 |
| 이슈 | GitHub Issues + Projects |
| 크래시 리포트 | Unreal Crash Reporter + Sentry(선택) |

### Lumen 비활성 결정
Lumen은 아름답지만 **1080p에서 GTX 1060급 하드웨어의 프레임을 절반으로 떨어뜨립니다.** 이 게임은 고속 비행 게임이라 **60fps가 타협 불가능한 요구사항**이며, Steam 유저 하드웨어 중앙값은 여전히 중급기입니다. 사전 구운 라이팅 + 볼류메트릭 구름만으로 충분히 아름다운 결과를 낼 수 있습니다.

## 10.3 성능 목표

| 사양 | 목표 |
|---|---|
| **최소** (GTX 1060 / RX 580, i5-8400) | 1080p Low **60 fps** |
| **권장** (RTX 3060 / RX 6600, i5-12400) | 1440p High 60 fps |
| **Steam Deck** | 800p Low-Medium **40 fps 이상** (검증 배지 목표) |
| 프레임 타임 예산 | 16.6 ms (CPU 8 / GPU 8) |
| 로딩 시간 | 맵 진입 15초 이내 (NVMe 기준 8초) |
| 패키지 용량 | 8 GB 이하 |
| 메모리 | 8 GB RAM / 4 GB VRAM 이내 |

### 성능 리스크와 대응
| 리스크 | 대응 |
|---|---|
| 볼류메트릭 구름이 GPU를 먹음 | 품질 3단계 스케일링, Low에서는 빌보드 구름으로 대체 |
| 10인 + 발사체 다수 | 발사체 풀링, 결정론적 궤적으로 복제 최소화 ([08](08_multiplayer_architecture.md)) |
| 광대한 지형 스트리밍 | World Partition + HLOD, 시야 거리 기반 LOD 5단계 |
| 고속 이동 시 스트리밍 히칭 | 진행 방향 예측 프리로드 (속도 벡터 × 6초 앞) |

## 10.4 프로젝트 구조 (계획)

> 모듈명은 **`FPG`로 영구 고정**되어 있습니다 (D-13). 게임 제목은 여기에 등장하지 않습니다.
> **소스 트리의 정본은 [16 §16.12](16_architecture.md)입니다.** 아래는 최상위 개요이며, 클래스 단위 배치는 16번 문서를 따르십시오.

```
ST_AIR_POG/
├── docs/                        # 설계 문서 (현재 단계)
├── Source/FPG/
│   ├── Core/                    # 게임 인스턴스, 세이브, 설정, 데이터 레지스트리
│   ├── Flight/                  # 비행 물리, 이동 컴포넌트, 스톨, 입력 상태 머신
│   ├── Combat/                  # 무기, 아이템, 데미지, 락온
│   ├── POI/                     # 상점 3종, 도킹 링, 상점 트랜잭션
│   ├── Modes/                   # GameMode/GameState/PlayerState + 모드별 3종
│   ├── Net/                     # 방 상태, 매치메이킹, 리플리케이션 정책
│   ├── AI/                      # 적기 AI, 채우기 봇
│   ├── World/                   # 코스 스플라인, 체크포인트, 지형 생성, 해저드
│   └── UI/                      # HUD, 미니맵, 전체지도, 토스트, 로비, 상점
├── Content/
│   ├── Aircraft/  Environments/  VFX/  UI/  Audio/  Maps/
├── Config/
│   └── DataTables/              # *.csv — 밸런스 수치 (프로그래머 없이 수정) → [17](17_data_schema.md)
└── Tools/                       # 빌드 스크립트, 밸런스 검증 스크립트
```

**클래스 접두어 규칙** ([16 §16.1](16_architecture.md)): 기반·공용 클래스에는 `FPG`를 붙이고(`AFPGGameModeBase`, `AFPGAircraftPawn`, `FFPGMove`), **모드별 파생 클래스는 접두어 없이** 역할명을 씁니다(`ASpeedRaceGameMode` → `/Script/FPG.SpeedRaceGameMode`). 모듈명이 이미 `FPG`라 경로에서 구분되기 때문입니다.

## 10.5 데이터 주도 설계 원칙

**밸런스 수치는 단 하나도 코드에 하드코딩하지 않습니다.**

```csv
# Config/DataTables/DT_Aircraft.csv
Id,Name,Speed,Accel,Turn,Durability,BoostPower,SpecialId,UnlockLevel
1,Falcon,20,20,20,20,20,NONE,0
2,Dart,32,24,14,12,18,AFTERBURNER_DASH,0
3,Bastion,14,14,16,38,18,TEMP_ARMOR,5
```

```csv
# Config/DataTables/DT_Item.csv
Id,Name,Grade,Damage,Ammo,Cooldown,LockOnRequired,ModeMask
A1,GuidedMissile,Common,30,3,2.0,true,ALL
A7,Railgun,Legendary,85,1,0,false,BATTLE
R3,RankSteal,Rare,0,1,0,false,SPEED
```

이유: 밸런싱은 출시 후 **수백 번** 반복됩니다. 매번 C++ 컴파일과 재패키징이 필요하면 밸런스 패치 주기가 주 단위가 되고, 그동안 유저는 떠납니다.

## 10.6 개발 환경 & 워크플로

| 항목 | 내용 |
|---|---|
| 브랜치 전략 | `main`(릴리스) / `develop` / `feature/*` |
| 커밋 규칙 | Conventional Commits (`feat:`, `fix:`, `balance:`, `art:`) |
| LFS 대상 | `*.uasset` `*.umap` `*.wav` `*.png` `*.fbx` |
| 빌드 | 야간 자동 개발 빌드, 주 1회 플레이테스트 빌드 |
| 배포 | SteamPipe (`steamcmd` 스크립트화), 브랜치: `default` / `beta` / `dev` |
| 텔레메트리 | 익명 통계 (모드별 플레이 횟수, 평균 세션 길이, POI 이용률, 이탈 지점) — **옵트아웃 제공** |

**텔레메트리는 밸런싱의 유일한 객관적 근거**입니다. 특히 `POI 이용률`과 `구간별 이탈 지점`은 이 게임의 핵심 설계 가설(POI 딜레마가 재미있는가)을 검증하는 지표이므로 v1.0부터 반드시 수집해야 합니다.

## 10.7 빌드 & 릴리스 체크리스트 (요약)

- [ ] Shipping 빌드에서 치트 콘솔 비활성
- [ ] 4개 언어 문자열 누락 검사 자동화
- [ ] Steam 도전과제 32개 전부 트리거 테스트
- [ ] 최소 사양 머신에서 60fps 실측
- [ ] Steam Deck 실기 검증 (컨트롤러 레이아웃, 텍스트 가독)
- [ ] 크래시 리포터 연동 확인
- [ ] 방 입장/이탈/호스트 이탈 시나리오 12종 테스트
- [ ] 네트워크 저품질 시뮬레이션 (200ms/2% 손실) 통과
