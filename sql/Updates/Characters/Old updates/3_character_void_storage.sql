/*
SQLyog Ultimate v10.00 Beta1
MySQL - 5.6.21 : Database
*********************************************************************
*/
/*Table structure for table `character_void_storage` */

DROP TABLE IF EXISTS `character_void_storage`;

CREATE TABLE `character_void_storage` (
  `itemId` bigint(20) unsigned NOT NULL,
  `playerGuid` mediumint(8) unsigned NOT NULL,
  `itemEntry` mediumint(8) unsigned NOT NULL,
  `slot` tinyint(3) unsigned NOT NULL,
  `creatorGuid` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `randomProperty` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `reforgeId` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `transmogrifyId` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `upgradeId` mediumint(8) unsigned NOT NULL DEFAULT '0',
  `suffixFactor` mediumint(8) unsigned NOT NULL DEFAULT '0',
  PRIMARY KEY (`itemId`),
  UNIQUE KEY `idx_player_slot` (`playerGuid`,`slot`),
  KEY `idx_player` (`playerGuid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
