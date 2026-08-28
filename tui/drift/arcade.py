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
    # Real-time, but edge-driven, which is an unusual pair and worth saying.
    #
    # 30 fps because the world moves whether or not a key was pressed, and 20
    # is visibly steppy once something is scrolling.
    #
    # hold_ms is 0 — the default — even though this is the only game here that
    # wants a held control. Held state in a terminal is inferred from key
    # repeat, and a keyboard sends one event and then goes silent for its
    # repeat *delay*, around half a second, before repeating. Thrust read
    # through that silence cuts out for a third of a second exactly when you
    # first press, which is a boost that does not work. So a press is an
    # impulse instead (see config.FLAP) and held state is never consulted.
    fps=30,
    hold_ms=0,
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
