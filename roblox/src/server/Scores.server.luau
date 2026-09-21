--!strict
-- High scores. Each run is simulated on the player's own client, because a
-- one-button game played at 60 Hz cannot wait on a round trip, so the server
-- never sees the flight. What it can see is the clock: a run has to be
-- started before it can be submitted, and a score is refused if the run was
-- too short to have earned it (Run.maxPlausibleScore). That does not make
-- cheating impossible, but it caps a cheat at what an unbroken run of the
-- same length would have scored.
--
-- Bests live in an OrderedDataStore keyed by UserId. When DataStores are
-- unavailable, which in Studio is the default until "Enable Studio Access to
-- API Services" is ticked, scores are kept for the session only and the
-- board says so rather than failing.

local DataStoreService = game:GetService("DataStoreService")
local Players = game:GetService("Players")
local ReplicatedStorage = game:GetService("ReplicatedStorage")

local Shared = ReplicatedStorage:WaitForChild("MoonlightDrift")
local Config = require(Shared:WaitForChild("Config"))
local Run = require(Shared:WaitForChild("Run"))

local Remotes = ReplicatedStorage:WaitForChild("MoonlightDriftRemotes")
local StartRun = Remotes:WaitForChild("StartRun") :: RemoteEvent
local SubmitRun = Remotes:WaitForChild("SubmitRun") :: RemoteFunction
local GetBoard = Remotes:WaitForChild("GetBoard") :: RemoteFunction

-- Bump the suffix to wipe the board, e.g. after a balance change.
local STORE_NAME = "MoonlightDriftBest_v1"
local BOARD_CACHE_SECONDS = 30

local store: OrderedDataStore? = nil
do
	local ok, result = pcall(function()
		return DataStoreService:GetOrderedDataStore(STORE_NAME)
	end)
	if ok then
		store = result
	else
		warn("[MoonlightDrift] DataStore unavailable, scores are session-only:", result)
	end
end

type Entry = { name: string, score: number }
type BoardReply = { entries: { Entry }, persistent: boolean }
type SubmitReply = { accepted: boolean, best: number, isNewBest: boolean, board: BoardReply }

local runStartedAt: { [Player]: number } = {}
local bests: { [Player]: number } = {}
local sessionBoard: { [number]: Entry } = {}
local nameCache: { [number]: string } = {}
local cachedBoard: BoardReply? = nil
local cachedAt = 0

local function nameFor(userId: number): string
	local cached = nameCache[userId]
	if cached then
		return cached
	end
	local ok, name = pcall(Players.GetNameFromUserIdAsync, Players, userId)
	local resolved = if ok and name then name else ("#" .. userId)
	nameCache[userId] = resolved
	return resolved
end

local function sessionEntries(): { Entry }
	local list: { Entry } = {}
	for _, e in sessionBoard do
		table.insert(list, e)
	end
	table.sort(list, function(a, b)
		return a.score > b.score
	end)
	while #list > Config.HIGH_SCORE_COUNT do
		table.remove(list)
	end
	return list
end

local function readBoard(force: boolean): BoardReply
	if not store then
		return { entries = sessionEntries(), persistent = false }
	end
	if not force and cachedBoard and os.clock() - cachedAt < BOARD_CACHE_SECONDS then
		return cachedBoard
	end
	local s = store :: OrderedDataStore
	local ok, pages = pcall(function()
		return s:GetSortedAsync(false, Config.HIGH_SCORE_COUNT)
	end)
	if not ok then
		warn("[MoonlightDrift] could not read the board:", pages)
		return cachedBoard or { entries = sessionEntries(), persistent = false }
	end
	local entries: { Entry } = {}
	for _, item in (pages :: DataStorePages):GetCurrentPage() do
		local userId = tonumber(item.key)
		if userId then
			table.insert(entries, { name = nameFor(userId), score = item.value })
		end
	end
	cachedBoard = { entries = entries, persistent = true }
	cachedAt = os.clock()
	return cachedBoard :: BoardReply
end

local function setLeaderstat(player: Player, best: number)
	local stats = player:FindFirstChild("leaderstats")
	local value = stats and stats:FindFirstChild("Best")
	if value and value:IsA("IntValue") then
		value.Value = best
	end
end

local function loadBest(player: Player)
	local best = 0
	if store then
		local s = store :: OrderedDataStore
		local ok, value = pcall(function()
			return s:GetAsync(tostring(player.UserId))
		end)
		if ok and typeof(value) == "number" then
			best = value
		end
	end
	if player.Parent then
		bests[player] = math.max(bests[player] or 0, best)
		setLeaderstat(player, bests[player])
	end
end

local function onPlayerAdded(player: Player)
	local stats = Instance.new("Folder")
	stats.Name = "leaderstats"
	local best = Instance.new("IntValue")
	best.Name = "Best"
	best.Parent = stats
	stats.Parent = player
	task.spawn(loadBest, player)
end

Players.PlayerAdded:Connect(onPlayerAdded)
-- In Studio the first player can join before this script has connected.
for _, player in Players:GetPlayers() do
	onPlayerAdded(player)
end

Players.PlayerRemoving:Connect(function(player)
	runStartedAt[player] = nil
	bests[player] = nil
end)

StartRun.OnServerEvent:Connect(function(player)
	runStartedAt[player] = os.clock()
end)

SubmitRun.OnServerInvoke = function(player: Player, score: unknown): SubmitReply
	local started = runStartedAt[player]
	runStartedAt[player] = nil
	local best = bests[player] or 0

	local valid = started ~= nil
		and typeof(score) == "number"
		and score == score -- not NaN
		and score >= 0
		and score % 1 == 0
	if not valid then
		return { accepted = false, best = best, isNewBest = false, board = readBoard(false) }
	end
	local n = score :: number
	local elapsed = os.clock() - (started :: number)
	if n > Run.maxPlausibleScore(elapsed) then
		warn(("[MoonlightDrift] refused %d from %s after %.1fs"):format(n, player.Name, elapsed))
		return { accepted = false, best = best, isNewBest = false, board = readBoard(false) }
	end

	local isNewBest = n > best
	if isNewBest then
		best = n
		bests[player] = n
		setLeaderstat(player, n)
		sessionBoard[player.UserId] = { name = player.Name, score = n }
		nameCache[player.UserId] = player.Name
		if store then
			local s = store :: OrderedDataStore
			local ok, err = pcall(function()
				s:UpdateAsync(tostring(player.UserId), function(old: number?)
					if old and old >= n then
						return nil -- keep the stored best; a nil return cancels the write
					end
					return n
				end)
			end)
			if not ok then
				warn("[MoonlightDrift] could not save a best:", err)
			end
		end
	end
	return { accepted = true, best = best, isNewBest = isNewBest, board = readBoard(isNewBest) }
end

GetBoard.OnServerInvoke = function(): BoardReply
	return readBoard(false)
end
