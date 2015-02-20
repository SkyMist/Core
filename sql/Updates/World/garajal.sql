INSERT INTO `spell_script_names` SET `spell_id`='116278',`ScriptName`='spell_soul_death';
INSERT INTO `spell_script_names` SET `spell_id`='116227',`ScriptName`='spell_spirit_intervation';
UPDATE `creature_template` SET `Health_mod`='140000' WHERE `entry`='60184';
+UPDATE `creature_template` SET `Health_mod`='1' WHERE `entry`='62003';
+UPDATE `creature_template` SET `modelid1`='42337',`modelid2`='0',`ScriptName`='mob_clone_player' WHERE `entry`='56405';