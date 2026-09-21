# 메인 화면

## 실행

UE 5.8에서 `Framework.uproject`를 열고 `Content/Map/MainMenu`를 플레이한다.
기본 게임 맵과 에디터 시작 맵 모두 MainMenu로 설정돼 있다.
이미 다른 맵을 열어둔 에디터에서는 MainMenu를 직접 열어야 한다.

- **게임 시작**: 로컬 `DefaultMap`으로 이동. 중복 클릭 방지와 맵 누락 안내 포함.
- **조작 방법**: 기존 이동·카메라·스킬 입력 설명을 펼치거나 접는다.
- **게임 종료**: Unreal의 QuitGame 사용. 일반 실행에서는 게임 종료, PIE에서는 플레이 종료.
- 검은 배경, 임시 제목 FRAMEWORK, 로컬 프로토타입 안내. 배경 이미지 없음.
- 기본 버튼 키보드 포커스는 게임 시작. Tab/Shift+Tab으로 버튼 이동.

## 구성 / 수정 위치

| 파일 | 역할 |
| --- | --- |
| `Content/Map/MainMenu.umap` | 메뉴 전용 빈 레벨. FWMainMenuGameMode 지정 |
| `FWMainMenuGameMode` | 메뉴 Controller 지정, 캐릭터/HUD 생성 생략 |
| `FWMainMenuPlayerController` | UI 입력 모드, 시작/종료, 레벨 전환 |
| `FWMainMenuWidget` | C++로 생성한 UMG 위젯 트리, 문구/색상/버튼/안내 |
| `Config/DefaultGame.ini` | GameplayLevel, 패키징에 포함할 맵과 입력 에셋 |

게임 제목과 문구는 `FWMainMenuWidget.cpp`의 LOCTEXT에서 수정한다.
목적지 변경 시 `GameplayLevel`과 `MapsToCook`를 함께 수정한다.
서버 연결을 추가할 때는 `StartGame()` 앞에 IP 입력/연결 절차를 붙이고,
연결 결과에 따라 서버 참가 또는 레벨 전환을 진행하도록 확장한다.
현재는 네트워크 연결을 수행하지 않는다.

게임 Controller에는 독립 카메라를 유지하도록 자동 시점 관리를 끄고,
OnPossess에서 카메라를 캐릭터 위치에 맞추는 처리를 추가했다.

## 검증

`FrameworkEditor Win64 Development` 빌드 후 별도 개발용 게임으로 실행:

```powershell
& 'C:/Program Files/Epic Games/UE_5.8/Engine/Binaries/Win64/UnrealEditor-Cmd.exe' `
  "$PWD/Framework.uproject" -game -RenderOffscreen -windowed -ResX=1280 -ResY=720 `
  -unattended -nosplash -NoSound `
  '-ExecCmds=Automation RunTests Framework.Menu.StartAndTravel' `
  '-TestExit=Automation Test Queue Empty'
```

검사 항목: 메뉴 생성, 메뉴에서 Pawn/미니맵 미생성, 조작 안내 토글,
시작 버튼 중복 입력 방지, DefaultMap 전환, 캐릭터/미니맵 생성,
탑다운 카메라 유지, 메뉴 제거.
스크린샷은 `Saved/Screenshots/Menu`에 저장한다.
자동 테스트는 종료 버튼의 바인딩까지 검사하며, 실제 종료는 수동 확인 항목이다.
패키지 빌드/서버 연결은 이 테스트의 검증 범위에 포함되지 않는다.

`Scripts/CreateMainMenuMap.py`는 처음 빈 맵을 만들 때 사용한 생성 도구다.
저장소에 MainMenu.umap이 포함돼 있으므로 팀원은 실행할 필요가 없다.
PythonScriptPlugin은 맵 생성 도구용이며 게임 실행에는 필요하지 않다.
