/*
 * Copyright (C) 2011-2014 Project SkyFire <http://www.projectskyfire.org/>
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2014 MaNGOS <http://getmangos.com/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptLoader.h"

// Examples
void AddSC_example_creature();
void AddSC_example_escort();
void AddSC_example_gossip_codebox();
void AddSC_example_misc();
void AddSC_example_commandscript();

// Spells
void AddSC_deathknight_spell_scripts();
void AddSC_druid_spell_scripts();
void AddSC_generic_spell_scripts();
void AddSC_hunter_spell_scripts();
void AddSC_mage_spell_scripts();
void AddSC_paladin_spell_scripts();
void AddSC_priest_spell_scripts();
void AddSC_rogue_spell_scripts();
void AddSC_shaman_spell_scripts();
void AddSC_warlock_spell_scripts();
void AddSC_warrior_spell_scripts();
void AddSC_quest_spell_scripts();
void AddSC_item_spell_scripts();
void AddSC_example_spell_scripts();
void AddSC_holiday_spell_scripts();

// Smart Scripts
void AddSC_SmartScripts();

// Commands
void AddSC_account_commandscript();
void AddSC_achievement_commandscript();
void AddSC_arena_commandscript();
void AddSC_ban_commandscript();
void AddSC_bf_commandscript();
void AddSC_cast_commandscript();
void AddSC_character_commandscript();
void AddSC_cheat_commandscript();
void AddSC_debug_commandscript();
void AddSC_deserter_commandscript();
void AddSC_disable_commandscript();
void AddSC_event_commandscript();
void AddSC_gm_commandscript();
void AddSC_go_commandscript();
void AddSC_gobject_commandscript();
void AddSC_group_commandscript();
void AddSC_guild_commandscript();
void AddSC_honor_commandscript();
void AddSC_instance_commandscript();
void AddSC_learn_commandscript();
void AddSC_lfg_commandscript();
void AddSC_list_commandscript();
void AddSC_lookup_commandscript();
void AddSC_message_commandscript();
void AddSC_misc_commandscript();
void AddSC_mmaps_commandscript();
void AddSC_modify_commandscript();
void AddSC_npc_commandscript();
void AddSC_pet_commandscript();
void AddSC_quest_commandscript();
void AddSC_rbac_commandscript();
void AddSC_reload_commandscript();
void AddSC_reset_commandscript();
void AddSC_send_commandscript();
void AddSC_server_commandscript();
void AddSC_tele_commandscript();
void AddSC_ticket_commandscript();
void AddSC_titles_commandscript();
void AddSC_wp_commandscript();

#ifdef SCRIPTS
// World (general)
void AddSC_areatrigger_scripts();
void AddSC_emerald_dragons();
void AddSC_generic_creature();
void AddSC_go_scripts();
void AddSC_guards();
void AddSC_item_scripts();
void AddSC_npc_professions();
void AddSC_npc_innkeeper();
void AddSC_npcs_special();
void AddSC_npc_taxi();
void AddSC_achievement_scripts();

/*** Eastern Kingdoms ***/

// Alterac Valley
void AddSC_alterac_valley();
void AddSC_boss_balinda();
void AddSC_boss_drekthar();
void AddSC_boss_galvangar();
void AddSC_boss_vanndar();

// Baradin Hold
void AddSC_boss_alizabal();
void AddSC_boss_occuthar();
void AddSC_boss_pit_lord_argaloth();
void AddSC_instance_baradin_hold();

// Blackrock Depths
void AddSC_blackrock_depths();
void AddSC_boss_ambassador_flamelash();
void AddSC_boss_anubshiah();
void AddSC_boss_draganthaurissan();
void AddSC_boss_general_angerforge();
void AddSC_boss_gorosh_the_dervish();
void AddSC_boss_grizzle();
void AddSC_boss_high_interrogator_gerstahn();
void AddSC_boss_magmus();
void AddSC_boss_moira_bronzebeard();
void AddSC_boss_tomb_of_seven();
void AddSC_instance_blackrock_depths();

// Blackrock Spire
void AddSC_boss_drakkisath();
void AddSC_boss_halycon();
void AddSC_boss_highlordomokk();
void AddSC_boss_mothersmolderweb();
void AddSC_boss_overlordwyrmthalak();
void AddSC_boss_shadowvosh();
void AddSC_boss_thebeast();
void AddSC_boss_warmastervoone();
void AddSC_boss_quatermasterzigris();
void AddSC_boss_pyroguard_emberseer();
void AddSC_boss_gyth();
void AddSC_boss_rend_blackhand();
void AddSC_boss_gizrul_the_slavener();
void AddSC_boss_urok_doomhowl();
void AddSC_boss_lord_valthalak();
void AddSC_instance_blackrock_spire();

// Blackwing lair
void AddSC_boss_razorgore();
void AddSC_boss_vaelastrasz();
void AddSC_boss_broodlord();
void AddSC_boss_firemaw();
void AddSC_boss_ebonroc();
void AddSC_boss_flamegor();
void AddSC_boss_chromaggus();
void AddSC_boss_nefarian();
void AddSC_instance_blackwing_lair();

// Deadmines
void AddSC_deadmines();
void AddSC_instance_deadmines();
void AddSC_boss_mr_smite();

// Gnomeregan
void AddSC_gnomeregan();
void AddSC_instance_gnomeregan();

// Karazhan
void AddSC_boss_attumen();
void AddSC_boss_curator();
void AddSC_boss_maiden_of_virtue();
void AddSC_boss_shade_of_aran();
void AddSC_boss_malchezaar();
void AddSC_boss_terestian_illhoof();
void AddSC_boss_moroes();
void AddSC_bosses_opera();
void AddSC_boss_netherspite();
void AddSC_instance_karazhan();
void AddSC_karazhan();
void AddSC_boss_nightbane();

// Magister's Terrace
void AddSC_boss_felblood_kaelthas();
void AddSC_boss_selin_fireheart();
void AddSC_boss_vexallus();
void AddSC_boss_priestess_delrissa();
void AddSC_instance_magisters_terrace();
void AddSC_magisters_terrace();

// Molten core
void AddSC_boss_lucifron();
void AddSC_boss_magmadar();
void AddSC_boss_gehennas();
void AddSC_boss_garr();
void AddSC_boss_baron_geddon();
void AddSC_boss_shazzrah();
void AddSC_boss_golemagg();
void AddSC_boss_sulfuron();
void AddSC_boss_majordomo();
void AddSC_boss_ragnaros();
void AddSC_instance_molten_core();

// Ragefire Chasm
void AddSC_instance_ragefire_chasm();

// Scarlet Monastery
void AddSC_boss_arcanist_doan();
void AddSC_boss_azshir_the_sleepless();
void AddSC_boss_bloodmage_thalnos();
void AddSC_boss_headless_horseman();
void AddSC_boss_herod();
void AddSC_boss_high_inquisitor_fairbanks();
void AddSC_boss_houndmaster_loksey();
void AddSC_boss_interrogator_vishas();
void AddSC_boss_scorn();
void AddSC_instance_scarlet_monastery();
void AddSC_boss_mograine_and_whitemane();

// Scholomance
void AddSC_boss_darkmaster_gandling();
void AddSC_boss_death_knight_darkreaver();
void AddSC_boss_theolenkrastinov();
void AddSC_boss_illuciabarov();
void AddSC_boss_instructormalicia();
void AddSC_boss_jandicebarov();
void AddSC_boss_kormok();
void AddSC_boss_lordalexeibarov();
void AddSC_boss_lorekeeperpolkelt();
void AddSC_boss_rasfrost();
void AddSC_boss_theravenian();
void AddSC_boss_vectus();
void AddSC_boss_kirtonos_the_herald();
void AddSC_instance_scholomance();

// Shadowfang keep
void AddSC_shadowfang_keep();
void AddSC_instance_shadowfang_keep();

// Stratholme
void AddSC_boss_magistrate_barthilas();
void AddSC_boss_maleki_the_pallid();
void AddSC_boss_nerubenkan();
void AddSC_boss_cannon_master_willey();
void AddSC_boss_baroness_anastari();
void AddSC_boss_ramstein_the_gorger();
void AddSC_boss_timmy_the_cruel();
void AddSC_boss_postmaster_malown();
void AddSC_boss_baron_rivendare();
void AddSC_boss_dathrohan_balnazzar();
void AddSC_boss_order_of_silver_hand();
void AddSC_instance_stratholme();
void AddSC_stratholme();

// Sunken Temple
void AddSC_sunken_temple();
void AddSC_instance_sunken_temple();

// Sunwell Plateau
void AddSC_instance_sunwell_plateau();
void AddSC_boss_kalecgos();
void AddSC_boss_brutallus();
void AddSC_boss_felmyst();
void AddSC_boss_eredar_twins();
void AddSC_boss_muru();
void AddSC_boss_kiljaeden();
void AddSC_sunwell_plateau();

// Uldaman
void AddSC_boss_archaedas();
void AddSC_boss_ironaya();
void AddSC_uldaman();
void AddSC_instance_uldaman();

// The Stockade
void AddSC_instance_the_stockade();

// Cataclysm

// Zul'Aman
void AddSC_boss_akilzon();
void AddSC_boss_halazzi();
void AddSC_boss_hex_lord_malacrass();
void AddSC_boss_janalai();
void AddSC_boss_nalorakk();
void AddSC_boss_daakara();
void AddSC_instance_zulaman();
void AddSC_zulaman();

// Zul'Gurub
void AddSC_boss_grilek();
void AddSC_boss_hazzarah();
void AddSC_boss_jindo_the_godbreaker();
void AddSC_boss_kilnara();
void AddSC_boss_mandokir();
void AddSC_boss_renataki();
void AddSC_boss_venoxis();
void AddSC_boss_wushoolay();
void AddSC_boss_zanzil();
void AddSC_instance_zulgurub();

// Quest Zones

// Scarlet Enclave - DK start
void AddSC_the_scarlet_enclave();            
void AddSC_the_scarlet_enclave_c1();
void AddSC_the_scarlet_enclave_c2();
void AddSC_the_scarlet_enclave_c5();

//void AddSC_alterac_mountains();
void AddSC_arathi_highlands();
void AddSC_blasted_lands();
void AddSC_burning_steppes();
void AddSC_duskwood();
void AddSC_eastern_plaguelands();
void AddSC_elwynn_forest();
void AddSC_eversong_woods();
void AddSC_ghostlands();
void AddSC_hinterlands();
void AddSC_ironforge();
void AddSC_isle_of_queldanas();
void AddSC_loch_modan();
void AddSC_redridge_mountains();
void AddSC_silverpine_forest();
void AddSC_stormwind_city();
void AddSC_stranglethorn_vale();
void AddSC_swamp_of_sorrows();
void AddSC_tirisfal_glades();
void AddSC_undercity();
void AddSC_western_plaguelands();
void AddSC_westfall();
void AddSC_wetlands();

/*** Kalimdor ***/

// Blackfathom Depths
void AddSC_blackfathom_deeps();
void AddSC_boss_gelihast();
void AddSC_boss_kelris();
void AddSC_boss_aku_mai();
void AddSC_instance_blackfathom_deeps();

// CoT: Battle for Mt. Hyjal
void AddSC_hyjal();
void AddSC_boss_archimonde();
void AddSC_instance_mount_hyjal();
void AddSC_hyjal_trash();
void AddSC_boss_rage_winterchill();
void AddSC_boss_anetheron();
void AddSC_boss_kazrogal();
void AddSC_boss_azgalor();

// CoT: Old Hillsbrad
void AddSC_boss_captain_skarloc();
void AddSC_boss_epoch_hunter();
void AddSC_boss_lieutenant_drake();
void AddSC_instance_old_hillsbrad();
void AddSC_old_hillsbrad();

// CoT: The Black Morass
void AddSC_boss_aeonus();
void AddSC_boss_chrono_lord_deja();
void AddSC_boss_temporus();
void AddSC_the_black_morass();
void AddSC_instance_the_black_morass();

// CoT: Culling Of Stratholme
void AddSC_boss_epoch();
void AddSC_boss_infinite_corruptor();
void AddSC_boss_salramm();
void AddSC_boss_mal_ganis();
void AddSC_boss_meathook();
void AddSC_culling_of_stratholme();
void AddSC_instance_culling_of_stratholme();

// Dire Maul
void AddSC_instance_dire_maul();

// Maraudon
void AddSC_boss_celebras_the_cursed();
void AddSC_boss_landslide();
void AddSC_boss_noxxion();
void AddSC_boss_ptheradras();
void AddSC_instance_maraudon();

// Onyxia's Lair
void AddSC_boss_onyxia();
void AddSC_instance_onyxias_lair();

// Razorfen Downs
void AddSC_boss_amnennar_the_coldbringer();
void AddSC_razorfen_downs();
void AddSC_instance_razorfen_downs();

// Razorfen Kraul
void AddSC_razorfen_kraul();
void AddSC_instance_razorfen_kraul();

// Ruins of Ahn'Qiraj
void AddSC_boss_kurinnaxx();
void AddSC_boss_rajaxx();
void AddSC_boss_moam();
void AddSC_boss_buru();
void AddSC_boss_ayamiss();
void AddSC_boss_ossirian();
void AddSC_instance_ruins_of_ahnqiraj();

// Temple of Ahn'Qiraj
void AddSC_boss_cthun();
void AddSC_boss_viscidus();
void AddSC_boss_fankriss();
void AddSC_boss_huhuran();
void AddSC_bug_trio();
void AddSC_boss_sartura();
void AddSC_boss_skeram();
void AddSC_boss_twinemperors();
void AddSC_boss_ouro();
void AddSC_npc_anubisath_sentinel();
void AddSC_instance_temple_of_ahnqiraj();

// Wailing caverns
void AddSC_wailing_caverns();
void AddSC_instance_wailing_caverns();

// Zul'Farrak
void AddSC_boss_zum_rah();
void AddSC_zulfarrak();
void AddSC_instance_zulfarrak();

// Cataclysm

// Halls of Origination
void AddSC_instance_halls_of_origination();
void AddSC_boss_temple_guardian_anhuur();
void AddSC_boss_earthrager_ptah();
void AddSC_boss_anraphet();

// Firelands
void AddSC_instance_firelands();
void AddSC_boss_alysrazor();

// Quest Zones
void AddSC_ashenvale();
void AddSC_azshara();
void AddSC_azuremyst_isle();
void AddSC_bloodmyst_isle();
void AddSC_darkshore();
void AddSC_desolace();
void AddSC_durotar();
void AddSC_dustwallow_marsh();
void AddSC_felwood();
void AddSC_feralas();
void AddSC_moonglade();
void AddSC_mulgore();
void AddSC_orgrimmar();
void AddSC_silithus();
void AddSC_stonetalon_mountains();
void AddSC_tanaris();
void AddSC_teldrassil();
void AddSC_the_barrens();
void AddSC_thousand_needles();
void AddSC_thunder_bluff();
void AddSC_ungoro_crater();
void AddSC_winterspring();

/*** Outland ***/

// Auchindoun - Auchenai Crypts
void AddSC_boss_shirrak_the_dead_watcher();
void AddSC_boss_exarch_maladaar();
void AddSC_instance_auchenai_crypts();

// Auchindoun - Mana Tombs
void AddSC_boss_pandemonius();
void AddSC_boss_nexusprince_shaffar();
void AddSC_instance_mana_tombs();

// Auchindoun - Sekketh Halls
void AddSC_boss_darkweaver_syth();
void AddSC_boss_talon_king_ikiss();
void AddSC_boss_anzu();
void AddSC_instance_sethekk_halls();

// Auchindoun - Shadow Labyrinth
void AddSC_boss_ambassador_hellmaw();
void AddSC_boss_blackheart_the_inciter();
void AddSC_boss_grandmaster_vorpil();
void AddSC_boss_murmur();
void AddSC_instance_shadow_labyrinth();

// Black Temple
void AddSC_black_temple();
void AddSC_boss_illidan();
void AddSC_boss_shade_of_akama();
void AddSC_boss_supremus();
void AddSC_boss_gurtogg_bloodboil();
void AddSC_boss_mother_shahraz();
void AddSC_boss_reliquary_of_souls();
void AddSC_boss_teron_gorefiend();
void AddSC_boss_najentus();
void AddSC_boss_illidari_council();
void AddSC_instance_black_temple();

// CR Serpent Shrine Cavern
void AddSC_boss_fathomlord_karathress();
void AddSC_boss_hydross_the_unstable();
void AddSC_boss_lady_vashj();
void AddSC_boss_leotheras_the_blind();
void AddSC_boss_morogrim_tidewalker();
void AddSC_instance_serpentshrine_cavern();
void AddSC_boss_the_lurker_below();

// CR Steam Vault
void AddSC_boss_hydromancer_thespia();
void AddSC_boss_mekgineer_steamrigger();
void AddSC_boss_warlord_kalithresh();
void AddSC_instance_steam_vault();

// CR The Slave Pens
void AddSC_instance_the_slave_pens();

// CR Underbog
void AddSC_boss_hungarfen();
void AddSC_boss_the_black_stalker();
void AddSC_instance_the_underbog();

// Gruul's Lair
void AddSC_boss_gruul();
void AddSC_boss_high_king_maulgar();
void AddSC_instance_gruuls_lair();

// HC Blood Furnace
void AddSC_boss_broggok();
void AddSC_boss_kelidan_the_breaker();
void AddSC_boss_the_maker();
void AddSC_instance_blood_furnace();

// HC Magtheridon's Lair
void AddSC_boss_magtheridon();
void AddSC_instance_magtheridons_lair();

// HC Shattered Halls
void AddSC_boss_grand_warlock_nethekurse();
void AddSC_boss_warbringer_omrogg();
void AddSC_boss_warchief_kargath_bladefist();
void AddSC_instance_shattered_halls();

// HC Ramparts
void AddSC_boss_watchkeeper_gargolmar();
void AddSC_boss_omor_the_unscarred();
void AddSC_boss_vazruden_the_herald();
void AddSC_instance_ramparts();

// TK Arcatraz
void AddSC_arcatraz();
void AddSC_boss_harbinger_skyriss();
void AddSC_instance_arcatraz();

// TK Botanica
void AddSC_boss_high_botanist_freywinn();
void AddSC_boss_laj();
void AddSC_boss_warp_splinter();
void AddSC_boss_thorngrin_the_tender();
void AddSC_boss_commander_sarannis();
void AddSC_instance_the_botanica();

// TK The Eye
void AddSC_boss_alar();
void AddSC_boss_kaelthas();
void AddSC_boss_void_reaver();
void AddSC_boss_high_astromancer_solarian();
void AddSC_instance_the_eye();
void AddSC_the_eye();

// TK The Mechanar
void AddSC_boss_gatewatcher_iron_hand();
void AddSC_boss_gatewatcher_gyrokill();
void AddSC_boss_nethermancer_sepethrea();
void AddSC_boss_pathaleon_the_calculator();
void AddSC_boss_mechano_lord_capacitus();
void AddSC_instance_mechanar();

// Quest Zones
void AddSC_blades_edge_mountains();
void AddSC_boss_doomlordkazzak();
void AddSC_boss_doomwalker();
void AddSC_hellfire_peninsula();
void AddSC_nagrand();
void AddSC_netherstorm();
void AddSC_shadowmoon_valley();
void AddSC_shattrath_city();
void AddSC_terokkar_forest();
void AddSC_zangarmarsh();

/*** Northrend ***/

// Gundrak
void AddSC_boss_slad_ran();
void AddSC_boss_moorabi();
void AddSC_boss_drakkari_colossus();
void AddSC_boss_gal_darah();
void AddSC_boss_eck();
void AddSC_instance_gundrak();

// Azjol-Nerub: Azjol-Nerub
void AddSC_boss_krik_thir();
void AddSC_boss_hadronox();
void AddSC_boss_anub_arak();
void AddSC_instance_azjol_nerub();

// Azjol-Nerub: Ahn'kahet
void AddSC_boss_elder_nadox();
void AddSC_boss_taldaram();
void AddSC_boss_amanitar();
void AddSC_boss_jedoga_shadowseeker();
void AddSC_boss_volazj();
void AddSC_instance_ahnkahet();

// Drak'Tharon Keep
void AddSC_boss_trollgore();
void AddSC_boss_novos();
void AddSC_boss_king_dred();
void AddSC_boss_tharon_ja();
void AddSC_instance_drak_tharon_keep();

// TOC: Trial of the Champion
void AddSC_boss_argent_challenge();
void AddSC_boss_black_knight();
void AddSC_boss_grand_champions();
void AddSC_instance_trial_of_the_champion();
void AddSC_trial_of_the_champion();

// TOC: Trial of the Crusader
void AddSC_boss_anubarak_trial();
void AddSC_boss_faction_champions();
void AddSC_boss_jaraxxus();
void AddSC_boss_northrend_beasts();
void AddSC_boss_twin_valkyr();
void AddSC_trial_of_the_crusader();
void AddSC_instance_trial_of_the_crusader();

// Naxxramas
void AddSC_boss_anubrekhan();
void AddSC_boss_maexxna();
void AddSC_boss_patchwerk();
void AddSC_boss_grobbulus();
void AddSC_boss_razuvious();
void AddSC_boss_kelthuzad();
void AddSC_boss_loatheb();
void AddSC_boss_noth();
void AddSC_boss_gluth();
void AddSC_boss_sapphiron();
void AddSC_boss_four_horsemen();
void AddSC_boss_faerlina();
void AddSC_boss_heigan();
void AddSC_boss_gothik();
void AddSC_boss_thaddius();
void AddSC_instance_naxxramas();

// The Nexus: Nexus
void AddSC_boss_magus_telestra();
void AddSC_boss_anomalus();
void AddSC_boss_ormorok();
void AddSC_boss_keristrasza();
void AddSC_instance_nexus();

// The Nexus: The Oculus
void AddSC_boss_drakos();
void AddSC_boss_urom();
void AddSC_boss_varos();
void AddSC_boss_eregos();
void AddSC_instance_oculus();
void AddSC_oculus();

// The Nexus: Eye of Eternity
void AddSC_boss_malygos();
void AddSC_instance_eye_of_eternity();

// Obsidian Sanctum
void AddSC_boss_sartharion();
void AddSC_instance_obsidian_sanctum();

// Ulduar: Halls of Lightning
void AddSC_boss_bjarngrim();
void AddSC_boss_loken();
void AddSC_boss_ionar();
void AddSC_boss_volkhan();
void AddSC_instance_halls_of_lightning();

// Ulduar: Halls of Stone
void AddSC_boss_maiden_of_grief();
void AddSC_boss_krystallus();
void AddSC_boss_sjonnir();
void AddSC_instance_halls_of_stone();
void AddSC_halls_of_stone();

// Ulduar: Ulduar
void AddSC_boss_auriaya();
void AddSC_boss_flame_leviathan();
void AddSC_boss_ignis();
void AddSC_boss_razorscale();
void AddSC_boss_xt002();
void AddSC_boss_kologarn();
void AddSC_boss_assembly_of_iron();
void AddSC_boss_general_vezax();
void AddSC_ulduar_teleporter();
void AddSC_boss_mimiron();
void AddSC_boss_hodir();
void AddSC_boss_freya();
void AddSC_boss_yogg_saron();
void AddSC_boss_algalon_the_observer();
void AddSC_instance_ulduar();

// Utgarde Keep - Utgarde Keep
void AddSC_boss_keleseth();
void AddSC_boss_skarvald_dalronn();
void AddSC_boss_ingvar_the_plunderer();
void AddSC_instance_utgarde_keep();
void AddSC_utgarde_keep();

// Utgarde Keep - Utgarde Pinnacle
void AddSC_boss_svala();
void AddSC_boss_palehoof();
void AddSC_boss_skadi();
void AddSC_boss_ymiron();
void AddSC_instance_utgarde_pinnacle();

// Vault of Archavon
void AddSC_boss_archavon();
void AddSC_boss_emalon();
void AddSC_boss_koralon();
void AddSC_boss_toravon();
void AddSC_instance_vault_of_archavon();

// Violet Hold
void AddSC_boss_cyanigosa();
void AddSC_boss_erekem();
void AddSC_boss_ichoron();
void AddSC_boss_lavanthor();
void AddSC_boss_moragg();
void AddSC_boss_xevozz();
void AddSC_boss_zuramat();
void AddSC_instance_violet_hold();
void AddSC_violet_hold();

// FH: Forge of Souls
void AddSC_instance_forge_of_souls();
void AddSC_forge_of_souls();
void AddSC_boss_bronjahm();
void AddSC_boss_devourer_of_souls();

// FH: Pit of Saron
void AddSC_instance_pit_of_saron();
void AddSC_pit_of_saron();
void AddSC_boss_garfrost();
void AddSC_boss_ick();
void AddSC_boss_tyrannus();

// FH: Halls of Reflection
void AddSC_instance_halls_of_reflection();
void AddSC_halls_of_reflection();
void AddSC_boss_falric();
void AddSC_boss_marwyn();

// Icecrown Citadel
void AddSC_boss_lord_marrowgar();
void AddSC_boss_lady_deathwhisper();
void AddSC_boss_deathbringer_saurfang();
void AddSC_boss_festergut();
void AddSC_boss_rotface();
void AddSC_boss_professor_putricide();
void AddSC_boss_blood_prince_council();
void AddSC_boss_blood_queen_lana_thel();
void AddSC_boss_valithria_dreamwalker();
void AddSC_boss_sindragosa();
void AddSC_boss_the_lich_king();
void AddSC_icecrown_citadel_teleport();
void AddSC_instance_icecrown_citadel();
void AddSC_icecrown_citadel();

// Ruby Sanctum
void AddSC_instance_ruby_sanctum();
void AddSC_ruby_sanctum();
void AddSC_boss_baltharus_the_warborn();
void AddSC_boss_saviana_ragefire();
void AddSC_boss_general_zarithrian();
void AddSC_boss_halion();

// Quest Zones
void AddSC_dalaran();
void AddSC_borean_tundra();
void AddSC_dragonblight();
void AddSC_grizzly_hills();
void AddSC_howling_fjord();
void AddSC_icecrown();
void AddSC_sholazar_basin();
void AddSC_storm_peaks();
void AddSC_wintergrasp();
void AddSC_zuldrak();
void AddSC_crystalsong_forest();
void AddSC_isle_of_conquest();

/*** Maelstrom ***/

// Quest Zones
void AddSC_kezan();

/*** Pandaria ***/

// Terrace of Endless Spring
void AddSC_boss_protectors_of_the_endless();
void AddSC_boss_tsulong();
void AddSC_boss_lei_shi();
void AddSC_boss_sha_of_fear();
void AddSC_terrace_of_endless_spring();
void AddSC_instance_terrace_of_endless_spring();

// Events
void AddSC_event_childrens_week();

// Pets
void AddSC_deathknight_pet_scripts();
void AddSC_generic_pet_scripts();
void AddSC_hunter_pet_scripts();
void AddSC_mage_pet_scripts();
void AddSC_priest_pet_scripts();
void AddSC_shaman_pet_scripts();

// Battlegrounds

// Outdoor PVP
void AddSC_outdoorpvp_hp();
void AddSC_outdoorpvp_na();
void AddSC_outdoorpvp_si();
void AddSC_outdoorpvp_tf();
void AddSC_outdoorpvp_zm();

// Player
void AddSC_chat_log();

#endif

// Call addition of all script types
void AddScripts()
{
    AddExampleScripts();
    AddSpellScripts();
    AddSC_SmartScripts();
    AddCommandScripts();
#ifdef SCRIPTS
    AddWorldScripts();
    AddEasternKingdomsScripts();
    AddKalimdorScripts();
    AddOutlandScripts();
    AddNorthrendScripts();
    AddMaelstromScripts();
    AddPandariaScripts();
    AddEventScripts();
    AddPetScripts();
    AddBattlegroundScripts();
    AddOutdoorPvPScripts();
    AddCustomScripts();
#endif
}

// Examples
void AddExampleScripts()
{
    AddSC_example_creature();
    AddSC_example_escort();
    AddSC_example_gossip_codebox();
    AddSC_example_misc();
    AddSC_example_commandscript();
}

// Spells
void AddSpellScripts()
{
    AddSC_deathknight_spell_scripts();
    AddSC_druid_spell_scripts();
    AddSC_generic_spell_scripts();
    AddSC_hunter_spell_scripts();
    AddSC_mage_spell_scripts();
    AddSC_paladin_spell_scripts();
    AddSC_priest_spell_scripts();
    AddSC_rogue_spell_scripts();
    AddSC_shaman_spell_scripts();
    AddSC_warlock_spell_scripts();
    AddSC_warrior_spell_scripts();
    AddSC_quest_spell_scripts();
    AddSC_item_spell_scripts();
    AddSC_example_spell_scripts();
    AddSC_holiday_spell_scripts();
}

// Commands
void AddCommandScripts()
{
    AddSC_account_commandscript();
    AddSC_achievement_commandscript();
    AddSC_arena_commandscript();
    AddSC_ban_commandscript();
    AddSC_bf_commandscript();
    AddSC_cast_commandscript();
    AddSC_character_commandscript();
    AddSC_cheat_commandscript();
    AddSC_debug_commandscript();
    AddSC_deserter_commandscript();
    AddSC_disable_commandscript();
    AddSC_event_commandscript();
    AddSC_gm_commandscript();
    AddSC_go_commandscript();
    AddSC_gobject_commandscript();
    AddSC_group_commandscript();
    AddSC_guild_commandscript();
    AddSC_honor_commandscript();
    AddSC_instance_commandscript();
    AddSC_learn_commandscript();
    AddSC_lookup_commandscript();
    AddSC_lfg_commandscript();
    AddSC_list_commandscript();
    AddSC_message_commandscript();
    AddSC_misc_commandscript();
    AddSC_mmaps_commandscript();
    AddSC_modify_commandscript();
    AddSC_npc_commandscript();
    AddSC_quest_commandscript();
    AddSC_pet_commandscript();
    AddSC_rbac_commandscript();
    AddSC_reload_commandscript();
    AddSC_reset_commandscript();
    AddSC_send_commandscript();
    AddSC_server_commandscript();
    AddSC_tele_commandscript();
    AddSC_ticket_commandscript();
    AddSC_titles_commandscript();
    AddSC_wp_commandscript();
}

// World (general)
void AddWorldScripts()
{
#ifdef SCRIPTS
    AddSC_areatrigger_scripts();
    AddSC_emerald_dragons();
    AddSC_generic_creature();
    AddSC_go_scripts();
    AddSC_guards();
    AddSC_item_scripts();
    AddSC_npc_professions();
    AddSC_npc_innkeeper();
    AddSC_npcs_special();
    AddSC_npc_taxi();
    AddSC_achievement_scripts();
    AddSC_chat_log();
#endif
}

// Eastern Kingdoms
void AddEasternKingdomsScripts()
{
#ifdef SCRIPTS
    // Alterac Valley
    AddSC_alterac_valley();
    AddSC_boss_balinda();
    AddSC_boss_drekthar();
    AddSC_boss_galvangar();
    AddSC_boss_vanndar();

    // Baradin Hold
    AddSC_boss_alizabal();
    AddSC_boss_occuthar();
    AddSC_boss_pit_lord_argaloth();
    AddSC_instance_baradin_hold();

    // Blackrock Depths
    AddSC_blackrock_depths();
    AddSC_boss_ambassador_flamelash();
    AddSC_boss_anubshiah();
    AddSC_boss_draganthaurissan();
    AddSC_boss_general_angerforge();
    AddSC_boss_gorosh_the_dervish();
    AddSC_boss_grizzle();
    AddSC_boss_high_interrogator_gerstahn();
    AddSC_boss_magmus();
    AddSC_boss_moira_bronzebeard();
    AddSC_boss_tomb_of_seven();
    AddSC_instance_blackrock_depths();

    // Blackrock Spire
    AddSC_boss_drakkisath();
    AddSC_boss_halycon();
    AddSC_boss_highlordomokk();
    AddSC_boss_mothersmolderweb();
    AddSC_boss_overlordwyrmthalak();
    AddSC_boss_shadowvosh();
    AddSC_boss_thebeast();
    AddSC_boss_warmastervoone();
    AddSC_boss_quatermasterzigris();
    AddSC_boss_pyroguard_emberseer();
    AddSC_boss_gyth();
    AddSC_boss_rend_blackhand();
    AddSC_boss_gizrul_the_slavener();
    AddSC_boss_urok_doomhowl();
    AddSC_boss_lord_valthalak();
    AddSC_instance_blackrock_spire();

    // Blackwing lair
    AddSC_boss_razorgore();
    AddSC_boss_vaelastrasz();
    AddSC_boss_broodlord();
    AddSC_boss_firemaw();
    AddSC_boss_ebonroc();
    AddSC_boss_flamegor();
    AddSC_boss_chromaggus();
    AddSC_boss_nefarian();
    AddSC_instance_blackwing_lair();

    // Deadmines
    AddSC_deadmines();
    AddSC_boss_mr_smite();
    AddSC_instance_deadmines();

    // Gnomeregan
    AddSC_gnomeregan();
    AddSC_instance_gnomeregan();

    // Karazhan
    AddSC_boss_attumen();
    AddSC_boss_curator();
    AddSC_boss_maiden_of_virtue();
    AddSC_boss_shade_of_aran();
    AddSC_boss_malchezaar();
    AddSC_boss_terestian_illhoof();
    AddSC_boss_moroes();
    AddSC_bosses_opera();
    AddSC_boss_netherspite();
    AddSC_instance_karazhan();
    AddSC_karazhan();
    AddSC_boss_nightbane();

    // Magister's Terrace
    AddSC_boss_felblood_kaelthas();
    AddSC_boss_selin_fireheart();
    AddSC_boss_vexallus();
    AddSC_boss_priestess_delrissa();
    AddSC_instance_magisters_terrace();
    AddSC_magisters_terrace();

    // Molten core
    AddSC_boss_lucifron();
    AddSC_boss_magmadar();
    AddSC_boss_gehennas();
    AddSC_boss_garr();
    AddSC_boss_baron_geddon();
    AddSC_boss_shazzrah();
    AddSC_boss_golemagg();
    AddSC_boss_sulfuron();
    AddSC_boss_majordomo();
    AddSC_boss_ragnaros();
    AddSC_instance_molten_core();

    // Ragefire Chasm
    AddSC_instance_ragefire_chasm();

    // Scarlet Monastery
    AddSC_boss_arcanist_doan();
    AddSC_boss_azshir_the_sleepless();
    AddSC_boss_bloodmage_thalnos();
    AddSC_boss_headless_horseman();
    AddSC_boss_herod();
    AddSC_boss_high_inquisitor_fairbanks();
    AddSC_boss_houndmaster_loksey();
    AddSC_boss_interrogator_vishas();
    AddSC_boss_scorn();
    AddSC_instance_scarlet_monastery();
    AddSC_boss_mograine_and_whitemane();

    // Scholomance
    AddSC_boss_darkmaster_gandling();
    AddSC_boss_death_knight_darkreaver();
    AddSC_boss_theolenkrastinov();
    AddSC_boss_illuciabarov();
    AddSC_boss_instructormalicia();
    AddSC_boss_jandicebarov();
    AddSC_boss_kormok();
    AddSC_boss_lordalexeibarov();
    AddSC_boss_lorekeeperpolkelt();
    AddSC_boss_rasfrost();
    AddSC_boss_theravenian();
    AddSC_boss_vectus();
    AddSC_boss_kirtonos_the_herald();
    AddSC_instance_scholomance();

    // Shadowfang keep
    AddSC_shadowfang_keep();
    AddSC_instance_shadowfang_keep();

    // Stratholme
    AddSC_boss_magistrate_barthilas();
    AddSC_boss_maleki_the_pallid();
    AddSC_boss_nerubenkan();
    AddSC_boss_cannon_master_willey();
    AddSC_boss_baroness_anastari();
    AddSC_boss_ramstein_the_gorger();
    AddSC_boss_timmy_the_cruel();
    AddSC_boss_postmaster_malown();
    AddSC_boss_baron_rivendare();
    AddSC_boss_dathrohan_balnazzar();
    AddSC_boss_order_of_silver_hand();
    AddSC_instance_stratholme();
    AddSC_stratholme();

    // Sunken Temple
    AddSC_sunken_temple();
    AddSC_instance_sunken_temple();

    // Sunwell Plateau
    AddSC_instance_sunwell_plateau();
    AddSC_boss_kalecgos();
    AddSC_boss_brutallus();
    AddSC_boss_felmyst();
    AddSC_boss_eredar_twins();
    AddSC_boss_muru();
    AddSC_boss_kiljaeden();
    AddSC_sunwell_plateau();

    // The Stockade
    AddSC_instance_the_stockade();

    // Uldaman
    AddSC_boss_archaedas();
    AddSC_boss_ironaya();
    AddSC_uldaman();
    AddSC_instance_uldaman();

    // Cataclysm

    // Zul'Aman
    AddSC_boss_akilzon();
    AddSC_boss_halazzi();
    AddSC_boss_hex_lord_malacrass();
    AddSC_boss_janalai();
    AddSC_boss_nalorakk();
    AddSC_boss_daakara();
    AddSC_instance_zulaman();
    AddSC_zulaman();

    // Zul'Gurub
    AddSC_boss_grilek();
    AddSC_boss_hazzarah();
    AddSC_boss_jindo_the_godbreaker();
    AddSC_boss_kilnara();
    AddSC_boss_mandokir();
    AddSC_boss_renataki();
    AddSC_boss_venoxis();
    AddSC_boss_wushoolay();
    AddSC_boss_zanzil();
    AddSC_instance_zulgurub();

    // Quest Zones

    // Scarlet Enclave - DK start
    AddSC_the_scarlet_enclave();
    AddSC_the_scarlet_enclave_c1();
    AddSC_the_scarlet_enclave_c2();
    AddSC_the_scarlet_enclave_c5();

    //AddSC_alterac_mountains();
    AddSC_arathi_highlands();
    AddSC_blasted_lands();
    AddSC_burning_steppes();
    AddSC_duskwood();
    AddSC_eastern_plaguelands();
    AddSC_elwynn_forest();
    AddSC_eversong_woods();
    AddSC_ghostlands();
    AddSC_hinterlands();
    AddSC_ironforge();
    AddSC_isle_of_queldanas();
    AddSC_loch_modan();
    AddSC_redridge_mountains();
    AddSC_silverpine_forest();
    AddSC_stormwind_city();
    AddSC_stranglethorn_vale();
    AddSC_swamp_of_sorrows();
    AddSC_tirisfal_glades();
    AddSC_undercity();
    AddSC_western_plaguelands();
    AddSC_westfall();
    AddSC_wetlands();
#endif
}

// Kalimdor
void AddKalimdorScripts()
{
#ifdef SCRIPTS
    // Blackfathom Depths
    AddSC_blackfathom_deeps();
    AddSC_boss_gelihast();
    AddSC_boss_kelris();
    AddSC_boss_aku_mai();
    AddSC_instance_blackfathom_deeps();

    // CoT Battle for Mt. Hyjal
    AddSC_hyjal();
    AddSC_boss_archimonde();
    AddSC_instance_mount_hyjal();
    AddSC_hyjal_trash();
    AddSC_boss_rage_winterchill();
    AddSC_boss_anetheron();
    AddSC_boss_kazrogal();
    AddSC_boss_azgalor();

    // CoT Old Hillsbrad
    AddSC_boss_captain_skarloc();
    AddSC_boss_epoch_hunter();
    AddSC_boss_lieutenant_drake();
    AddSC_instance_old_hillsbrad();
    AddSC_old_hillsbrad();

    // CoT The Black Morass
    AddSC_boss_aeonus();
    AddSC_boss_chrono_lord_deja();
    AddSC_boss_temporus();
    AddSC_the_black_morass();
    AddSC_instance_the_black_morass();

    // CoT Culling Of Stratholme
    AddSC_boss_epoch();
    AddSC_boss_infinite_corruptor();
    AddSC_boss_salramm();
    AddSC_boss_mal_ganis();
    AddSC_boss_meathook();
    AddSC_culling_of_stratholme();
    AddSC_instance_culling_of_stratholme();

    // Dire Maul
    AddSC_instance_dire_maul();

    // Maraudon
    AddSC_boss_celebras_the_cursed();
    AddSC_boss_landslide();
    AddSC_boss_noxxion();
    AddSC_boss_ptheradras();
    AddSC_instance_maraudon();

    // Onyxia's Lair
    AddSC_boss_onyxia();
    AddSC_instance_onyxias_lair();

    // Razorfen Downs
    AddSC_boss_amnennar_the_coldbringer();
    AddSC_razorfen_downs();
    AddSC_instance_razorfen_downs();

    // Razorfen Kraul
    AddSC_razorfen_kraul();
    AddSC_instance_razorfen_kraul();

    // Ruins of Ahn'Qiraj
    AddSC_boss_kurinnaxx();
    AddSC_boss_rajaxx();
    AddSC_boss_moam();
    AddSC_boss_buru();
    AddSC_boss_ayamiss();
    AddSC_boss_ossirian();
    AddSC_instance_ruins_of_ahnqiraj();

    // Temple of Ahn'Qiraj
    AddSC_boss_cthun();
    AddSC_boss_viscidus();
    AddSC_boss_fankriss();
    AddSC_boss_huhuran();
    AddSC_bug_trio();
    AddSC_boss_sartura();
    AddSC_boss_skeram();
    AddSC_boss_twinemperors();
    AddSC_boss_ouro();
    AddSC_npc_anubisath_sentinel();
    AddSC_instance_temple_of_ahnqiraj();

    // Wailing caverns
    AddSC_wailing_caverns();
    AddSC_instance_wailing_caverns();

    // Zul'Farrak
    AddSC_boss_zum_rah();
    AddSC_zulfarrak();
    AddSC_instance_zulfarrak();

    // Cataclysm

    // Halls of Origination
    AddSC_instance_halls_of_origination();
    AddSC_boss_temple_guardian_anhuur();
    AddSC_boss_earthrager_ptah();
    AddSC_boss_anraphet();

    // Firelands
    AddSC_instance_firelands();
    AddSC_boss_alysrazor();

    // Quest Zones
    AddSC_ashenvale();
    AddSC_azshara();
    AddSC_azuremyst_isle();
    AddSC_bloodmyst_isle();
    AddSC_darkshore();
    AddSC_desolace();
    AddSC_durotar();
    AddSC_dustwallow_marsh();
    AddSC_felwood();
    AddSC_feralas();
    AddSC_moonglade();
    AddSC_mulgore();
    AddSC_orgrimmar();
    AddSC_silithus();
    AddSC_stonetalon_mountains();
    AddSC_tanaris();
    AddSC_teldrassil();
    AddSC_the_barrens();
    AddSC_thousand_needles();
    AddSC_thunder_bluff();
    AddSC_ungoro_crater();
    AddSC_winterspring();
#endif
}

// Outland
void AddOutlandScripts()
{
#ifdef SCRIPTS
    // Auchindoun - Auchenai Crypts
    AddSC_boss_shirrak_the_dead_watcher();
    AddSC_boss_exarch_maladaar();
    AddSC_instance_auchenai_crypts();

    // Auchindoun - Mana Tombs
    AddSC_boss_pandemonius();
    AddSC_boss_nexusprince_shaffar();
    AddSC_instance_mana_tombs();

    // Auchindoun - Sekketh Halls
    AddSC_boss_darkweaver_syth();
    AddSC_boss_talon_king_ikiss();
    AddSC_boss_anzu();
    AddSC_instance_sethekk_halls();

    // Auchindoun - Shadow Labyrinth
    AddSC_boss_ambassador_hellmaw();
    AddSC_boss_blackheart_the_inciter();
    AddSC_boss_grandmaster_vorpil();
    AddSC_boss_murmur();
    AddSC_instance_shadow_labyrinth();

    // Black Temple
    AddSC_black_temple();                   
    AddSC_boss_illidan();
    AddSC_boss_shade_of_akama();
    AddSC_boss_supremus();
    AddSC_boss_gurtogg_bloodboil();
    AddSC_boss_mother_shahraz();
    AddSC_boss_reliquary_of_souls();
    AddSC_boss_teron_gorefiend();
    AddSC_boss_najentus();
    AddSC_boss_illidari_council();
    AddSC_instance_black_temple();

    // CR Serpent Shrine Cavern
    AddSC_boss_fathomlord_karathress();
    AddSC_boss_hydross_the_unstable();
    AddSC_boss_lady_vashj();
    AddSC_boss_leotheras_the_blind();
    AddSC_boss_morogrim_tidewalker();
    AddSC_instance_serpentshrine_cavern();
    AddSC_boss_the_lurker_below();

    // CR Steam Vault
    AddSC_boss_hydromancer_thespia();
    AddSC_boss_mekgineer_steamrigger();
    AddSC_boss_warlord_kalithresh();
    AddSC_instance_steam_vault();

    // CR The Slave Pens
    AddSC_instance_the_slave_pens();

    // CR Underbog
    AddSC_boss_hungarfen();
    AddSC_boss_the_black_stalker();
    AddSC_instance_the_underbog();

    // Gruul's Lair
    AddSC_boss_gruul();                     
    AddSC_boss_high_king_maulgar();
    AddSC_instance_gruuls_lair();

    // HC Blood Furnace
    AddSC_boss_broggok();
    AddSC_boss_kelidan_the_breaker();
    AddSC_boss_the_maker();
    AddSC_instance_blood_furnace();

    // HC Magtheridon's Lair
    AddSC_boss_magtheridon();
    AddSC_instance_magtheridons_lair();

    // HC Shattered Halls
    AddSC_boss_grand_warlock_nethekurse();
    AddSC_boss_warbringer_omrogg();
    AddSC_boss_warchief_kargath_bladefist();
    AddSC_instance_shattered_halls();

    // HC Ramparts
    AddSC_boss_watchkeeper_gargolmar();
    AddSC_boss_omor_the_unscarred();
    AddSC_boss_vazruden_the_herald();
    AddSC_instance_ramparts();

    // TK Arcatraz
    AddSC_arcatraz();
    AddSC_boss_harbinger_skyriss();
    AddSC_instance_arcatraz();

    // TK Botanica
    AddSC_boss_high_botanist_freywinn();
    AddSC_boss_laj();
    AddSC_boss_warp_splinter();
    AddSC_boss_thorngrin_the_tender();
    AddSC_boss_commander_sarannis();
    AddSC_instance_the_botanica();

    // TK The Eye
    AddSC_boss_alar();
    AddSC_boss_kaelthas();
    AddSC_boss_void_reaver();
    AddSC_boss_high_astromancer_solarian();
    AddSC_instance_the_eye();
    AddSC_the_eye();

    // TK The Mechanar
    AddSC_boss_gatewatcher_iron_hand();
    AddSC_boss_gatewatcher_gyrokill();
    AddSC_boss_nethermancer_sepethrea();
    AddSC_boss_pathaleon_the_calculator();
    AddSC_boss_mechano_lord_capacitus();
    AddSC_instance_mechanar();

    // Quest Zones
    AddSC_blades_edge_mountains();
    AddSC_boss_doomlordkazzak();
    AddSC_boss_doomwalker();
    AddSC_hellfire_peninsula();
    AddSC_nagrand();
    AddSC_netherstorm();
    AddSC_shadowmoon_valley();
    AddSC_shattrath_city();
    AddSC_terokkar_forest();
    AddSC_zangarmarsh();
#endif
}

// Northrend
void AddNorthrendScripts()
{
#ifdef SCRIPTS
    // Gundrak
    AddSC_boss_slad_ran();
    AddSC_boss_moorabi();
    AddSC_boss_drakkari_colossus();
    AddSC_boss_gal_darah();
    AddSC_boss_eck();
    AddSC_instance_gundrak();

    // Azjol-Nerub - Ahn'kahet
    AddSC_boss_elder_nadox();
    AddSC_boss_taldaram();
    AddSC_boss_amanitar();
    AddSC_boss_jedoga_shadowseeker();
    AddSC_boss_volazj();
    AddSC_instance_ahnkahet();

    // Azjol-Nerub - Azjol-Nerub
    AddSC_boss_krik_thir();
    AddSC_boss_hadronox();
    AddSC_boss_anub_arak();
    AddSC_instance_azjol_nerub();

    // Drak'Tharon Keep
    AddSC_boss_trollgore();
    AddSC_boss_novos();
    AddSC_boss_king_dred();
    AddSC_boss_tharon_ja();
    AddSC_instance_drak_tharon_keep();

    // Trial of the Champion
    AddSC_boss_argent_challenge();      
    AddSC_boss_black_knight();
    AddSC_boss_grand_champions();
    AddSC_instance_trial_of_the_champion();
    AddSC_trial_of_the_champion();

    // Trial of the Crusader
    AddSC_boss_anubarak_trial();
    AddSC_boss_faction_champions();
    AddSC_boss_jaraxxus();
    AddSC_trial_of_the_crusader();
    AddSC_boss_twin_valkyr();
    AddSC_boss_northrend_beasts();
    AddSC_instance_trial_of_the_crusader();

    // Naxxramas
    AddSC_boss_anubrekhan();
    AddSC_boss_maexxna();
    AddSC_boss_patchwerk();
    AddSC_boss_grobbulus();
    AddSC_boss_razuvious();
    AddSC_boss_kelthuzad();
    AddSC_boss_loatheb();
    AddSC_boss_noth();
    AddSC_boss_gluth();
    AddSC_boss_sapphiron();
    AddSC_boss_four_horsemen();
    AddSC_boss_faerlina();
    AddSC_boss_heigan();
    AddSC_boss_gothik();
    AddSC_boss_thaddius();
    AddSC_instance_naxxramas();

    // The Nexus Nexus
    AddSC_boss_magus_telestra();
    AddSC_boss_anomalus();
    AddSC_boss_ormorok();
    AddSC_boss_keristrasza();
    AddSC_instance_nexus();

    // The Nexus The Oculus
    AddSC_boss_drakos();
    AddSC_boss_urom();
    AddSC_boss_varos();
    AddSC_boss_eregos();
    AddSC_instance_oculus();
    AddSC_oculus();

    // The Nexus: Eye of Eternity
    AddSC_boss_malygos();
    AddSC_instance_eye_of_eternity();

    // Obsidian Sanctum
    AddSC_boss_sartharion();
    AddSC_instance_obsidian_sanctum();

    // Ulduar Halls of Lightning
    AddSC_boss_bjarngrim();
    AddSC_boss_loken();
    AddSC_boss_ionar();
    AddSC_boss_volkhan();
    AddSC_instance_halls_of_lightning();

    // Ulduar Halls of Stone
    AddSC_boss_maiden_of_grief();
    AddSC_boss_krystallus();
    AddSC_boss_sjonnir();
    AddSC_instance_halls_of_stone();
    AddSC_halls_of_stone();

    // Ulduar Ulduar
    AddSC_boss_auriaya();
    AddSC_boss_flame_leviathan();
    AddSC_boss_ignis();
    AddSC_boss_razorscale();
    AddSC_boss_xt002();
    AddSC_boss_general_vezax();
    AddSC_boss_assembly_of_iron();
    AddSC_boss_kologarn();
    AddSC_ulduar_teleporter();
    AddSC_boss_mimiron();
    AddSC_boss_hodir();
    AddSC_boss_freya();
    AddSC_boss_yogg_saron();
    AddSC_boss_algalon_the_observer();
    AddSC_instance_ulduar();

    // Utgarde Keep - Utgarde Keep
    AddSC_boss_keleseth();
    AddSC_boss_skarvald_dalronn();
    AddSC_boss_ingvar_the_plunderer();
    AddSC_instance_utgarde_keep();
    AddSC_utgarde_keep();

    // Utgarde Keep - Utgarde Pinnacle
    AddSC_boss_svala();
    AddSC_boss_palehoof();
    AddSC_boss_skadi();
    AddSC_boss_ymiron();
    AddSC_instance_utgarde_pinnacle();

    // Vault of Archavon
    AddSC_boss_archavon();
    AddSC_boss_emalon();
    AddSC_boss_koralon();
    AddSC_boss_toravon();
    AddSC_instance_vault_of_archavon();

    // Violet Hold
    AddSC_boss_cyanigosa();
    AddSC_boss_erekem();
    AddSC_boss_ichoron();
    AddSC_boss_lavanthor();
    AddSC_boss_moragg();
    AddSC_boss_xevozz();
    AddSC_boss_zuramat();
    AddSC_instance_violet_hold();
    AddSC_violet_hold();

    // FH Forge of Souls
    AddSC_instance_forge_of_souls();
    AddSC_forge_of_souls();
    AddSC_boss_bronjahm();
    AddSC_boss_devourer_of_souls();

    // FH Pit of Saron
    AddSC_instance_pit_of_saron();
    AddSC_pit_of_saron();
    AddSC_boss_garfrost();
    AddSC_boss_ick();
    AddSC_boss_tyrannus();

    // FH Halls of Reflection
    AddSC_instance_halls_of_reflection();
    AddSC_halls_of_reflection();
    AddSC_boss_falric();
    AddSC_boss_marwyn();

    // Icecrown Citadel
    AddSC_boss_lord_marrowgar();
    AddSC_boss_lady_deathwhisper();
    AddSC_boss_deathbringer_saurfang();
    AddSC_boss_festergut();
    AddSC_boss_rotface();
    AddSC_boss_professor_putricide();
    AddSC_boss_blood_prince_council();
    AddSC_boss_blood_queen_lana_thel();
    AddSC_boss_valithria_dreamwalker();
    AddSC_boss_sindragosa();
    AddSC_boss_the_lich_king();
    AddSC_icecrown_citadel_teleport();
    AddSC_instance_icecrown_citadel();
    AddSC_icecrown_citadel();

    // Ruby Sanctum
    AddSC_instance_ruby_sanctum();
    AddSC_ruby_sanctum();
    AddSC_boss_baltharus_the_warborn();
    AddSC_boss_saviana_ragefire();
    AddSC_boss_general_zarithrian();
    AddSC_boss_halion();

    // Quest Zones
    AddSC_dalaran();
    AddSC_borean_tundra();
    AddSC_dragonblight();
    AddSC_grizzly_hills();
    AddSC_howling_fjord();
    AddSC_icecrown();
    AddSC_sholazar_basin();
    AddSC_storm_peaks();
    AddSC_wintergrasp();
    AddSC_zuldrak();
    AddSC_crystalsong_forest();
    AddSC_isle_of_conquest();
#endif
}

// Maelstrom
void AddMaelstromScripts()
{
#ifdef SCRIPTS
    // Quest Zones
    AddSC_kezan();
#endif
}

// Pandaria
void AddPandariaScripts()
{
#ifdef SCRIPTS
    // Terrace of Endless Spring
    AddSC_boss_protectors_of_the_endless();
    AddSC_boss_tsulong();
    AddSC_boss_lei_shi();
    AddSC_boss_sha_of_fear();
    AddSC_terrace_of_endless_spring();
	AddSC_instance_terrace_of_endless_spring();
#endif
}

// Events
void AddEventScripts()
{
#ifdef SCRIPTS
    AddSC_event_childrens_week();
#endif
}

// Pets
void AddPetScripts()
{
#ifdef SCRIPTS
    AddSC_deathknight_pet_scripts();
    AddSC_generic_pet_scripts();
    AddSC_hunter_pet_scripts();
    AddSC_mage_pet_scripts();
    AddSC_priest_pet_scripts();
    AddSC_shaman_pet_scripts();
#endif
}

// Outdoor PVP
void AddOutdoorPvPScripts()
{
#ifdef SCRIPTS
    AddSC_outdoorpvp_hp();
    AddSC_outdoorpvp_na();
    AddSC_outdoorpvp_si();
    AddSC_outdoorpvp_tf();
    AddSC_outdoorpvp_zm();
#endif
}

// Battlegrounds
void AddBattlegroundScripts()
{
#ifdef SCRIPTS
#endif
}

#ifdef SCRIPTS
/* This is where custom scripts' loading functions should be declared. */

#endif

void AddCustomScripts()
{
#ifdef SCRIPTS
    /* This is where custom scripts should be added. */

#endif
}
