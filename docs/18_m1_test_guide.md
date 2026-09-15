# 18. M1 수직 슬라이스 — 빌드 & 테스트 가이드

> 지금 저장소에 들어 있는 코드를 **실제로 실행해 보는 절차**입니다.
> 코드는 리눅스 컨테이너에서 작성했고 **언리얼로 컴파일한 적이 없습니다.** 첫 빌드에서 오류가 날 수 있으니, 그런 경우 아래 §6을 보세요.

---

## 1. 준비물

| 항목 | 버전 |
|---|---|
| Unreal Engine | **5.6** (Epic Games Launcher에서 설치) |
| Visual Studio | 2022 — **"C++를 사용한 게임 개발"** 워크로드 필수 |
| 디스크 | 약 30 GB (엔진 제외 프로젝트 빌드 산출물만 5~10 GB) |

> UE 버전이 5.6이 아니면 `FPG.uproject`의 `EngineAssociation` 값을 설치한 버전으로 바꾸세요.

---

## 2. 첫 빌드

```powershell
git clone https://github.com/minkyu0617/ST_AIR_POG.git C:\Users\82109\git\BS_STUDIO
cd C:\Users\82109\git\BS_STUDIO
```

1. `FPG.uproject` **우클릭 → Generate Visual Studio project files**
2. 생성된 `FPG.sln`을 열고 **Development Editor / Win64**로 빌드
   - 또는 `FPG.uproject`를 더블클릭하면 "모듈을 다시 빌드할까요?" 프롬프트가 뜹니다. **예**를 누르면 자동 빌드됩니다
3. 빌드 성공 시 에디터가 열립니다

---

## 3. 데이터 테이블 임포트 (최초 1회)

코드는 DataTable을 `/Game/Data/`에서 찾습니다. CSV는 저장소의 `Config/DataTables/`에 있습니다.

1. 콘텐츠 브라우저에서 **`Content/Data`** 폴더 생성
2. `Config/DataTables/DT_Aircraft.csv`를 그 폴더로 **드래그 앤 드롭**
3. 임포트 창에서:
   - **Import As**: `DataTable`
   - **Row Struct**: **`FPGAircraftRow`** 선택 ← 중요
4. 같은 방식으로 나머지 둘도 임포트

| CSV | Row Struct | 저장 이름 |
|---|---|---|
| `DT_Aircraft.csv` | `FPGAircraftRow` | `DT_Aircraft` |
| `DT_Item.csv` | `FPGItemRow` | `DT_Item` |
| `DT_Poi.csv` | `FPGPoiRow` | `DT_Poi` |

> 에셋 이름이 정확히 `DT_Aircraft` / `DT_Item` / `DT_Poi`여야 합니다. 코드가 그 경로를 직접 참조합니다.

**검증 확인**: 에디터 **Output Log**에 이렇게 나오면 정상입니다.
```
LogFPG: DataRegistry initialised. Valid=true Errors=0 Warnings=0
```
`Errors`가 0이 아니면 CSV 값이 규칙을 어긴 것입니다 (예: 기체 스탯 합이 100이 아님).

---

## 4. 테스트 레벨 만들기

`.umap`은 바이너리라 저장소에 넣지 않았습니다. 5분이면 만듭니다.

1. **File → New Level → Basic** (바닥과 조명이 있는 기본 레벨)
2. 바닥(Floor)을 선택해 **Scale을 (200, 200, 1)** 정도로 키웁니다 — 충돌 테스트용 지면
3. **PlayerStart**를 배치하고 **Z를 30000** 정도로 올립니다 (= 300 m 상공에서 시작)
   - 지면에서 시작하면 카운트다운 직후 바로 충돌합니다
4. 콘텐츠 브라우저 옆 **Place Actors** 패널에서 **`FPGPoiStation`**을 검색해 2~3개 배치
   - PlayerStart 정면(+X 방향)으로 **5~15 km 떨어뜨려** 놓으세요 (X = 500000 ~ 1500000)
   - 각각 Details에서 **`Poi Id`**를 `POI_REPAIR_BAY` / `POI_WEAPON_SHOP`로 설정
   - 고도를 다르게 주면(Z 차이) 레이더의 고도 표기를 확인할 수 있습니다
5. **World Settings → GameMode Override → `FPGSingleEnduranceGameMode`**
   - `Config/DefaultEngine.ini`에 전역 기본값을 넣어뒀지만, 레벨에서 명시하는 편이 확실합니다
6. 레벨을 **`Content/Maps/M1_TestFlight`**로 저장

---

## 5. 플레이

에디터에서 **Play (Alt+P)**. 새 창 모드(New Editor Window)를 권장합니다.

### 조작
| 키 | 동작 |
|---|---|
| `W` / `S` | 가속 / 감속 |
| `A` / `D` | 좌/우 기울기 (유지하면 뱅크 턴으로 선회) |
| `Space` + `WASD` | 벡터 기동 (상승·하강·좌우 급선회) |
| `Ctrl` | 부스트 |
| `LMB` | 기총 발사 |
| `Q` | 아이템 슬롯 순환 |
| `R` | 레벨 재시작 |

### 확인할 것 (M1 통과 기준과 직결)

- [ ] **BOARDING → 3 → 2 → 1 → GO!** 순서로 카운트다운이 뜨고, GO 전에는 조작이 먹지 않는가
- [ ] GO 직후 기체가 **이미 날고 있는 속도**로 출발하는가 (떨어지지 않음)
- [ ] `A`/`D`를 **좌우 대칭**으로 반응하고, 유지하면 선회하는가 (D-03)
- [ ] `Space`를 누른 채 상승할 때 **속도가 유지**되는가 (D-04 자동 스로틀)
- [ ] 급상승을 오래 유지하면 **STALL**이 뜨고, 기수가 내려가며, 속도가 붙으면 풀리는가
- [ ] `Ctrl` 부스트가 3초 쓰고 8초 쿨다운을 갖는가 (좌상단 BOOST 바)
- [ ] `LMB` 연사 시 **HEAT 바가 차고 OVERHEAT**가 걸리는가
- [ ] 좌하단 **레이더에 POI가 표시**되고, 고도 차이가 색과 `+/-` 수치로 읽히는가
- [ ] POI에 **속도를 줄여 들어가면 3초 뒤 거래가 성사**되고 토스트가 뜨는가
- [ ] 지면/물체에 충돌하면 **RUN OVER**가 뜨고 `R`로 재시작되는가
- [ ] **DISTANCE**가 늘어나고 1 km마다 **CREDITS +15**가 들어오는가

> POI 거래는 **크레딧이 있어야** 성사됩니다. 정비소 수리비가 60이므로 **최소 4 km 이상 비행한 뒤** 들르세요. 크레딧이 모자라면 `Repair bay: need 60 credits` 토스트가 뜹니다 — 이건 정상 동작입니다.

---

## 6. 자동화 테스트 (비행 결정론)

**Tools → Session Frontend → Automation** 탭에서 `FPG.Flight`를 체크하고 **Start Tests**.

| 테스트 | 검증 내용 |
|---|---|
| `FPG.Flight.Determinism` | 같은 입력 600프레임을 두 번 돌려 **결과가 완전히 같은가** |
| `FPG.Flight.Bounds` | 후진 불가 / 최고속 상한 / 롤 클램프 / 급상승 시 스톨 진입 |

> **Determinism이 깨지면 M3 멀티플레이 예측이 반드시 실패합니다.** 이 테스트는 그 계약을 지키는 장치이므로, 비행 물리를 수정할 때마다 돌리세요.

---

## 7. 빌드가 실패하면

이 코드는 **언리얼로 컴파일해 본 적이 없습니다.** 첫 빌드에서 오류가 날 수 있고, 대부분 아래 유형입니다.

| 증상 | 대처 |
|---|---|
| `Cannot open include file` | 해당 `#include` 경로 확인. 모듈 루트는 `Source/FPG/`이므로 `Flight/...` 형태여야 함 |
| `Unrecognized type 'FFPGxxx'` | `*.generated.h`가 마지막 include인지 확인 |
| `EnhancedInput` 관련 링크 오류 | `Source/FPG/FPG.Build.cs`의 의존성 목록 확인 |
| DataTable이 비어 있음 | §3의 Row Struct 선택이 틀렸을 가능성. 임포트 다시 |
| 기체가 안 보임 | `/Engine/BasicShapes/Cone` 로드 실패. 콘텐츠 브라우저 설정에서 **Show Engine Content** 켜기 |
| 이륙하자마자 추락 | PlayerStart의 Z를 더 높이세요 |

오류 메시지를 그대로 알려주시면 수정하겠습니다.

---

## 8. 이 빌드에 **없는** 것

M1 범위 밖이라 의도적으로 뺐습니다 ([docs/16 §16.17](16_architecture.md)).

- 멀티플레이 일체 (예측/화해, 방 시스템, Steam 연동)
- 아이템 실제 효과 (미사일·EMP 등 — 슬롯 구조와 기총만 동작)
- 적 전투기 AI
- 비행기 스토어 UI
- 진짜 HUD/미니맵 (UMG 대신 Canvas 디버그 표시)
- 사운드, VFX, 실제 기체 메시
- 지형·맵 콘텐츠
