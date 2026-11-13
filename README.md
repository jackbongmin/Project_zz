# Project ZZ

방사능으로 무너진 세계에서 폐가를 털고 거점을 키워나가는 로우폴리 서바이벌 게임.
기획: [docs/GDD.md](docs/GDD.md)

- 엔진: Godot 4.7.2
- 언어: C++ (GDExtension, godot-cpp)

## 빌드 준비 (최초 1회)
1. **Visual Studio 2022 Build Tools** — "C++를 사용한 데스크톱 개발" 워크로드
2. **Python 3.12** + `python -m pip install --user scons`
3. godot-cpp 받기 (10.0.0-stable 고정):
   ```bash
   git submodule update --init --recursive
   ```

## 빌드
SCons가 MSVC를 자동으로 찾아요. 아무 터미널에서:
```bash
python -m SCons api_version=4.7
```
→ `bin/project_zz.windows.template_debug.x86_64.dll` 생성 (첫 빌드만 몇 분, 이후엔 바뀐 파일만 빌드)

> `python`이 Microsoft Store를 열면 전체 경로를 쓰세요: `%LOCALAPPDATA%\Programs\Python\Python312\python.exe`
>
> 에디터가 켜진 상태에서 다시 빌드하면 바로 반영돼요 (reloadable).

## 실행
Godot 에디터로 `project.godot`을 열고 F5.

| 키 | 동작 |
|---|---|
| WASD / 마우스 | 이동 / 시점 |
| Shift / Space | 달리기 / 점프 |
| **V** | 1인칭 ↔ 3인칭 |
| Esc / 클릭 | 마우스 해제 / 다시 잡기 |
