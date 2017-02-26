ALTER TABLE `boss_loot_weekly_quest`
	ADD COLUMN `difficulty` MEDIUMINT(8) UNSIGNED NOT NULL DEFAULT '0' COMMENT 'Instance Difficulty' AFTER `entry`,
	DROP PRIMARY KEY,
	ADD PRIMARY KEY (`entry`, `difficulty`);
