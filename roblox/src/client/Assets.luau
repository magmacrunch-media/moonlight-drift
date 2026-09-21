--!strict
-- Roblox asset ids for the game's art and sound. Roblox serves images and
-- audio only by id, and an id exists only once somebody has uploaded the file
-- (Studio: View > Asset Manager > Bulk Import, then right-click > Copy ID).
--
-- Paste each as "rbxassetid://<number>". An empty id is skipped: without the
-- sheet the pilots are drawn as their accent colour and glyph, and without a
-- sound id that sound is silent. Nothing else changes.
--
-- A sprite change means re-running tools/make_atlas.py, uploading the new
-- assets/pilots.png and pasting its new id here. An upload is never updated
-- in place.

return table.freeze({
	-- roblox/assets/pilots.png
	pilotSheet = "",
	-- web/audio/moonlightdrift-gameloop.ogg ("Moonlight Drift" by C.P. Rutledge)
	music = "",
	-- web/audio/crashsound.ogg
	crash = "",
	-- web/audio/buttonsound1.ogg, played on menu buttons
	button = "",
	-- web/audio/buttonsound2.ogg, played when a run starts
	start = "",
})
