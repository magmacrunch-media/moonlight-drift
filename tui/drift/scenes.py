"""The two screens, as scenes.

Modality is the stack, not a flag — the engine's own rule. :class:`TitleScene`
sits at the bottom; starting a run pushes a :class:`GameScene` on top of it,
and Esc pops back. Nothing needs an ``in_game`` boolean.

Both draw through the engine's ``Renderer``/``UISurface`` protocols and neither
knows what Textual is.

**This is the first real-time game on the terminal backend**, and two things
follow from that which the two turn-based ones never had to face.

*Thrust is a held button, and a terminal has none.* A terminal reports key
presses and never releases, so "is the key down right now" cannot be asked —
it can only be inferred from auto-repeat, which is what the engine's
``TuiInput`` decay does and what ``GameInfo.hold_ms`` sets. The game polls once
a frame and gets an answer good to about one repeat interval. It is not a
gamepad, but it is a playable approximation, and it is the reason this game
declares ``hold_ms`` at all.

*Poll once a frame; do not ask ``is_pressed``.* ``TuiInput`` reports a button
as active if it is inside its hold window **or** in the edge set, and the edge
set is cleared by ``poll()`` and by nothing else. A real-time game built on
``is_pressed`` alone latches the first press on forever — which reads as a
game that flies into the ceiling and cannot be steered, with no error
anywhere. Both of the two bugs that cost this port an afternoon were this
family: that one, and the Enter that starts a run being the same gamepad
button as thrust.

*The frame is a real frame.* A turn-based game repaints on a keypress; this one
repaints thirty times a second whether anything happened or not, which is the
first honest measurement of what the backend costs.
"""

from __future__ import annotations

import textwrap
from dataclasses import replace

from texastoast import scores
from texastoast.ui import DEFAULT_THEME, Menu, bigtext

from drift import characters, config, obstacles, projection, stars, theme
from drift.player import new_player

MENU_HELP = "↑↓ choose    Enter select    Q quit"
GAME_HELP = "SPACE/↑ thrust    R restart    Esc title    Q quit"
PILOT_HELP = "↑↓ choose    Enter fly    Esc back"
RULES_HELP = "↑↓ scroll    any other key goes back"
INITIALS_HELP = "Enter confirms    Backspace fixes"
SCORES_HELP = "any key goes back"

#: Ported from the browser build's "how to play", which is three lines, plus
#: the one thing a terminal has to explain that a browser does not: a keyboard
#: reports presses and never releases, so a held thrust is inferred from
#: auto-repeat and behaves a little differently from holding a mouse button.
RULES = (
    ("FLYING", (
        "Hold SPACE or the up arrow to climb. Release to fall. That is the "
        "whole control scheme.",
        "In a terminal a held key is not something a program can observe - "
        "keyboards report presses and never releases. Thrust is inferred from "
        "the key repeating, so it lets go about a tenth of a second after you "
        "do.",
    )),
    ("DYING", (
        "The columns are lethal, and so are the top and bottom of the world. "
        "Drifting off the screen ends the run exactly like hitting something.",
        "The exhaust plume below you is not solid, so it can overlap a column "
        "without killing you.",
    )),
    ("SCORING", (
        "One point per column cleared. Every tenth column is marked and "
        "brings a new palette with it.",
    )),
    ("PILOTS", (
        "Each of the 24 pilots has its own thrust, gravity and top speed, and "
        "they fly differently enough to notice in the first few seconds. "
        "Choose one from the title screen.",
    )),
)

#: Keys that count as thrust. Space is the browser's; the arrow is what a
#: player reaches for without being told.
#:
#: These are *key* names, used only to swallow the keypress in ``handle_key``.
THRUST_KEYS = ("space", "up", "w")

# Note the two vocabularies. These are *key* names; the engine speaks
# *buttons*, and ``TuiInput.KEY_MAP`` folds space/enter/z into the gamepad
# button "a" and up/w/k into "up". Asking ``is_pressed("space")`` is therefore
# always False — a trap worth naming, because it fails silently as a game that
# simply will not fly. ``update()`` reads the buttons off ``poll()``.


def _fit(text: str, width: int) -> str:
    if width <= 1 or len(text) <= width:
        return text
    return text[:width - 1] + "…"


def _too_small(renderer, cols: int, rows: int) -> bool:
    """Say so rather than drawing a clipped screen."""
    if renderer.width >= cols and renderer.height >= rows:
        return False
    renderer.ui_text(1, 1, "TERMINAL TOO SMALL", fill=theme.DEAD)
    renderer.ui_text(1, 2, f"need {cols}x{rows}, "
                           f"have {renderer.width}x{renderer.height}",
                     fill=theme.DIM)
    return True


class TitleScene:
    """Name, and a way in.

    ``render_below`` is deliberately not set: a run pushed on top covers this
    completely, so drawing underneath it would be wasted work.
    """

    ITEMS = ("START DRIFTING", "CHOOSE A PILOT", "HIGH SCORES",
             "HOW TO PLAY", "QUIT")

    def __init__(self, app):
        self.app = app
        self.menu = Menu(
            app.renderer,
            theme=_menu_theme(),
            # Cells, not pixels. The engine's defaults (280 wide, 32-cell rows)
            # would put the whole widget off-screen here.
            menu_width=theme.MENU_W,
            item_height=theme.MENU_ITEM_H,
            title_height=theme.MENU_TITLE_H,
            item_padding=theme.MENU_PAD,
            border_pad=theme.MENU_BORDER,
            selected_color=theme.MENU_SELECTED,
            normal_color=theme.VALUE,
        )
        self._show()

    def _show(self) -> None:
        self.menu.show(list(self.ITEMS), on_select=self._chose)

    def _chose(self, index: int, label: str) -> None:  # noqa: ARG002
        if index == 0:
            self.app.start_run()
        elif index == 1:
            self.app.choose_pilot()
        elif index == 2:
            self.app.show_scores()
        elif index == 3:
            self.app.show_rules()
        else:
            self.app.host.quit()

    def on_resume(self) -> None:
        # Menu.confirm() hides the menu as it fires, so the screen underneath
        # would otherwise come back blank.
        self._show()

    def handle_key(self, key: str) -> bool:
        if key in ("up", "w", "k"):
            self.menu.move_up()
        elif key in ("down", "s", "j"):
            self.menu.move_down()
        elif key in ("enter", "space"):
            self.menu.confirm()
        elif key == "q":
            self.app.host.quit()
        elif key == "escape":
            # Leave the game, not the process. Run on its own this is the last
            # scene and the session ends; under a launcher the arcade menu is
            # underneath and this returns to it. Same call either way.
            self.app.leave()
        else:
            return False
        return True

    def update(self, dt: float) -> None:
        pass

    def render(self) -> None:
        r = self.app.renderer
        r.clear()
        r.draw_rect(0, 0, r.width, r.height, theme.BG)
        if _too_small(r, theme.MENU_MIN_COLS, theme.MENU_MIN_ROWS):
            r.present()
            return

        cx = r.width // 2
        box_top = self._menu_box_top(r)
        y = _draw_title(r, cx, box_top)
        # Only if there is a row left for it. The title takes what it needs
        # first — it is the bigger thing on the screen — and a tagline drawn
        # anyway would land under the menu box and be painted over, which is a
        # missing line that reads as a design choice rather than a bug.
        if y < box_top:
            r.ui_text(cx, y, theme.TAGLINE, fill=theme.DIM, anchor="n")

        self.menu.render()

        pilot = f"{self.app.character.glyph}  {self.app.character.name}"
        r.ui_text(cx, r.height - 4, pilot, fill=self.app.character.accent,
                  anchor="n")
        if self.app.best:
            r.ui_text(cx, r.height - 3, f"best: {self.app.best}",
                      fill=theme.LABEL, anchor="n")
        r.ui_text(cx, r.height - 2, _fit(MENU_HELP, r.width - 2),
                  fill=theme.DIM, anchor="n")
        r.present()

    def _menu_box_top(self, renderer) -> int:
        """The row the menu's box starts on.

        The engine's ``Menu`` centres itself vertically, so the room a title
        has is everything above where the box lands, and that moves with the
        window. Working it out beats reserving a fixed number of rows, which
        lets a tall terminal draw a title the menu then paints over.
        """
        rows = len(self.ITEMS) * theme.MENU_ITEM_H + 2 * theme.MENU_PAD
        return (renderer.height - rows) // 2 - theme.MENU_BORDER


class PilotScene:
    """The roster.

    Twenty-four pilots will not fit a standard terminal, and the engine's Menu
    widget lays a box around its whole list, so this is drawn by hand: a
    scrolling window with the selection kept inside it.

    What separates one pilot from another here is thrust, gravity and top
    speed. The art cannot come across at two cells wide — see
    :mod:`drift.characters` — so the numbers are shown instead, for the
    highlighted pilot, because they are the part you actually feel.
    """

    def __init__(self, app):
        self.app = app
        self.selected = characters.ROSTER.index(app.character)
        self.offset = 0

    # -- Flow --------------------------------------------------------

    def _move(self, delta: int) -> None:
        self.selected = (self.selected + delta) % len(characters.ROSTER)
        self._scroll_into_view()

    def _viewport(self) -> int:
        """Rows of roster on screen, between the heading and the footer."""
        return max(1, self.app.renderer.height - 6)

    def _scroll_into_view(self) -> None:
        """Keep the selection on screen without moving the list more than it
        has to — a list that recentres on every keypress is unreadable."""
        viewport = self._viewport()
        if self.selected < self.offset:
            self.offset = self.selected
        elif self.selected >= self.offset + viewport:
            self.offset = self.selected - viewport + 1

    def handle_key(self, key: str) -> bool:
        if key in ("up", "w", "k"):
            self._move(-1)
        elif key in ("down", "s", "j"):
            self._move(1)
        elif key in ("enter", "space"):
            self.app.set_character(characters.ROSTER[self.selected])
            self.app.to_title()
        elif key in ("escape", "q"):
            # Back without choosing. Leaving without a way to cancel is how a
            # menu becomes a trap.
            self.app.to_title()
        else:
            return False
        return True

    def update(self, dt: float) -> None:
        pass

    def render(self) -> None:
        r = self.app.renderer
        r.clear()
        r.draw_rect(0, 0, r.width, r.height, theme.BG)
        if _too_small(r, theme.MENU_MIN_COLS, theme.MENU_MIN_ROWS):
            r.present()
            return

        viewport = self._viewport()
        self.offset = min(self.offset,
                          max(0, len(characters.ROSTER) - viewport))
        self._scroll_into_view()

        r.ui_text(r.width // 2, 1, "CHOOSE A PILOT", fill=theme.TITLE,
                  anchor="n")

        left = max(2, r.width // 2 - 16)
        y = 3
        window = characters.ROSTER[self.offset:self.offset + viewport]
        for i, pilot in enumerate(window, start=self.offset):
            if y >= r.height - 3:
                break
            chosen = i == self.selected
            marker = ">" if chosen else " "
            colour = pilot.accent if chosen else theme.LABEL
            r.ui_text(left, y, f"{marker} {pilot.glyph}  {pilot.name}",
                      fill=colour)
            y += 1

        pilot = characters.ROSTER[self.selected]
        stats = (f"thrust {pilot.thrust:+.2f}   gravity {pilot.gravity:.2f}   "
                 f"top speed {pilot.max_velocity:.1f}")
        r.ui_text(r.width // 2, r.height - 3, _fit(stats, r.width - 2),
                  fill=theme.VALUE, anchor="n")
        r.ui_text(r.width // 2, r.height - 2, _fit(PILOT_HELP, r.width - 2),
                  fill=theme.DIM, anchor="n")
        r.present()


class RulesScene:
    """How to play. Scrolls, for the reason George Boole's does: the text is
    longer than a standard terminal is tall, and a screen that stopped would
    cut off mid-sentence."""

    def __init__(self, app):
        self.app = app
        self.offset = 0

    def handle_key(self, key: str) -> bool:
        if key in ("up", "w", "k"):
            self.offset = max(0, self.offset - 1)
        elif key in ("down", "s", "j"):
            self.offset = min(self._max_offset(), self.offset + 1)
        else:
            self.app.to_title()
        return True

    def update(self, dt: float) -> None:
        pass

    def _lines(self, width: int) -> list[tuple[int, str, str]]:
        out: list[tuple[int, str, str]] = []
        for heading, paragraphs in RULES:
            out.append((0, heading, theme.MENU_SELECTED))
            for paragraph in paragraphs:
                for line in textwrap.wrap(paragraph, max(10, width - 2)):
                    out.append((2, line, theme.VALUE))
            out.append((0, "", theme.DIM))
        return out

    def _viewport(self) -> int:
        """Rows of text: the height less the heading block and the hint."""
        return max(1, self.app.renderer.height - 5)

    def _max_offset(self) -> int:
        r = self.app.renderer
        return max(0, len(self._lines(r.width - 4)) - self._viewport())

    def render(self) -> None:
        r = self.app.renderer
        r.clear()
        r.draw_rect(0, 0, r.width, r.height, theme.BG)
        if _too_small(r, theme.MENU_MIN_COLS, theme.MENU_MIN_ROWS):
            r.present()
            return

        lines = self._lines(r.width - 4)
        viewport = self._viewport()
        self.offset = min(self.offset, max(0, len(lines) - viewport))

        r.ui_text(2, 1, "HOW TO PLAY", fill=theme.TITLE)
        y = 3
        for indent, text, colour in lines[self.offset:self.offset + viewport]:
            if y >= r.height - 2:
                break
            if text:
                r.ui_text(2 + indent, y, text, fill=colour)
            y += 1

        more = len(lines) - (self.offset + viewport)
        r.ui_text(2, r.height - 2, _fit(RULES_HELP, r.width - 2), fill=theme.DIM)
        if more > 0:
            tail = f"{more} more ↓"
            r.ui_text(r.width - len(tail) - 2, r.height - 2, tail,
                      fill=theme.MENU_SELECTED)
        r.present()


class InitialsScene:
    """Who just did that.

    Three characters, the way every cabinet has asked since before anyone
    reading this was born. It sits over the finished run rather than replacing
    it, so the board and the wreck stay visible behind the question.

    Only shown when the score actually makes the table — being asked for your
    name and then told you came 213th is worse than not being asked.
    """

    #: The run underneath stays on screen. This is the one scene in the game
    #: that draws over something rather than instead of it.
    render_below = True

    def __init__(self, app, score: int):
        self.app = app
        self.score = score
        self.typed = ""

    def handle_key(self, key: str) -> bool:
        if key == "backspace":
            self.typed = self.typed[:-1]
        elif key in ("enter", "space"):
            self._confirm()
        elif key == "escape":
            # Recorded under whatever was last used rather than discarded. A
            # score is a fact; the initials are a label on it.
            self._confirm()
        elif len(key) == 1 and key.isalnum():
            if len(self.typed) < scores.INITIALS_LENGTH:
                self.typed += key.upper()
                if len(self.typed) == scores.INITIALS_LENGTH:
                    # Filled up. Confirming still takes a keypress, so a typo
                    # on the third letter is fixable.
                    pass
        else:
            return False
        return True

    def _confirm(self) -> None:
        self.app.record(self.score, self.typed or None)
        self.app.host.pop_scene()

    def update(self, dt: float) -> None:
        pass

    def render(self) -> None:
        r = self.app.renderer
        cx, cy = r.width // 2, r.height // 2
        box_w = min(r.width - 4, 40)
        r.draw_rect(cx - box_w // 2, cy - 3, box_w, 7, theme.MENU_BOX)

        r.ui_text(cx, cy - 2, "A NEW HIGH SCORE", fill=theme.MENU_SELECTED,
                  anchor="n")
        r.ui_text(cx, cy - 1, str(self.score), fill=theme.VALUE, anchor="n")

        slots = self.typed.ljust(scores.INITIALS_LENGTH, "_")
        r.ui_text(cx, cy + 1, "  ".join(slots), fill=self.app.character.accent,
                  anchor="n")
        r.ui_text(cx, cy + 2, _fit(INITIALS_HELP, box_w - 2), fill=theme.DIM,
                  anchor="n")
        r.present()


class ScoresScene:
    """The table. Scrolls, because a hundred scores do not fit anywhere."""

    def __init__(self, app):
        self.app = app
        self.offset = 0

    def handle_key(self, key: str) -> bool:
        if key in ("up", "w", "k"):
            self.offset = max(0, self.offset - 1)
        elif key in ("down", "s", "j"):
            self.offset = min(self._max_offset(), self.offset + 1)
        else:
            self.app.to_title()
        return True

    def update(self, dt: float) -> None:
        pass

    def _viewport(self) -> int:
        return max(1, self.app.renderer.height - 5)

    def _max_offset(self) -> int:
        return max(0, len(self.app.scores.load()) - self._viewport())

    def render(self) -> None:
        r = self.app.renderer
        r.clear()
        r.draw_rect(0, 0, r.width, r.height, theme.BG)
        if _too_small(r, theme.MENU_MIN_COLS, theme.MENU_MIN_ROWS):
            r.present()
            return

        entries = self.app.scores.load()
        viewport = self._viewport()
        self.offset = min(self.offset, max(0, len(entries) - viewport))

        r.ui_text(r.width // 2, 1, "HIGH SCORES", fill=theme.TITLE, anchor="n")

        if not entries:
            r.ui_text(r.width // 2, r.height // 2, "no scores yet",
                      fill=theme.DIM, anchor="n")
            r.ui_text(r.width // 2, r.height // 2 + 1,
                      "fly something and come back", fill=theme.DIM, anchor="n")
            r.ui_text(2, r.height - 2, SCORES_HELP, fill=theme.DIM)
            r.present()
            return

        left = max(2, r.width // 2 - 18)
        y = 3
        for i, entry in enumerate(entries[self.offset:self.offset + viewport],
                                  start=self.offset + 1):
            if y >= r.height - 2:
                break
            pilot = characters.BY_KEY.get(entry.extra.get("pilot", ""))
            glyph = pilot.glyph if pilot else "   "
            colour = pilot.accent if pilot else theme.VALUE
            r.ui_text(left, y, f"{i:>3}. {entry.initials}", fill=theme.VALUE)
            r.ui_text(left + 10, y, glyph, fill=colour)
            r.ui_text(left + 15, y, f"{entry.score:>6}", fill=theme.MENU_SELECTED)
            y += 1

        more = len(entries) - (self.offset + viewport)
        hint = "↑↓ scroll    any other key goes back" if (
            more > 0 or self.offset) else SCORES_HELP
        r.ui_text(2, r.height - 2, _fit(hint, r.width - 2), fill=theme.DIM)
        r.present()


class GameScene:
    """A run. Real time, thirty frames a second."""

    def __init__(self, app):
        self.app = app
        self.field = obstacles.ObstacleField(rng=app.rng)
        self.sky = stars.Starfield(rng=app.rng)
        self.player = new_player()
        self.score = 0
        self.frame = 0
        self.dead = False
        self.projection = projection.compute(1, 1)
        self.restart()

    # -- Flow --------------------------------------------------------

    def restart(self) -> None:
        # A run must not inherit the keypress that started it. `TuiInput`
        # folds enter, space and z into the one gamepad button "a", and thrust
        # reads that button — so without this, choosing START DRIFTING with
        # Enter holds thrust for `hold_ms` into the run and flies you into the
        # ceiling before you have touched anything. The same applies to the
        # Enter that restarts after a crash.
        self.app.host.input.clear()

        self.player = new_player()
        self.app.character.apply(self.player)
        self.field.reset()
        self.score = 0
        self.frame = 0
        self.dead = False
        # The last run's placing is not this run's. Cleared here rather than in
        # __init__ so that R clears it too.
        self.app.last_rank = None
        self._reproject()
        self.sky.seed(self.projection.world_w)

    def _reproject(self) -> None:
        """Recompute where the world lands. Cheap, and called every frame.

        A terminal is resized constantly and the projection is four
        multiplications; caching it would be an optimisation with a correctness
        bug attached, since ``world_w`` feeds the spawn and cull edges.
        """
        r = self.app.renderer
        self.projection = projection.compute(
            cols=r.width,
            rows=max(1, r.height - theme.HEADER_ROWS - theme.FOOTER_ROWS),
            origin_y=theme.HEADER_ROWS,
        )

    def _die(self) -> None:
        self.dead = True
        if self.app.qualifies(self.score):
            # Pushed over the wreck, which stays on screen behind it.
            self.app.enter_initials(self.score)
        elif self.score > 0:
            # Still recorded — it just did not earn being asked about.
            self.app.record(self.score)

    # -- Frame -------------------------------------------------------

    def update(self, dt: float) -> None:
        self._reproject()
        if self.dead:
            return

        self.frame += 1
        self.sky.update()

        # Held state, inferred from auto-repeat. See the module docstring.
        #
        # `poll()` rather than `is_pressed()`, and the difference is not
        # stylistic. `TuiInput` reports a button as active if it is either
        # within its hold window *or* in the edge set — and the edge set is
        # cleared by `poll()` and nothing else. A real-time game that only ever
        # asked `is_pressed` would latch the first press on forever and fly
        # straight into the ceiling. One poll per frame is the protocol's own
        # shape; `is_pressed` is for asking again about a frame already polled.
        state = self.app.host.input.poll()
        thrust = state.a or state.up

        if self.player.update(thrust):
            self._die()
            return

        if self.frame % config.OBSTACLE_SPAWN_INTERVAL == 0:
            self.field.create(self.projection.world_w)
        self.field.update()

        if self.field.collides(self.player.hitbox):
            self._die()
            return

        self.score += self.field.score(int(self.player.x))

    def handle_key(self, key: str) -> bool:
        if key == "r":
            self.restart()
        elif key == "q":
            self.app.host.quit()
        elif key == "escape":
            self.app.to_title()
        elif self.dead and key in ("enter", "space"):
            self.restart()
        elif key in THRUST_KEYS:
            # The press itself is handled by the held-state decay in update();
            # swallowing it here stops it reaching anything underneath.
            return True
        else:
            return False
        return True

    # -- Drawing -----------------------------------------------------

    def render(self) -> None:
        r = self.app.renderer
        r.clear()
        r.draw_rect(0, 0, r.width, r.height, theme.BG)
        if _too_small(r, theme.MIN_COLS, theme.MIN_ROWS):
            r.present()
            return

        p = self.projection
        r.draw_rect(0, theme.HEADER_ROWS, r.width,
                    r.height - theme.HEADER_ROWS - theme.FOOTER_ROWS,
                    theme.FIELD_BG)

        self._draw_stars(r, p)
        self._draw_columns(r, p)
        self._draw_player(r, p)
        self._draw_hud(r)
        if self.dead:
            self._draw_death(r)
        r.present()

    def _draw_stars(self, r, p) -> None:
        for star in self.sky.lit:
            x, y = p.col(star.x), p.row(star.y)
            if 0 <= x < r.width and theme.HEADER_ROWS <= y < r.height - theme.FOOTER_ROWS:
                r.ui_text(x, y, star.glyph, fill=star.color)

    def _draw_columns(self, r, p) -> None:
        """One cell-column at a time, asking the silhouette where the ink is.

        Drawing the bounding box instead would show a rectangle where the
        collision is a taper — the shape you see has to be the shape you hit,
        or the game is lying about why you died.
        """
        top_row = theme.HEADER_ROWS
        bottom_row = r.height - theme.FOOTER_ROWS
        for o in self.field.obstacles:
            palette = o.theme
            for row in range(top_row, bottom_row):
                # The world y this row's centre stands for.
                world_y = int((row + 0.5 - p.origin_y) / p.scale_y)
                if o.top_height <= world_y <= o.bottom_y:
                    continue
                left, right = o.silhouette(world_y)
                if right <= left:
                    continue
                c0, c1 = p.col(left), p.col(right)
                for col in range(max(0, c0), min(r.width, max(c1, c0 + 1))):
                    edge = col in (c0, c1 - 1)
                    r.ui_text(col, row,
                              theme.COLUMN_EDGE if edge else theme.COLUMN,
                              fill=palette.secondary if edge else palette.primary)
            if o.milestone:
                label = str(o.milestone)
                col = p.col(o.x + config.OBSTACLE_WIDTH / 2) - len(label) // 2
                row = p.row(o.top_height + config.GAP / 2)
                if 0 <= col < r.width - len(label) and top_row <= row < bottom_row:
                    r.ui_text(col, row, label, fill=palette.accent)

    def _draw_player(self, r, p) -> None:
        glyph = self.app.character.glyph
        col = p.col(self.player.x)
        row = p.row(self.player.y)
        row = min(max(row, theme.HEADER_ROWS), r.height - theme.FOOTER_ROWS - 1)
        colour = theme.DEAD if self.dead else self.app.character.accent
        r.ui_text(col, row, glyph, fill=colour)

        # The plume, when climbing. Below the body, and not solid — collision
        # excludes it, so drawing it as part of the ship would be a lie.
        if not self.dead and self.player.velocity < 0:
            below = row + 1
            if below < r.height - theme.FOOTER_ROWS:
                r.ui_text(col, below, "v", fill=theme.FLAME)

    def _draw_hud(self, r) -> None:
        r.ui_text(1, 0, f"SCORE {self.score}", fill=theme.VALUE)
        if self.app.best:
            best = f"BEST {self.app.best}"
            r.ui_text(r.width - len(best) - 1, 0, best, fill=theme.LABEL)
        name = self.app.character.name
        r.ui_text(r.width // 2 - len(name) // 2, 0, name, fill=theme.LABEL)
        r.ui_text(1, r.height - 1, _fit(GAME_HELP, r.width - 2), fill=theme.DIM)

    def _draw_death(self, r) -> None:
        cx, cy = r.width // 2, r.height // 2
        rank = self.app.last_rank
        placed = f"score {self.score}" + (f"    rank {rank}" if rank else "")
        for i, line in enumerate(("YOU DRIFTED OFF", placed,
                                  "Enter to fly again    Esc for the title")):
            fill = theme.DEAD if i == 0 else theme.VALUE if i == 1 else theme.DIM
            r.ui_text(cx, cy - 1 + i, _fit(line, r.width - 2),
                      fill=fill, anchor="n")


def _draw_title(renderer, cx: int, box_top: int) -> int:
    """The name, set as large as the window allows. Returns the row below it.

    Every rung shows the whole name; what changes is how much of it is drawn in
    block letters rather than typed. See :mod:`texastoast.ui.bigtext`.
    """
    budget = box_top - 1
    for big, rest in theme.TITLE_LADDER:
        needed = bigtext.height(big) + (1 if rest else 0)
        if bigtext.width(big) > renderer.width - 2 or needed > budget:
            continue
        y = 1
        for line in bigtext.lines(big):
            renderer.ui_text(cx, y, line, fill=theme.TITLE, anchor="n")
            y += 1
        if rest:
            renderer.ui_text(cx, y, rest, fill=theme.SUBTITLE, anchor="n")
            y += 1
        return y
    renderer.ui_text(cx, 1, theme.BANNER, fill=theme.TITLE, anchor="n")
    return 2


def _menu_theme():
    """The engine's Theme, recoloured. ``dataclasses.replace`` because
    :class:`~texastoast.ui.theme.Theme` is frozen."""
    return replace(
        DEFAULT_THEME,
        primary=theme.MENU_SELECTED,
        text=theme.VALUE,
        dim_text=theme.DIM,
        box_fill=theme.MENU_BOX,
        box_outline=theme.LABEL,
        outline_width=1,
        selection_fill=theme.MENU_SELECTION_BG,
    )


__all__ = ["GAME_HELP", "MENU_HELP", "THRUST_KEYS", "GameScene", "TitleScene"]
