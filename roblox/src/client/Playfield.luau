--!strict
-- Draws a Run into a 16:9 GUI frame. The simulation's 1280x720 world maps
-- onto the frame by scale alone, so every screen size gets the same game,
-- letterboxed, exactly as the Wii port projects it onto a TV.
--
-- Columns are built once per obstacle, as horizontal slices whose edges come
-- from Obstacles.profileTop/profileBottom, the functions collision uses. Each
-- frame after that only moves the column's container, so a screen full of
-- obstacles costs a dozen Position writes rather than a thousand.

local ReplicatedStorage = game:GetService("ReplicatedStorage")

local Shared = ReplicatedStorage:WaitForChild("MoonlightDrift")
local Config = require(Shared:WaitForChild("Config"))
local Characters = require(Shared:WaitForChild("Characters"))
local Obstacles = require(Shared:WaitForChild("Obstacles"))
local Theme = require(Shared:WaitForChild("Theme"))
local Run = require(Shared:WaitForChild("Run"))
local Assets = require(script.Parent:WaitForChild("Assets"))

local WW, WH = Config.WORLD_WIDTH, Config.WORLD_HEIGHT
local OW = Config.OBSTACLE_WIDTH
local SKY = Color3.fromRGB(8, 10, 32)

-- web/js/stars.js's palette, white weighted three to one.
local STAR_COLORS = {
	Color3.fromHex("#ffffff"),
	Color3.fromHex("#ffffff"),
	Color3.fromHex("#ffffff"),
	Color3.fromHex("#e0e0ff"),
	Color3.fromHex("#ffe0e0"),
	Color3.fromHex("#00d4ff"),
	Color3.fromHex("#ffd700"),
}

local function rgb(c: Theme.RGB): Color3
	return Color3.new(c.r, c.g, c.b)
end

local function box(frame: GuiObject, x: number, y: number, w: number, h: number)
	frame.Position = UDim2.fromScale(x / WW, y / WH)
	frame.Size = UDim2.fromScale(w / WW, h / WH)
end

local function plain(name: string, color: Color3, parent: Instance): Frame
	local f = Instance.new("Frame")
	f.Name = name
	f.BackgroundColor3 = color
	f.BorderSizePixel = 0
	f.Parent = parent
	return f
end

type Star = { frame: Frame, counter: number, speed: number, phase: number, pulsing: boolean, pulse: number }

export type Playfield = {
	root: Frame,
	setPilot: (self: Playfield, character: Characters.Character) -> (),
	clear: (self: Playfield) -> (),
	draw: (self: Playfield, run: Run.Run?, thrustHeld: boolean) -> (),
	tick: (self: Playfield) -> (),
	flash: (self: Playfield) -> (),
}

local Playfield = {}

function Playfield.new(gui: ScreenGui): Playfield
	local root = plain("Playfield", SKY, gui)
	root.AnchorPoint = Vector2.new(0.5, 0.5)
	root.Position = UDim2.fromScale(0.5, 0.5)
	root.Size = UDim2.fromScale(1, 1)
	root.ClipsDescendants = true
	local aspect = Instance.new("UIAspectRatioConstraint")
	aspect.AspectRatio = WW / WH
	aspect.Parent = root

	local starLayer = Instance.new("Folder")
	starLayer.Name = "Stars"
	starLayer.Parent = root
	local columnLayer = Instance.new("Folder")
	columnLayer.Name = "Columns"
	columnLayer.Parent = root

	-- Stars: cosmetic, so Random rather than the run's rng, which must stay
	-- reserved for the simulation or seeded replays would stop matching.
	local rand = Random.new()
	local stars: { Star } = {}
	for i = 1, Config.STAR_COUNT do
		local size = rand:NextInteger(2, 5)
		local f = plain("Star" .. i, STAR_COLORS[rand:NextInteger(1, #STAR_COLORS)], starLayer)
		box(f, rand:NextNumber() * WW, rand:NextNumber() * WH, size, size)
		table.insert(stars, {
			frame = f,
			counter = 0,
			speed = rand:NextInteger(30, 70),
			phase = rand:NextInteger(0, 3),
			pulsing = rand:NextNumber() < 0.3,
			pulse = 0,
		})
	end

	-- Pilot, drawn from the sprite sheet the way the Wii port draws it: the
	-- 128x128 sprite at world scale, with the character's origin on the
	-- pilot's position. Idle and thrust are separate sprites, and the thrust
	-- one carries its own flame.
	local sprite = Instance.new("ImageLabel")
	sprite.Name = "PilotSprite"
	sprite.BackgroundTransparency = 1
	sprite.Image = Assets.pilotSheet
	sprite.ImageRectSize = Vector2.new(Characters.SPRITE_SIZE, Characters.SPRITE_SIZE)
	sprite.ZIndex = 3
	sprite.Visible = false
	sprite.Parent = root
	local hasSheet = Assets.pilotSheet ~= ""
	local idleOffset, thrustOffset = Vector2.zero, Vector2.zero

	-- Without the sheet: a coloured body the size of the hitbox, so the box
	-- you steer is the box that collides, with the flame hanging beneath it.
	local pilot = plain("Pilot", Color3.new(1, 1, 1), root)
	pilot.ZIndex = 3
	local corner = Instance.new("UICorner")
	corner.CornerRadius = UDim.new(0.3, 0)
	corner.Parent = pilot
	local stroke = Instance.new("UIStroke")
	stroke.Color = Color3.new(1, 1, 1)
	stroke.Thickness = 1.5
	stroke.Parent = pilot
	local glyph = Instance.new("TextLabel")
	glyph.BackgroundTransparency = 1
	glyph.Size = UDim2.fromScale(1, 1)
	glyph.Font = Enum.Font.Arcade
	glyph.TextScaled = true
	glyph.TextColor3 = SKY
	glyph.ZIndex = 4
	glyph.Parent = pilot
	local flame = plain("Flame", Color3.fromHex("#ffb347"), pilot)
	flame.AnchorPoint = Vector2.new(0.5, 0)
	flame.Position = UDim2.fromScale(0.5, 1)
	flame.ZIndex = 2
	local flameCorner = Instance.new("UICorner")
	flameCorner.CornerRadius = UDim.new(0.5, 0)
	flameCorner.Parent = flame

	-- The cyan lines drawBackground() puts across the top and bottom of the
	-- canvas: the edges of the world, which are lethal.
	for _, edge in { 0, WH - 2 } do
		local line = plain("Boundary", Color3.fromHex("#00d4ff"), root)
		box(line, 0, edge, WW, 2)
		line.BackgroundTransparency = 0.7
		line.ZIndex = 1
	end

	local flashFrame = plain("Flash", Color3.new(1, 1, 1), root)
	flashFrame.Size = UDim2.fromScale(1, 1)
	flashFrame.BackgroundTransparency = 1
	flashFrame.ZIndex = 8

	local columns: { [number]: Frame } = {}
	local flameFrame = 0
	local flashLeft = 0

	-- Every tenth column is marked, as drawMilestoneMarkers() in
	-- web/js/obstacles.js does it: a chunky dashed line down the whole height
	-- in the theme's complement, and the count in a bordered box in the gap.
	local function buildMilestone(c: Frame, o: Obstacles.Obstacle)
		local mark = rgb(Theme.complementary(o.theme))
		local DASH, GAP_, LINE = 8, 8, 4
		local y = 0
		while y < WH do
			local dash = plain("Dash", mark, c)
			dash.AnchorPoint = Vector2.new(0.5, 0)
			dash.Position = UDim2.fromScale(0.5, y / WH)
			dash.Size = UDim2.fromScale(LINE / OW, math.min(DASH, WH - y) / WH)
			dash.ZIndex = 1
			y += DASH + GAP_
		end

		local BOX_W, BOX_H = 80, 50
		local middle = (o.topHeight + o.bottomY) / 2
		local function plate(w: number, h: number, color: Color3, transparency: number, z: number): Frame
			local f = plain("Plate", color, c)
			f.AnchorPoint = Vector2.new(0.5, 0.5)
			f.Position = UDim2.fromScale(0.5, middle / WH)
			f.Size = UDim2.fromScale(w / OW, h / WH)
			f.BackgroundTransparency = transparency
			f.ZIndex = z
			return f
		end
		plate(BOX_W + 8, BOX_H + 8, Color3.fromRGB(26, 26, 46), 0, 4)
		plate(BOX_W + 4, BOX_H + 4, mark, 0, 5)
		plate(BOX_W, BOX_H, Color3.fromRGB(26, 26, 46), 0.1, 6)
		local inner = plate(BOX_W - 4, BOX_H - 4, rgb(o.theme.secondary), 0, 7)
		local face = plate(BOX_W - 8, BOX_H - 8, Color3.fromRGB(26, 26, 46), 0.1, 8)
		inner.Name = "InnerBorder"
		face.Name = "Face"

		local label = Instance.new("TextLabel")
		label.BackgroundTransparency = 1
		label.AnchorPoint = Vector2.new(0.5, 0.5)
		label.Position = UDim2.fromScale(0.5, middle / WH)
		label.Size = UDim2.fromScale((BOX_W - 12) / OW, (BOX_H - 14) / WH)
		label.Font = Enum.Font.Arcade
		label.Text = tostring(o.milestone)
		label.TextScaled = true
		label.TextColor3 = mark
		label.ZIndex = 9
		local stroke = Instance.new("UIStroke")
		stroke.Color = Color3.new(0, 0, 0)
		stroke.Thickness = 2
		stroke.Parent = label
		label.Parent = c
	end

	-- A column is built once, out of horizontal bands, and then only moved.
	-- The three styles are web/js/obstacles.js's drawCandyStripedObstacle,
	-- drawFacetedCrystalObstacle and drawRoughCrystalObstacle: the same band
	-- heights, colour cycles and highlights.
	--
	-- What is not copied is the web's *widths*. There it steps the drawn width
	-- every few bands while collision uses the smooth formula, so the picture
	-- and the hitbox disagree by a few units. Here every band is measured with
	-- profileTop/profileBottom, the functions collision reads, and the
	-- decoration is laid inside that silhouette. The web's rough style also
	-- juts blocks out past its own collision width; those are left out for the
	-- same reason. What you see is what hits you.
	local BAND = { [0] = 6, [1] = 8, [2] = 10 }

	local function buildColumn(o: Obstacles.Obstacle): Frame
		local c = Instance.new("Frame")
		c.Name = "Column" .. o.id
		c.BackgroundTransparency = 1
		c.Size = UDim2.fromScale(OW / WW, 1)
		c.ZIndex = 2

		local primary, secondary, accent = rgb(o.theme.primary), rgb(o.theme.secondary), rgb(o.theme.accent)
		local palette = { primary, secondary, accent }

		-- x and w are world units; the frame is positioned inside the column,
		-- which is OBSTACLE_WIDTH wide and spans the full height.
		local function piece(x: number, y: number, w: number, h: number, color: Color3, transparency: number?): Frame
			local f = plain("B", color, c)
			f.Position = UDim2.fromScale((x - o.x) / OW, y / WH)
			-- One unit of overlap hides the seams scaling would otherwise open.
			f.Size = UDim2.fromScale(w / OW, (h + 1) / WH)
			f.BackgroundTransparency = transparency or 0
			f.ZIndex = 2
			return f
		end

		-- One band of a column: the body, then the style's highlights, all
		-- clipped to the band's own width so nothing leaves the silhouette.
		local function band(index: number, y: number, h: number, left: number, right: number)
			local w = right - left
			-- Every piece of a band is clamped to the band, so a highlight can
			-- never reach past the outline collision reads, whatever the
			-- arithmetic below works out to.
			local function part(x: number, py: number, pw: number, ph: number, color: Color3, transparency: number?)
				local x0 = math.clamp(x, left, right)
				local x1 = math.clamp(x + pw, left, right)
				if x1 > x0 then
					piece(x0, py, x1 - x0, ph, color, transparency)
				end
			end
			if o.style == 0 then
				-- Candy stripes: three colours in turn, a glossy accent stripe
				-- and a brighter shine inside it.
				part(left, y, w, h, palette[index % 3 + 1])
				local gloss = math.max(4, w * 0.2)
				part(left + math.min(3, w - gloss), y, gloss, h, accent)
				local shine = math.max(2, w * 0.1)
				if h > 2 then
					part(left + math.min(5, w - shine), y + 1, shine, h - 2, Color3.new(1, 1, 1), 0.6)
				end
			elseif o.style == 1 then
				-- Faceted crystal: the facet's angle decides which side catches
				-- the light, so a column reads as turning as it tapers.
				part(left, y, w, h, palette[(index + math.floor(o.seed * 10)) % 4 % 3 + 1])
				local angle = math.sin(o.seed * 200 + index)
				if angle > 0.2 then
					part(left, y, w * 0.35, h, accent)
					part(left + w * 0.08, y + 1, math.max(2, w * 0.12), math.max(1, h - 2), Color3.new(1, 1, 1), 0.65)
				elseif angle < -0.2 then
					part(left + w * 0.65, y, w * 0.35, h, secondary)
				end
			else
				-- Rough crystal: blocks of unrelated colour, with an occasional
				-- accent edge where the web version grows a jut.
				part(left, y, w, h, palette[math.floor(o.seed * 1000 + index * 7) % 3 + 1])
				if math.sin(o.seed * 400 + index) > 0.6 then
					local jut = math.min(w * 0.3, 8)
					local onLeft = math.cos(o.seed * 150 + index) > 0
					part(if onLeft then left else right - jut, y + 1, jut, math.max(1, h - 2), accent)
				end
			end
		end

		-- Stalactite: widest at its root, so each band takes its top row.
		local step = BAND[o.style]
		local y, index = 0, 0
		while y < o.topHeight do
			local h = math.min(step, o.topHeight - y)
			band(index, y, h, Obstacles.profileTop(o, y))
			y += h
			index += 1
		end
		-- Stalagmite: widest at its foot, so each band takes its bottom row.
		y, index = o.bottomY, 0
		while y < WH do
			local h = math.min(step, WH - y)
			band(index, y, h, Obstacles.profileBottom(o, y + h))
			y += h
			index += 1
		end

		if o.milestone > 0 then
			buildMilestone(c, o)
		end

		c.Parent = columnLayer
		return c
	end

	local self = {} :: Playfield
	self.root = root

	function self:setPilot(character)
		pilot.BackgroundColor3 = Color3.fromHex(character.accent)
		glyph.Text = character.glyph
		local index = Characters.indexOf(character.key) or 1
		idleOffset = Vector2.new(Characters.spriteOffset(index, false))
		thrustOffset = Vector2.new(Characters.spriteOffset(index, true))
	end

	function self:clear()
		for id, c in columns do
			c:Destroy()
			columns[id] = nil
		end
	end

	function self:draw(run, thrustHeld)
		local p = run and run.pilot
		local burning = thrustHeld and run ~= nil and run.crash == nil
		pilot.Visible = p ~= nil and not hasSheet
		sprite.Visible = p ~= nil and hasSheet
		if p and hasSheet then
			local o = p.origin
			local s = Characters.SPRITE_SIZE
			box(sprite, p.x - o.x, p.y - o.y, s, s)
			sprite.ImageRectOffset = if burning then thrustOffset else idleOffset
		elseif p then
			local hb = p.hitbox
			box(pilot, p.x + hb.ox, p.y + hb.oy, hb.w, hb.h)
			pilot.Rotation = math.clamp(p.velocity * 2, -18, 18)
		end
		flameFrame += 1
		local flicker = if flameFrame % 6 < 3 then 1 else 0.8
		flame.Size = UDim2.fromScale(if burning then 0.45 else 0.25, (if burning then 0.8 else 0.25) * flicker)
		flame.BackgroundColor3 = if burning then Color3.fromHex("#ffb347") else Color3.fromHex("#ff5a36")

		if not run then
			return
		end
		local alive: { [number]: boolean } = {}
		for _, o in run.field.list do
			alive[o.id] = true
			local c = columns[o.id]
			if not c then
				c = buildColumn(o)
				columns[o.id] = c
			end
			c.Position = UDim2.fromScale(o.x / WW, 0)
		end
		for id, c in columns do
			if not alive[id] then
				c:Destroy()
				columns[id] = nil
			end
		end
	end

	-- One animation frame for the stars and the crash flash, stepwise like
	-- the web game's, not smooth.
	function self:tick()
		for _, s in stars do
			s.counter += 1
			if s.counter >= s.speed then
				s.counter = 0
				s.phase = (s.phase + 1) % 4
				if s.pulsing and s.phase == 0 then
					s.pulse = 1 - s.pulse
				end
			end
			s.frame.Visible = s.phase ~= 3
			s.frame.BackgroundTransparency = if s.pulsing and s.pulse == 0 then 0.5 else 0
		end
		if flashLeft > 0 then
			flashLeft -= 1
			flashFrame.BackgroundTransparency = 1 - flashLeft / 20
		end
	end

	function self:flash()
		flashLeft = 12
	end

	return self
end

return Playfield
