/*Table structure for table `character_rated_battlegrounds` */

DROP TABLE IF EXISTS `character_rated_battlegrounds`;

CREATE TABLE `character_rated_battlegrounds` (
  `guid` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Global Unique Identifier',
  `rating` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Player BG Rating',
  `played` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Player Rated Bg''s played',
  `won` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Player Rated Bg''s won',
  `playedWeek` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Player Rated Bg''s played week',
  `wonWeek` int(10) unsigned NOT NULL DEFAULT '0' COMMENT 'Player Rated Bg''s won week',
  PRIMARY KEY (`guid`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8 COMMENT='BG Rating System';