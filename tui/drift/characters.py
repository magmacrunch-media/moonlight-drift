"""The roster: who you fly as, and how differently they fly.

The physics — ``thrust``, ``gravity`` and ``max_velocity`` — are lifted
verbatim from ``wii/source/characters.c``, which took them from the
``js/characters/`` modules. They are the part of a character that a terminal
can carry perfectly, and the part that actually matters: a heavier character
falls faster and climbs slower, and you feel it in the first second.

**The art cannot come across, and pretending otherwise would be worse.** On the
web each of these is drawn with canvas primitives — gradients, hue cycling, a
bobbing offset — and on the Wii they are PNGs baked into the binary. At the
scale a terminal gives, the player is about two cells wide and one tall (see
:mod:`drift.projection`), and ``draw_image`` is a documented no-op in the
engine's terminal backend anyway. So a character here is three things: a glyph
small enough to fit, an accent colour taken from its web palette, and its
physics.

The hitboxes come across exactly, because collision is in world space and has
nothing to do with what is drawn.
"""

from __future__ import annotations

from dataclasses import dataclass


@dataclass(frozen=True)
class Character:
    """One pilot. Frozen — a run mutates a :class:`~drift.player.Player`."""

    key: str
    name: str
    #: What the player is drawn as. Three cells at most; a wider one would be
    #: bigger than the hitbox it stands for, which is a lie the player pays for.
    glyph: str
    accent: str
    #: ``(w, h, offset_x, offset_y)`` in world units, from the browser build.
    hitbox: tuple[int, int, int, int]
    thrust: float
    gravity: float
    max_velocity: float

    def apply(self, player) -> None:
        """Put this character's physics and hitbox on a player."""
        player.set_physics(self.thrust, self.gravity, self.max_velocity)
        player.set_hitbox(*self.hitbox)


#: Roster order matches the browser build's load order, which is the order the
#: Wii port carries too. Three versions, one order — a character's number is
#: the same wherever you play.
ROSTER: tuple[Character, ...] = (
    Character("cat-synth", "Synth Cat", '=^=', "#ff4fd8",
              hitbox=(42, 30, -1, 18),
              thrust=-0.6, gravity=0.36, max_velocity=10.5),
    Character("beava", "Beava", '<@>', "#c98a3a",
              hitbox=(40, 35, 0, 0),
              thrust=-0.6, gravity=0.4, max_velocity=10.0),
    Character("vinny-bobarino", "Vinny Bobarino", '<D>', "#ffb347",
              hitbox=(35, 35, 15, 0),
              thrust=-0.6, gravity=0.4, max_velocity=10.0),
    Character("darius-hodgekins", "Darius Hodgekins", '<H>', "#8ad3ff",
              hitbox=(40, 35, 0, 0),
              thrust=-0.6, gravity=0.4, max_velocity=10.0),
    Character("roderick-tron", "Roderick Tron", '[T]', "#39ff6e",
              hitbox=(40, 35, 0, 0),
              thrust=-0.6, gravity=0.4, max_velocity=10.0),
    Character("juanito-thompson", "Juanito Thompson", '<J>', "#ff7043",
              hitbox=(40, 35, 0, 0),
              thrust=-0.6, gravity=0.4, max_velocity=10.0),
    Character("carl-spatski", "Carl Spatski", '<%>', "#b388ff",
              hitbox=(42, 42, -1, -2),
              thrust=-0.65, gravity=0.4, max_velocity=10.5),
    Character("dspum-balloon", "dspum balloon", '(o)', "#ff5fa2",
              hitbox=(30, 38, 5, -3),
              thrust=-0.5, gravity=0.3, max_velocity=9.0),
    Character("foresters-soul", "Forester's Soul", '}|{', "#5fe08a",
              hitbox=(40, 40, 0, -2),
              thrust=-0.62, gravity=0.38, max_velocity=11.0),
    Character("grocery-harrison", "Grocery Harrison", '[G]', "#ffd54f",
              hitbox=(35, 30, 8, 5),
              thrust=-0.6, gravity=0.4, max_velocity=10.0),
    Character("didgeridoo-man", "Didgeridoo Man", '==D', "#d2691e",
              hitbox=(45, 35, 0, 0),
              thrust=-0.7, gravity=0.35, max_velocity=9.5),
    Character("mountain-gnome", "Mountain Gnome", '/A\\', "#9ccc65",
              hitbox=(32, 32, 4, 0),
              thrust=-0.55, gravity=0.32, max_velocity=9.5),
    Character("prince-vince", "Prince Vince", '<P>', "#7e57c2",
              hitbox=(27, 38, 7, -3),
              thrust=-0.65, gravity=0.38, max_velocity=10.5),
    Character("dag-henderson", "Dag Henderson", '<b>', "#4fc3f7",
              hitbox=(40, 35, 0, 0),
              thrust=-0.6, gravity=0.4, max_velocity=10.0),
    Character("ban-daniel", "BANDANIEL", '<B>', "#ff1744",
              hitbox=(38, 38, 1, -2),
              thrust=-0.6, gravity=0.4, max_velocity=10.0),
    Character("plantain-jane", "PLANTAIN JANE", '<J>', "#cddc39",
              hitbox=(38, 38, 1, -2),
              thrust=-0.6, gravity=0.4, max_velocity=10.0),
    Character("fire-toad", "Fire Toad", '<*>', "#ff6d00",
              hitbox=(32, 32, 4, 0),
              thrust=-0.65, gravity=0.38, max_velocity=10.5),
    Character("backpack-man", "Backpack Man", '[M]', "#8d6e63",
              hitbox=(34, 36, 6, -2),
              thrust=-0.6, gravity=0.4, max_velocity=10.0),
    Character("tollbooth-lady", "Tollbooth Lady", '<T>', "#f06292",
              hitbox=(40, 35, 0, 0),
              thrust=-0.6, gravity=0.4, max_velocity=10.0),
    Character("svfp-van", "SVFP Van", '[V]', "#26c6da",
              hitbox=(36, 18, 4, 12),
              thrust=-0.6, gravity=0.4, max_velocity=10.0),
    Character("tardigrade", "Tardigrade", '(~)', "#a1887f",
              hitbox=(26, 25, 7, 8),
              thrust=-0.7, gravity=0.35, max_velocity=11.0),
    Character("elektra", "Elektra", '<E>', "#00e5ff",
              hitbox=(38, 38, 1, 0),
              thrust=-0.58, gravity=0.38, max_velocity=10.5),
    Character("strawberto", "Strawberto", '<o>', "#ff3d71",
              hitbox=(36, 38, 2, 0),
              thrust=-0.6, gravity=0.39, max_velocity=10.0),
    Character("carl", "Carl", '<@>', "#ffee58",
              hitbox=(38, 38, 1, 0),
              thrust=-0.62, gravity=0.37, max_velocity=11.0),
)

BY_KEY = {c.key: c for c in ROSTER}

#: Who you fly as before choosing. The browser opens on the same one.
DEFAULT = ROSTER[0]


def get(key: str) -> Character:
    """The named character, or :data:`DEFAULT` if there is no such name."""
    return BY_KEY.get(key, DEFAULT)


__all__ = ["BY_KEY", "DEFAULT", "ROSTER", "Character", "get"]
