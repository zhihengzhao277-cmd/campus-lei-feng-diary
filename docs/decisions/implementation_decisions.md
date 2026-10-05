# Implementation Decisions

## DEC-UI-DIARY-001 — IU-UI-07 Diary Wall architecture deferral

**Context:** IU-UI-07 originally requested Diary Wall public honors while simultaneously forbidding DiaryPostPublicView / Service-layer work.

**Decision:**
- Gate3.5 D225–D227 remain unchanged and authoritative.
- No Gate3 reopen.
- IU-UI-07 is limited to Presentation/navigation modernization.
- DiaryPostPublicView / public query aggregation is deferred to the later core architecture cleanup.
- Diary Wall Lei Feng Star / specialty honor integration is deferred until that public-view boundary exists.
- Current legacy publisher lookup remains known architecture debt and is not treated as final architecture.
