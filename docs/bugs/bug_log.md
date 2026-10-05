# Bug Log

## BUG-RUNTIME-001 — Relative data path depends on process working directory

- **Discovered:** IU-UI-06 verification
- **Status:** OPEN / DEFERRED

### Reproduction

1. Launch `build/iu-ui-06-final/leifeng_qt.exe` with the process working directory set to the build directory rather than the project root.
2. Attempt to log in with a known-valid Student or Administrator account.

### Actual

Every account is reported as invalid.

### Expected

Valid accounts should authenticate regardless of where the executable is launched from, provided the application can locate its runtime data.

### Root cause

`DataManager` opens relative paths including `data/students.txt`, `data/administrators.txt`, `data/records.txt`, and `data/diaries.txt`. These paths resolve against the process current working directory.

### Observed confirmation

Launching the same executable with the project root as its working directory restores normal login.

### Current workaround

Launch the development executable with the project root as its working directory.

### Planned handling

Resolve explicitly during the later Persistence / runtime-path architecture cleanup. Do not change path semantics during the current UI phase.

### Fix and regression

- **Fix commit:** None; this bug is not fixed in IU-GIT-05.
- **Regression:** The project-root launch was observed to restore login; post-fix regression remains pending until the planned runtime-path work.
