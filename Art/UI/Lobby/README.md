# Close IN 로비 시안 및 공용 UI 에셋

사용자가 선택한 숲 입구 `Backgrounds/CloseIN_Lobby_Background_ForestEntrance_v1.png`를 로비 전용 배경으로 사용합니다. 기존 로그인 `Forest_v3`와 로고 원본은 유지합니다. `Logo_Mix_v7`은 이번 로비에 사용한 후보이며 v6 로고를 폐기하거나 최종 선택한 것은 아닙니다.

`index.html`을 열면 A(하단 중앙), B(왼쪽 메뉴), 매칭 중 상태를 전환해 검토할 수 있습니다. 두 안 모두 1920×1080 기준입니다. 추천안은 숲길을 넓게 보여주는 A이며, 최종 UI 디자인 승인은 별도입니다. 최신 합성 캡처는 커밋하지 않고 로컬 `.local-work/reports/lobby-refresh-20261029/Previews/`에 보관합니다.

현재 UI는 짙은 청록 바탕, 크림색 주 버튼 글자, 강화된 금빛 이중 테두리와 모서리 선, 은은한 표면 질감, 별도 금색 잎 장식을 사용합니다. 모드 선택은 하나의 프레임 안에 묶습니다. 매칭 중에는 밝은 취소 버튼과 잠긴 보조 버튼으로 상태를 구분합니다.

- `index.html` / `lobby.css` / `lobby.js`: 배경·로고·UI 스킨·문자가 분리된 편집 원본. 외부 서비스나 폰트 다운로드 없이 실행합니다.
- `Components/`: 문자 없는 투명 SVG 원본, 게임용 PNG와 별도 `@2x.png`, 잎 장식·자물쇠, 가독성 보조 그라데이션 이미지.
- `components.html`: 스킨 상태별 보기.
- `LobbyDesign.json`: 색상, 글자 크기, 원본 경로, 9-slice 값, 미정 사양.
- `LobbyLayoutBounds.json`: 1920×1080에서 렌더링한 각 컨트롤의 실제 좌표·크기. 문자는 스킨 이미지에 포함되지 않습니다.

## PNG 사용

게임용 버튼 스킨은 128×96px PNG입니다. UMG Brush의 Draw As를 `Box`, Margin을 `Left .1875 / Top .25 / Right .1875 / Bottom .25`, Image Size를 `128×96`, Tint를 흰색으로 사용합니다. 모서리는 24px로 유지됩니다. `@2x.png`는 256×192px 별도 고해상도 자료이며 현재 게임용 텍스처와 구분합니다. Normal / Hovered / Pressed / Disabled 이미지와 TextBlock을 분리합니다. 그라데이션·잎 장식·자물쇠는 Box가 아닌 일반 Image로 표시합니다.

현재 시안의 글꼴은 OS의 맑은 고딕을 사용하며 폰트 파일은 재배포하지 않습니다. Unreal에서는 엔진 기본 폰트 및 폴백을 사용하므로 최종 글꼴 선정 전까지 자폭과 한글 모양에 차이가 있을 수 있습니다.

로비 PNG 경로는 `FWLobbyWidget`의 `BackgroundImagePath`로 지정합니다. 배경은 별도 NonUFS 파일로 패키징하며, 파일 로드 실패 시 기존 imported texture를 대체로 사용합니다. 기존 로그인 바람 레이어의 가지 좌표는 새 배경에 맞지 않으므로 새 로비 배경에는 겹치지 않습니다. 로그인 화면의 배경과 바람 효과는 변경하지 않습니다.

## 클라이언트 임시 흐름

추가 요청에 따라 A를 기준으로 `UFWLobbyWidget`과 기존 로그인 컨트롤러의 로컬 진입 경로를 연결했습니다. 최신 클라이언트 흐름은 `로컬 플레이 → 로비 → 매칭 시작 → 약 5초 대기 → 중앙 완료 배너 3초 → 기존 DefaultMap` 순서입니다. 취소하면 검색 대기를 중단하고 모드/캐릭터 버튼을 다시 사용할 수 있습니다. 캐릭터 버튼은 준비 중 안내만 표시합니다.

`Config/DefaultGame.ini`의 `FWLoginPlayerController` 섹션에서 `PrototypeMatchDelaySeconds=5.0`을 조절할 수 있습니다. `CharacterSelectionLevel`은 현재 `None`이며, 추후 캐릭터 선택 맵 경로를 지정하면 매칭 완료 후 해당 맵으로 이동합니다. 새 맵은 패키징의 MapsToCook에도 추가해야 합니다. 비어 있으면 기존 `GameplayLevel`로 이동합니다.

`MatchCompleteDisplaySeconds=3.0`이 완료 배너 대기 시간입니다. 제공된 Matchmaking 배경판/글자 PNG를 수정 없이 겹쳐 화면 중앙에 표시하고 뒤 화면을 32% 어둡게 합니다. 완료 상태의 브라우저 검토는 최신 `../MenuTeamConcept/index.html?view=complete`를 사용합니다. 이 폴더의 과거 브라우저 시안은 보존본입니다.

두 모드의 실제 제공 여부와 규칙은 미정입니다. 이번 모드 선택은 로비 UI 표시만 변경하며 서버 매칭이나 1:1/3:3 게임 규칙을 구현하지 않습니다. 기존 로그인/회원가입 서비스와 서버 코드·프로토콜은 변경하지 않았습니다. 브라우저의 5초 완료 이벤트는 검토용이며 게임 맵을 실행하지 않습니다.

Unreal C++는 정적 검토만 수행했으며 빌드·실행·패키징은 미검증입니다. 새 UCLASS가 있으므로 에디터 종료 후 Development Editor / Win64 빌드가 필요합니다. 브라우저 시안의 렌더링·상태 검사는 실제 Unreal 화면 검증과 구분합니다.
