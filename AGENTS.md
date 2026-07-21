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

## GitHub Pull Request workflow

Use GitHub Pull Requests as the standard merge path when `gh` is available and authenticated. Do not merge or push directly to `main` as a substitute for a PR.

- Create or update work on a feature, fix, or chore branch.
- Confirm the PR base and compare branches before review.
- Review the PR diff and changed-file scope before merging.
- Confirm GitHub checks with `gh pr checks` when checks are configured.
- Merge only when the user or current task explicitly requests a merge.
- Use a normal Merge Commit.
- Do not use squash merge.
- Do not use rebase merge.
- After merge, checkout `main`, pull with `--ff-only`, and rerun required validation on `main`.
- Do not continue new development on `main` after merge validation.
- Stop after validation and report the merge result.

Standard commands:

```powershell
gh auth status
gh pr view <number> --repo <owner/repo>
gh pr checks <number> --repo <owner/repo>
gh pr merge <number> --repo <owner/repo> --merge
```

If `gh` is unavailable, unauthenticated, lacks permission, is blocked by branch protection, is waiting for approval, or reports failed checks, stop and report the blocker. Do not work around this by local direct merge/push to `main`.

## Pull Request review requirements

Before merging a PR, confirm at minimum:

- Task scope match.
- Changed files.
- Production changed files and expected change scope.
- No unrelated protected-system changes.
- Documentation consistency.
- Build result.
- Automation result.
- Runtime Smoke result.
- Current-run Log Scan result.
- `git diff --check`.
- Working tree clean.
- GitHub checks state.
- No unresolved conflicts.
- Human-only verification items are clearly labeled.

Stop without merging if any of these are present:

- Unrelated production changes.
- Protected-system changes outside the task.
- Material mismatch between documentation and implementation.
- Build, Automation, Runtime Smoke, Log Scan, or required GitHub checks fail.
- Unresolved conflicts.
- PR base/compare mismatch.
- Missing expected dependency branch or commit.
- Authentication, permission, or approval blocker.

## Post-merge validation

After a PR merge, run verification on `main`:

- Update `main` with `git pull --ff-only origin main`.
- Confirm expected commits are ancestors of `main` when the task names them.
- Run Generate Project Files as part of the standard validation script.
- Run Development Editor / Win64 build.
- Run Automation.
- Run Runtime Smoke.
- Run current-run Log Scan.
- Run `git diff --check`.
- Confirm `git status` is clean.

If post-merge validation fails, do not commit directly to `main`. Investigate the cause and report whether a new fix branch is needed. Do not automatically start the next task.

## Branch dependency rule

Create new task branches from latest `main` by default. If a task explicitly depends on an unmerged branch:

- Branch from the dependency branch instead of `main` only when the task says to do so.
- Record the dependency in the task document and final report.
- Use this format:

```text
Depends on:
feature/example-branch
```

- State the required merge order.
- Do not merge the child branch to `main` before its dependency is merged.
- Stop and report when an expected dependency is missing.

## Human verification rule

Codex can claim only checks it actually ran, such as:

- Build.
- Automation.
- Runtime Smoke.
- Structural logs.
- Static diff review.
- Log Scan.

Codex must not claim human-only checks as newly verified, including:

- PIE visual confirmation.
- Visual quality.
- Control feel.
- Level readability.
- Presentation quality.
- Subjective gameplay feel.

Use clear labels such as:

- `Human verification required`.
- `Human PIE not verified`.
- `Previously human-verified`.

Do not use claims like `gameplay visually verified` unless the user supplied that human result.

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
