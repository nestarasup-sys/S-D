# ADR 0035: AI artwork replaces the default native appearance

- Status: accepted
- Date: 2026-10-28

## Context

The user-provided AI Asset Library v1 contains 48 transparent object PNGs and 12 low-resolution scene previews. The first integration briefly exposed those images under additional `illustrated:<nativeId>` entries. That created duplicate library items and left the simpler procedural art as the default, contrary to the intended visual direction.

## Decision

- Keep the existing 72 native IDs and registry entries. The 48 object PNGs and 12 scene images are attached as embedded artwork metadata to the corresponding original native IDs.
- `nativeLibrary()` remains the single source of truth for the library. The QPainter native-art resolver selects embedded PNG art for those 60 IDs and uses the prior procedural renderer only as an internal fallback if the embedded image cannot be loaded.
- Do not present procedural fallback drawings as a second user-selectable appearance.
- At the time of this decision, `person` was left procedural while the 11 graph/data assets stayed procedural because their dynamic behavior matters. The `person` part was superseded by [ADR 0036](0036-ai-presenter-pose-animation.md): its original procedural drawing is now internal fallback, while the existing `person` ID defaults to the 12 embedded pose PNGs. The 11 graph/data assets remain procedural.
- The 12 scene PNGs remain explicitly marked preview-only. Replacing them with Full HD files must preserve their original native IDs and resource paths.
- Keep compatibility for `illustrated:<nativeId>` values emitted by the short-lived prior integration: the core library resolver maps those aliases to the corresponding original native metadata. Workspace migration marks built-in alias rows as deleted, preserving catalogue history and project data while removing them from active library counts and selections.
- Do not claim pixel recoloring for PNG artwork. Existing color properties remain accepted for project/recipe compatibility, but they do not change PNG pixels.

## Consequences

- Existing recipes using ordinary IDs such as `car` now render the new artwork without schema changes.
- The library still contains 72 natives, not 132. The personal library and nine premium resources remain separate and unchanged.
- Transformations, groups, keyframes, camera movement, opacity, and supported Visual Beats continue to apply to the PNG as normal node-level operations.
- The QPainter image cache stores decoded embedded PNGs and bounds their retained memory. The project stores stable native IDs rather than external file paths.
- The procedural fallback remains code-only and is never presented as an alternate library item.
- The artwork package was provided by the user and is identified as AI-generated user content; the package does not grant rights to third-party marks or material.

## Validation

The registry and capability catalogue should report 72 native IDs, with the 48 object assets, 12 scene previews and the `person` pose set marked as `embedded-ai-png`; the 12 scene images are marked as low-resolution previews, and the 11 graph/data entries remain `procedural`. Pose timing and the optional walk cycle are specified in [ADR 0036](0036-ai-presenter-pose-animation.md). UI, workspace migration, project serialization, recipe compilation, QPainter preview and a short MP4 export are verified with focused tests. Long renderer benchmarks and experimental GPU tests are outside this decision.
