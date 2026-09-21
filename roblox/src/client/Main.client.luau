--!strict
-- Moonlight Drift for Roblox: the state machine, input and the fixed-step
-- loop. The game is a 2D arcade game drawn in the GUI; nobody has an avatar
-- (the project sets Players.CharacterAutoLoads to false), which is also why
-- this lives in StarterPlayerScripts and builds its own ScreenGui: without a
-- character, StarterGui's contents are never copied into PlayerGui.

local Players = game:GetService("Players")
local ReplicatedStorage = game:GetService("ReplicatedStorage")
local RunService = game:GetService("RunService")
local SoundService = game:GetService("SoundService")
local StarterGui = game:GetService("StarterGui")
local TweenService = game:GetService("TweenService")
local UserInputService = game:GetService("UserInputService")

local Shared = ReplicatedStorage:WaitForChild("MoonlightDrift")
local Config = require(Shared:WaitForChild("Config"))
local Characters = require(Shared:WaitForChild("Characters"))
local Run = require(Shared:WaitForChild("Run"))
local Assets = require(script.Parent:WaitForChild("Assets"))
local Playfield = require(script.Parent:WaitForChild("Playfield"))
local Screens = require(script.Parent:WaitForChild("Screens"))

local Remotes = ReplicatedStorage:WaitForChild("MoonlightDriftRemotes")
local StartRun = Remotes:WaitForChild("StartRun") :: RemoteEvent
local SubmitRun = Remotes:WaitForChild("SubmitRun") :: RemoteFunction
local GetBoard = Remotes:WaitForChild("GetBoard") :: RemoteFunction

local STEP = 1 / Config.FRAME_RATE
local MAX_STEPS_PER_RENDER = 6
-- A crash ignores thrust briefly, so a held button does not restart the run
-- the instant it ends.
local RESTART_LOCKOUT = 0.6

local localPlayer = Players.LocalPlayer

pcall(function()
	StarterGui:SetCoreGuiEnabled(Enum.CoreGuiType.Backpack, false)
	StarterGui:SetCoreGuiEnabled(Enum.CoreGuiType.Health, false)
	StarterGui:SetCoreGuiEnabled(Enum.CoreGuiType.EmotesMenu, false)
end)

------------------------------------------------------------------------------
-- Sound, as web/js/main.js sets it up: the music loops from the first menu
-- press for the rest of the session, fading in over two seconds, and does not
-- stop for a crash. One switch mutes music and effects together.

type SoundSpec = { id: string, volume: number, looped: boolean }
local SOUNDS: { [string]: SoundSpec } = {
	music = { id = Assets.music, volume = 0.3, looped = true },
	crash = { id = Assets.crash, volume = 0.5, looped = false },
	button = { id = Assets.button, volume = 0.4, looped = false },
	start = { id = Assets.start, volume = 0.4, looped = false },
}
local MUSIC_FADE_IN = 2

local soundGroup = Instance.new("SoundGroup")
soundGroup.Name = "MoonlightDrift"
soundGroup.Parent = SoundService

local sounds: { [string]: Sound } = {}
for name, spec in SOUNDS do
	if spec.id ~= "" then
		local s = Instance.new("Sound")
		s.Name = name
		s.SoundId = spec.id
		s.Looped = spec.looped
		s.Volume = spec.volume
		s.SoundGroup = soundGroup
		s.Parent = soundGroup
		sounds[name] = s
	end
end

local function play(name: string)
	local s = sounds[name]
	if s then
		s:Play()
	end
end

local musicStarted = false
local function startMusic()
	local music = sounds.music
	if musicStarted or not music then
		return
	end
	musicStarted = true
	music.Volume = 0
	music:Play()
	TweenService:Create(music, TweenInfo.new(MUSIC_FADE_IN), { Volume = SOUNDS.music.volume }):Play()
end

local soundOn = true
local function setSound(on: boolean)
	soundOn = on
	soundGroup.Volume = if on then 1 else 0
end

------------------------------------------------------------------------------
-- Input: one button, polled once per rendered frame. Every simulation step
-- inside that frame sees the same answer, as the Wii port's input_a_held()
-- does once per vsync.

local touches: { [InputObject]: boolean } = {}
UserInputService.TouchStarted:Connect(function(input)
	touches[input] = true
end)
UserInputService.TouchEnded:Connect(function(input)
	touches[input] = nil
end)

local THRUST_KEYS = { Enum.KeyCode.Space, Enum.KeyCode.W, Enum.KeyCode.Up }

local function thrustHeld(): boolean
	if UserInputService:GetFocusedTextBox() then
		return false -- typing in chat
	end
	for _, key in THRUST_KEYS do
		if UserInputService:IsKeyDown(key) then
			return true
		end
	end
	if UserInputService:IsMouseButtonPressed(Enum.UserInputType.MouseButton1) then
		return true
	end
	if UserInputService:IsGamepadButtonDown(Enum.UserInputType.Gamepad1, Enum.KeyCode.ButtonA) then
		return true
	end
	return next(touches) ~= nil
end

------------------------------------------------------------------------------
-- State

type State = "title" | "select" | "board" | "ready" | "playing" | "over"

local gui = Instance.new("ScreenGui")
gui.Name = "MoonlightDrift"
gui.IgnoreGuiInset = true
gui.ResetOnSpawn = false
gui.ZIndexBehavior = Enum.ZIndexBehavior.Sibling
local backdrop = Instance.new("Frame")
backdrop.Size = UDim2.fromScale(1, 1)
backdrop.BackgroundColor3 = Color3.new(0, 0, 0)
backdrop.BorderSizePixel = 0
backdrop.Parent = gui
gui.Parent = localPlayer:WaitForChild("PlayerGui")

local field = Playfield.new(gui)

local state: State = "title"
local characterIndex = 1
local run: Run.Run? = nil
local rng = Random.new()
local accumulator = 0
local wasHeld = true
local overAt = 0
local animAccumulator = 0

local screens: Screens.Screens

local function character(): Characters.Character
	return Characters.get(characterIndex)
end

-- A run that has not started yet: the pilot hanging at the start, drawn with
-- no obstacles, so READY shows exactly where the first frame begins.
local function freshRun(): Run.Run
	return Run.new(character(), function()
		return rng:NextNumber()
	end)
end

local function enter(to: State)
	state = to
	if to == "title" then
		field:clear()
		run = nil
		screens.show(screens.title)
	elseif to == "select" then
		screens.show(screens.select)
	elseif to == "board" then
		screens.show(screens.board)
		screens.showBoard(nil, localPlayer.Name)
		task.spawn(function()
			local ok, board = pcall(GetBoard.InvokeServer, GetBoard)
			if ok and state == "board" then
				screens.showBoard(board, localPlayer.Name)
			end
		end)
	elseif to == "ready" then
		field:clear()
		run = freshRun()
		screens.show(screens.ready)
	elseif to == "playing" then
		accumulator = 0
		screens.setScore(0)
		screens.show(screens.hud)
		StartRun:FireServer()
		play("start")
		startMusic()
	elseif to == "over" then
		play("crash")
		field:flash()
		overAt = os.clock()
		local score = if run then run.score else 0
		screens.show(screens.over)
		screens.showOver(score, nil, localPlayer.Name)
		task.spawn(function()
			local ok, reply = pcall(SubmitRun.InvokeServer, SubmitRun, score)
			if state == "over" then
				screens.showOver(score, if ok then reply else nil, localPlayer.Name)
			end
		end)
	end
end

local function clicked(fn: () -> ()): () -> ()
	return function()
		play("button")
		startMusic()
		fn()
	end
end

screens = Screens.new(field.root, {
	play = clicked(function()
		enter("ready")
	end),
	openSelect = clicked(function()
		enter("select")
	end),
	openBoard = clicked(function()
		enter("board")
	end),
	back = clicked(function()
		enter("title")
	end),
	choose = function(index: number)
		play("button")
		startMusic()
		characterIndex = index
		screens.setPilot(character())
		field:setPilot(character())
		enter("title")
	end,
	again = clicked(function()
		enter("ready")
	end),
	toggleSound = function()
		setSound(not soundOn)
		play("button")
		screens.setSound(soundOn)
	end,
})

screens.setPilot(character())
screens.setSound(soundOn)
field:setPilot(character())
enter("title")

------------------------------------------------------------------------------
-- Loop

RunService.RenderStepped:Connect(function(dt)
	local held = thrustHeld()
	local pressed = held and not wasHeld
	wasHeld = held

	if state == "title" and pressed and UserInputService:IsKeyDown(Enum.KeyCode.Space) then
		enter("ready")
	elseif state == "ready" and pressed then
		enter("playing")
	elseif state == "over" and pressed and os.clock() - overAt > RESTART_LOCKOUT then
		-- Mouse and touch are left to the buttons, so a click on MENU does
		-- not also restart the run underneath it.
		if not UserInputService:IsMouseButtonPressed(Enum.UserInputType.MouseButton1) and next(touches) == nil then
			enter("ready")
		end
	end

	if state == "playing" and run then
		accumulator += dt
		local steps = 0
		while accumulator >= STEP and steps < MAX_STEPS_PER_RENDER do
			accumulator -= STEP
			steps += 1
			local crash, gained = Run.step(run, held)
			if gained > 0 then
				screens.setScore(run.score)
			end
			if crash then
				enter("over")
				break
			end
		end
		-- After a hitch, drop the backlog rather than fast-forwarding through
		-- it: the run never outpaces the wall clock, which is the one thing
		-- the server's plausibility check relies on.
		accumulator = math.min(accumulator, STEP)
	end

	field:draw(run, held and state == "playing")
	-- Stars blink on the 60 Hz clock too, whatever the display's refresh.
	animAccumulator = math.min(animAccumulator + dt, 4 * STEP)
	while animAccumulator >= STEP do
		animAccumulator -= STEP
		field:tick()
	end
end)
