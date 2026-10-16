# SQLite 아말감

- 출처: https://sqlite.org/2026/sqlite-amalgamation-3540000.zip (SQLite 3.54.0, 2026-10-11 내려받음)
- 내려받은 zip 의 SHA3-256: 7b670a62fdfbd672b75fef004cb703c8a3e87d3a5cc7d675b4a08337004a2d93 (download.html 게시값과 일치 확인)
- 라이선스: public domain (https://sqlite.org/copyright.html)
- 이 폴더에는 `sqlite3.c`, `sqlite3.h` 만 둔다(`shell.c`, `sqlite3ext.h` 제외). 용도: 3SM(Bentley ScalableMesh) 읽기 전용 판독(`core/src/sm3.cpp`).
- 빌드 정의(`CMakeLists.txt`): SQLITE_THREADSAFE=2(멀티스레드 — 연결마다 한 스레드), SQLITE_OMIT_LOAD_EXTENSION, SQLITE_DEFAULT_MEMSTATUS=0, SQLITE_DQS=0, SQLITE_OMIT_DEPRECATED
