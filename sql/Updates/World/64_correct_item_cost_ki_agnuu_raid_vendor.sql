DELETE FROM npc_vendor WHERE entry = 74012;
INSERT INTO npc_vendor
SELECT 74012, slot, item, maxcount, incrtime, ExtendedCost, type
FROM npc_vendor WHERE entry = 74021;
