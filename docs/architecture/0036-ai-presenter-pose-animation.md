# ADR 0036: Replace `person` artwork and add timed pose and walk animation

- Status: accepted
- Date: 2026-10-28
- Supersedes: the `person` artwork decision in ADR 0035

## Context

The Studio Dark already renders the 48 object PNGs and 12 provisional scene PNGs under their original native IDs. The presenter package contributes 12 transparent 410×410 PNGs, including two walk poses that the user explicitly approved. A second library ID would create a duplicate and break existing recipes, presets, and projects.

The existing `person` node has one stable identity, general transform keyframes, groups, camera motion, and an older `pose` property. Its `motion` property controls entry/exit animation, so it must not be repurposed for expressions or walking.

## Decision

- Keep native ID `person` and make the embedded `neutral.png` the library thumbnail and default appearance. Resolve the remaining pose names through the same `LibraryItem` artwork metadata and QPainter decoded-image cache used for the AI artwork library.
- Preserve the 12 source PNGs byte-for-byte. Align their measured foot-support anchors to one shared node baseline; do not crop the gesture bounds. Retain the old vector painter only as an internal recovery path for unreadable/missing embedded art.
- Keep `pose` as the initial/static pose and accept the canonical names `neutral`, `smile`, `wave`, `point-right`, `point-left`, `explain`, `thinking`, `surprised`, `tablet`, `thumbs-up`, `walk-a`, and `walk-b`. Preserve aliases `point` → `point-right` and `worried` → `thinking`.
- Add recipe-v3 `poseKeyframes`, each containing `at` and `pose`. Resolve `at` with the existing measured cue, cueProgress, offsetSeconds, or aligned wordIndex timing. Pose changes are discrete, deterministic and persistent, and modify only the artwork selected for the same node.
- Add recipe-v3 `walk` with `start`, `end`, `stepFrames`, and `stopPose`. The interval is half-open. Within it, alternate the two approved walk PNGs without crossfade; after it, hold `stopPose` until a later pose keyframe. Require ordinary node position keyframes to produce at least eight pixels of displacement across the interval. This avoids claiming that a leg cycle alone moves the character.
- Store resolved pose keyframes and the walk interval in project schema 16. Preserve schemas 1–15. Keep `motion` semantics, numeric transform interpolation, and existing QPainter preview/export unchanged.
- Keep the raster colors fixed. `color` and `secondaryColor` remain compatible node fields but do not recolor the illustration.

## Consequences

- Existing `kind: "native", asset: "person"` recipes now render the illustrated presenter without changing their ID. Older `neutral`, `point`, `wave`, and `worried` values continue to resolve deterministically.
- The library still has 72 native IDs. The new art adds 12 embedded images but no additional item: 48 object IDs + 12 scene IDs + `person` use embedded PNGs; the 11 data/chart IDs stay procedural.
- QPainter and the shared `RenderSnapshot` select pose artwork for both preview and export. The image cache avoids decoding a pose from disk every frame; the PNGs are embedded as Qt resources.
- Pose selection in the inspector records a discrete keyframe through the editor command/history path. Recipe-generated keyframes remain editable project data. During a walk interval, the cycle controls the displayed pose; regular transform keyframes control actual travel.
- The walk drawings have slightly different perspective by design. A discrete switch preserves both original artworks and may show that designed perspective change; no artificial crossfade is used.
- There is no automatic footstep-to-distance physics, pose retargeting, pixel recoloring, or automatic walking displacement. Recipes must define node transform keyframes and an aligned time interval.

## Validation

The official small production fixture parses, compiles with measured cue/word timing, validates as schema 16, renders representative pose frames through QPainter, and round-trips through project storage. A short local MP4 export and the 12-pose contact sheet are kept under ignored `artifacts/presenter-01/`; no generated video is committed.
