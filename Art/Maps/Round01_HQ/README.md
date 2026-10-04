# Round 01 HQ — 숲 디테일 작업본

8개 캠프를 가진 Round01_v2를 바탕으로 만든 별도 환경 아트 버전이다.
기존 로우폴리 시안은 보존했으며, v1·v2·HQ의 좌표 방향을 함께 정리했다.
맵은 112 × 112m, 중앙 공터는 지름 36m다. 캠프·스폰·수풀 영역·주요 경로는 v2와 같다.

## 열기

- Unreal 콘텐츠 브라우저: `Map/L_Round01_Emberwild_HQ`
- Unreal 리소스: `Environment/Round01_HQ`의 Meshes, Materials, Textures
- Blender 원본: `Round01_Emberwild_HQ.blend` (Blender 5.2.1)
- 전체·탑뷰·스폰·캠프 렌더: `Previews/`
- 교환용 FBX 13개: `Exports/`
- 배치와 재질 정보: `Round01_Layout.json`

Blender 파일과 `Textures/` 폴더를 함께 보관한다. 이미지 경로는 `//Textures/` 상대 경로다.
Blender에서 개별 나무·바위·소품을 편집할 수 있고, `11_Reusable_Foliage_Library`는
렌더에서 숨긴 나무 원형 보관 컬렉션이다. 나무 216그루는 6개 원형 메시를 공유한다.

## 좌표와 탑뷰

Unreal 월드 **+X는 북쪽/위, +Y는 동쪽/오른쪽, +Z는 높이**다.
Blender는 **+X가 북쪽/위, -Y가 동쪽/오른쪽**이다. 미터를 사용한다.
변환식은 UE `(X,Y,Z) = (Blender X, -Blender Y, Blender Z) × 100`이다.
FBX는 cm 단위를 명시하고 객체 변환에 100배 단위 변환을 담는다.
메시 액터를 임의로 100배 확대할 필요는 없다.

Unreal Outliner에서 `Round01_NorthTopCamera`를 우클릭 → Pilot으로 +X 위쪽 탑뷰를 확인한다.
전체 사선 뷰는 `Round01_OverviewCamera`다. 이미 열려 있던 에디터는 이전 뷰를 기억할 수 있다.
Blender의 저장된 시작 뷰와 `Camera_Top`도 +X가 위다.
숫자패드 7은 Blender의 기본 탑뷰 방향으로 돌아가므로 이 방위 기준과 다르다.

## 아트 변경

- 기존 단순 수관을 줄기·뿌리·가지·개별 잎을 가진 나무로 교체했다.
- 바위와 폐허 모서리를 다듬고, 절벽 가장자리에 풍화 암석을 추가했다.
- 지면·바위·나무껍질·이끼·잎·천의 6개 텍스처 세트를 제작했다.
  각 세트는 1024² BaseColor/Normal/Roughness PNG로 구성되며 총 18장이다.
- 통행 경계와 수풀에 양치류·잔풀을 보강하고 모닥불 주변 소품을 세분화했다.
- 나무 위치와 캠프 배치는 유지하고 색감·재질·조명을 자연스러운 숲 방향으로 조정했다.

외부 에셋을 사용하지 않은 절차적 모델링·PBR 재질 작업본이다.
실사 최종 아트나 최적화가 끝난 출시용 맵은 아니다. 벽의 큰 윤곽은 v2 시안에서 이어진다.
노멀맵은 지형의 미세 질감을 표현하며 거대한 암반 형태 자체를 대체하지 않는다.
유일 메시 합계는 약 112만 삼각형이고, 실제 화면의 폴리곤 수에는 반복 나무 배치가 더해진다.
Unreal에는 환경 메시 7개와 나무 액터 216개를 배치한다.
나무는 6개 Static Mesh를 공유하지만 HISM/LOD/Nanite 최적화는 별도 작업이다.

## 확인 범위

`Geometry_Verification.json`은 실제 벽 메시와 주요 경로·공터의 교차 검사다.
`Layout_Preservation_Verification.json`은 v2와 HQ의 게임 배치가 일치하는지 검사한 결과다.
`Unreal_Verification.json`은 저장된 레벨을 재로딩하여 지면 크기, 기준점, 나무 위치,
머티리얼 연결, 충돌 프로필, 깃발의 실제 좌표와 탑뷰 카메라 방향을 확인한 결과다.

지면과 벽만 BlockAll이며 나머지 장식은 NoCollision이다.
스폰·몬스터·은신·시야·라운드 규칙은 기준점만 제공하고 게임 기능은 추가하지 않았다.
미리보기 이미지는 Blender 렌더다. Unreal 파일 검증은 NullRHI 모드로 수행하며
실제 Unreal 화면, NavMesh 주행, 전투 가독성 및 프레임 성능 검증은 남아 있다.

전체 임포트는 오류 0건으로 완료됐다. 초기 FBX 빌드 과정에서 일부 벽·소품의 접선 경고와
RecastNavMesh 부재 경고가 발생했다. 저장 메시에는 Built-in 접선 계산을 적용했다.
실제 엔진 화면에서 노멀맵의 음영과 UV 경계를 확인하는 작업은 남아 있다.

## 재생성

프로젝트 루트에서 `Scripts/Blender/CreateRound01HQ.py`를 Blender의 백그라운드 Python으로 실행한다.
`-- --no-render`는 미리보기 생략, `-- --render-only`는 저장된 원본에서 렌더만 다시 만든다.
`Scripts/Blender/ExportRound01HQ.py`는 저장된 원본에서 FBX만 다시 내보낸다.
생성기는 이 폴더를 다시 쓰므로 수동 수정본은 다른 이름으로 보관한다.

Unreal용 `Scripts/UpgradeRound01Art.py`는 PythonScriptPlugin, EditorScriptingUtilities,
ProceduralMeshComponent를 실행 시 활성화해 사용한다. 영구 플러그인 설정 변경은 없다.
이미 적용한 시안의 축을 반복해서 회전하지 않는다. `-Round01HQRefresh`는 HQ 리소스만
갱신하며 기존 액터 배치를 유지한다. 레벨 배치까지 새로 만들려면 별도 이름을 사용한다.

## 로컬 작업 보조 파일

이 문서에서 언급하는 제작·임포트·검증용 Python 스크립트와 자동 검증 리포트는 로컬 작업 보조 파일로, Git 커밋에서 제외한다. 맵·모델·텍스처·미리보기와 배치 JSON은 저장소에 포함한다.
