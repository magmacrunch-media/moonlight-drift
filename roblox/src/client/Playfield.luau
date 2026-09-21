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

local WW, WH = Config.WORLD_WIDTH, Config.WORLD_HEIGHT
local OW = Config.OBSTACLE_WIDTH
local SLICE = 8
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

	-- Pilot: a coloured body the size of the hitbox, so the box you steer is
	-- the box that collides, with the flame hanging beneath it.
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

	local flashFrame = plain("Flash", Color3.new(1, 1, 1), root)
	flashFrame.Size = UDim2.fromScale(1, 1)
	flashFrame.BackgroundTransparency = 1
	flashFrame.ZIndex = 8

	local columns: { [number]: Frame } = {}
	local flameFrame = 0
	local flashLeft = 0

	local function buildColumn(o: Obstacles.Obstacle): Frame
		local c = Instance.new("Frame")
		c.Name = "Column" .. o.id
		c.BackgroundTransparency = 1
		c.Size = UDim2.fromScale(OW / WW, 1)
		c.ZIndex = 2

		local primary, secondary, accent = rgb(o.theme.primary), rgb(o.theme.secondary), rgb(o.theme.accent)
		local function slice(y: number, h: number, left: number, right: number, color: Color3)
			local s = plain("S", color, c)
			s.Position = UDim2.fromScale((left - o.x) / OW, y / WH)
			-- One unit of overlap hides the seams scaling would otherwise open.
			s.Size = UDim2.fromScale((right - left) / OW, (h + 1) / WH)
			s.ZIndex = 2
		end

		-- Stalactite: widest at its root, so each slice takes its top row.
		local y, band = 0, 0
		while y < o.topHeight do
			local h = math.min(SLICE, o.topHeight - y)
			local l, r = Obstacles.profileTop(o, y)
			local tip = y + h >= o.topHeight
			slice(y, h, l, r, if tip then accent elseif band % 2 == 0 then primary else secondary)
			y += h
			band += 1
		end
		-- Stalagmite: widest at its foot, so each slice takes its bottom row.
		y, band = o.bottomY, 0
		while y < WH do
			local h = math.min(SLICE, WH - y)
			local l, r = Obstacles.profileBottom(o, y + h)
			slice(y, h, l, r, if band == 0 then accent elseif band % 2 == 0 then primary else secondary)
			y += h
			band += 1
		end

		if o.milestone > 0 then
			local label = Instance.new("TextLabel")
			label.BackgroundTransparency = 1
			label.AnchorPoint = Vector2.new(0.5, 0.5)
			label.Position = UDim2.fromScale(0.5, (o.topHeight + Config.GAP / 2) / WH)
			label.Size = UDim2.fromScale(2.5, 40 / WH)
			label.Font = Enum.Font.Arcade
			label.Text = tostring(o.milestone)
			label.TextScaled = true
			label.TextColor3 = accent
			label.TextTransparency = 0.25
			label.ZIndex = 2
			label.Parent = c
		end

		c.Parent = columnLayer
		return c
	end

	local self = {} :: Playfield
	self.root = root

	function self:setPilot(character)
		pilot.BackgroundColor3 = Color3.fromHex(character.accent)
		glyph.Text = character.glyph
	end

	function self:clear()
		for id, c in columns do
			c:Destroy()
			columns[id] = nil
		end
	end

	function self:draw(run, thrustHeld)
		local p = run and run.pilot
		pilot.Visible = p ~= nil
		if p then
			local hb = p.hitbox
			box(pilot, p.x + hb.ox, p.y + hb.oy, hb.w, hb.h)
			pilot.Rotation = math.clamp(p.velocity * 2, -18, 18)
		end
		flameFrame += 1
		local flicker = if flameFrame % 6 < 3 then 1 else 0.8
		local burning = thrustHeld and run ~= nil and run.crash == nil
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
