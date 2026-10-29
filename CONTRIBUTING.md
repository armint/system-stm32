# Contributing

## Branches

The repository has one branch: `main`. All work, bug fixes too, goes to
`main` through a pull request. The project has no releases yet.

## Code format

The repository uses clang-format with the rules in `.clang-format`. These
are the rules of the libArT project. Use clang-format 20. The file has
deprecated keys, and other major versions can give a different format.
Format each C or C++ file before you commit it:

```sh
clang-format -i <file>
```

## Formatting commits and git blame

`.git-blame-ignore-revs` lists commits that change only the format. Tell git
to skip them in `git blame`:

```sh
git config blame.ignoreRevsFile .git-blame-ignore-revs
```

The GitHub and GitLab blame views read this file too.

Put a pure formatting change into a commit of its own. Add its full SHA to
`.git-blame-ignore-revs` in a later commit. Merge such a pull request with
a merge commit. A rebase or a squash changes the SHA, and the entry then
has no effect.
