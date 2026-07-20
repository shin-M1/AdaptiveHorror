# AGENTS.md — Adaptive Horror Codex Rules

This file contains persistent Codex rules for this repository only. Put product requirements in `REQUIREMENTS.md`, verification commands in `TEST_PLAN.md`, milestone order in `ROADMAP.md`, and task-specific scope in `TASKS/*.md`.

## Source of truth

Use this priority order when instructions conflict:

1. Current user instruction.
2. Active `TASKS/*.md`.
3. `AGENTS.md`.
4. `REQUIREMENTS.md`.
5. `TEST_PLAN.md`.
6. `ROADMAP.md`.

`README.md`, `DEV_LOG.md`, `TODO.md`, and `BUILD_CHECK.md` are supporting context. They must not override the documents above.

If a document disagrees with code, tests, or runtime logs, prefer the verified code/test/runtime evidence and record the mismatch in `DEV_LOG.md` or the final report.

## Repository inspection policy

Do not scan or summarize the whole repository unless the task explicitly requires it.

Inspect in this order:

1. Active `TASKS/*.md`.
2. Files listed under that task's `Primary files`.
3. Files listed under that task's `Secondary files`, only when needed.
4. Direct dependencies referenced by inspected files.
5. Files named by build, test, or runtime errors.

Repository-wide search is allowed only to locate an unknown symbol, identify call sites before a behavioral change, investigate a regression with no known owner, or check whether a required capability already exists. Stop searching once there is enough evidence to implement safely.

## Completion stop rule

When all automated acceptance criteria in the active task pass:

1. Stop implementation.
2. Do not add optional polish.
3. Do not refactor unrelated code.
4. Update only required documentation.
5. Commit and push the task branch if the task requires it.
6. Produce the final report.

Record extra ideas in `TODO.md` or a future task instead of expanding the current scope.

## Autonomous fix loop

For implementation tasks, repeat:

1. Read the active task and related source-of-truth documents.
2. Inspect only related code, tests, and logs.
3. Implement the smallest viable change.
4. Run the required automated verification from `TEST_PLAN.md` or the task.
5. Fix the first real failure.
6. Re-run the required verification.

Treat Live Coding as non-verification. Required C++ verification is Development Editor / Win64 without Live Coding unless a task explicitly says otherwise.

Do not claim visual feel, controls, horror presentation, audio feel, or PIE-only behavior as passed unless a human PIE result exists. Record unrun or impossible checks as `Not verified`, not `Passed`.

Stop instead of guessing when:

- The same failure cause appears three times.
- The fix loop reaches five iterations.
- Toolchain or environment dependencies block progress.
- A destructive Git/filesystem operation would be required.
- Requirements contradict each other.
- Authentication, external service access, or user-only input is required.

When stopping, report the cause, attempts, evidence, remaining work, and next safe action.

## Git rules

- Do not change `main` directly.
- Start from latest `main` and create the task branch requested by the task or user.
- Do not merge into `main`.
- Do not rebase, cherry-pick, rewrite commits, delete branches, or discard user work unless explicitly instructed.
- Commit only after required automated verification passes, or after documenting why verification is impossible for documentation-only work.
- Push the feature/chore branch when required.
- End with a clean working tree.
- Do not stage generated solution/workspace files unless they are intentional task outputs.

## Final report format

Final reports should include:

- Summary.
- Review or implementation result.
- Changed files.
- Verification commands and results.
- Acceptance criteria status.
- PIE/manual items not verified.
- Known issues/TBD.
- Commit hash.
- Push result.
- Working tree state.
