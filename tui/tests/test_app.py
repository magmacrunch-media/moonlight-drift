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

pytest.importorskip("textual", reason='needs: pip install -e ".[dev]"')

from magmacrunch.engine import scores as score_mod  # noqa: E402
from magmacrunch.engine.arcade import ArcadeGame  # noqa: E402
from magmacrunch.engine.core.tui_host import TuiHost  # noqa: E402

from drift import characters, config, scenes, theme  # noqa: E402
from drift.app import DriftApp  # noqa: E402
from drift.arcade import GAME  # noqa: E402
from drift.scenes import GameScene, TitleScene  # noqa: E402


@pytest.fixture(autouse=True)
def isolated_scores(tmp_path, monkeypatch):
    """No test may touch a real player's score file.

    Autouse rather than opt-in: a suite that can quietly delete somebody's
    high scores is not one you want to run twice, and remembering to ask for
    a fixture is exactly the kind of thing that gets forgotten in the test
    added six months from now.
    """
    monkeypatch.setenv(score_mod.DATA_DIR_ENV, str(tmp_path))


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


def tap(app: DriftApp, key: str = "space") -> None:
    """A keypress the way the host delivers one.

    TuiHost drains the input source into the top scene and *then* updates, so a
    test that only calls press() never reaches handle_key — and thrust is a
    press now, not a held state. Getting this wrong is how the first version of
    these tests passed against a game nobody could fly.
    """
    app.host.input.press(key)
    for pressed in app.host.input.drain():
        app.host.stack.dispatch_key(pressed)


def flying(seed: int = 3) -> tuple[DriftApp, GameScene]:
    app = hosted(seed)
    app.start_run()
    settle(app)
    return app, app.host.scene


async def _piloted(app: DriftApp, size=(80, 24)):
    from magmacrunch.engine.core.tui_game import _GameApp

    textual_app = _GameApp(app.host.game, app.host.game.surface)
    app.host.game._app = textual_app
    return textual_app.run_test(size=size)


def run(coro):
    return asyncio.run(coro)


# ── The declaration ─────────────────────────────────────────────────


def test_the_game_is_a_valid_arcade_cabinet():
    assert isinstance(GAME, ArcadeGame)
    assert GAME.info.key == "moonlight-drift"


def test_it_asks_for_a_real_time_terminal_but_edge_driven_input():
    """An unusual pair, and worth pinning.

    30 fps because the world moves whether or not a key was pressed. hold_ms 0
    because held state in a terminal is inferred from key repeat, and a
    keyboard is silent for its repeat *delay* before repeating — thrust read
    through that silence cuts out exactly when you press. Thrust is an impulse
    instead, so held state is never consulted.
    """
    assert GAME.info.fps == 30
    assert GAME.info.hold_ms == 0


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


def test_a_tap_climbs():
    app, scene = flying()
    start = scene.player.y
    tap(app)
    app.host.stack.update(1 / 30)
    assert scene.player.velocity < 0, "a press should set a climb"
    assert scene.player.y < start


def test_a_tap_sets_the_climb_rather_than_adding_to_it():
    """Holding pins a steady climb instead of accelerating without limit, and
    a rapid tapper cannot stack flaps into something the physics never
    intended."""
    app, scene = flying()
    for _ in range(5):
        tap(app)
    expected = scene.player.thrust * config.FLAP
    assert scene.player.velocity == pytest.approx(expected)


def test_one_tap_carries_past_the_keyboards_repeat_delay():
    """The whole reason thrust is an impulse.

    A keyboard sends one event and then goes silent for about 500ms before
    repeating. If a single flap stopped lifting before that, holding the key
    would visibly stall — which is precisely the bug this replaced.
    """
    app, scene = flying()
    tap(app)
    frames = 0
    while scene.player.velocity < 0:
        app.host.stack.update(1 / 30)
        frames += 1
    assert frames / 30 * 1000 > 500, "a flap must outlast the repeat delay"


def test_holding_climbs_without_a_dead_patch():
    """Simulated against a real keyboard: one press, silence, then repeats.
    The old held-state reading fell for a third of a second in the middle of
    this, which is what 'the boost does not work' looked like."""
    app, scene = flying()
    start = scene.player.y
    highest = start
    last_repeat = None
    for frame in range(30):
        now_ms = frame * 1000 / 30
        if now_ms == 0 or (now_ms >= 500 and
                           (last_repeat is None or now_ms - last_repeat >= 33)):
            tap(app)
            last_repeat = now_ms
        app.host.stack.update(1 / 30)
        highest = min(highest, scene.player.y)
        assert scene.player.y <= start + 1, (
            f"sank below the start at {now_ms:.0f}ms — the dead patch is back")
    assert highest < start - 40, "should have made real height"


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


def test_thrust_stops_when_the_taps_do():
    app, scene = flying()
    tap(app)
    for _ in range(25):
        app.host.stack.update(1 / 30)
    assert scene.player.velocity > 0, "should be falling again"


def test_the_ceiling_is_lethal_too():
    app, scene = flying()
    for _ in range(400):
        tap(app)
        app.host.stack.update(1 / 30)
        if scene.dead:
            break
    assert scene.dead, "climbing forever should fly you into the ceiling"


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
    settle(app)
    # A qualifying score asks for initials first; confirming is what records it.
    if not app.in_game:
        app.host.scene.handle_key("enter")
        settle(app)
    assert app.best == 7
    assert scene.dead


def test_restarting_clears_the_run_but_keeps_the_best():
    app, scene = flying()
    scene.score = 5
    scene._die()
    settle(app)
    if not app.in_game:
        app.host.scene.handle_key("enter")
        settle(app)
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


# ── Pause ─────────────────────────────────────────────────────────────


def test_p_stops_the_clock():
    app, scene = flying()
    for _ in range(5):
        app.host.stack.update(1 / 30)
    scene.handle_key("p")
    frozen = scene.frame
    for _ in range(10):
        app.host.stack.update(1 / 30)
    assert scene.paused
    assert scene.frame == frozen


def test_p_again_starts_it():
    app, scene = flying()
    scene.handle_key("p")
    scene.handle_key("p")
    frozen = scene.frame
    for _ in range(10):
        app.host.stack.update(1 / 30)
    assert not scene.paused
    assert scene.frame > frozen


def test_a_pause_does_not_bank_thrust():
    """The one way a pause can change a run rather than suspend it.

    Thrust is an impulse that sets ``velocity`` outright, so a flap taken
    while the world is stopped would sit on the player untouched by gravity
    and spend itself the moment play resumed — a free climb bought by
    pausing. It has to be inert instead, not queued.
    """
    app, scene = flying()
    for _ in range(5):
        app.host.stack.update(1 / 30)
    scene.handle_key("p")
    before = scene.player.velocity
    for _ in range(5):
        tap(app)
    assert scene.player.velocity == before, "a paused flap should do nothing"


def test_a_pause_swallows_the_thrust_key_rather_than_passing_it_on():
    """Inert is not the same as unhandled: the key must not fall through to
    whatever would otherwise act on it."""
    app, scene = flying()
    scene.handle_key("p")
    assert scene.handle_key("space") is True


def test_pausing_still_reprojects_so_a_resize_survives_it():
    """``update`` returns early while paused, and the projection is computed
    before that return for exactly this case."""
    app, scene = flying()

    async def go():
        async with await _piloted(app, size=(60, 24)) as pilot:
            await pilot.pause()
            await asyncio.sleep(0.25)
            scene.handle_key("p")
            narrow = scene.projection.world_w
            await pilot.resize_terminal(120, 40)
            await asyncio.sleep(0.3)
            assert scene.paused, "the resize should not have resumed the run"
            assert scene.projection.world_w > narrow
            app.host.quit()

    run(go())


def test_a_crashed_run_cannot_be_paused():
    app, scene = flying()
    scene._die()
    scene.handle_key("p")
    assert not scene.paused


def test_restarting_clears_the_pause():
    app, scene = flying()
    scene.handle_key("p")
    scene.restart()
    assert not scene.paused


def test_the_game_help_fits_the_smallest_terminal():
    """``_fit`` truncates from the right and says nothing about it, so a hint
    line that outgrows the floor loses its last key silently — which was
    ``Q quit``, the one a stuck player most needs."""
    assert len(scenes.GAME_HELP) <= theme.MIN_COLS - 2


def test_the_same_seed_flies_the_same_run():
    """Seeded, so a collision report is reproducible rather than a story about
    one time it happened."""
    def fly(seed):
        app, scene = flying(seed)
        for _ in range(200):
            tap(app)
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
            assert "START DRIFTING" in text
            assert "HIGH SCORES" in text
            app.host.quit()

    run(go())


def test_the_title_takes_the_room_and_the_tagline_yields():
    """Nothing is ever drawn where the menu box will land. At 80x24 the block
    title fills the gap and the tagline is dropped; give it more rows and the
    tagline comes back. The alternative was drawing it anyway and having the
    box paint over it, which reads as a design choice rather than a bug."""
    async def go(size, tagline_expected):
        app = hosted()
        async with await _piloted(app, size=size) as pilot:
            await pilot.pause()
            await asyncio.sleep(0.25)
            text = buffer_text(app)
            assert (theme.TAGLINE in text) is tagline_expected, size
            assert "START DRIFTING" in text, "the menu is always reachable"
            app.host.quit()

    run(go((80, 24), False))
    run(go((80, 30), True))


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
            # Recorded without the prompt, so the banner is what is on top.
            app.record(4)
            scene.dead = True
            await asyncio.sleep(0.25)
            text = buffer_text(app)
            assert "DRIFTED OFF" in text
            assert "score 4" in text
            app.host.quit()

    run(go())


def test_the_pause_banner_appears_and_says_the_score():
    app, scene = flying()

    async def go():
        async with await _piloted(app) as pilot:
            await pilot.pause()
            scene.score = 6
            scene.handle_key("p")
            await asyncio.sleep(0.25)
            text = buffer_text(app)
            assert "PAUSED" in text
            assert "score 6" in text
            assert "DRIFTED OFF" not in text, "a stopped run is not a dead one"
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
            "print([m for m in sys.modules if m.startswith('magmacrunch') "
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
                                    "HIGH SCORES", "HOW TO PLAY", "QUIT")


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

# ── High scores ─────────────────────────────────────────────────────


def test_the_board_is_filed_under_the_key_the_browser_uses():
    """A shared board later has to mean a shared board, not two boards with
    the same name."""
    assert DriftApp.SCORE_KEY == "moonlight-drift"


def test_a_score_survives_the_session(tmp_path):
    """The whole point of the file. A fresh app on the same directory sees
    what the last one recorded."""
    from magmacrunch.engine.scores import ScoreBook

    book = ScoreBook("moonlight-drift", directory=tmp_path)
    first = DriftApp(TuiHost(title="t"), scores=book)
    first.record(300, "jam")

    second = DriftApp(TuiHost(title="t"),
                      scores=ScoreBook("moonlight-drift", directory=tmp_path))
    assert second.best == 300


def test_a_score_records_who_flew_it():
    app = hosted()
    app.set_character(characters.get("fire-toad"))
    app.record(120, "jam")
    assert app.scores.load()[0].extra == {"pilot": "fire-toad"}


def test_dying_with_a_qualifying_score_asks_for_initials():
    app, scene = flying()
    scene.score = 50
    scene._die()
    settle(app)
    assert isinstance(app.host.scene, scenes.InitialsScene)
    assert scene.dead


def test_a_score_of_nothing_is_not_worth_asking_about():
    """Being asked for your name to record a zero is worse than not being
    asked."""
    app, scene = flying()
    scene.score = 0
    scene._die()
    settle(app)
    assert not isinstance(app.host.scene, scenes.InitialsScene)
    assert app.scores.load() == []


def test_a_score_that_misses_the_table_is_still_recorded():
    """It just does not earn being asked about."""
    app = hosted()
    for i in range(scores_limit := app.scores.limit):
        app.scores.save("aaa", 1000 + i)
    app.start_run()
    settle(app)
    scene = app.host.scene
    scene.score = 5
    scene._die()
    settle(app)
    assert not isinstance(app.host.scene, scenes.InitialsScene)
    assert scores_limit == app.scores.limit


def test_typing_initials_puts_them_on_the_board():
    app, scene = flying()
    scene.score = 77
    scene._die()
    settle(app)
    entry = app.host.scene
    for key in ("j", "a", "m"):
        entry.handle_key(key)
    entry.handle_key("enter")
    settle(app)
    assert [(e.initials, e.score) for e in app.scores.load()] == [("JAM", 77)]


def test_backspace_fixes_a_typo():
    app, scene = flying()
    scene.score = 10
    scene._die()
    settle(app)
    entry = app.host.scene
    for key in ("j", "x"):
        entry.handle_key(key)
    entry.handle_key("backspace")
    entry.handle_key("m")
    entry.handle_key("enter")
    settle(app)
    assert app.scores.load()[0].initials == "JMA"


def test_only_three_characters_are_taken():
    app, scene = flying()
    scene.score = 10
    scene._die()
    settle(app)
    entry = app.host.scene
    for key in "jamie":
        entry.handle_key(key)
    assert entry.typed == "JAM"


def test_escaping_the_prompt_still_records_the_score():
    """A score is a fact; the initials are a label on it. Throwing the run
    away because somebody did not want to type would be the wrong trade."""
    app, scene = flying()
    scene.score = 33
    scene._die()
    settle(app)
    app.host.scene.handle_key("escape")
    settle(app)
    assert app.scores.best() == 33


def test_the_prompt_draws_over_the_wreck_rather_than_replacing_it():
    app, scene = flying()
    assert scenes.InitialsScene.render_below is True


def test_the_death_banner_says_where_the_run_landed():
    app, scene = flying()
    app.record(90, "jam")
    assert app.last_rank == 1


def test_a_new_run_forgets_the_last_runs_rank():
    app, scene = flying()
    app.record(90, "jam")
    scene.restart()
    assert app.last_rank is None


def test_the_table_shows_what_was_recorded():
    app = hosted()
    app.scores.save("jam", 128, pilot="fire-toad")

    async def go():
        async with await _piloted(app) as pilot:
            await pilot.pause()
            app.show_scores()
            settle(app)
            await asyncio.sleep(0.3)
            text = buffer_text(app)
            assert "JAM" in text
            assert "128" in text
            assert characters.get("fire-toad").glyph in text
            app.host.quit()

    run(go())


def test_an_empty_table_says_so():
    app = hosted()

    async def go():
        async with await _piloted(app) as pilot:
            await pilot.pause()
            app.show_scores()
            settle(app)
            await asyncio.sleep(0.3)
            assert "no scores yet" in buffer_text(app)
            app.host.quit()

    run(go())


def test_the_title_shows_the_best_on_record_not_just_this_session():
    app = hosted()
    app.scores.save("jam", 512)
    assert app.best == 512

# ── Portraits ───────────────────────────────────────────────────────


def test_every_pilot_has_a_portrait_and_a_colour():
    """Generated from the Wii port's sprites, so there are 24 of each without
    anybody drawing one."""
    from drift import portraits

    for pilot in characters.ROSTER:
        assert pilot.key in portraits.PORTRAITS, pilot.key
        assert pilot.portrait, pilot.key
        assert pilot.accent.startswith("#") and len(pilot.accent) == 7, pilot.key


def test_portraits_fit_the_box_they_are_laid_out_against():
    """Fitted rather than scaled to width. Scaling by width alone produced
    portraits between five and fifteen rows, and fifteen does not fit beside a
    roster on an 80x24 terminal."""
    from drift import portraits

    for key, art in portraits.PORTRAITS.items():
        assert len(art) <= 8, f"{key} is {len(art)} rows"
        assert max(len(row) for row in art) <= portraits.WIDTH, key


def test_a_portrait_is_half_blocks_with_colours():
    """Two pixels per cell is what makes the grid square in a terminal whose
    cells are twice as tall as they are wide."""
    from drift import portraits

    art = portraits.PORTRAITS["cat-synth"]
    inked = [c for row in art for c in row if c is not None]
    assert inked, "nothing drawn"
    for glyph, fg, bg in inked:
        assert glyph in ("▀", "▄")
        assert fg.startswith("#")
        assert bg is None or bg.startswith("#")


def test_the_accent_comes_from_the_sprite_not_from_a_guess():
    """The hand-picked ones were wrong often enough to be worth deriving:
    Fire Toad was coloured from its flames rather than the toad."""
    from drift import portraits

    assert characters.get("fire-toad").accent == portraits.ACCENTS["fire-toad"]
    # Green, because the toad is green. Not the orange of its flames.
    r, g, b = (int(characters.get("fire-toad").accent[i:i + 2], 16)
               for i in (1, 3, 5))
    assert g > r and g > b, "Fire Toad should be green"


def test_the_generated_data_needs_no_image_library():
    """Pillow makes the portraits; it is not there when anybody plays."""
    import subprocess
    import sys

    code = ("import sys, drift.portraits, drift.characters; "
            "print([m for m in sys.modules if m.startswith('PIL')])")
    out = subprocess.run([sys.executable, "-c", code], capture_output=True,
                         text=True, check=True)
    assert out.stdout.strip() == "[]", out.stdout


def test_the_roster_draws_the_portrait_in_colour():
    app = hosted()

    async def go():
        async with await _piloted(app, size=(80, 24)) as pilot:
            await pilot.pause()
            app.choose_pilot()
            settle(app)
            await asyncio.sleep(0.3)
            buf = app.host.game.surface.buffer
            found = {buf.get(x, y).fg
                     for y in range(3, 11) for x in range(50, 79)
                     if buf.get(x, y).fg}
            assert len(found) > 10, f"portrait not drawn in colour: {found}"
            app.host.quit()

    run(go())


def test_a_narrow_window_drops_the_portrait_rather_than_the_names():
    """The list is the point of the screen; the picture is the bonus."""
    app = hosted()

    async def go():
        async with await _piloted(app, size=(theme.MENU_MIN_COLS, 20)) as pilot:
            await pilot.pause()
            app.choose_pilot()
            settle(app)
            await asyncio.sleep(0.3)
            text = buffer_text(app)
            assert "Synth Cat" in text, "the roster must survive"
            app.host.quit()

    run(go())

# ── Being able to see your ship ─────────────────────────────────────


def test_every_ship_out_contrasts_every_star():
    """The whole fix, as one assertion.

    Before this the sky was louder than the ship: a white star sat at 18.3:1
    against the playfield and Roderick Tron at 2.8:1 — sixty bright marks and
    one dim one, the dim one being the thing you steer. Pinning the *hierarchy*
    rather than particular hex values means the palettes can move without this
    quietly stopping being true.
    """
    from drift import stars

    brightest_star = max(theme.contrast(theme.dim(c), theme.FIELD_BG)
                         for c in stars.PALETTE)
    dimmest_ship = min(theme.contrast(p.flight_colour, theme.FIELD_BG)
                       for p in characters.ROSTER)
    assert dimmest_ship > brightest_star, (
        f"the sky wins: ship {dimmest_ship:.1f}:1 vs star {brightest_star:.1f}:1")


def test_every_pilot_clears_the_contrast_floor_in_flight():
    for pilot in characters.ROSTER:
        ratio = theme.contrast(pilot.flight_colour, theme.FIELD_BG)
        assert ratio >= theme.SHIP_CONTRAST - 0.05, f"{pilot.name}: {ratio:.1f}:1"


def test_lifting_is_measured_not_guessed_at_lightness():
    """Lightness is not luminance. Blue contributes 0.0722 where green gives
    0.7152, so a blue at a respectable HSL lightness is still dark — which is
    exactly how Roderick Tron ended up at 2.8:1."""
    tron = characters.get("roderick-tron")
    assert theme.contrast(tron.accent, theme.FIELD_BG) < 3.0, "the premise moved"
    assert theme.contrast(tron.flight_colour, theme.FIELD_BG) >= 7.0


def test_the_roster_still_shows_the_sprites_own_colour():
    """Only flight gets the lifted value. The roster sits beside the pilot's
    own portrait, where the colour should match the sprite it came from."""
    from drift import portraits

    for pilot in characters.ROSTER:
        assert pilot.accent == portraits.ACCENTS[pilot.key]


def test_a_pilot_bright_enough_already_is_left_alone():
    """Lifting is a floor, not a wash. Most pilots should come through
    untouched or nearly so."""
    unchanged = [p for p in characters.ROSTER if p.flight_colour == p.accent]
    assert len(unchanged) >= len(characters.ROSTER) // 2, (
        f"only {len(unchanged)} of {len(characters.ROSTER)} kept their colour")


def test_the_ship_is_drawn_as_a_solid_object():
    """Filled, with the glyph punched out. Without the backing it is three
    characters that happen to be adjacent, in the same weight as the stars."""
    app = hosted()
    app.set_character(characters.get("roderick-tron"))

    async def go():
        async with await _piloted(app, size=(80, 24)) as pilot:
            await pilot.pause()
            app.start_run()
            settle(app)
            await asyncio.sleep(0.35)
            scene = app.host.scene
            buf = app.host.game.surface.buffer
            col = scene.projection.col(scene.player.x)
            row = scene.projection.row(scene.player.y)
            cell = buf.get(col, row)
            assert cell.bg == app.character.flight_colour, "no backing"
            assert cell.fg == theme.FIELD_BG, "glyph not punched out"
            app.host.quit()

    run(go())


def test_the_stars_are_dimmed_on_the_way_to_the_screen():
    """The palette itself is the browser's and stays as it is; only what
    reaches the terminal is dimmed."""
    from drift import stars

    app = hosted()

    async def go():
        async with await _piloted(app, size=(80, 24)) as pilot:
            await pilot.pause()
            app.start_run()
            settle(app)
            await asyncio.sleep(0.35)
            buf = app.host.game.surface.buffer
            drawn = {buf.get(x, y).fg
                     for y in range(1, 22) for x in range(buf.width)
                     if buf.get(x, y).char in stars.GLYPHS and buf.get(x, y).fg}
            assert drawn, "no stars drawn"
            assert "#ffffff" not in drawn, "a star is still at full brightness"
            app.host.quit()

    run(go())


# ── Seated under a launcher ─────────────────────────────────────────


class _Floor:
    """Stands in for whatever a launcher has underneath a cabinet."""

    def update(self, dt):
        pass

    def render(self):
        pass


def seated():
    """The cabinet seated over a floor, the way the arcade does it."""
    host = TuiHost(title=GAME.info.title, fps=GAME.info.fps,
                   hold_ms=GAME.info.hold_ms)
    host.push_scene(_Floor())
    host.stack.update(0.0)
    scene = host.seat(GAME)
    host.stack.update(0.0)
    return host, scene


def screen(host) -> str:
    return host.game.surface.buffer.to_text()


def test_a_seated_cabinet_says_how_to_get_back():
    host, scene = seated()
    scene.render()
    assert scenes.ARCADE_HELP in screen(host)


def test_a_game_launched_on_its_own_does_not_promise_an_arcade():
    # The same screen, the same key, a different truth: Esc ends the session
    # here, and Q already says so.
    app = hosted()
    app.host.scene.render()
    assert scenes.ARCADE_HELP not in buffer_text(app)


def test_esc_from_a_seated_cabinet_returns_instead_of_quitting():
    host, scene = seated()
    scene.handle_key("escape")
    host.stack.update(0.0)
    assert isinstance(host.scene, _Floor), "should be back on the floor"
