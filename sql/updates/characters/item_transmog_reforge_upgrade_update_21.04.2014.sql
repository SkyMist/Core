alter table `character_void_storage` add column `reforgeId` int (10)UNSIGNED  DEFAULT '0' NOT NULL  after `randomProperty`, 
                                     add column `transmogrifyId` int (10)UNSIGNED  DEFAULT '0' NOT NULL  after `reforgeId`,
                                     add column `upgradeId` int (10)UNSIGNED  DEFAULT '0' NOT NULL  after `transmogrifyId`;

alter table `item_instance` add column `reforgeId` int (10)UNSIGNED  DEFAULT '0' NOT NULL  after `randomPropertyId`, 
                            add column `transmogrifyId` int (10)UNSIGNED  DEFAULT '0' NOT NULL  after `reforgeId`,
                            add column `upgradeId` int (10)UNSIGNED  DEFAULT '0' NOT NULL  after `transmogrifyId`;
