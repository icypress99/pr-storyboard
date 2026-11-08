# PR Storyboard

GitEase plugin that turns commits ahead of your base into a scrubbable **storyboard of intent**, then drafts and opens a pull request from that story.

## Features

- Horizontal commit / cluster frames with +/- heat
- Clustering by conventional-commit type or top-level path
- Frame detail with file list and copyable `git show` / `git diff` command
- PR title + Markdown body generated from frames
- Create PR via `gh pr create` (or open an existing PR)
- Branch context-menu: **Open Storyboard**

## Build

```bash
cmake -S . -B build -G Ninja -DCMAKE_PREFIX_PATH=<Qt6> -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Requires GitEase SDK headers (defaults to `../../GitEase/Src/Plugins`, or pass `-DGITEASE_SDK_DIR=...`) and Qt 6 Quick.

On success the build deploys to `%APPDATA%/GitEase/plugins/pr-storyboard` and writes `pr-storyboard-0.1.0.gep`.

## Usage

1. Open a repo with commits on a feature branch.
2. Open **Storyboard** from the navigation rail.
3. Pick a base if needed, scrub frames (←/→ or Play).
4. Edit the draft title/body, then **Create PR**.

Storyboard works offline with `git`. Creating a PR requires authenticated `gh`.

## Test fixture

Use [`pr-storyboard-fixture`](https://github.com/icypress99/pr-storyboard-fixture) (local twin: `C:\work\Ronia\Magic\pr-storyboard-fixture`).

1. Open that repo in GitEase and check out `demo/storyboard`.
2. Open **Storyboard** — you should see ~8 commits ahead of `main`, clustering into fewer frames.
3. Scrub with Play / arrows, then **Create PR** against `main`.
