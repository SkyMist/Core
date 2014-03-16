alter table `characters` 
change `power1` `power1` int (10) DEFAULT '0' NOT NULL , 
change `power2` `power2` int (10) DEFAULT '0' NOT NULL , 
change `power3` `power3` int (10) DEFAULT '0' NOT NULL , 
change `power4` `power4` int (10) DEFAULT '0' NOT NULL , 
change `power5` `power5` int (10) DEFAULT '0' NOT NULL;

alter table `character_stats` 
change `maxpower1` `maxpower1` int (10) DEFAULT '0' NOT NULL , 
change `maxpower2` `maxpower2` int (10) DEFAULT '0' NOT NULL , 
change `maxpower3` `maxpower3` int (10) DEFAULT '0' NOT NULL , 
change `maxpower4` `maxpower4` int (10) DEFAULT '0' NOT NULL , 
change `maxpower5` `maxpower5` int (10) DEFAULT '0' NOT NULL;