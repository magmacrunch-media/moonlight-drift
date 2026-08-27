"""Wiring — what outlives a single run, and how the screens reach it.

Everything that draws lives in :mod:`drift.scenes`; everything that decides
lives in :mod:`drift.player`, :mod:`drift.obstacles` and
:mod:`drift.projection`. This holds them together.

**The terminal is not owned here.** It belongs to a
:class:`~texastoast.core.tui_host.TuiHost`, which this is handed — the same
arrangement the other two terminal games use. That is what lets the game run as
its own command *and* be seated by a launcher without knowing which happened.

What is left is genuinely this game's: who you fly as, the best score so far,
and the run's random source.
"""

from __future__ import annotations

import random
from typing import Any

from drift import characters
from drift.scenes import GameScene, TitleScene


class DriftApp:
    """A session of Moonlight Drift, drawing on somebody else's terminal."""

    def __init__(self, host: Any, character: str | None = None,
                 seed: int | None = None):
        self.host = host
        #: Seeded so a run can be replayed, which is what makes a collision
        #: report reproducible rather than a story about one time it happened.
        self.rng = random.Random(seed)
        self.character = characters.get(character or characters.DEFAULT.key)
        #: Best score for as long as this session lasts.
        self.best = 0

        #: The title screen. The caller pushes it — a game that pushed its own
        #: scene would take that decision away from whatever is seating it.
        self.root_scene = TitleScene(self)

    # -- What the scenes reach for -----------------------------------

    @property
    def renderer(self):
        return self.host.renderer

    # -- Flow --------------------------------------------------------

    def start_run(self) -> None:
        """Begin a run, over the title screen, which waits underneath."""
        self.host.push_scene(GameScene(self))

    def choose_pilot(self) -> None:
        """The roster, over the title screen."""
        from drift.scenes import PilotScene

        self.host.push_scene(PilotScene(self))

    def show_rules(self) -> None:
        """How to play, over the title screen."""
        from drift.scenes import RulesScene

        self.host.push_scene(RulesScene(self))

    def set_character(self, character) -> None:
        """Fly as somebody else from the next run on.

        A run in progress keeps whoever it started with — swapping physics
        underneath a player mid-flight would be a different game, not a
        setting.
        """
        self.character = character

    def to_title(self) -> None:
        """Leave the current run for the title screen underneath it."""
        self.host.pop_scene()

    def leave(self) -> None:
        """Leave the game entirely.

        Popping the title screen. Run on its own that is the last scene and the
        session ends; seated by a launcher the arcade menu is underneath and
        this returns to it. The game does not need to know which — see
        ``TuiHost.pop_scene``.
        """
        self.host.pop_scene()

    def record(self, score: int) -> None:
        if score > self.best:
            self.best = score

    # -- Introspection, for tests ------------------------------------

    @property
    def scene(self):
        return self.host.scene

    @property
    def in_game(self) -> bool:
        return isinstance(self.host.scene, GameScene)


def run(character: str | None = None, seed: int | None = None,
        skip_title: bool = False) -> None:
    """Play Moonlight Drift as its own command."""
    from texastoast.core.tui_host import TuiHost

    from drift.arcade import GAME

    host = TuiHost(title=GAME.info.title, fps=GAME.info.fps,
                   hold_ms=GAME.info.hold_ms)
    app = DriftApp(host, character, seed)
    host.push_scene(app.root_scene)
    if skip_title:
        host.push_scene(GameScene(app))
    host.run()
