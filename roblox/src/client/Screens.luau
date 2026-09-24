--!strict
-- The screens laid over the playfield: title, pilot select, ready, HUD, game
-- over and the score board. Each is a full-size frame inside the playfield,
-- so it letterboxes and scales with the game. Screens only build and fill
-- themselves; which one is showing is Main's business.

local ReplicatedStorage = game:GetService("ReplicatedStorage")

local Shared = ReplicatedStorage:WaitForChild("MoonlightDrift")
local Characters = require(Shared:WaitForChild("Characters"))
local Config = require(Shared:WaitForChild("Config"))
local Logo = require(script.Parent:WaitForChild("Logo"))
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
		text(list, "loading...", 0.4, 0.1, DIM)
		return
	end
	if #board.entries == 0 then
		text(list, "no scores yet", 0.4, 0.1, DIM)
	end
	-- Two columns, top 5 and ranks 6-10, as updateScoreboard() in
	-- web/js/scoring.js lays them out.
	text(list, "top 5", 0, 0.09, CYAN, 0.25, 0.4)
	if #board.entries > 5 then
		text(list, "ranks 6-10", 0, 0.09, CYAN, 0.75, 0.4)
	end
	for i, e in board.entries do
		local column = if i <= 5 then 0 else 1
		local y = 0.12 + ((i - 1) % 5) * 0.14
		local mine = e.name == highlight
		local color = if mine then GOLD else WHITE
		local left = 0.02 + column * 0.5
		local rank = text(list, ("#%d"):format(i), y, 0.12, color, left + 0.06, 0.12)
		rank.TextXAlignment = Enum.TextXAlignment.Left
		local name = text(list, e.name, y, 0.12, color, left + 0.26, 0.26)
		name.TextXAlignment = Enum.TextXAlignment.Left
		local score = text(list, e.score .. " pts", y, 0.12, color, left + 0.44, 0.2)
		score.TextXAlignment = Enum.TextXAlignment.Right
	end
	if not board.persistent then
		text(list, "(this server only)", 0.88, 0.08, DIM)
	end
end

local function boardList(parent: Instance, y: number, h: number): Frame
	local f = Instance.new("Frame")
	f.Name = "Board"
	f.AnchorPoint = Vector2.new(0.5, 0)
	f.Position = UDim2.fromScale(0.5, y)
	f.Size = UDim2.fromScale(0.86, h)
	f.BackgroundTransparency = 1
	f.ZIndex = 11
	f.Parent = parent
	return f
end

export type Actions = {
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
	-- The website's title card: the ASCII logo in cyan on black inside a cyan
	-- border, with "press [space] or click to start!" under it. The buttons
	-- are this version's own; on the web they are HTML controls around the
	-- canvas, and there is nowhere here to put them but the screen.
	local card = Instance.new("Frame")
	card.AnchorPoint = Vector2.new(0.5, 0)
	card.Position = UDim2.fromScale(0.5, 0.08)
	card.Size = UDim2.fromScale(0.82, 0.34)
	card.BackgroundColor3 = Color3.new(0, 0, 0)
	card.BorderSizePixel = 0
	card.ZIndex = 11
	local cardStroke = Instance.new("UIStroke")
	cardStroke.Color = CYAN
	cardStroke.Thickness = 3
	cardStroke.Parent = card
	card.Parent = title
	local logo = Instance.new("TextLabel")
	logo.BackgroundTransparency = 1
	logo.AnchorPoint = Vector2.new(0.5, 0.5)
	logo.Position = UDim2.fromScale(0.5, 0.5)
	logo.Size = UDim2.fromScale(0.94, 0.8)
	logo.Font = Enum.Font.Code
	logo.Text = Logo
	logo.TextScaled = true
	logo.TextColor3 = CYAN
	logo.ZIndex = 12
	logo.Parent = card

	text(title, "press [space] or click to start!", 0.45, 0.05, WHITE)
	local pilotButton = button(title, "PILOT", 0.56, actions.openSelect)
	button(title, "SCORES", 0.68, actions.openBoard)
	local soundButton = button(title, "SOUND ON", 0.03, actions.toggleSound)
	soundButton.AnchorPoint = Vector2.new(1, 0)
	soundButton.Position = UDim2.fromScale(0.98, 0.03)
	soundButton.Size = UDim2.fromScale(0.16, 0.06)

	-- Pilot select
	local select = overlay("Select", playfield, 0.15)
	text(select, "select your character", 0.04, 0.08, GOLD)
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
	layout.CellSize = UDim2.new(1 / 3, -8, 0, 0)
	layout.CellPadding = UDim2.fromOffset(8, 8)
	layout.SortOrder = Enum.SortOrder.LayoutOrder
	layout.Parent = grid
	-- Cells are a fixed fraction of the grid's width, kept square-ish by an
	-- aspect constraint, so the roster reads the same on a phone as a monitor.
	local cellAspect = Instance.new("UIAspectRatioConstraint")
	cellAspect.AspectRatio = 2.0
	cellAspect.Parent = layout
	local cells: { TextButton } = {}
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
			local name = text(cell, ch.name, 0.08, 0.32, WHITE, 0.66, 0.64)
		name.TextXAlignment = Enum.TextXAlignment.Left
		local stats = text(
			cell,
			("up %.2f  down %.2f  max %.1f"):format(-ch.thrust, ch.gravity, ch.maxVelocity),
			0.46,
			0.2,
			DIM,
			0.66,
			0.64
		)
		stats.TextXAlignment = Enum.TextXAlignment.Left
		-- What the character modal on the website says about how it flies.
		local blurb = text(cell, ch.description, 0.68, 0.26, DIM, 0.66, 0.64)
		blurb.TextXAlignment = Enum.TextXAlignment.Left
		blurb.TextWrapped = true
		blurb.TextScaled = false
		blurb.TextSize = 12
		blurb.Font = Enum.Font.Gotham
		cell.Activated:Connect(function()
			actions.choose(i)
		end)
		cell.Parent = grid
		cells[i] = cell
	end
	button(select, "done", 0.87, actions.back)

	-- Ready: the pilot hangs at the start until the first thrust. The text is
	-- the website's how-to-play panel, word for word.
	local ready = overlay("Ready", playfield, 0.55)
	text(ready, "how to play", 0.2, 0.08, GOLD)
	text(ready, "hold [space] or click to fly up", 0.34, 0.06, WHITE)
	text(ready, "release to fall down", 0.42, 0.06, WHITE)
	text(ready, "avoid hitting the obstacles and boundaries!", 0.52, 0.05, DIM)
	text(ready, "press [space] or click to start!", 0.64, 0.06, CYAN)
	local readyPilot = text(ready, "", 0.74, 0.05, DIM)

	-- HUD: the score plate from drawScoreOnCanvas() in web/js/scoring.js, a
	-- 180x50 box in the world's top-left corner with chunky layered borders.
	local hud = overlay("Hud", playfield, 1)
	local WW, WH = Config.WORLD_WIDTH, Config.WORLD_HEIGHT
	local function plate(x: number, y: number, w: number, h: number, color: Color3, parent: Instance): Frame
		local f = Instance.new("Frame")
		f.Position = UDim2.fromScale(x / WW, y / WH)
		f.Size = UDim2.fromScale(w / WW, h / WH)
		f.BackgroundColor3 = color
		f.BorderSizePixel = 0
		f.ZIndex = 11
		f.Parent = parent
		return f
	end
	plate(10, 10, 180, 50, Color3.fromHex("#6a7a9a"), hud)
	plate(13, 13, 174, 44, Color3.fromHex("#3a4466"), hud)
	plate(16, 16, 168, 38, Color3.fromHex("#2a3a5a"), hud)
	plate(18, 18, 164, 34, Color3.fromHex("#3a4466"), hud)
	local scoreLabel = Instance.new("TextLabel")
	scoreLabel.BackgroundTransparency = 1
	scoreLabel.Position = UDim2.fromScale(25 / WW, 24 / WH)
	scoreLabel.Size = UDim2.fromScale(150 / WW, 22 / WH)
	scoreLabel.Font = Enum.Font.Arcade
	scoreLabel.Text = "score: 0"
	scoreLabel.TextScaled = true
	scoreLabel.TextXAlignment = Enum.TextXAlignment.Left
	scoreLabel.TextColor3 = GOLD
	scoreLabel.ZIndex = 12
	local scoreStroke = Instance.new("UIStroke")
	scoreStroke.Color = Color3.new(0, 0, 0)
	scoreStroke.Thickness = 2
	scoreStroke.Parent = scoreLabel
	scoreLabel.Parent = hud

	-- Game over, in the website's words, with the board under it.
	local over = overlay("Over", playfield, 0.3)
	text(over, "GAME OVER!", 0.04, 0.1, Color3.fromHex("#ff5a5a"))
	local overScore = text(over, "final score: 0", 0.15, 0.07, WHITE)
	local overNote = text(over, "", 0.23, 0.05, GOLD)
	text(over, "too cheap for ya?!", 0.29, 0.045, DIM)
	local overList = boardList(over, 0.35, 0.4)
	local overAgain = button(over, "AGAIN", 0.8, actions.again)
	overAgain.Size = UDim2.fromScale(0.2, 0.085)
	overAgain.Position = UDim2.fromScale(0.36, 0.8)
	local overMenu = button(over, "MENU", 0.8, actions.back)
	overMenu.Size = UDim2.fromScale(0.2, 0.085)
	overMenu.Position = UDim2.fromScale(0.64, 0.8)
	text(over, "press [space] to restart", 0.9, 0.045, DIM)

	-- Scores, which on the website is a modal with the credits beneath it.
	local board = overlay("Scores", playfield, 0.2)
	text(board, "all-time high scores", 0.04, 0.08, GOLD)
	local boardFrame = boardList(board, 0.16, 0.42)
	text(board, "game design & development: Jake McCoy", 0.6, 0.04, DIM)
	text(board, 'music: "Moonlight Drift" by C.P. Rutledge', 0.65, 0.04, DIM)
	text(board, "inspired by Simon Dorsey's Jetman (2007), itself after", 0.7, 0.035, DIM)
	text(board, "David McCandless's Helicopter (2002) and SFCave (1995)", 0.745, 0.035, DIM)
	text(board, "publisher: magmacrunch media", 0.79, 0.04, DIM)
	button(board, "CLOSE", 0.86, actions.back)

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
			for i, cell in cells do
				local chosen = Characters.list[i].key == ch.key
				cell.BackgroundColor3 = if chosen then Color3.fromRGB(30, 38, 92) else PANEL
				local stroke = cell:FindFirstChildWhichIsA("UIStroke")
				if stroke then
					stroke.Thickness = if chosen then 4 else 2
					stroke.Color = if chosen then GOLD else Color3.fromHex(Characters.list[i].accent)
				end
			end
		end,
		setScore = function(score)
			scoreLabel.Text = "score: " .. score
		end,
		setSound = function(on)
			soundButton.Text = if on then "SOUND ON" else "SOUND OFF"
		end,
		showOver = function(score, reply, me)
			overScore.Text = "final score: " .. score
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
