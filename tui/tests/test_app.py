"""The screens, driven headlessly.

These cover what ``test_physics.py`` cannot: that the simulation is wired to
the screen correctly, and that the title and a run hand off to each other. They
need the engine and its terminal extra, and are skipped without them so the
simulation tests still run on a bare checkout.

Textual's ``run_test`` pilot gives a real app with a real event loop and a real
size, so key handling, the frame loop and resize are exercised as they are in
play — no mocking of the parts most likely to break.
"""

import asyncio

import pytest

pytest.importorskip("textual", reason='needs: pip install -e ".[dev]" with texastoast[tui]')

from texastoast.arcade import ArcadeGame  # noqa: E402
from texastoast.core.tui_host import TuiHost  # noqa: E402

from drift import characters, config, scenes, theme  # noqa: E402  # noqa: E402
from drift.app import DriftApp  # noqa: E402
from drift.arcade import GAME  # noqa: E402
from drift.scenes import GameScene, TitleScene  # noqa: E402


def buffer_text(app: DriftApp) -> str:
    return app.host.game.surface.buffer.to_text()


def settle(app: DriftApp) -> None:
    """Apply the stack's pending push/pop, which the engine defers a frame."""
    app.host.stack.update(0.0)


def hosted(seed: int = 3) -> DriftApp:
    """A session on a real host, built the way the standalone command does."""
    host = TuiHost(title=GAME.info.title, fps=GAME.info.fps,
                   hold_ms=GAME.info.hold_ms)
    app = DriftApp(host, seed=seed)
    host.push_scene(app.root_scene)
    settle(app)
    return app


def flying(seed: int = 3) -> tuple[DriftApp, GameScene]:
    app = hosted(seed)
    app.start_run()
    settle(app)
    return app, app.host.scene


async def _piloted(app: DriftApp, size=(80, 24)):
    from texastoast.core.tui_game import _GameApp

    textual_app = _GameApp(app.host.game, app.host.game.surface)
    app.host.game._app = textual_app
    return textual_app.run_test(size=size)


def run(coro):
    return asyncio.run(coro)


# ── The declaration ─────────────────────────────────────────────────


def test_the_game_is_a_valid_arcade_cabinet():
    assert isinstance(GAME, ArcadeGame)
    assert GAME.info.key == "moonlight-drift"


def test_it_asks_for_a_real_time_terminal():
    """The first game in the family to want either of these to be anything but
    the default. Without hold_ms a held thrust is unreadable; at 20 fps a
    scrolling world is visibly steppy."""
    assert GAME.info.fps == 30
    assert GAME.info.hold_ms >= 100


def test_start_returns_a_scene_without_pushing_it():
    """The launcher decides what to do with it — a game that pushed its own
    scene would take that decision away."""
    host = TuiHost(title="t", fps=GAME.info.fps, hold_ms=GAME.info.hold_ms)
    scene = GAME.start(host)
    assert isinstance(scene, TitleScene)
    assert len(host.stack) == 0


# ── The stack ───────────────────────────────────────────────────────


def test_the_title_is_the_bottom_of_the_stack():
    app = hosted()
    assert isinstance(app.host.scene, TitleScene)
    assert not app.in_game


def test_starting_a_run_pushes_it_over_the_title():
    app, scene = flying()
    assert app.in_game
    assert len(app.host.stack) == 2
    assert isinstance(app.host.stack.scenes[0], TitleScene)


def test_escape_pops_back_to_the_title():
    app, scene = flying()
    scene.handle_key("escape")
    settle(app)
    assert isinstance(app.host.scene, TitleScene)
    assert len(app.host.stack) == 1


def test_the_title_is_usable_again_after_a_run():
    """Menu.confirm() hides the menu as it fires, so without on_resume the
    screen underneath comes back empty."""
    app, scene = flying()
    scene.handle_key("escape")
    settle(app)
    assert app.host.scene.menu.active


def test_a_run_is_not_updated_while_the_title_is_on_top():
    """Modality is the stack: popping the run means it stops getting frames,
    with no flag anywhere saying so."""
    app, scene = flying()
    before = scene.frame
    app.host.stack.update(0.1)
    assert scene.frame > before

    scene.handle_key("escape")
    settle(app)
    frozen = scene.frame
    for _ in range(5):
        app.host.stack.update(0.1)
    assert scene.frame == frozen


# ── Flight ──────────────────────────────────────────────────────────


def test_falling_with_no_input_ends_the_run():
    """Gravity is the default state. Doing nothing has to kill you, or the
    game has no stakes."""
    app, scene = flying()
    for _ in range(400):
        app.host.stack.update(1 / 30)
        if scene.dead:
            break
    assert scene.dead


def test_holding_thrust_keeps_you_up():
    """The held-state read is the whole reason this game declares hold_ms."""
    app, scene = flying()
    source = app.host.input
    start = scene.player.y
    # Fifteen frames, not sixty: thrust held long enough climbs into the
    # ceiling, which is what the next test is about.
    for _ in range(15):
        source.press("space")          # auto-repeat, as a terminal delivers it
        app.host.stack.update(1 / 30)
    assert not scene.dead
    assert scene.player.y < start, "should have climbed"


def test_a_run_does_not_inherit_the_key_that_started_it():
    """TuiInput folds enter, space and z into the one gamepad button "a", and
    thrust reads that button. Without a clear on restart, choosing START
    DRIFTING with Enter holds thrust into the run and flies you into the
    ceiling before you have touched anything."""
    app = hosted()
    app.host.input.press("enter")
    assert app.host.input.is_pressed("a"), "the press should register at all"

    app.start_run()
    settle(app)
    assert not app.host.input.is_pressed("a"), "the run inherited the keypress"

    scene = app.host.scene
    start = scene.player.y
    for _ in range(3):
        app.host.stack.update(1 / 30)
    assert scene.player.y > start, "should be falling, not climbing"


def test_restarting_after_a_crash_does_not_inherit_the_enter_either():
    app, scene = flying()
    scene._die()
    app.host.input.press("enter")
    scene.handle_key("enter")
    assert not app.host.input.is_pressed("a")
    assert not scene.dead


def test_thrust_lets_go_when_the_keys_stop_coming():
    """The regression that cost this port an afternoon.

    TuiInput reports a button active if it is inside its hold window *or* in
    the edge set, and the edge set is cleared by poll() and nothing else. Built
    on is_pressed alone, the first press latches thrust on forever and the game
    flies into the ceiling with no error anywhere. Polling once a frame is what
    makes letting go mean anything.
    """
    app, scene = flying()
    source = app.host.input
    # A clock the test advances, so hold_ms means frames rather than
    # whatever wall time a fast loop happens to take.
    now = [0.0]
    source._clock = lambda: now[0]
    source.clear()

    for _ in range(6):
        now[0] += 1000 / 30
        source.press("space")
        app.host.stack.update(1 / 30)
    climbing = scene.player.velocity
    assert climbing < 0, "should be climbing while the key repeats"

    # Stop pressing. Well past hold_ms, thrust must have let go.
    for _ in range(20):
        now[0] += 1000 / 30
        app.host.stack.update(1 / 30)
    assert scene.player.velocity > 0, "thrust latched on after the key stopped"


def test_the_ceiling_is_lethal_too():
    app, scene = flying()
    source = app.host.input
    for _ in range(400):
        source.press("space")
        app.host.stack.update(1 / 30)
        if scene.dead:
            break
    assert scene.dead, "holding thrust forever should fly you into the ceiling"


def test_clearing_a_column_scores():
    app, scene = flying()
    scene.field.create(scene.projection.world_w)
    scene.field.obstacles[0].x = float(config.PLAYER_X - config.OBSTACLE_WIDTH - 1)
    app.host.stack.update(1 / 30)
    assert scene.score == 1


def test_dying_records_the_best_score():
    app, scene = flying()
    scene.score = 7
    scene._die()
    assert app.best == 7
    assert scene.dead


def test_restarting_clears_the_run_but_keeps_the_best():
    app, scene = flying()
    scene.score = 5
    scene._die()
    scene.restart()
    assert scene.score == 0
    assert not scene.dead
    assert scene.field.obstacles == []
    assert app.best == 5


def test_a_dead_run_stops_simulating():
    app, scene = flying()
    scene._die()
    frozen = scene.frame
    for _ in range(10):
        app.host.stack.update(1 / 30)
    assert scene.frame == frozen


def test_the_same_seed_flies_the_same_run():
    """Seeded, so a collision report is reproducible rather than a story about
    one time it happened."""
    def fly(seed):
        app, scene = flying(seed)
        for _ in range(200):
            app.host.input.press("space")
            app.host.stack.update(1 / 30)
        return [(round(o.x, 3), o.top_height) for o in scene.field.obstacles]

    assert fly(11) == fly(11)


# ── Drawing ─────────────────────────────────────────────────────────


def test_the_title_screen_says_the_whole_name():
    app = hosted()

    async def go():
        async with await _piloted(app) as pilot:
            await pilot.pause()
            await asyncio.sleep(0.25)
            text = buffer_text(app)
            assert theme.TAGLINE in text
            assert "START DRIFTING" in text
            app.host.quit()

    run(go())


def test_the_run_draws_the_pilot_the_score_and_the_sky():
    app, scene = flying()

    async def go():
        async with await _piloted(app) as pilot:
            await pilot.pause()
            await asyncio.sleep(0.3)
            text = buffer_text(app)
            assert app.character.glyph in text
            assert "SCORE" in text
            assert app.character.name in text
            assert any(g in text for g in ("·", "*", "✦")), "no starfield"
            app.host.quit()

    run(go())


def test_columns_are_drawn_where_the_collision_is():
    """The shape you see has to be the shape you hit, or the game is lying
    about why you died — so the drawing asks the silhouette, not the box."""
    app, scene = flying()
    scene.field.create(config.WORLD_WIDTH)
    o = scene.field.obstacles[0]
    o.x = 400.0

    async def go():
        async with await _piloted(app) as pilot:
            await pilot.pause()
            await asyncio.sleep(0.3)
            text = buffer_text(app)
            assert theme.COLUMN in text or theme.COLUMN_EDGE in text
            app.host.quit()

    run(go())


def test_the_death_banner_appears_and_says_the_score():
    app, scene = flying()

    async def go():
        async with await _piloted(app) as pilot:
            await pilot.pause()
            scene.score = 4
            scene._die()
            await asyncio.sleep(0.25)
            text = buffer_text(app)
            assert "DRIFTED OFF" in text
            assert "score 4" in text
            app.host.quit()

    run(go())


def test_a_terminal_below_the_floor_is_told_so_rather_than_drawn_in():
    app, scene = flying()

    async def go():
        async with await _piloted(app, size=(30, 10)) as pilot:
            await pilot.pause()
            await asyncio.sleep(0.25)
            text = buffer_text(app)
            assert "TOO SMALL" in text
            assert "SCORE" not in text
            app.host.quit()

    run(go())


def test_resizing_reprojects_rather_than_keeping_a_stale_world():
    """world_w feeds the spawn and cull edges, so a cached projection would be
    an optimisation with a gameplay bug attached."""
    app, scene = flying()

    async def go():
        async with await _piloted(app, size=(60, 24)) as pilot:
            await pilot.pause()
            await asyncio.sleep(0.25)
            narrow = scene.projection.world_w
            await pilot.resize_terminal(120, 40)
            await asyncio.sleep(0.3)
            assert scene.projection.world_w > narrow
            app.host.quit()

    run(go())


# ── The engine seam ─────────────────────────────────────────────────


def test_listing_this_game_does_not_drag_in_its_rules_or_its_screens():
    """A menu loads one arcade module per installed game just to draw a row."""
    import subprocess
    import sys

    code = ("import sys, drift.arcade; "
            "print([m for m in ('drift.obstacles', 'drift.scenes', 'drift.app', "
            "'textual') if m in sys.modules])")
    out = subprocess.run([sys.executable, "-c", code], capture_output=True,
                         text=True, check=True)
    assert out.stdout.strip() == "[]", out.stdout


def test_the_simulation_needs_no_engine():
    """drift.config, projection, player, obstacles and characters claim to
    import nothing outside the standard library. This is that claim, tested."""
    import subprocess
    import sys

    code = ("import sys, drift.config, drift.projection, drift.player, "
            "drift.obstacles, drift.characters, drift.stars; "
            "print([m for m in sys.modules if m.startswith('texastoast') "
            "or m == 'textual'])")
    out = subprocess.run([sys.executable, "-c", code], capture_output=True,
                         text=True, check=True)
    assert out.stdout.strip() == "[]", out.stdout


def test_the_theme_moves_in_runs_on_screen_too():
    """Every column on screen shares a palette; a fresh one per column reads as
    noise. Pinned here as well as in test_physics because it is the thing that
    made the Wii port not look like the arcade version."""
    app, scene = flying()
    for _ in range(config.THEME_CHANGE_INTERVAL - 1):
        scene.field.create(config.WORLD_WIDTH)
    themes = {o.theme for o in scene.field.obstacles}
    assert len(themes) == 1, "palette changed mid-run"

# ── Choosing a pilot ────────────────────────────────────────────────


def test_the_title_offers_a_roster_and_the_rules():
    app = hosted()
    assert app.host.scene.ITEMS == ("START DRIFTING", "CHOOSE A PILOT",
                                    "HOW TO PLAY", "QUIT")


def test_the_roster_opens_on_whoever_you_are_flying():
    """Reopening it should put the cursor where you left it, not make you
    scroll back to yourself every time."""
    app = hosted()
    app.set_character(characters.ROSTER[9])
    app.choose_pilot()
    settle(app)
    assert app.host.scene.selected == 9


def test_choosing_a_pilot_changes_who_flies():
    app = hosted()
    app.choose_pilot()
    settle(app)
    roster = app.host.scene
    roster.handle_key("down")
    roster.handle_key("enter")
    settle(app)
    assert app.character is characters.ROSTER[1]
    assert isinstance(app.host.scene, TitleScene)


def test_escape_leaves_the_roster_without_choosing():
    """A menu with no way out is a trap."""
    app = hosted()
    before = app.character
    app.choose_pilot()
    settle(app)
    app.host.scene.handle_key("down")
    app.host.scene.handle_key("escape")
    settle(app)
    assert app.character is before
    assert isinstance(app.host.scene, TitleScene)


def test_the_roster_wraps_at_both_ends():
    app = hosted()
    app.choose_pilot()
    settle(app)
    roster = app.host.scene
    roster.selected = 0
    roster.handle_key("up")
    assert roster.selected == len(characters.ROSTER) - 1
    roster.handle_key("down")
    assert roster.selected == 0


def test_a_new_pilot_is_flown_on_the_next_run():
    app = hosted()
    app.set_character(characters.get("fire-toad"))
    app.start_run()
    settle(app)
    scene = app.host.scene
    assert scene.player.thrust == characters.get("fire-toad").thrust


def test_a_run_in_progress_keeps_the_pilot_it_started_with():
    """Swapping physics underneath a player mid-flight would be a different
    game, not a setting."""
    app = hosted()
    app.start_run()
    settle(app)
    scene = app.host.scene
    before = scene.player.gravity
    app.set_character(characters.get("fire-toad"))
    app.host.stack.update(1 / 30)
    assert scene.player.gravity == before


def test_the_roster_scrolls_so_every_pilot_is_reachable():
    """Twenty-four pilots do not fit a standard terminal, which is why this is
    drawn by hand rather than with the engine's Menu widget."""
    async def go():
        app = hosted()
        async with await _piloted(app, size=(80, 24)) as pilot:
            await pilot.pause()
            await asyncio.sleep(0.25)
            app.choose_pilot()
            settle(app)
            await asyncio.sleep(0.25)
            roster = app.host.scene
            assert len(characters.ROSTER) > roster._viewport(), "no scroll needed?"

            for _ in range(len(characters.ROSTER) - 1):
                await pilot.press("down")
            await asyncio.sleep(0.25)
            text = buffer_text(app)
            last = characters.ROSTER[-1]
            assert last.name in text, "the last pilot is unreachable"
            assert roster.selected == len(characters.ROSTER) - 1
            app.host.quit()

    run(go())


def test_the_highlighted_pilots_physics_are_shown():
    """The art cannot come across at two cells wide, so the numbers are what
    distinguishes one pilot from another."""
    async def go():
        app = hosted()
        async with await _piloted(app) as pilot:
            await pilot.pause()
            app.choose_pilot()
            settle(app)
            await asyncio.sleep(0.3)
            text = buffer_text(app)
            first = characters.ROSTER[0]
            assert f"{first.gravity:.2f}" in text
            assert "thrust" in text and "top speed" in text
            app.host.quit()

    run(go())


# ── How to play ─────────────────────────────────────────────────────


def test_the_rules_open_and_any_other_key_closes_them():
    app = hosted()
    app.show_rules()
    settle(app)
    assert isinstance(app.host.scene, scenes.RulesScene)
    app.host.scene.handle_key("x")
    settle(app)
    assert isinstance(app.host.scene, TitleScene)


def test_the_rules_scroll_rather_than_truncating():
    async def go():
        app = hosted()
        async with await _piloted(app, size=(80, 24)) as pilot:
            await pilot.pause()
            app.show_rules()
            settle(app)
            await asyncio.sleep(0.3)
            assert "FLYING" in buffer_text(app)

            for _ in range(40):
                await pilot.press("down")
            await asyncio.sleep(0.3)
            assert "PILOTS" in buffer_text(app), "the last heading is unreachable"
            app.host.quit()

    run(go())


def test_the_rules_never_write_over_their_own_hint():
    async def go():
        app = hosted()
        async with await _piloted(app, size=(80, 24)) as pilot:
            await pilot.pause()
            app.show_rules()
            settle(app)
            await asyncio.sleep(0.3)
            rows = buffer_text(app).splitlines()
            hint = rows[app.renderer.height - 2]
            assert hint.strip().startswith("↑↓ scroll"), hint
            app.host.quit()

    run(go())


def test_scrolling_the_rules_stops_at_both_ends():
    app = hosted()
    app.show_rules()
    settle(app)
    rules = app.host.scene
    rules.handle_key("up")
    assert rules.offset == 0
    for _ in range(200):
        rules.handle_key("down")
    assert rules.offset == rules._max_offset()
    assert isinstance(app.host.scene, scenes.RulesScene)
