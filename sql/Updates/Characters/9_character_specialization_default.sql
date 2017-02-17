ALTER TABLE `characters`
	CHANGE COLUMN `specialization1` `specialization1` INT(11) NOT NULL DEFAULT '0' AFTER `activespec`,
	CHANGE COLUMN `specialization2` `specialization2` INT(11) NOT NULL DEFAULT '0' AFTER `specialization1`;
