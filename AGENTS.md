# AGENTS.md — Adaptive Horror Codex Rules

This file defines persistent rules for Codex work in this repository. Keep it short. Put specifications in `REQUIREMENTS.md`, verification details in `TEST_PLAN.md`, roadmap order in `ROADMAP.md`, and task-specific instructions in `TASKS/*.md`.

## Source of truth

Use this priority order when instructions conflict:

1. Current user instruction.
2. The target `TASKS/*.md`.
3. `AGENTS.md`.
4. `REQUIREMENTS.md`.
5. `TEST_PLAN.md`.
6. `ROADMAP.md`.

If documents disagree with code or runnable test results, prefer code and runnable results. Record the mismatch in `DEV_LOG.md` or the final report.

## Work rules

- Do not change `main` directly. Start from latest `main`, then create a task branch.
- Do not merge into `main`.
- Do not rebase, cherry-pick, rewrite existing commits, or delete branches unless the user explicitly asks.
- Do not refactor unrelated code.
- Preserve stable systems: game flow, player death/retry, Stage Clear, Runtime NavMesh, Zombie, HUNTER, ADAM, HUD, Debug HUD, Content Pass progression, and existing tests.
- Treat Live Coding as non-verification. Required build verification is Development Editor / Win64 without Live Coding.
- Do not mark visual feel, controls, horror presentation, audio feel, or PIE-only behavior as confirmed unless a human PIE result exists.
- Record unrun or impossible checks as `Not verified`, not `Passed`.
- Keep changes as small as the task allows.

## Autonomous fix loop

For implementation tasks, repeat:

1. Read the current task and relevant documents.
2. Inspect only related code and tests.
3. Implement the smallest viable change.
4. Run Development Editor build.
5. Run `Automation RunTests AdaptiveHorror`.
6. Run Runtime Smoke.
7. Scan logs.
8. Fix the first real failure.
9. Re-run required verification.

Stop instead of guessing when any of these occur:

- Same failure cause appears 3 times.
- Total fix loop reaches 5 iterations.
- Environment/toolchain dependency blocks progress.
- A destructive Git/filesystem operation would be required.
- Requirements contradict each other.
- Authentication, external service access, or user-only input is required.

When stopping, report cause, attempts, evidence, remaining work, and the next safe action.

## Git rules

- Commit only after required build and automated verification pass, or after clearly documenting why verification is impossible for documentation-only work.
- Push the feature/chore branch.
- End with a clean working tree.
- Do not leave generated solution/workspace files staged unless they are intentional task outputs.

## Standard short launch prompt

```text
feature/field-pass1としてTASKS/field-pass-1.mdを実装してください。

最初にAGENTS.mdを読み、
REQUIREMENTS.md、TEST_PLAN.md、ROADMAP.mdの関連箇所を確認してください。

自動検証可能な受け入れ条件を満たすまで、
実装、Build、Automation、Runtime Smoke、ログ確認、原因修正を繰り返してください。

条件を満たした場合のみcommitし、
origin/feature/field-pass1へpushしてください。

人間のPIE確認が必要な項目は成功扱いにせず、
最終報告へPIE未確認として残してください。

途中報告や質問は、作業継続不能な場合を除いて不要です。
```

## Final report format

- Summary.
- Changed files.
- Build result.
- Automation result.
- Runtime Smoke result.
- Log scan result.
- Acceptance criteria status.
- PIE/manual items not verified.
- Known issues/TBD.
- Commit hash.
- Push result.
- Working tree state.
