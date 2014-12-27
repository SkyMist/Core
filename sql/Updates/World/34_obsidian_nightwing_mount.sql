UPDATE `creature_template` SET `npcflag`=`npcflag`|16777216, `VehicleId`=2237 WHERE `entry`=62454;

DELETE FROM `npc_spellclick_spells` WHERE `npc_entry`=62454;
INSERT INTO `npc_spellclick_spells` (`npc_entry`, `spell_id`, `cast_flags`, `user_type`) VALUES 
(62454, 93970, 1, 0);