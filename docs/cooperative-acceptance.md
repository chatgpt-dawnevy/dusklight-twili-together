# Cooperative release acceptance

Agreed 2026-11-15. This checklist describes required work, not implemented coverage.
A release is complete only when every requirement has implementation and recorded test evidence.

- Two separately downloadable artifacts built from the same commit:
  `twili_together-windows-x86_64.dusk` and `twili_together-linux-x86_64.dusk`.
- All enemy actor types: ordinary enemies, bosses, story encounters, summoned enemies,
  projectiles and combat dependencies. Synchronize movement, facing, attacks, phases,
  damage, health, defeats, rewards and progression triggers.
- Every item pickup grants the item to all teammates, including randomizer checks.
- Room option selects a shared consumable stock or individual consumable stocks.
  In both modes pickups are shared. Shared stock also shares expenditure.
- Players can explore different rooms, layers and dungeons concurrently. Rejoining,
  reconnecting and loading saves converge on common progress without duplicate rewards.
- TwilitRealm/dusklight-randomizer with identical version, seed and settings for teammates.
  Test progression items, keys, bottles, purchases, trades, traps, duplicate/concurrent
  checks, late joining and reconnection; complete vanilla and randomized adventures.
- Preserve and run existing tests; add behavioral tests for each new requirement.
  Test both directions of player actions, concurrent actions and authority changes.
- Game tests must never open windows on the user's desktop. The Linux runner owns a
  separate Xvfb server and refuses to launch the game if isolation is unavailable.
- Use feature branches and pull requests on chatgpt-dawnevy/dusklight-twili-together;
  integrate only verified work. Keep reports explicit about pending criteria.

## Evidence baseline

Prior commits implement partial enemy support. They do not satisfy universal enemy
synchronization or full-campaign acceptance. Previous Linux combat tests do not prove
Windows compatibility or randomizer campaign completion. Treat each missing proof as
pending, not as a pass.
