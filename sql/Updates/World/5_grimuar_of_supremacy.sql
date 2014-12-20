INSERT INTO `spell_learn_spell` SET `entry`='108501',`SpellID`='111859',`Active`='1';
INSERT INTO `spell_learn_spell` SET `entry`='108501',`SpellID`='111895',`Active`='1';
INSERT INTO `spell_learn_spell` SET `entry`='108501',`SpellID`='111896',`Active`='1';
INSERT INTO `spell_learn_spell` SET `entry`='108501',`SpellID`='111897',`Active`='1';
INSERT INTO `spell_learn_spell` SET `entry`='108501',`SpellID`='111898',`Active`='1';

UPDATE `creature_template` SET `ScriptName`='npc_grimuar_minion' WHERE `entry` IN (416,1860,1863,417,17252);