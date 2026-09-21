--!strict
-- The screens laid over the playfield: title, pilot select, ready, HUD, game
-- over and the score board. Each is a full-size frame inside the playfield,
-- so it letterboxes and scales with the game. Screens only build and fill
-- themselves; which one is showing is Main's business.

local ReplicatedStorage = game:GetService("ReplicatedStorage")

local Shared = ReplicatedStorage:WaitForChild("MoonlightDrift")
local Characters = require(Shared:WaitForChild("Characters"))
local Assets = require(script.Parent:WaitForChild("Assets"))

-- The middle of each sprite, which is where the art is; the rest of the
-- 128x128 is room for glows and exhaust.
local PORTRAIT_CROP = 80

local GOLD = Color3.fromHex("#ffd700")
local CYAN = Color3.fromHex("#00d4ff")
local WHITE = Color3.new(1, 1, 1)
local DIM = Color3.fromRGB(170, 170, 200)
local PANEL = Color3.fromRGB(14, 16, 44)

export type Entry = { name: string, score: number }
export type Board = { entries: { Entry }, persistent: boolean }

local function overlay(name: string, parent: Instance, shade: number): Frame
	local f = Instance.new("Frame")
	f.Name = name
	f.Size = UDim2.fromScale(1, 1)
	f.BackgroundColor3 = Color3.new(0, 0, 0)
	f.BackgroundTransparency = shade
	f.BorderSizePixel = 0
	f.Visible = false
	f.ZIndex = 10
	f.Parent = parent
	return f
end

local function text(parent: Instance, s: string, y: number, h: number, color: Color3?, x: number?, w: number?): TextLabel
	local l = Instance.new("TextLabel")
	l.BackgroundTransparency = 1
	l.AnchorPoint = Vector2.new(0.5, 0)
	l.Position = UDim2.fromScale(x or 0.5, y)
	l.Size = UDim2.fromScale(w or 0.9, h)
	l.Font = Enum.Font.Arcade
	l.Text = s
	l.TextScaled = true
	l.TextColor3 = color or WHITE
	l.ZIndex = 11
	l.Parent = parent
	return l
end

local function button(parent: Instance, s: string, y: number, onClick: () -> ()): TextButton
	local b = Instance.new("TextButton")
	b.AnchorPoint = Vector2.new(0.5, 0)
	b.Position = UDim2.fromScale(0.5, y)
	b.Size = UDim2.fromScale(0.34, 0.085)
	b.BackgroundColor3 = PANEL
	b.BorderSizePixel = 0
	b.AutoButtonColor = true
	b.Font = Enum.Font.Arcade
	b.Text = s
	b.TextScaled = true
	b.TextColor3 = CYAN
	b.ZIndex = 11
	local pad = Instance.new("UIPadding")
	pad.PaddingTop = UDim.new(0.18, 0)
	pad.PaddingBottom = UDim.new(0.18, 0)
	pad.Parent = b
	local stroke = Instance.new("UIStroke")
	stroke.ApplyStrokeMode = Enum.ApplyStrokeMode.Border
	stroke.Color = CYAN
	stroke.Thickness = 2
	stroke.Parent = b
	b.Activated:Connect(onClick)
	b.Parent = parent
	return b
end

local function fillBoard(list: Frame, board: Board?, highlight: string?)
	for _, child in list:GetChildren() do
		if child:IsA("TextLabel") then
			child:Destroy()
		end
	end
	if not board then
		text(list, "loading...", 0, 0.1, DIM)
		return
	end
	if #board.entries == 0 then
		text(list, "no scores yet", 0, 0.1, DIM)
	end
	for i, e in board.entries do
		local y = (i - 1) * 0.1
		local color = if e.name == highlight then GOLD else WHITE
		local rank = text(list, ("%2d."):format(i), y, 0.09, color, 0.12, 0.2)
		rank.TextXAlignment = Enum.TextXAlignment.Right
		local name = text(list, e.name, y, 0.09, color, 0.55, 0.6)
		name.TextXAlignment = Enum.TextXAlignment.Left
		local score = text(list, tostring(e.score), y, 0.09, color, 0.93, 0.2)
		score.TextXAlignment = Enum.TextXAlignment.Right
	end
	if not board.persistent then
		text(list, "(this server only)", 1.02, 0.07, DIM)
	end
end

local function boardList(parent: Instance, y: number, h: number): Frame
	local f = Instance.new("Frame")
	f.Name = "Board"
	f.AnchorPoint = Vector2.new(0.5, 0)
	f.Position = UDim2.fromScale(0.5, y)
	f.Size = UDim2.fromScale(0.42, h)
	f.BackgroundTransparency = 1
	f.ZIndex = 11
	f.Parent = parent
	return f
end

export type Actions = {
	play: () -> (),
	openSelect: () -> (),
	openBoard: () -> (),
	back: () -> (),
	choose: (index: number) -> (),
	again: () -> (),
	toggleSound: () -> (),
}

export type Screens = {
	title: Frame,
	select: Frame,
	ready: Frame,
	hud: Frame,
	over: Frame,
	board: Frame,
	setPilot: (character: Characters.Character) -> (),
	setScore: (score: number) -> (),
	setSound: (on: boolean) -> (),
	showOver: (score: number, reply: { best: number, isNewBest: boolean, accepted: boolean, board: Board }?, me: string) -> (),
	showBoard: (board: Board?, me: string) -> (),
	show: (which: Frame?) -> (),
}

local Screens = {}

function Screens.new(playfield: Frame, actions: Actions): Screens
	-- Title
	local title = overlay("Title", playfield, 0.35)
	text(title, "MOONLIGHT DRIFT", 0.14, 0.14, GOLD)
	text(title, "hold to rise, release to fall, dodge the cave", 0.3, 0.05, DIM)
	button(title, "FLY", 0.44, actions.play)
	local pilotButton = button(title, "PILOT", 0.56, actions.openSelect)
	button(title, "SCORES", 0.68, actions.openBoard)
	local soundButton = button(title, "SOUND ON", 0.03, actions.toggleSound)
	soundButton.AnchorPoint = Vector2.new(1, 0)
	soundButton.Position = UDim2.fromScale(0.98, 0.03)
	soundButton.Size = UDim2.fromScale(0.16, 0.06)
	text(title, "SPACE / CLICK / TAP / (A) to thrust", 0.86, 0.045, DIM)

	-- Pilot select
	local select = overlay("Select", playfield, 0.15)
	text(select, "CHOOSE YOUR PILOT", 0.04, 0.08, GOLD)
	local grid = Instance.new("ScrollingFrame")
	grid.AnchorPoint = Vector2.new(0.5, 0)
	grid.Position = UDim2.fromScale(0.5, 0.15)
	grid.Size = UDim2.fromScale(0.9, 0.68)
	grid.BackgroundTransparency = 1
	grid.BorderSizePixel = 0
	grid.ScrollBarThickness = 6
	grid.AutomaticCanvasSize = Enum.AutomaticSize.Y
	grid.CanvasSize = UDim2.new()
	grid.ZIndex = 11
	grid.Parent = select
	local layout = Instance.new("UIGridLayout")
	layout.CellSize = UDim2.new(1 / 4, -8, 0, 0)
	layout.CellPadding = UDim2.fromOffset(8, 8)
	layout.SortOrder = Enum.SortOrder.LayoutOrder
	layout.Parent = grid
	-- Cells are a fixed fraction of the grid's width, kept square-ish by an
	-- aspect constraint, so the roster reads the same on a phone as a monitor.
	local cellAspect = Instance.new("UIAspectRatioConstraint")
	cellAspect.AspectRatio = 2.4
	cellAspect.Parent = layout
	for i, ch in Characters.list do
		local cell = Instance.new("TextButton")
		cell.LayoutOrder = i
		cell.BackgroundColor3 = PANEL
		cell.BorderSizePixel = 0
		cell.Text = ""
		cell.ZIndex = 11
		local stroke = Instance.new("UIStroke")
		stroke.ApplyStrokeMode = Enum.ApplyStrokeMode.Border
		stroke.Color = Color3.fromHex(ch.accent)
		stroke.Thickness = 2
		stroke.Parent = cell
		local swatch = Instance.new("TextLabel")
		swatch.Position = UDim2.fromScale(0.04, 0.15)
		swatch.Size = UDim2.fromScale(0.26, 0.7)
		swatch.BackgroundColor3 = Color3.fromHex(ch.accent)
		swatch.BorderSizePixel = 0
		swatch.Font = Enum.Font.Arcade
		swatch.Text = ch.glyph
		swatch.TextScaled = true
		swatch.TextColor3 = PANEL
		swatch.ZIndex = 12
		swatch.Parent = cell
		if Assets.pilotSheet ~= "" then
			swatch.Text = ""
			swatch.BackgroundColor3 = Color3.fromRGB(8, 10, 32)
			local portrait = Instance.new("ImageLabel")
			portrait.BackgroundTransparency = 1
			portrait.Size = UDim2.fromScale(1, 1)
			portrait.Image = Assets.pilotSheet
			local x, y = Characters.spriteOffset(i, false)
			local inset = (Characters.SPRITE_SIZE - PORTRAIT_CROP) / 2
			portrait.ImageRectOffset = Vector2.new(x + inset, y + inset)
			portrait.ImageRectSize = Vector2.new(PORTRAIT_CROP, PORTRAIT_CROP)
			portrait.ScaleType = Enum.ScaleType.Fit
			portrait.ZIndex = 13
			portrait.Parent = swatch
		end
		local name = text(cell, ch.name, 0.1, 0.42, WHITE, 0.66, 0.64)
		name.TextXAlignment = Enum.TextXAlignment.Left
		local stats = text(
			cell,
			("up %.2f  down %.2f  max %.1f"):format(-ch.thrust, ch.gravity, ch.maxVelocity),
			0.58,
			0.28,
			DIM,
			0.66,
			0.64
		)
		stats.TextXAlignment = Enum.TextXAlignment.Left
		cell.Activated:Connect(function()
			actions.choose(i)
		end)
		cell.Parent = grid
	end
	button(select, "BACK", 0.87, actions.back)

	-- Ready: the pilot hangs at the start until the first thrust.
	local ready = overlay("Ready", playfield, 1)
	text(ready, "HOLD TO THRUST", 0.4, 0.08, WHITE)
	local readyPilot = text(ready, "", 0.5, 0.05, DIM)

	-- HUD
	local hud = overlay("Hud", playfield, 1)
	local scoreLabel = text(hud, "0", 0.03, 0.08, WHITE)

	-- Game over
	local over = overlay("Over", playfield, 0.3)
	text(over, "CRASHED", 0.05, 0.1, Color3.fromHex("#ff5a5a"))
	local overScore = text(over, "", 0.17, 0.08, WHITE)
	local overNote = text(over, "", 0.26, 0.05, GOLD)
	local overList = boardList(over, 0.34, 0.4)
	local overAgain = button(over, "AGAIN", 0.8, actions.again)
	overAgain.Size = UDim2.fromScale(0.2, 0.085)
	overAgain.Position = UDim2.fromScale(0.36, 0.8)
	local overMenu = button(over, "MENU", 0.8, actions.back)
	overMenu.Size = UDim2.fromScale(0.2, 0.085)
	overMenu.Position = UDim2.fromScale(0.64, 0.8)

	-- Board
	local board = overlay("Scores", playfield, 0.2)
	text(board, "HIGH SCORES", 0.05, 0.1, GOLD)
	local boardFrame = boardList(board, 0.2, 0.55)
	button(board, "BACK", 0.84, actions.back)

	local all = { title, select, ready, hud, over, board }

	local self: Screens = {
		title = title,
		select = select,
		ready = ready,
		hud = hud,
		over = over,
		board = board,
		setPilot = function(ch)
			pilotButton.Text = "PILOT: " .. ch.name:upper()
			readyPilot.Text = ch.name
		end,
		setScore = function(score)
			scoreLabel.Text = tostring(score)
		end,
		setSound = function(on)
			soundButton.Text = if on then "SOUND ON" else "SOUND OFF"
		end,
		showOver = function(score, reply, me)
			overScore.Text = "SCORE " .. score
			if not reply then
				overNote.Text = "saving..."
				fillBoard(overList, nil)
			else
				overNote.Text = if reply.isNewBest
					then "NEW BEST!"
					elseif not reply.accepted then "score not recorded"
					else "best " .. reply.best
				fillBoard(overList, reply.board, me)
			end
		end,
		showBoard = function(b, me)
			fillBoard(boardFrame, b, me)
		end,
		show = function(which)
			for _, f in all do
				f.Visible = f == which
			end
		end,
	}
	return self
end

return Screens
