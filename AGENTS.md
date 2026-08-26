# Moonlight Drift — agent brief

One game, two versions, one repo:

- `web/` — browser version (adenosine engine, plain JS). Source of truth for
  gameplay and balance. Deployed by the website repo: run
  `make sync-moonlight-drift` there to copy `web/` into `arcade/moonlight-drift/`.
  Never edit the website repo's copy directly — it gets overwritten.
- `wii/` — Wii port (magnolia engine, C99). Has its own `AGENTS.md` with build
  and porting detail. Expects magnolia checked out beside this repo.

A gameplay change is not done until both versions have it (or the commit says
why one is skipped). `web/js/` is the reference the Wii port's comments cite.
