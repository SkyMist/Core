DROP TABLE IF EXISTS `scene_template`;
CREATE TABLE `scene_template` (
  `SceneId` int(10) unsigned NOT NULL,
  `Flags` int(10) unsigned NOT NULL DEFAULT '0',
  `ScriptPackageID` int(10) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`SceneId`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

DELETE FROM `scene_template` WHERE `SceneId`=1181;
INSERT INTO `scene_template` (`SceneId`, `Flags`, `ScriptPackageID`) VALUES (1181, 16, 1550);
