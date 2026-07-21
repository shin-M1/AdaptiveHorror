# Pull Request Review Checklist

Use this checklist for repeatable Pull Request review and merge decisions. It is intentionally short and practical; task-specific acceptance criteria stay in `TASKS/*.md`.

## Identity

- [ ] PR number:
- [ ] URL:
- [ ] Base branch:
- [ ] Compare branch:
- [ ] Expected commits:
- [ ] Dependencies:

## Scope

- [ ] Matches task scope.
- [ ] Changed files reviewed.
- [ ] Production files reviewed.
- [ ] No unrelated changes.
- [ ] Protected systems unchanged unless explicitly in scope.
- [ ] Expected change scope respected.

## Verification

- [ ] Build result recorded.
- [ ] Automation result recorded.
- [ ] Runtime Smoke result recorded.
- [ ] Current-run Log Scan result recorded.
- [ ] `git diff --check` passed.
- [ ] Working tree clean.
- [ ] GitHub checks reviewed or explicitly absent.

## Documentation

- [ ] Task document matches implementation.
- [ ] Status documents are consistent.
- [ ] Human verification wording is accurate.
- [ ] Markdown code blocks are closed.
- [ ] No stale artifacts or citation placeholders.

## Merge

- [ ] No unresolved conflicts.
- [ ] Required checks passed.
- [ ] Merge Commit will be used.
- [ ] Squash merge will not be used.
- [ ] Rebase merge will not be used.
- [ ] Merge is explicitly requested by the user or current task.

## Post-Merge

- [ ] `main` updated with `git pull --ff-only origin main`.
- [ ] Expected commits confirmed as ancestors of `main`.
- [ ] Main validation passed.
- [ ] Working tree clean.
- [ ] Stop and report.
