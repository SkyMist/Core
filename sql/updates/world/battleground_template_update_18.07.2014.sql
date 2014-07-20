delete from battleground_template;
insert  into `battleground_template`(`id`,`MinPlayersPerTeam`,`MaxPlayersPerTeam`,`MinLvl`,`MaxLvl`,`AllianceStartLoc`,`AllianceStartO`,`HordeStartLoc`,`HordeStartO`,`StartMaxDist`,`Weight`,`ScriptName`,`Comment`) values 
(1,20,40,45,90,611,3.16312,610,0.715504,100,1,'','Alterac Valley'),
(2,5,10,10,90,769,3.11803,770,0.151581,75,1,'','Warsong Gulch'),
(3,8,15,10,90,890,3.89215,889,0.813671,75,1,'','Arathi Basin'),
(4,0,5,70,90,929,0,936,0,0,1,'','Nagrand Arena'),
(5,0,5,70,90,939,0,940,0,0,1,'','Blades\'s Edge Arena'),
(6,0,5,70,90,0,0,0,0,0,1,'','All Arena'),
(7,8,15,35,90,1103,3.11762,1104,0.055761,75,1,'','Eye of The Storm'),
(8,0,5,70,90,1258,0,1259,0,0,1,'','Ruins of Lordaeron'),
(9,8,15,65,90,1367,0,1368,0,0,1,'','Strand of the Ancients'),
(10,0,5,80,90,1362,0,1363,0,0,1,'','Dalaran Sewers'),
(11,0,5,80,90,1365,5.82783,1364,2.78888,0,1,'','The Ring of Valor'),
(30,20,40,65,90,1485,6.27926,1486,3.16124,200,1,'','Isle of Conquest'),
(32,5,15,45,90,0,0,0,0,0,1,'','Random battleground'),
(100,5,10,70,90,0,0,0,0,0,1,'','Rated Battleground 10x10'),
(108,5,10,85,90,1726,2.57218,1727,6.16538,0,1,'','Twin Peaks'),
(120,5,10,85,90,1798,5.95725,1799,1.55116,0,1,'','The Battle for Gilneas'),
(699,5,10,90,90,4059,0,4060,0,0,1,'','Temple of Kotmogu'),
(708,5,10,90,90,4062,0,4061,0,0,1,'','Silvershard Mines'),
(719,0,5,90,90,4136,0,4137,0,0,1,'','Tol\'vir Arena'),
(754,8,15,90,90,4487,0,4486,0,0,1,'','Deepwind Gorge'),
(757,0,5,90,90,4534,0,4535,0,0,1,'','Tiger\'s Peak');

insert into instance_template values
(617, 0, '', 0),
(618, 0, '', 0),
(726, 0, '', 1),
(761, 0, '', 1),
(998, 0, '', 1),
(1010, 0, '', 1),
(727, 0, '', 1),
(980, 0, '', 0),
(1105, 0, '', 1),
(1134, 0, '', 0);

/*
// DG

// horde wagon DG
-91.648 791.245 133.8193 5.605
// alliance wagon DG
-241.697 208.404 133.8193 1.364

// center mine
-165.85 499.967 92.8453 1.13
// Pandaren mine
65.961 431.756 111.878 4.42
// goblin mine
-399.714 573.111 111.5109 1.608

// horde graveyard spirit
-221.6748 805.479 137.45197 5.208
//goblin mine graveyard spirit
-333.910 242.645 132.5693 6.148
//pandaren mine graveyard spirit
-0.868 757.698 132.5693 3.594
//alliance graveyard spirit
-111.285 193.291 137.4516 1.966

*/
