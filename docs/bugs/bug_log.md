# Bug Log

## BUG-RUNTIME-001 — Relative data path depends on process working directory

- **Discovered:** IU-UI-06 verification
- **Status:** CLOSED

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

### Pre-fix workaround

Launch the development executable with the project root as its working directory.

### Original planned handling (superseded)

Superseded by the IU-ARCH-03B fix recorded below.

### Fix and regression

- **Fix:** Qt resolves runtime data from the executable-adjacent `data/` directory. Missing required runtime data causes an explicit startup failure before login.
- **Implementation commit:** `1c68f36`
- **Regression:**
  - Build-directory working directory: PASS
  - Project-root working directory: PASS
  - Unrelated working directory: PASS
  - Missing required file startup failure: PASS
  - CTest: 1/1 PASS
  - Tracked source `data/`: unchanged
