# Development instructions

- The user does not need to understand programming. Own implementation, organization and verification; explain outcomes in Portuguese.
- Save local commits for code changes. Do not push without a request.
- Keep domain code in `modules/core` free of Qt and UI dependencies.
- UI changes call validated commands through the application controller. Never mutate project data in QML.
- Share composition evaluation and drawing between preview and render. Selection overlays remain UI-only.
- Keep object IDs stable and list model rows stable during gestures; one drag must undo as one action.
- Keep tool downloads in `.tools`, generated binaries in `build`, verification output in `artifacts`; never commit these folders or personal projects.
- Preserve file revisions, atomic writes and prior copies. Test stale-revision failures without overwriting the current project.
- On Windows use `scripts/build.ps1 -Test` for code changes and `scripts/smoke.ps1` when composition, QML startup or rendering changes.
- Update `README.md` and the relevant architecture decision when behavior or limitations change. Do not describe planned capabilities as implemented.
