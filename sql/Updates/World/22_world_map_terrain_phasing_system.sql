/*
SQLyog Ultimate v10.00 Beta1
MySQL - 5.6.21 : Database
*********************************************************************
*/

DROP TABLE IF EXISTS `phase_definitions`;

CREATE TABLE `phase_definitions` (
  `zoneId` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `entry` mediumint(8) unsigned NOT NULL AUTO_INCREMENT,
  `phasemask` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `phaseId` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `terrainswapmap` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `worldmaparea` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `flags` tinyint(3) unsigned NOT NULL DEFAULT '0',
  `comment` text,
  PRIMARY KEY (`zoneId`,`entry`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*Data for the table `phase_definitions` */

insert  into `phase_definitions`(`zoneId`,`entry`,`phasemask`,`phaseId`,`terrainswapmap`,`worldmaparea`,`flags`,`comment`) values 
(1519,1,129,0,0,0,0,'Stormwind: [A] Heros Call: Vashj\'ir'),
(1519,2,257,0,0,0,0,'Stormwind: [A] Heros Call: Hyjal'),
(1519,3,513,0,0,0,0,'Stormwind: [A] Heros Call: Deepholm'),
(1519,4,1025,0,0,0,0,'Stormwind: [A] Heros Call: Uldum'),
(1519,5,2049,0,0,0,0,'Stormwind: [A] Heros Call: Twilight Highlands'),
(1637,1,129,0,0,0,0,'Orgrimmar: [H] Warchiefs Command: Vashj\'ir'),
(1637,2,257,0,0,0,0,'Orgrimmar: [H] Warchiefs Command: Hyjal'),
(1637,3,513,0,0,0,0,'Orgrimmar: [H] Warchiefs Command: Deepholm'),
(1637,4,1025,0,0,0,0,'Orgrimmar: [H] Warchiefs Command: Uldum'),
(1637,5,2049,0,0,0,0,'Orgrimmar: [H] Warchiefs Command: Twilight Highlands'),
(616,1,0,165,719,0,0,'Mount Hyjal: Default Terrainswap'),
(5736,3,0,0,976,0,0,'Wandering Island - Turtle Healed'),
(5736,1,1,0,0,0,0,'Wandering Island - Default Phase'),
(5736,2,0,0,975,0,0,'Wandering Island - Turtle Hurted'),
(4755,1,0,102,638,0,0,'Gilneas City: [A] Default Terrainswap');

/*Table structure for table `spell_phase` */

DROP TABLE IF EXISTS `spell_phase`;

CREATE TABLE `spell_phase` (
  `id` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `phasemask` mediumint(8) unsigned NOT NULL DEFAULT '1',
  `terrainswapmap` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `worldmaparea` mediumint(8) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`id`)
) ENGINE=MyISAM DEFAULT CHARSET=utf8;

/*Data for the table `spell_phase` */
