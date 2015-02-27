INSERT INTO `gameobject` SET `guid`='529537',`id`='211584',`map`='1008',`zoneId`='6125',`areaId`='6125',`spawnMask`='8',`phaseMask`='1',`position_x`='3944.38',
`position_y`='1550.06',`position_z`='369,563',`orientation`='6.28192',`rotation2`='0.000632675',`rotation3`='-1',`spawntimesecs`='300',`state`='1';

UPDATE `gameobject` SET `rotation2`='1',`spawntimesecs`='7200' WHERE `id` IN(213506);
UPDATE `gameobject` SET `state`='2' WHERE `id`='213506';
UPDATE `creature` SET `position_x`='4029.170',`position_y`='1910.724',`position_z`='380.251' WHERE `id`='65293';
DELETE FROM `gameobject` WHERE `id` IN(213528,213527,213529,213526,211650);
INSERT INTO `gameobject` VALUES ('529390', '213527', '1008', '6125', '6125', '15', '1', '4023.16', '1907.61', '326.197', '3.14159', '0', '0', '1', '-0.0000000437114', '7200', '255', '0', '0', null);
INSERT INTO `gameobject` VALUES ('524831', '213528', '1008', '6125', '6125', '15', '1', '4023.16', '1907.61', '300.17', '3.14159', '0', '0', '1', '-0.0000000437114', '7200', '255', '0', '0', null);
INSERT INTO `gameobject` VALUES ('524852', '213529', '1008', '6125', '6125', '15', '1', '4023.16', '1907.61', '353.23', '3.14159', '0', '0', '1', '-0.0000000437114', '7200', '255', '0', '0', null);
UPDATE `gameobject` SET `position_x`='4023.15',`position_y`='1907.6',`position_z`='360',`orientation`='3.14159' WHERE `id`='213526';
DELETE FROM `gameobject` WHERE `id` IN(213526);