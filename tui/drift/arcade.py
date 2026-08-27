"""What the arcade needs to know to launch this game.

The entry point a launcher resolves. Kept tiny and free of imports that cost
anything: a menu listing several games loads one of these per installed game
just to draw a row, and it should not pay for the game's rules or its screens
to do that.
"""

from __future__ import annotations

from typing import Any

from texastoast.arcade import GameInfo

from drift import theme

INFO = GameInfo(
    key="moonlight-drift",
    title="Moonlight Drift",
    blurb="Thread the columns. Hold to climb, release to fall.",
    # The first real-time game in the family, and the first to want either of
    # these to be anything but the default.
    #
    # 30 fps because the world moves whether or not a key was pressed, and 20
    # is visibly steppy once something is scrolling.
    #
    # hold_ms is the whole reason the engine has it. A terminal reports key
    # presses and never releases, so a held thrust has to be inferred from
    # auto-repeat: 140ms is comfortably above a typical repeat interval, so a
    # held key reads as held, and short enough that letting go registers
    # within about two frames.
    fps=30,
    hold_ms=140,
    min_cols=theme.MIN_COLS,
    min_rows=theme.MIN_ROWS,
)


class DriftGame:
    """Satisfies :class:`texastoast.arcade.ArcadeGame`."""

    info = INFO

    def start(self, host: Any) -> Any:
        """The title screen, ready to be pushed.

        Imported here rather than at module scope so that listing this game in
        an arcade menu does not drag in its rules, its screens, or Textual.
        """
        from drift.app import DriftApp

        return DriftApp(host).root_scene


#: What the entry point resolves to. Stateless — a run's state belongs to the
#: DriftApp that :meth:`DriftGame.start` creates, so replaying makes a new one.
GAME = DriftGame()

__all__ = ["GAME", "INFO", "DriftGame"]
