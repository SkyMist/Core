ALTER TABLE `quest_template`
ADD COLUMN `RewardPackageItemId` smallint(5) unsigned NOT NULL DEFAULT '0' AFTER `QuestTurnInPortrait`,
MODIFY COLUMN `RewardCurrencyCount1` smallint(5) unsigned NOT NULL DEFAULT '0',
MODIFY COLUMN `RewardCurrencyCount2` smallint(5) unsigned NOT NULL DEFAULT '0',
MODIFY COLUMN `RewardCurrencyCount3` smallint(5) unsigned NOT NULL DEFAULT '0',
MODIFY COLUMN `RewardCurrencyCount4` smallint(5) unsigned NOT NULL DEFAULT '0';