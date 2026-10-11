#!/usr/bin/env python3
"""MSVC 빌드로 휴대용(포터블) 폴더 + zip 만들기(사용자 요청 2026-10-11) — 설치 없이 아무 PC 의 아무 폴더에서 실행.

  python packaging/make_portable_msvc.py <빌드 폴더(…/build-msvc)> <출력 부모 폴더(…/dist)> [--qt D:/OSGeo4W/apps/Qt6] [--osgeo D:/OSGeo4W] [--no-zip]

만드는 것: <출력>/Trench-<버전>-portable/
  SectionViewer.exe · Qt DLL · 플러그인(platforms · imageformats · styles · iconengines · tls · generic)
  · exe 가 직접 또는 간접으로 쓰는 OSGeo4W DLL(dumpbin /dependents 로 되짚어 모두)
  · VC 런타임(vcruntime140*.dll · msvcp140*.dll — 재배포 패키지 설치 불필요)
  · app.ico(창 아이콘) · 읽어보기.txt · licenses/
  · portable.txt(이 표시가 있으면 설정을 레지스트리 대신 exe 옆 settings\\ 에 둔다) · projects/(프로젝트 자동 저장) · settings/
그리고 mkzip.py 로 UTF-8 이름 zip. 모델 데이터는 넣지 않는다.
"""
import os, re, shutil, subprocess, sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(HERE)
PLUGIN_DIRS = ["platforms", "imageformats", "styles", "iconengines", "tls", "generic", "networkinformation"]
SYSTEM_PREFIX = ("api-ms-", "ext-ms-")
SYSTEM_DLLS = {"kernel32.dll", "user32.dll", "gdi32.dll", "advapi32.dll", "shell32.dll", "ole32.dll", "oleaut32.dll", "comdlg32.dll", "ws2_32.dll",
               "winmm.dll", "version.dll", "netapi32.dll", "userenv.dll", "uxtheme.dll", "dwmapi.dll", "imm32.dll", "opengl32.dll", "glu32.dll",
               "d3d11.dll", "d3d9.dll", "dxgi.dll", "setupapi.dll", "shlwapi.dll", "crypt32.dll", "bcrypt.dll", "ncrypt.dll", "secur32.dll",
               "iphlpapi.dll", "winhttp.dll", "wtsapi32.dll", "authz.dll", "dwrite.dll", "d2d1.dll", "mpr.dll", "wintrust.dll", "msvcrt.dll",
               "ntdll.dll", "rpcrt4.dll", "comctl32.dll", "usp10.dll", "powrprof.dll", "propsys.dll", "dnsapi.dll", "cfgmgr32.dll", "synchronization.dll",
               "ucrtbase.dll", "dxcompiler.dll", "dxil.dll", "d3dcompiler_47.dll",
               "d3d12.dll", "uiautomationcore.dll", "icuuc.dll"}   # 뒤 셋: Qt 가 지연 로드(delay-load)로만 적어 둔 것 — 없어도 실행된다


def version():
    with open(os.path.join(ROOT, "CMakeLists.txt"), encoding="utf-8") as f:
        m = re.search(r"project\s*\(\s*\S+\s+VERSION\s+([\d.]+)", f.read())
    return m.group(1) if m else "0.0.0"


def find_tool(name, hint_dirs):
    for d in hint_dirs:
        p = os.path.join(d, name)
        if os.path.isfile(p):
            return p
    p = shutil.which(name)
    if p:
        return p
    return None


def vs_dumpbin():
    # vswhere 로 MSVC 의 dumpbin.exe 를 찾는다(VS BuildTools 2022)
    vswhere = os.path.join(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"), "Microsoft Visual Studio", "Installer", "vswhere.exe")
    if os.path.isfile(vswhere):
        out = subprocess.run([vswhere, "-latest", "-products", "*", "-property", "installationPath"], capture_output=True, text=True).stdout.strip()
        if out:
            tools = os.path.join(out, "VC", "Tools", "MSVC")
            if os.path.isdir(tools):
                for v in sorted(os.listdir(tools), reverse=True):
                    p = os.path.join(tools, v, "bin", "Hostx64", "x64", "dumpbin.exe")
                    if os.path.isfile(p):
                        return p
    return shutil.which("dumpbin")


def dependents(dumpbin, path):
    out = subprocess.run([dumpbin, "/nologo", "/dependents", path], capture_output=True, text=True, errors="replace").stdout
    return [d for d in re.findall(r"^\s+(\S+\.dll)\s*$", out, re.M | re.I)]


def main():
    args = sys.argv[1:]
    if len(args) < 2:
        print(__doc__); return 2
    build, outparent = os.path.abspath(args[0]), os.path.abspath(args[1])
    qt = r"D:\OSGeo4W\apps\Qt6"; osgeo = r"D:\OSGeo4W"; do_zip = True
    i = 2
    while i < len(args):
        if args[i] == "--qt": qt = args[i + 1]; i += 2
        elif args[i] == "--osgeo": osgeo = args[i + 1]; i += 2
        elif args[i] == "--no-zip": do_zip = False; i += 1
        else: print("모르는 옵션:", args[i]); return 2
    exe = os.path.join(build, "app", "SectionViewer.exe")
    if not os.path.isfile(exe):
        print("exe 없음:", exe); return 1
    ver = version()
    top = f"Trench-{ver}-portable"
    out = os.path.join(outparent, top)
    if os.path.isdir(out):
        shutil.rmtree(out)
    os.makedirs(out)
    shutil.copy2(exe, out)
    # 1) Qt: windeployqt(플러그인 · Qt DLL). 번역 · D3D 컴파일러 · 가상 키보드 · 네트워크 필요 없음
    wdq = find_tool("windeployqt.exe", [os.path.join(qt, "bin")])
    if not wdq:
        print("windeployqt.exe 없음(--qt 확인)"); return 1
    env = dict(os.environ); env["PATH"] = os.path.join(qt, "bin") + os.pathsep + os.path.join(osgeo, "bin") + os.pathsep + env.get("PATH", "")
    r = subprocess.run([wdq, "--release", "--no-translations", "--no-system-d3d-compiler", "--no-opengl-sw", "--no-compiler-runtime",
                        "--no-virtualkeyboard", os.path.join(out, "SectionViewer.exe")], env=env, capture_output=True, text=True, errors="replace")
    if r.returncode != 0:
        print(r.stdout); print(r.stderr); print("windeployqt 실패"); return 1
    # 2) 나머지 DLL: exe · Qt DLL · 플러그인이 쓰는 것을 되짚어 OSGeo4W\bin · Qt\bin 에서(zlib · jpeg · png · tiff · webp · freetype · brotli · VC 런타임 …)
    dumpbin = vs_dumpbin()
    if not dumpbin:
        print("dumpbin.exe 없음(VS BuildTools 필요)"); return 1
    srcs = [os.path.join(qt, "bin"), os.path.join(osgeo, "bin")]
    todo = [os.path.join(d, f) for d, _, fs in os.walk(out) for f in fs if f.lower().endswith((".exe", ".dll"))]
    have = {os.path.basename(p).lower() for p in todo}
    copied, missing = [], set()
    while todo:
        for dep in dependents(dumpbin, todo.pop()):
            k = dep.lower()
            if k in have or k in SYSTEM_DLLS or k.startswith(SYSTEM_PREFIX):
                continue
            have.add(k)
            for s in srcs:
                p = os.path.join(s, dep)
                if os.path.isfile(p):
                    dst = os.path.join(out, dep); shutil.copy2(p, dst); copied.append(dep); todo.append(dst); break
            else:
                missing.add(dep)
    # VC 런타임은 의존 목록에 vcruntime140.dll · msvcp140.dll 로 나오므로 위에서 함께 복사된다. 안 나왔으면 분명히 넣는다
    for f in ("vcruntime140.dll", "vcruntime140_1.dll", "msvcp140.dll", "msvcp140_1.dll", "msvcp140_2.dll"):
        if f not in have:
            for s in srcs:
                p = os.path.join(s, f)
                if os.path.isfile(p):
                    shutil.copy2(p, out); copied.append(f); have.add(f); break
    # 3) 아이콘 · 읽어보기 · 라이선스 · 포터블 표시 · 빈 폴더
    shutil.copy2(os.path.join(HERE, "assets", "app.ico"), out)
    shutil.copy2(os.path.join(HERE, "읽어보기.txt"), out)
    shutil.copytree(os.path.join(HERE, "licenses"), os.path.join(out, "licenses"))
    with open(os.path.join(out, "portable.txt"), "w", encoding="utf-8") as f:
        f.write("이 파일이 SectionViewer.exe 옆에 있으면 설정을 레지스트리 대신 이 폴더의 settings\\ 에 둡니다(폴더째 옮겨도 그대로).\n"
                "작업 프로젝트(<모델이름>.trench.json)는 projects\\ 에 자동 저장됩니다. 지우면 설정은 레지스트리로 갑니다.\n")
    for d in ("projects", "settings"):
        os.makedirs(os.path.join(out, d), exist_ok=True)
    print(f"portable: {out}")
    print("copied:", " ".join(copied) or "(none)")
    if missing:
        print("찾지 못한 DLL(시스템 것이면 괜찮음):", " ".join(sorted(missing)))
    if do_zip:
        zip_path = os.path.join(outparent, top + ".zip")
        r = subprocess.run([sys.executable, "-I", os.path.join(HERE, "mkzip.py"), outparent, top, zip_path], capture_output=True, text=True, errors="replace")
        print(r.stdout.strip()); print(r.stderr.strip())
        if r.returncode != 0:
            return 1
        print(f"zip: {zip_path} ({os.path.getsize(zip_path) / 1e6:.1f} MB)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
