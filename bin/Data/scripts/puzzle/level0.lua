local signals = require("scripts/signals.lua")

entered_trigger = false

trigger_end = signals.triggerEnter(function()
    if not entered_trigger then
        entered_trigger = true
        showObjective("Well done.")
    end
    fadeToBlack(2.1)
    delay(2, function ()
        loadMap("maps/puzzle/level1.mp")
    end)
end)

playAudio("event:/ambience/wind_calm")
fadeFromBlack(2)
delay(2, function ()
    if not entered_trigger then
        showObjective("Test. test.")
    end
end)
