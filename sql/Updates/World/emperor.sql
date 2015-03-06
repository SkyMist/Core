 INSERT INTO `gameobject` SET `guid`='529537',`id`='211584',`map`='1008',`zoneId`='6125',`areaId`='6125',`spawnMask`='8',`phaseMask`='1',`position_x`='3944.38',
`position_y`='1550.06',`position_z`='369,563',`orientation`='6.28192',`rotation2`='0.000632675',`rotation3`='-1',`spawntimesecs`='300',`state`='1';
-- mogu mashine

INSERT INTO `creature` SET `guid`='998006',`id`='60648',`map`='1008',`zoneId`='6125',`areaId`='6125',`spawnMask`='8',`phaseMask`='1',`position_x`='3869.32',
`position_y`='1550.49',`position_z`='362.199',`orientation`='6.24798',`spawntimesecs`='300',`curhealth`='100';
-- Qin-xi
INSERT INTO `creature` SET `guid`='999108',`id`='60399',`map`='1008',`zoneId`='6125',`areaId`='6125',`spawnMask`='8',`phaseMask`='1',`position_x`='381.666',
`position_y`='1517.68',`position_z`='368.245',`orientation`='0.447018',`spawntimesecs`='300',`curhealth`='100';


-- Jan-Xi
INSERT INTO `creature` SET `guid`='999109',`id`='60400',`map`='1008',`zoneId`='6125',`areaId`='6125',`spawnMask`='8',`phaseMask`='1',`position_x`='381.670',
`position_y`='1582.81',`position_z`='368.244',`orientation`='5.714532',`spawntimesecs`='300',`curhealth`='100';

UPDATE `creature_template` SET `faction_A`='14',`faction_H`='14' WHERE `entry` IN (60648,60399,60400);
UPDATE `creature_template` SET `minlevel`='92',`maxlevel`='92',`faction_A`='14',`faction_H`='14',`mindmg`='3000',`maxdmg`='9000',`attackpower`='45000' WHERE `entry`='60396';
UPDATE `creature_template` SET `ScriptName`='mob_woe_add_generic' WHERE `entry` IN (60396,60397,60398);