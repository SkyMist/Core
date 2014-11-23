/*
SQLyog Ultimate v10.00 Beta1
MySQL - 5.6.21 : Database
*********************************************************************
*/
/*Table structure for table `battleground_template` */

DROP TABLE IF EXISTS `battleground_template`;

CREATE TABLE `battleground_template` (
  `id` mediumint(8) unsigned NOT NULL,
  `MinPlayersPerTeam` smallint(5) unsigned NOT NULL DEFAULT '0',
  `MaxPlayersPerTeam` smallint(5) unsigned NOT NULL DEFAULT '0',
  `MinLvl` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `MaxLvl` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `AllianceStartLoc` mediumint(8) unsigned NOT NULL,
  `AllianceStartO` float NOT NULL,
  `HordeStartLoc` mediumint(8) unsigned NOT NULL,
  `HordeStartO` float NOT NULL,
  `StartMaxDist` float NOT NULL DEFAULT '0',
  `Weight` tinyint(3) unsigned NOT NULL DEFAULT '1',
  `holiday` int(3) NOT NULL DEFAULT '0',
  `ScriptName` char(64) NOT NULL DEFAULT '',
  `Name_loc1` char(255) NOT NULL DEFAULT '',
  `Name_loc2` char(255) NOT NULL DEFAULT '',
  `Name_loc3` char(255) NOT NULL DEFAULT '',
  `Name_loc4` char(255) NOT NULL DEFAULT '',
  `Name_loc5` char(255) NOT NULL DEFAULT '',
  `Name_loc6` char(255) NOT NULL DEFAULT '',
  `Name_loc7` char(255) NOT NULL DEFAULT '',
  `Name_loc8` char(255) NOT NULL DEFAULT '',
  `Name_loc9` char(255) NOT NULL DEFAULT '',
  `Name_loc10` char(255) NOT NULL DEFAULT '',
  `Name_loc11` char(255) NOT NULL DEFAULT '',
  `Comment` char(32) NOT NULL,
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*Data for the table `battleground_template` */

insert  into `battleground_template`(`id`,`MinPlayersPerTeam`,`MaxPlayersPerTeam`,`MinLvl`,`MaxLvl`,`AllianceStartLoc`,`AllianceStartO`,`HordeStartLoc`,`HordeStartO`,`StartMaxDist`,`Weight`,`holiday`,`ScriptName`,`Name_loc1`,`Name_loc2`,`Name_loc3`,`Name_loc4`,`Name_loc5`,`Name_loc6`,`Name_loc7`,`Name_loc8`,`Name_loc9`,`Name_loc10`,`Name_loc11`,`Comment`) values (1,10,40,45,90,611,3.16312,610,0.715504,100,2,283,'','Alterac Valley','','','','','','','','','','','Alterac Valley'),(2,5,10,10,90,769,3.14159,770,0.151581,75,8,284,'','Warsong Gulch','','','','','','','','','','','Warsong Gulch'),(3,8,15,10,90,890,3.91571,889,0.813671,75,8,285,'','Arathi Basin','','','','','','','','','','','Arathi Basin'),(7,8,15,35,90,1103,3.03123,1104,0.055761,75,8,353,'','Eye of The Storm','','','','','','','','','','','Eye of The Storm'),(4,2,5,10,90,929,0,936,3.14159,0,1,0,'','Nagrand Arena','','','','','','','','','','','Nagrand Arena'),(5,2,5,10,90,939,0,940,3.14159,0,1,0,'','Blades\'s Edge Arena','','','','','','','','','','','Blades\'s Edge Arena'),(6,2,5,10,90,0,0,0,0,0,1,0,'','All Arena','','','','','','','','','','','All Arena'),(8,2,5,10,90,1258,0,1259,3.14159,0,1,0,'','Ruins of Lordaeron','','','','','','','','','','','Ruins of Lordaeron'),(9,8,15,65,90,1367,0,1368,0,0,6,400,'','Strand of the Ancients','','','','','','','','','','','Strand of the Ancients'),(10,2,5,10,90,1362,0,1363,3.14159,0,1,0,'','Dalaran Sewers','','','','','','','','','','','Dalaran Sewers'),(11,2,5,91,91,1364,0,1365,0,0,1,0,'','The Ring of Valor','','','','','','','','','','','The Ring of Valor'),(30,10,40,71,90,1485,0,1486,3.16124,200,2,420,'','Isle of Conquest','','','','','','','','','','','Isle of Conquest'),(32,5,40,45,90,0,0,0,0,0,1,0,'','Random battleground','','','','','','','','','','','Random battleground'),(699,5,10,90,90,4059,0,4060,0,20,8,489,'','Temple of Kotmogu','','','','','','','','','','','Temple of Kotmogu'),(108,5,10,85,90,1726,0,1727,0,0,8,436,'','Twin Peaks','','','','','','','','','','','Twin Peaks'),(120,5,10,85,90,1740,0,1799,0,0,8,435,'','The Battle for Gilneas','','','','','','','','','','','Battle for Gilneas'),(754,8,15,90,90,4487,0,4486,0,0,6,0,'','Deepwind Gorge','','','','','','','','','','','Deepwind Gorge'),(757,2,5,10,90,4534,0,4535,0,0,1,0,'','The Tiger\'s Peak','','','','','','','','','','','The Tiger\'s Peak'),(100,10,10,90,90,0,0,0,0,0,1,0,'','Rated Battleground 10v10','','','','','','','','','','','Rated Battleground 10v10'),(719,2,5,90,90,4136,0,4137,0,10,1,0,'','Tol\'viron Arena','','','','','','','','','','','Tol\'vir Arena');
