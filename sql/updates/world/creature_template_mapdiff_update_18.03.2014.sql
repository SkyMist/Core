-- Diff entry 4 is for LFR, 5 is for Flex Raid.
alter table `creature_template` 
add column `difficulty_entry_4` mediumint (8)UNSIGNED  DEFAULT '0' NOT NULL  after `difficulty_entry_3`, 
add column `difficulty_entry_5` mediumint (8)UNSIGNED  DEFAULT '0' NOT NULL  after `difficulty_entry_4`;
