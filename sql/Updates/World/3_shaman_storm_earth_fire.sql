UPDATE `creature_template` SET `modelid1`='45960',`ScriptName`='npc_monk_spirit' WHERE `entry` IN(69680,69791,69792);

INSERT INTO `spell_script_names` SET `spell_id`='137639',`ScriptName`='spell_monk_storm_earth_and_fire';

DELETE FROM `disables` WHERE `entry`='137639';