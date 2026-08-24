# Contributing to Chronos

Thanks for taking an interest. Bug reports, fixes and features are all welcome.

## Reporting a bug

Open an issue with:

- what you did, what you expected, what happened
- the output of `chronos stats`, and `chronos voicestats` if it involves voice
- server console output, including anything printed with the `[chronos]` prefix
- whether the module or the addon was rebuilt since it last worked

For voice problems, run `chronos voicechain` first. It traces the whole path
from Auris to the client and names the broken link, which is usually the answer.
`chronos voicedebug 1` then logs every capture and every drop.

## Building

```
premake5 --gmcommon=garrysmod_common vs2019
msbuild projects/windows/vs2019/chronos.sln /p:Configuration=Release /p:Platform=Win32
```

Requires [garrysmod_common](https://github.com/danielga/garrysmod_common) and a
`sourcesdk-minimal` checkout beside it. Win32 only; the module is 32-bit srcds.

`deploy.sh` copies the built DLL and the Lua addon into a local srcds install.
Edit the `GMOD` path at the top to match yours. If a server has the DLL loaded
the copy is staged as `.dll.new` instead of failing.

## Testing a change

There is no test suite. Verify by hand:

1. `chronos record`, move props, fire a weapon, talk, spawn and delete things
2. `chronos stop`, then scrub the whole range with the slider
3. `chronos play` from the start and watch effects, sounds and voice land where
   they were recorded
4. `chronos exit` and confirm the live world is intact: nothing hidden, no
   leftover ghosts, players unfrozen and moving

Changes to the capture plan deserve a `chronos.GetPlan(index)` check on an
affected entity, to confirm the props you expect were actually resolved.

## Code style

**C++**

- Match the surrounding style: Valve-ish spacing inside parens, tabs, one type
  per file group. Look at a neighbouring file before adding a new one.
- Never dereference an entity, edict or plan without checking it first. A bad
  pointer here takes the whole server down.
- SendProp offsets are trusted only after they pass the bounds and type checks
  in `ResolveLeaf`. Anything new that reads from an entity goes through a plan.

**Lua**

- EmmyLua annotations on functions, classes and any non-obvious value.
- Comment why, not what. Historical gotchas and non-obvious decisions earn a
  comment; standard Lua patterns do not.
- Max 4 functions per file, max 30 lines per function. Split rather than grow.
- The client is an attacker. Every value from a `net.Receive` or concommand is
  type-checked, range-clamped and length-capped before it is used, and every
  entry point is admin-gated or rate-limited.

**Both**

- English only, in code and comments.
- No trailing whitespace.

## Pull requests

Keep a PR to one change. Explain what broke or what was missing, not just what
the diff does; the reasoning is what the comments in this codebase are for, and
it is what makes a change reviewable.

By contributing you agree that your work is licensed under
[CC BY-NC-SA 4.0](LICENSE), the same terms as the project.
