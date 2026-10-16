# Copy non-system DLL dependencies (from OSGeo4W\bin / Qt6\bin) next to SectionViewer.exe, recursively.
import os, re, shutil, subprocess, sys
APP = r"D:\desca\build-msvc\app"
SRC = [r"D:\OSGeo4W\apps\Qt6\bin", r"D:\OSGeo4W\bin"]
DUMPBIN = sys.argv[1]
def deps(path):
    out = subprocess.run([DUMPBIN, "/nologo", "/dependents", path], capture_output=True, text=True, errors="replace").stdout
    return re.findall(r"^\s+(\S+\.dll)\s*$", out, re.M | re.I)
todo = [os.path.join(r, f) for r, _, fs in os.walk(APP) for f in fs if f.lower().endswith((".exe", ".dll"))]
seen = {os.path.basename(p).lower() for p in todo}
copied = []
while todo:
    for d in deps(todo.pop()):
        if d.lower() in seen: continue
        seen.add(d.lower())
        for s in SRC:
            p = os.path.join(s, d)
            if os.path.isfile(p):
                dst = os.path.join(APP, d); shutil.copy2(p, dst); copied.append(d); todo.append(dst); break
print("copied:", " ".join(copied) or "(none)")
