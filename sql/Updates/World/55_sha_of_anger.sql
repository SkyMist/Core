delete from creature_text where entry = 60491;
insert into creature_text values
(60491, 0, 0, "Give in to your anger.", 14, 0, 20, 0, 0, 29005, 'Sha of Anger'),
(60491, 0, 1, "Your rage gives you strength!", 14, 0, 20, 0, 0, 29006, 'Sha of Anger'),
(60491, 0, 2, "Your rage sustains me!", 14, 0, 20, 0, 0, 29007, 'Sha of Anger'),
(60491, 0, 3, "You will not bury me again!", 14, 0, 20, 0, 0, 29008, 'Sha of Anger'),
(60491, 0, 4, "My wrath flows freely!", 14, 0, 20, 0, 0, 29009, 'Sha of Anger'),
(60491, 1, 0, "Yes, YES! Bring your rage to bear! Try to strike me down!", 14, 0, 100, 0, 0, 28999, 'Sha of Anger'),
(60491, 2, 0, "Extinguished!", 14, 0, 25, 0, 0, 29001, 'Sha of Anger'),
(60491, 2, 1, "Does that make you angry?", 14, 0, 25, 0, 0, 29002, 'Sha of Anger'),
(60491, 2, 2, "Feel your rage!", 14, 0, 25, 0, 0, 29003, 'Sha of Anger'),
(60491, 2, 3, "Let your rage consume you.", 14, 0, 25, 0, 0, 29004, 'Sha of Anger'),
(60491, 3, 0, "Feed me with your ANGER!", 14, 0, 100, 0, 0, 29010, 'Sha of Anger'),
(60491, 4, 0, "My fury is UNLEASHED!", 14, 0, 100, 0, 0, 29011, 'Sha of Anger');

delete from spell_script_names where spell_id in (119592, 119622, 119626, 119489, 129356);
insert into spell_script_names values
(119592, 'spell_sha_of_anger_endless_rage'),
(119622, 'spell_sha_of_anger_growing_anger'),
(119626, 'spell_sha_of_anger_aggressive_behaviour'),
(119489, 'spell_sha_of_anger_unleashed_wrath'),
(129356, 'spell_sha_of_anger_overcome_by_anger');