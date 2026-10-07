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

## DEC-DIARY-MOD-001 — IU-DIARY-01A human rulings

**Context:** IU-DIARY-01A resolved the Diary Moderation scope before Core implementation.

**Decision:**
- Use an explicit `Rejected` display state.
- Legacy `Displayed` and legacy-derived `TakenDown` posts may retain an absent historical `publishedAt`; never fabricate one.
- This course-scale implementation keeps `data/diaries.txt` and embedded `likedStudentIds` / `likeCount`.
- Migration to `LikeRelation` and `likes.csv` is deferred.
