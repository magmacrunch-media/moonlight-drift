"""The simulation, checked against the two versions that came before it.

These import nothing but the standard library and ``drift`` itself — no engine,
no terminal. That is the claim the modules under test make in their docstrings,
and running this suite on a machine with nothing but pytest installed is what
proves it.

Every number here traces to ``wii/source/`` and through it to ``web/js/``. When
one disagrees, the browser is the source of truth: it is the version people
actually play.
"""

from __future__ import annotations

import random

from drift import config, obstacles, player, projection

# ── The world ───────────────────────────────────────────────────────


def test_the_world_is_the_browsers_canvas():
    """Every constant is a measurement in 1280x720. If this drifts, the gap
    and the player stop being the share of the screen they are on the web."""
    assert (config.WORLD_WIDTH, config.WORLD_HEIGHT) == (1280, 720)
    assert config.GAP == 180
    assert config.OBSTACLE_WIDTH == 60
    assert config.OBSTACLE_SPEED == 3
    assert config.OBSTACLE_SPAWN_INTERVAL == 120


def test_the_tallest_column_still_leaves_a_gap_and_a_floor():
    assert config.MAX_OBSTACLE_HEIGHT == 490
    assert (config.MAX_OBSTACLE_HEIGHT + config.GAP
            + config.MIN_OBSTACLE_HEIGHT) == config.WORLD_HEIGHT


# ── Physics ─────────────────────────────────────────────────────────


def test_gravity_pulls_down_and_thrust_pulls_up():
    p = player.new_player()
    start = p.y
    p.update(thrust_active=False)
    assert p.y > start, "falling is +y"

    p = player.new_player()
    p.update(thrust_active=True)
    assert p.y < start, "thrust is -y"


def test_velocity_is_clamped_both_ways():
    p = player.new_player()
    for _ in range(200):
        p.update(thrust_active=False)
        if p.y > config.WORLD_HEIGHT:
            break
    assert p.velocity <= p.max_velocity

    p = player.new_player()
    p.velocity = -999.0
    p.update(thrust_active=True)
    assert p.velocity >= -p.max_velocity


def test_one_frame_of_gravity_is_exactly_the_browsers_number():
    """0.4 down, then position follows velocity — not the other way round."""
    p = player.new_player()
    y0 = p.y
    p.update(thrust_active=False)
    assert p.velocity == config.DEFAULT_GRAVITY
    assert p.y == y0 + config.DEFAULT_GRAVITY


def test_the_ceiling_and_the_floor_are_both_lethal():
    p = player.new_player()
    p.y = 1.0
    p.velocity = -50.0
    assert p.update(thrust_active=True) is True

    p = player.new_player()
    p.y = config.WORLD_HEIGHT - 1.0
    assert p.update(thrust_active=False) is True


def test_death_is_measured_against_the_hitbox_not_the_sprite():
    """A character whose art sits off its origin must not die early. Matches
    updatePlayer() in js/player.js."""
    p = player.new_player()
    p.set_hitbox(w=40, h=35, ox=0, oy=20)
    p.y = -15.0
    p.velocity = 0.0
    # The sprite box is above the ceiling; the hitbox is not.
    assert p.update(thrust_active=False) is False


# ── Projection ──────────────────────────────────────────────────────


def test_a_cell_is_twice_as_tall_as_it_is_wide():
    """The one value the whole projection turns on. The Wii derives it from
    the video mode; a terminal has a single answer."""
    p = projection.compute(80, 24)
    assert p.par == 0.5
    assert p.scale_x == p.scale_y * projection.CELL_ASPECT


def test_the_world_fills_the_rows_it_is_given():
    """Fitting the height is what keeps the gap and the player the share of the
    screen they have in the browser."""
    p = projection.compute(80, 24)
    assert p.h(config.WORLD_HEIGHT) == 24
    assert p.row(0) == 0
    assert p.row(config.WORLD_HEIGHT) == 24


def test_a_wider_terminal_sees_more_of_the_world_not_a_smaller_one():
    """world_w feeds the spawn edge, the cull edge and the starfield spread,
    so this is a gameplay figure and not a cosmetic one."""
    narrow = projection.compute(60, 24)
    wide = projection.compute(100, 24)
    assert wide.world_w > narrow.world_w
    assert narrow.scale_y == wide.scale_y, "height fit is unchanged"


def test_the_world_is_never_wider_than_it_is():
    """Never show more game than exists."""
    p = projection.compute(400, 24)
    assert p.world_w == config.WORLD_WIDTH


def test_a_degenerate_grid_gets_the_identity_rather_than_a_divide_by_zero():
    for cols, rows in ((0, 24), (80, 0), (-1, -1)):
        p = projection.compute(cols, rows)
        assert p.valid is False
        assert p.scale_x == 1.0 and p.scale_y == 1.0


def test_the_origin_offsets_the_whole_field():
    """The playfield is not the whole window — the score bar is above it."""
    p = projection.compute(80, 20, origin_x=1, origin_y=3)
    assert p.col(0) == 1
    assert p.row(0) == 3


def test_the_gap_stays_a_quarter_of_the_screen():
    """180 of 720 is exactly a quarter, at any terminal size."""
    for cols, rows in ((80, 24), (120, 40), (60, 20)):
        p = projection.compute(cols, rows)
        assert abs(p.h(config.GAP) / rows - 0.25) < 1e-9


# ── Obstacles ───────────────────────────────────────────────────────


def field(seed: int = 1) -> obstacles.ObstacleField:
    f = obstacles.ObstacleField(rng=random.Random(seed))
    f.reset()
    return f


def test_a_column_spawns_at_the_right_edge_with_a_gap_in_it():
    f = field()
    f.create(config.WORLD_WIDTH)
    o = f.obstacles[0]
    assert o.x == config.WORLD_WIDTH
    assert o.bottom_y - o.top_height == config.GAP
    assert config.MIN_OBSTACLE_HEIGHT <= o.top_height < config.MAX_OBSTACLE_HEIGHT


def test_columns_scroll_left_at_the_browsers_speed():
    f = field()
    f.create(config.WORLD_WIDTH)
    x0 = f.obstacles[0].x
    f.update()
    assert f.obstacles[0].x == x0 - config.OBSTACLE_SPEED


def test_a_column_that_has_left_is_dropped():
    f = field()
    f.create(config.WORLD_WIDTH)
    f.obstacles[0].x = -config.OBSTACLE_WIDTH
    f.update()
    assert f.obstacles == []


def test_the_field_never_grows_past_its_cap():
    f = field()
    for _ in range(obstacles.MAX_OBSTACLES + 10):
        f.create(config.WORLD_WIDTH)
    assert len(f.obstacles) == obstacles.MAX_OBSTACLES


def test_clearing_a_column_scores_once_and_only_once():
    f = field()
    f.create(config.WORLD_WIDTH)
    f.obstacles[0].x = float(config.PLAYER_X - config.OBSTACLE_WIDTH - 1)
    assert f.score(config.PLAYER_X) == 1
    assert f.score(config.PLAYER_X) == 0, "a column scores once"


def test_the_palette_moves_in_runs_not_per_column():
    """A fresh theme per obstacle makes every column a different random colour,
    which is the main reason the Wii port did not read like the arcade version
    until this was fixed there."""
    f = field()
    for _ in range(config.THEME_CHANGE_INTERVAL - 1):
        f.create(config.WORLD_WIDTH)
    themes = [o.theme for o in f.obstacles]
    assert len(set(themes)) == 1, f"palette changed mid-run: {themes}"


def test_the_milestone_column_is_the_one_that_brings_the_new_palette():
    """The count is incremented before the palette is asked for, so the change
    lands *on* the tenth column rather than after it — and the tenth is the
    milestone. The new run announces itself."""
    f = field()
    for _ in range(config.THEME_CHANGE_INTERVAL):
        f.create(config.WORLD_WIDTH)

    opening = f.obstacles[:config.THEME_CHANGE_INTERVAL - 1]
    tenth = f.obstacles[config.THEME_CHANGE_INTERVAL - 1]
    assert len({o.theme for o in opening}) == 1
    assert tenth.milestone == config.THEME_CHANGE_INTERVAL
    # Seeded, so this is a fact about this run rather than a coin flip.
    assert tenth.theme != opening[0].theme


def test_every_tenth_column_is_a_milestone():
    """The browser marks the 10th, 20th ... and uses the count as the label."""
    f = field()
    for _ in range(obstacles.MAX_OBSTACLES):
        f.create(config.WORLD_WIDTH)
    marked = [o.milestone for o in f.obstacles if o.milestone]
    assert marked == [10, 20]


# ── Collision ───────────────────────────────────────────────────────


def one_column(top_height: int = 300, style: int = 1) -> obstacles.ObstacleField:
    f = field()
    f.obstacles.append(obstacles.Obstacle(
        x=100.0, top_height=top_height, bottom_y=top_height + config.GAP,
        style_index=style, seed=0.0, roughness=0.5, asymmetry=0.0))
    return f


def test_flying_through_the_gap_is_safe():
    f = one_column()
    middle = 300 + config.GAP // 2 - config.PLAYER_HEIGHT // 2
    assert f.collides((110, middle, config.PLAYER_WIDTH, config.PLAYER_HEIGHT)) is False


def test_flying_into_a_column_is_not():
    f = one_column()
    assert f.collides((110, 10, config.PLAYER_WIDTH, config.PLAYER_HEIGHT)) is True


def test_a_column_you_are_not_level_with_cannot_touch_you():
    f = one_column()
    assert f.collides((10, 10, config.PLAYER_WIDTH, config.PLAYER_HEIGHT)) is False
    assert f.collides((900, 10, config.PLAYER_WIDTH, config.PLAYER_HEIGHT)) is False


def test_the_exhaust_plume_is_not_solid():
    """Collision measures the player's bottom edge above the flame — a plume
    that killed you would make every character with one unplayable."""
    f = one_column(top_height=300)
    # Bottom edge lands just inside the lower column, flame included.
    y = 300 + config.GAP - config.PLAYER_HEIGHT + config.FLAME_HEIGHT
    assert f.collides((110, y, config.PLAYER_WIDTH, config.PLAYER_HEIGHT)) is False


def test_the_silhouette_tapers_rather_than_being_a_box():
    """The column narrows along its length, and the collision test follows it.
    A rectangular test would be a harder game at the tip and an easier one at
    the mouth."""
    o = obstacles.Obstacle(x=100.0, top_height=400, bottom_y=580,
                           style_index=1, seed=0.0, roughness=0.5, asymmetry=0.0)
    at_tip = o.silhouette(0)
    at_mouth = o.silhouette(399)
    assert (at_tip[1] - at_tip[0]) > (at_mouth[1] - at_mouth[0])


def test_the_gap_has_no_silhouette_at_all():
    o = obstacles.Obstacle(x=100.0, top_height=300, bottom_y=480,
                           style_index=0, seed=0.5, roughness=0.5, asymmetry=0.0)
    assert o.silhouette(400) == (0.0, 0.0)


def test_the_column_never_narrows_past_its_style_floor():
    """Each style bottoms out at a minimum width; without it the tip would
    taper to nothing and become impossible to see but still lethal."""
    floors = {0: 22.0, 1: 24.0, 2: 20.0}
    for style, floor in floors.items():
        o = obstacles.Obstacle(x=100.0, top_height=700, bottom_y=880,
                               style_index=style, seed=0.0, roughness=0.5,
                               asymmetry=0.0)
        left, right = o.silhouette(699)
        assert right - left >= floor - 1e-6, style


def test_two_runs_from_the_same_seed_are_identical():
    """Seeded, so a run can be replayed — which is what makes a collision bug
    reproducible rather than a story about one time it happened."""
    a, b = field(seed=7), field(seed=7)
    for f in (a, b):
        for _ in range(5):
            f.create(config.WORLD_WIDTH)
            f.update()
    assert [(o.x, o.top_height, o.style_index) for o in a.obstacles] == \
           [(o.x, o.top_height, o.style_index) for o in b.obstacles]
