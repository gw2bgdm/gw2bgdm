#include "core/types.h"
#include "core/debug.h"
#include "meter/enums.h"
#include "meter/offsets.h"
#include "meter/localization.h"

#if !(defined BGDM_TOS_COMPLIANT)

const char *wep_name_from_id(u32 id)
{
	static char idstr[4];

	if (id > WEP_TYPE_NOEQUIP)
		id = WEP_TYPE_UNKNOWN;

#define STR(a,b) a ## b
#define WEAPON_LOCALTEXT(x)	case( x ): return LOCALTEXT( STR(TEXT_, x) )
	switch (id)
	{
		WEAPON_LOCALTEXT(WEP_TYPE_SWORD);
		WEAPON_LOCALTEXT(WEP_TYPE_HAMMER);
		WEAPON_LOCALTEXT(WEP_TYPE_LONGBOW);
		WEAPON_LOCALTEXT(WEP_TYPE_SHORTBOW);
		WEAPON_LOCALTEXT(WEP_TYPE_AXE);
		WEAPON_LOCALTEXT(WEP_TYPE_DAGGER);
		WEAPON_LOCALTEXT(WEP_TYPE_GREATSWORD);
		WEAPON_LOCALTEXT(WEP_TYPE_MACE);
		WEAPON_LOCALTEXT(WEP_TYPE_PISTOL);
		WEAPON_LOCALTEXT(WEP_TYPE_UNKNOWN);
		WEAPON_LOCALTEXT(WEP_TYPE_RIFLE);
		WEAPON_LOCALTEXT(WEP_TYPE_SCEPTER);
		WEAPON_LOCALTEXT(WEP_TYPE_STAFF);
		WEAPON_LOCALTEXT(WEP_TYPE_FOCUS);
		WEAPON_LOCALTEXT(WEP_TYPE_TORCH);
		WEAPON_LOCALTEXT(WEP_TYPE_WARHORN);
		WEAPON_LOCALTEXT(WEP_TYPE_SHIELD);
		case(WEP_TYPE_NOEQUIP): return LOCALTEXT(TEXT_GEAR_NOEQUIP);
	}

	wsprintfA(&idstr[0], "%04d", id);
	return idstr;
}

#endif	// !(defined BGDM_TOS_COMPLIANT)


const char *stat_name_from_id(u32 id)
{
	static char idstr[4];
	static char* const stat_strings[] = {
		"Berserker",
		"Apothecary",
		"Assassin",
		"Carrion",
		"Cavalier",
		"Celestial",
		"Cleric",
		"Commander",
		"Crusader",
		"Dire",
		"Knight",
		"Magi",
		"Marauder",
		"Minstrel",
		"Nomad",
		"Rabid",
		"Rampager",
		"Sentinel",
		"Settler",
		"Shaman",
		"Sinister",
		"Soldier",
		"Trailblazer",
		"Valkyrie",
		"Vigilant",
		"Viper",
		"Wanderer",
		"Zealot",
		"Dire+Rabid",
		"Zerk+Valk",
		"Rabid+Apo",
		"Mighty",
		"Precise",
		"Healing",
		"Hunter",
		"Hearty",
		"Honed",
		"Strong",
		"Resilient",
		"Penetrating",
		"Ravaging",
		"Vital",
		"Rejuvenating",
		"Vigorous",
		"Mending",
		"Stout",
		"Potent",
		"Lingering",
		"Malign",
		"Captain",
		"Giver",
		"Apostate",
		"Seraph",
		"BASIC",
		"NOSTATS",
		"NOEQUIP"
	};

	switch (id) {
		//
		//	Resolved using https://api.guildwars2.com/v2/itemstats
		//	e.g. https://api.guildwars2.com/v2/itemstats/161
		//
	case (161):
	case (584):
	case (599):
	case (1046):
		return stat_strings[0];		// Berserker
	case (605):
	case (659):
	case (1043):
		return stat_strings[1];		// Apothercary
	case (753):
	case (1040):
	case (1128):
		return stat_strings[2];		// Assassin
	case (160):
	case (1038):
		return stat_strings[3];		// Carrion
	case (583):
	case (602):
	case (616):
	case (1050):
		return stat_strings[4];		// Cavalier
	case (559):
	case (588):
	case (593):
	case (1052):
	case (520):						// Not sure about this one, no attribute name returned from API
		return stat_strings[5];		// Celestial
	case (155):
	case (656):
	case (661):
	case (1044):
		return stat_strings[6];		// Cleric
	case (1125):
	case (1131):
		return stat_strings[7];		// Commander
	case (1098):
	case (1109):
		return stat_strings[8];		// Crusader
	case (754):
	case (756):
	case (1114):
		return stat_strings[9];		// Dire
	case (158):
	case (657):
	case (662):
	case (1051):
		return stat_strings[10];	// Knight
	case (156):
	case (1036):
	case (1037):
		return stat_strings[11];	// Magi
	case (1111):
	case (1145):
		return stat_strings[12];	// Marauder
	case (1123):
	case (1034):
	case (1134):
		return stat_strings[13];	// Minstrel
	case (1026):
	case (1063):
	case (1066):
		return stat_strings[14];	// Nomad
	case (154):
	case (585):
	case (594):
	case (1042):
		return stat_strings[15];	// Rabid
	case (159):
	case (658):
	case (663):
	case (1047):
		return stat_strings[16];	// Rampager
	case (686):
	case (1035):
		return stat_strings[17];	// Sentinel
	case (690):
	case (693):
	case (700):
		return stat_strings[18];	// Settler
	case (153):
		return stat_strings[19];	// Shaman
	case (1064):
	case (1065):
	case (1067):
		return stat_strings[20];	// Sinister
	case (162):
	case (586):
	case (601):
	case (1048):
		return stat_strings[21];	// Soldier
	case (1085):
	case (1115):
		return stat_strings[22];	// Trailblazer
	case (157):
	case (1119):
		return stat_strings[23];	// Valkyrie
	case (1118):
	case (1139):
		return stat_strings[24];	// Vigilant
	case (1130):
	case (1153):
		return stat_strings[25];	// Viper
	case (1140):
	case (1162):
		return stat_strings[26];	// Wanderer
	case (799):
	case (1163):
		return stat_strings[27];	// Zealot
	case (581):
	case (596):
		return stat_strings[28];	// Dire+Rabid
	case (591):
	case (600):
		return stat_strings[29];	// Zerk+Valk
	case (592):
	case (595):
		return stat_strings[30];	// Rabid+Apo
	case (137):
		return stat_strings[31];	// Mighty
	case (138):
		return stat_strings[32];	// Precise
	case (175):
		return stat_strings[33];	// Healing
	case (755):
		return stat_strings[34];	// Hunter
	case (149):
		return stat_strings[35];	// Hearty
	case (152):
		return stat_strings[36];	// Honed
	case (142):
		return stat_strings[37];	// Strong
	case (140):
		return stat_strings[38];	// Resilient
	case (151):
		return stat_strings[39];	// Penetrating
	case (144):
		return stat_strings[40];	// Ravaging
	case (139):
		return stat_strings[41];	// Vital
	case (145):
		return stat_strings[42];	// Rejuvenating
	case (146):
		return stat_strings[43];	// Vigorous
	case (147):
		return stat_strings[44];	// Mending
	case (148):
		return stat_strings[45];	// Stout
	case (150):
		return stat_strings[46];	// Potent
	case (141):
		return stat_strings[47];	// Lingering
	case (176):
		return stat_strings[48];	// Malign
	case (660):
	case (665):
	case (1041):
		return stat_strings[49];	// Captain
	case (627):
	case (628):
	case (629):
	case (632):
	case (1031):
	case (1032):
		return stat_strings[50];	// Giver
	case (1012):
		return stat_strings[51];	// Apostate
	case (1220):
	case (1222):
		return stat_strings[52];	// Seraph
	case (112):
		return stat_strings[53];	// BASIC	; NOTE: doesn't exist in the API
	case (0):
		return stat_strings[54];	// NOSTATS
	case (0xFFFF):
		return stat_strings[55];	// NOEQUIP
	}

	wsprintfA(&idstr[0], "%04d", id);
	return idstr;
}

const char *infusion_name_from_id(u32 id, u32 *ar)
{
	static char idstr[6];
	static char* const infus_strings[] = {
		"NONE",
		"3AR",
		"5AR",
		"7AR",
		"WvW:HPR",
		"WvW:TGH",
		"WvW:VIT",
		"WvW:CND",
		"WvW:PWR",
		"WvW:PRC",
		"5AR:HPR",
		"5AR:TGH",
		"5AR:VIT",
		"5AR:CND",
		"5AR:PWR",
		"5AR:PRC",
		"SAB:HPR",
		"SAB:TGH",
		"SAB:VIT",
		"SAB:CND",
		"SAB:PWR",
		"SAB:PRC",
		"GHO:HPR",
		"GHO:TGH",
		"GHO:VIT",
		"GHO:CND",
		"GHO:PWR",
		"GHO:PRC",
		"RESERVED28",
		"RESERVED29",
		"+XP",
		"+Gold",
		"+Karma",
		"+Magic",
		"4HPR",
		"4TGH",
		"4VIT",
		"4CND",
		"4PWR",
		"4PRC",
		"WH:HPR",
		"WH:TGH",
		"WH:VIT",
		"WH:CND",
		"WH:PWR",
		"WH:PRC",
		"Koda's Warmth",
		"RESERRVED47",
		"RESERRVED48",
		"RESERRVED49",
		"7AR:HPR",
		"7AR:TGH",
		"7AR:VIT",
		"7AR:CND",
		"7AR:PWR",
		"7AR:PRC",
		"9AR:HPR",
		"9AR:TGH",
		"9AR:VIT",
		"9AR:CND",
		"9AR:PWR",
		"9AR:PRC",
		"PHO:HPR",
		"PHO:TGH",
		"PHO:VIT",
		"PHO:CND",
		"PHO:PWR",
		"PHO:PRC",
	};

#define INFUSE_AR1	49424
#define INFUSE_AR22	49445

	if (id >= INFUSE_AR1 && id <= INFUSE_AR22) {
		if (ar) *ar += id - INFUSE_AR1 + 1;
		wsprintfA(&idstr[0], "%dAR", id - INFUSE_AR1 + 1);
		return idstr;
	};

#define INFUSE_CN_PWR1  63172
#define INFUSE_CN_PWR15 63186

	if (id >= INFUSE_CN_PWR1 && id <= INFUSE_CN_PWR15) {
		wsprintfA(&idstr[0], "%dPWR", id - INFUSE_CN_PWR1 + 1);
		return idstr;
	};

	switch (id) {
	case (0):
		return infus_strings[0];
	case (75480):
		if (ar) *ar += 3;
		return infus_strings[1]; // +3 Simple Infusion
	case (37137):
	case (37138):
		if (ar) *ar += 5;
		return infus_strings[2]; // +5 Simple Infusion
	case (70852):
		if (ar) *ar += 7;
		return infus_strings[3]; // +7 Simple Infusion
	case (43250):
		return infus_strings[4]; // WvW Healing
	case (43251):
		return infus_strings[5]; // WvW Resilient
	case (43252):
		return infus_strings[6]; // WvW Vital
	case (43253):
		return infus_strings[7]; // WvW Malign
	case (43254):
		return infus_strings[8]; // WvW Mighty
	case (43255):
		return infus_strings[9]; // WvW Precise
	case (39616):
		if (ar) *ar += 5;
		return infus_strings[10]; // AR Healing
	case (39617):
		if (ar) *ar += 5;
		return infus_strings[11]; // AR Resilient
	case (39618):
		if (ar) *ar += 5;
		return infus_strings[12]; // AR Vital
	case (39619):
		if (ar) *ar += 5;
		return infus_strings[13]; // AR Malign
	case (39620):
		if (ar) *ar += 5;
		return infus_strings[14]; // AR Mighty
	case (39621):
		if (ar) *ar += 5;
		return infus_strings[15]; // AR Precise
	case (78016):
	case (78079):
		if (ar) *ar += 9;
		return infus_strings[16]; // SAB Moto Red/Blue Healing
	case (78030):
	case (78086):
		if (ar) *ar += 9;
		return infus_strings[17]; // SAB Moto Red/Blue Toughness
	case (78090):
	case (78097):
		if (ar) *ar += 9;
		return infus_strings[18]; // SAB Moto Red/Blue Vitality
	case (78012):
	case (78057):
		if (ar) *ar += 9;
		return infus_strings[19]; // SAB Moto Red/Blue Condi
	case (78028):
	case (78052):
		if (ar) *ar += 9;
		return infus_strings[20]; // SAB Moto Red/Blue Power
	case (78031):
	case (78054):
		if (ar) *ar += 9;
		return infus_strings[21]; // SAB Moto Red/Blue Precision
	case (77274):
		if (ar) *ar += 9;
		return infus_strings[22]; // Ghostly Healing
	case (77394):
		if (ar) *ar += 9;
		return infus_strings[23]; // Ghostly Resilient
	case (77303):
		if (ar) *ar += 9;
		return infus_strings[24]; // Ghostly Vital
	case (77366):
		if (ar) *ar += 9;
		return infus_strings[25]; // Ghostly Malign
	case (77310):
		if (ar) *ar += 9;
		return infus_strings[26]; // Ghostly Mighty
	case (77316):
		if (ar) *ar += 9;
		return infus_strings[27]; // Ghostly Precision
	case (39330):
		return infus_strings[30]; // XP Enrich
	case (39331):
		return infus_strings[31]; // Gilded Enrich
	case (39332):
		return infus_strings[32]; // Karma Enrich
	case (39333):
		return infus_strings[33]; // Magic Enrich
	case (39340):
		return infus_strings[34]; // Simple Healing
	case (39339):
		return infus_strings[35]; // Simple Resilient
	case (39338):
		return infus_strings[36]; // Simple Vital
	case (39337):
		return infus_strings[37]; // Simple Malign
	case (39336):
		return infus_strings[38]; // Simple Mighty
	case (39335):
		return infus_strings[39]; // Simple Precise
	case (79978):
		if (ar) *ar += 9;
		return infus_strings[40]; // Winter's Heart Healing
	case (80063):
		if (ar) *ar += 9;
		return infus_strings[41]; // Winter's Heart Tougness
	case (79943):
		if (ar) *ar += 9;
		return infus_strings[42]; // Winter's Heart Vitality
	case (79994):
		if (ar) *ar += 9;
		return infus_strings[43]; // Winter's Heart Condi
	case (79959):
		if (ar) *ar += 9;
		return infus_strings[44]; // Winter's Heart Power
	case (79957):
		if (ar) *ar += 9;
		return infus_strings[45]; // Winter's Heart Precision
	case (79926):
		return infus_strings[46]; // Koda's Warmth
	case (37123):
		if (ar) *ar += 7;
		return infus_strings[50]; // AR Healing
	case (37133):
		if (ar) *ar += 7;
		return infus_strings[51]; // AR Resilient
	case (37134):
		if (ar) *ar += 7;
		return infus_strings[52]; // AR Vital
	case (37129):
		if (ar) *ar += 7;
		return infus_strings[53]; // AR Malign
	case (37127):
		if (ar) *ar += 7;
		return infus_strings[54]; // AR Mighty
	case (37128):
		if (ar) *ar += 7;
		return infus_strings[55]; // AR Precise
	case (37125):
		if (ar) *ar += 9;
		return infus_strings[56]; // AR Healing
	case (37135):
		if (ar) *ar += 9;
		return infus_strings[57]; // AR Resilient
	case (37136):
		if (ar) *ar += 9;
		return infus_strings[58]; // AR Vital
	case (37130):
		if (ar) *ar += 9;
		return infus_strings[59]; // AR Malign
	case (37131):
		if (ar) *ar += 9;
		return infus_strings[60]; // AR Mighty
	case (37132):
		if (ar) *ar += 9;
		return infus_strings[61]; // AR Precise
	case (79669):
		if (ar) *ar += 9;
		return infus_strings[62]; // Phospholuminescent Infusion Healing
	case (79661):
		if (ar) *ar += 9;
		return infus_strings[63]; // Phospholuminescent Infusion Toughness
	case (79653):
		if (ar) *ar += 9;
		return infus_strings[64]; // Phospholuminescent Infusion Vitality
	case (79674):
		if (ar) *ar += 9;
		return infus_strings[65]; // Phospholuminescent Infusion Condi
	case (79665):
		if (ar) *ar += 9;
		return infus_strings[66]; // Phospholuminescent Infusion Power
	case (79639):
		if (ar) *ar += 9;
		return infus_strings[67]; // Phospholuminescent Infusion Precision
	default:
		wsprintfA(&idstr[0], "%06d", id);
		return idstr;
	};
}

const char *jewel_name_from_id(u32 id)
{
	static char* const jewel_strings[] = {
		"NONE",
		"Maguuma Burl",
		"Black Diamond",
		"Copper Doubloon",
		"Silver Doubloon",
		"Gold Doubloon",
		"Plat Doubloon",
		"Topaz Jewel",
		"Ebony Jewel",
		"Garnet Jewel",
		"Amber Jewel",
		"Sunstone Jewel",
		"Amethyst Jewel",
		"Emerald Jewel",
		"Peridot Jewel",
		"Ruby Jewel",
		"Beryl Jewel",
		"Sapphire Jewel",
		"Coral Jewel",
		"Opal Jewel",
		"Chrysocola Jewel",
		"Carnelian Jewel",
		"Azurite Jewel",
		"Ambrite Jewel",
		"Agate Jewel",
		"Moonstone Jewel",
		"Turquoise Jewel",
		"Malachite Jewel",
		"Spinel Jewel",
		"Lapis Jewel",
		"Soldier Crest",
		"Magi Crest",
		"Assassin Crest",
		"Rabid Crest",
		"Shaman Crest",
		"RESERVED35",
		"RESERVED36",
		"RESERVED37",
		"RESERVED38",
		"RESERVED39",
		"Queen Bee",
		"Poly (Black)",
		"Poly (Teal)",
		"Poly (Orange)",
		"Poly (Green)",
		"RESERVED45",
		"RESERVED46",
		"RESERVED47",
		"RESERVED48",
		"RESERVED49",
		"Lily Blossom",
		"Flax Blossom",
		"Passion Flower",
		"Freshwater Pearl",
		"Charged Quartz",
		"Chak Egg Sac",
		"Maguuma Lily",
		"Tiger's Eye",
		"Pearl",
	};

	switch (id) {
	case (0):
		return jewel_strings[0];
	case (72315):
		return jewel_strings[1]; // Maguuma Burl
	case (72435):
	case (76491):
		return jewel_strings[2]; // Black Diamond
	case (24884):
		return jewel_strings[3]; // Copper Doubloon
	case (24502):
		return jewel_strings[4]; // Silver Doubloon
	case (24772):
		return jewel_strings[5]; // Gold Doubloon
	case (24773):
		return jewel_strings[6]; // Plat Doubloon
	case (24490):
	case (24506):
	case (24904):
	case (24911):
		return jewel_strings[7]; // Topaz Jewel
	case (75654):
	case (76764):
	case (76765):
		return jewel_strings[8]; // Ebony Jewel
	case (24464):
	case (24483):
		return jewel_strings[9]; // Garnet Jewel
	case (24477):
	case (24534):
		return jewel_strings[10]; // Amber Jewel
	case (24537):
	case (24539):
	case (24910):
		return jewel_strings[11]; // Sunstone Jewel
	case (24484):
	case (24538):
	case (24898):
	case (24905):
		return jewel_strings[12]; // Amethyst Jewel
	case (24473):
	case (24493):
	case (24497):
	case (24922):
	case (24515):
		return jewel_strings[13]; // Emerald Jewel
	case (24468):
	case (24481):
	case (24482):
	case (24504):
	case (64677):
		return jewel_strings[14]; // Peridot Jewel
	case (24474):
	case (24494):
	case (24498):
	case (24508):
	case (24873):
	case (24924):
		return jewel_strings[15]; // Ruby Jewel
	case (24519):
	case (24520):
	case (24540):
	case (24543):
	case (24872):
	case (24919):
		return jewel_strings[16]; // Beryl Jewel
	case (24475):
	case (24499):
	case (24516):
	case (24925):
		return jewel_strings[17]; // Sapphire Jewel
	case (24510):
	case (24541):
	case (24544):
	case (24921):
		return jewel_strings[18]; // Coral Jewel
	case (24521):
	case (24522):
	case (24542):
	case (24545):
	case (24916):
	case (24923):
		return jewel_strings[19]; // Opal Jewel
	case (24920):
	case (24492):
	case (24496):
	case (24511):
	case (24512):
	case (24870):
	case (24878):
		return jewel_strings[20]; // Chrysocola Jewel
	case (24469):
	case (24478):
	case (24479):
	case (24899):
		return jewel_strings[21]; // Carnelian Jewel
	case (42006):
	case (42007):
	case (42008):
	case (42009):
	case (42010):
		return jewel_strings[22]; // Azurite Jewel
	case (67914):
		return jewel_strings[23]; // Ambrite Jewel
	case (72436):
		return jewel_strings[24]; // Agate Jewel
	case (71725):
	case (72504):
		return jewel_strings[25]; // Moonstone Jewel
	case (24465):
	case (24485):
		return jewel_strings[26]; // Turquoise Jewel
	case (24466):
		return jewel_strings[27]; // Malachite Jewel
	case (24526):
	case (24889):
	case (24897):
	case (24909):
	case (64655):
		return jewel_strings[28]; // Spinel Jewel
	case (24486):
	case (24487):
	case (24900):
		return jewel_strings[29]; // Lapis Jewel

	case (24523):
	case (24524):
	case (24894):
		return jewel_strings[30]; // Soldier Crest
	case (24517):
	case (24518):
	case (24891):
		return jewel_strings[31]; // Magi Crest
	case (24513):
	case (24514):
		return jewel_strings[32]; // Assassin Crest
	case (24531):
	case (24532):
	case (24892):
		return jewel_strings[33]; // Rabid Crest
	case (24476):
	case (24533):
		return jewel_strings[34]; // Shaman Crest

	case (68440):
		return jewel_strings[40]; // Queen Bee
	case (67375):
		return jewel_strings[41]; // Poly (Black)
	case (79647):
		return jewel_strings[42]; // Poly (Teal)
	case (67372):
		return jewel_strings[43]; // Poly (Orange)
	case (67370):
		return jewel_strings[44]; // Poly (Green)

	case (75965):
		return jewel_strings[50]; // Lily Blossom
	case (74988):
		return jewel_strings[51]; // Flax Blossom
	case (37906):
	case (37907):
		return jewel_strings[52]; // Passion Flower
	case (73386):
	case (76179):
		return jewel_strings[53]; // Freshwater Pearl
	case (43866):
		return jewel_strings[54]; // Charged Quartz
	case (72021):
		return jewel_strings[55]; // Chak Egg Sac
	case (70957):
		return jewel_strings[56]; // Maguuma Lily
	case (24467):
		return jewel_strings[57]; // Tiger's Eye Pebble
	case (24500):
	case (24536):
		return jewel_strings[58]; // Pearl

	default:
		return infusion_name_from_id(id, NULL);
	}
}

const char *rune_name_from_id(u32 id)
{
	static char* const rune_strings[] = {
		"NONE",
		"Strength",
		"Water",
		"Rage",
		"Lyssa",
		"Berserker",
		"Herald",
		"Mercy",
		"Monk",
		"Nightmare",
		"Trapper",
		"Ranger",
		"Scholar",
		"Tormenting",
		"Chronomancer",
		"Durability",
		"Undead",
		"Eagle",
		"Daredevil",
		"Orr",
		"Hoelbrak",
		"Grove",
		"Trooper",
		"Dragonhunter",
		"Scrapper",
		"Druid",
		"Tempest",
		"Grenth",
		"Flame Legion",
		"Lich",
		"Vampirism",
		"Speed",
		"Elementalist",
		"Baelfire",
		"Radiance",
		"Snowflake",
		"Divinity",
		"Forgeman",
		"Sunless",
		"Ogre",
		"Balthazar",
		"Traveler",
		"Citadel",
		"Centaur",
		"Rata Sum",
		"Necromancer",
		"Snowfall",
		"Air",
		"EMPTY48",
		"Wurm",
		"EMPTY50",
		"EMPTY51",
		"Melandru",
		"EMPTY53",
		"Leadership",
		"Pack",
		"Fire",
		"EMPTY57",
		"Infiltration",
		"EMPTY59",
		"Mad King",
		"EMPTY61",
		"Guardian",
		"EMPTY63",
		"Afflicted",
		"EMPTY65",
		"Engineer",
		"Thief",
		"Krait",
		"Warrior",
		"Flock",
		"Brawler",
		"EMPTY72",
		"EMPTY73",
		"EMPTY74",
		"Thorns",
		"Perplexity",
		"EMPTY77",
		"Adventurer",
		"EMPTY79",
		"Life",
		"Mesmer",
		"Reaper",
		"Revenant",
		"Dwayna",
		"Privateer",
		"EMPTY86",
		"Scavenging",
		"Ice",
		"Aristocracy",
		"Dolyak",
		"Sanctuary",
		"Defender",
		"Antitoxin",
		"Altruism",
		"Golemancer",
		"Earth",
		"Exuberance",
		"Surging",
	};

	switch (id) {
	case (0):
		return rune_strings[0];
	case (24712):
	case (24713):
	case (24714):
		return rune_strings[1];	// Strength
	case (24837):
	case (24838):
	case (24839):
		return rune_strings[2];	// Water
	case (24561):
	case (24715):
	case (24716):
	case (24717):
		return rune_strings[3];	// Rage
	case (24774):
	case (24775):
	case (24776):
		return rune_strings[4];	// Lyssa
	case (71425):
		return rune_strings[5];	// Berserker
	case (76100):
		return rune_strings[6];	// Herald
	case (24706):
	case (24707):
	case (24708):
		return rune_strings[7];	// Mercy
	case (24840):
	case (24841):
	case (24842):
		return rune_strings[8];	// Monk
	case (24846):
	case (24847):
	case (24848):
		return rune_strings[9]; // Nightmare
	case (67339):
		return rune_strings[10]; // Trapper
	case (24813):
	case (24814):
	case (24815):
		return rune_strings[11]; // Ranger
	case (24834):
	case (24835):
	case (24836):
		return rune_strings[12]; // Scholar
	case (44956):
		return rune_strings[13]; // Tormenting
	case (73399):
		return rune_strings[14]; // Chronomancer
	case (73653):
		return rune_strings[15]; // Durability
	case (24757):
	case (24758):
	case (24759):
		return rune_strings[16]; // Undead
	case (24721):
	case (24722):
	case (24723):
		return rune_strings[17]; // Eagle
	case (72852):
		return rune_strings[18]; // Daredevil
	case (24858):
	case (24859):
	case (24860):
		return rune_strings[19]; // Orr
	case (24727):
	case (24728):
	case (24729):
		return rune_strings[20]; // Hoelbrak
	case (24733):
	case (24734):
	case (24735):
		return rune_strings[21]; // Grove
	case (24825):
	case (24826):
	case (24827):
		return rune_strings[22]; // Trooper
	case (74978):
		return rune_strings[23]; // Dragonhunter
	case (71276):
		return rune_strings[24]; // Scrapper
	case (70450):
		return rune_strings[25]; // Druid
	case (76166):
		return rune_strings[26]; // Tempest
	case (24777):
	case (24778):
	case (24779):
		return rune_strings[27]; // Grenth
	case (24795):
	case (24796):
	case (24797):
		return rune_strings[28]; // Flame Legion
	case (24688):
	case (24689):
	case (24690):
		return rune_strings[29]; // Lich
	case (24709):
	case (24710):
	case (24711):
		return rune_strings[30]; // Vampirism
	case (24718):
	case (24719):
	case (24720):
		return rune_strings[31]; // Speed
	case (24798):
	case (24799):
	case (24800):
		return rune_strings[32]; // Elementalist
	case (24852):
	case (24853):
	case (24854):
		return rune_strings[33]; // Baelfire
	case (67342):
		return rune_strings[34]; // Radiance
	case (38138):
	case (38143):
		return rune_strings[35]; // Snowflake
	case (24730):
	case (24731):
	case (24732):
		return rune_strings[36]; // Divinity
	case (24849):
	case (24850):
	case (24851):
		return rune_strings[37]; // Forgeman
	case (47908):
		return rune_strings[38]; // Sunless
	case (24755):
	case (24756):
		return rune_strings[39]; // Ogre
	case (24763):
	case (24764):
	case (24765):
		return rune_strings[40]; // Balthazar
	case (24691):
	case (24692):
	case (24693):
		return rune_strings[41]; // Traveler
	case (24739):
	case (24740):
	case (24741):
		return rune_strings[42]; // Citadel
	case (24786):
	case (24787):
	case (24788):
		return rune_strings[43]; // Centaur
	case (24724):
	case (24725):
	case (24726):
		return rune_strings[44]; // Rata Sum
	case (24804):
	case (24805):
	case (24806):
		return rune_strings[45]; // Necromancer
	case (68435):
	case (68437):
	case (68438):
		return rune_strings[46]; // Snowfall
	case (24748):
	case (24749):
	case (24750):
		return rune_strings[47]; // Air
	case (24789):
	case (24790):
	case (24791):
		return rune_strings[49]; // Wurm
	case (24769):
	case (24770):
	case (24771):
		return rune_strings[52]; // Melandru
	case (70600):
		return rune_strings[54]; // Leadership
	case (24700):
	case (24701):
	case (24702):
		return rune_strings[55]; // Pack
	case (24745):
	case (24746):
	case (24747):
		return rune_strings[56]; // Fire
	case (24703):
	case (24704):
	case (24705):
		return rune_strings[58]; // Infiltration
	case (36042):
	case (36043):
	case (36044):
		return rune_strings[60]; // Mad King
	case (24824):
		return rune_strings[62]; // Guardian
	case (24685):
	case (24686):
	case (24687):
		return rune_strings[64]; // Afflicted
	case (24810):
	case (24811):
	case (24812):
		return rune_strings[66]; // Engineer
	case (24816):
	case (24817):
	case (24818):
		return rune_strings[67]; // Thief
	case (24760):
	case (24761):
	case (24762):
		return rune_strings[68]; // Krait
	case (24819):
	case (24820):
	case (24821):
		return rune_strings[69]; // Warrior
	case (24694):
	case (24695):
	case (24696):
		return rune_strings[70]; // Flock
	case (24831):
	case (24832):
	case (24833):
		return rune_strings[71]; // Brawler
	case (72912):
		return rune_strings[75]; // Thorns
	case (44957):
	case (44958):
	case (44959):
		return rune_strings[76]; // Perplexity
	case (24828):
	case (24829):
	case (24830):
		return rune_strings[78]; // Adventurer
	case (24581):
	case (24582):
	case (24869):
		return rune_strings[80]; // Life
	case (24801):
	case (24802):
	case (24803):
		return rune_strings[81]; // Mesmer
	case (70829):
		return rune_strings[82]; // Reaper
	case (69370):
		return rune_strings[83]; // Revenant
	case (24766):
	case (24767):
	case (24768):
		return rune_strings[84]; // Dwayna
	case (24780):
	case (24781):
	case (24782):
		return rune_strings[85]; // Privateer
	case (24736):
	case (24737):
	case (24738):
		return rune_strings[87]; // Scavenging
	case (24751):
	case (24752):
	case (24753):
		return rune_strings[88]; // Ice
	case (24843):
	case (24844):
	case (24845):
		return rune_strings[89]; // Aristocracy
	case (24697):
	case (24698):
	case (24699):
		return rune_strings[90]; // Dolyak
	case (24855):
	case (24856):
	case (24857):
		return rune_strings[91]; // Sanctuary
	case (67912):
		return rune_strings[92]; // Defender
	case (48907):
		return rune_strings[93]; // Antitoxin
	case (38204):
	case (38205):
	case (38206):
		return rune_strings[94]; // Altruism
	case (24783):
	case (24784):
	case (24785):
		return rune_strings[95]; // Golemancer
	case (24742):
	case (24743):
	case (24744):
		return rune_strings[96]; // Earth
	case (44951):
	case (44952):
	case (44953):
		return rune_strings[97]; // Exuberance
	case (76813):
		return rune_strings[98]; // Surging

	default:
		return jewel_name_from_id(id);
	};
}

const char *sigil_name_from_id(u32 id)
{
	static char* const sigil_strings[] = {
		"NONE",
		"Force",
		"Air",
		"Blood",
		"Fire",
		"Ice",
		"Malice",
		"Energy",
		"Bursting",
		"Earth",
		"Concentration",
		"Water",
		"Transference",
		"Geomancy",
		"Doom",
		"Bloodlust",
		"Accuracy",
		"Leeching",
		"Paralyzation",
		"Hydromancy",
		"Corruption",
		"Smothering",
		"Mischief",
		"Night",
		"Strength",
		"Momentum",
		"Celerity",
		"Life",
		"Golemancer",
		"Frailty",
		"Purity",
		"Cleansing",
		"Generosity",
		"Restoration",
		"Smoldering",
		"Ruthlessness",
		"Agony",
		"Undead Slaying",
		"Rage",
		"Justice",
		"Chilling",
		"Grawl Slaying",
		"Mad Scientist",
		"Sorrow",
		"Speed",
		"Dwayna",
		"Ogre Slaying",
		"Intelligence",
		"Battle",
		"Torment",
		"Luck",
		"Blight",
		"Defender",
		"Renewal",
		"Impact",
		"Perception",
		"Nullification",
		"Ghost Slaying",
		"Dreams",
		"Debility",
		"Venom",
		"Peril",
		"Stamina",
		"Wrath",
		"Demon Slaying",
		"Absorption",
		"Benevolence",
		"Hobbling",
		"Cruelty",
		"Serpent Slaying",
		"Demon Summoning",
		"Incapacitation",
		"Draining",
	};

	switch (id) {
	case (0):
		return sigil_strings[0];
	case (24613):
	case (24614):
	case (24615):
		return sigil_strings[1]; // Force
	case (24552):
	case (24553):
	case (24554):
		return sigil_strings[2]; // Air
	case (24568):
	case (24569):
	case (24570):
		return sigil_strings[3]; // Blood
	case (24546):
	case (24547):
	case (24548):
		return sigil_strings[4]; // Fire
	case (24555):
	case (24556):
	case (24557):
		return sigil_strings[5]; // Ice
	case (44948):
	case (44949):
	case (44950):
		return sigil_strings[6]; // Malice
	case (24606):
	case (24607):
		return sigil_strings[7]; // Energy
	case (44942):
	case (44943):
	case (44944):
		return sigil_strings[8]; // Bursting
	case (24558):
	case (24559):
	case (24560):
		return sigil_strings[9]; // Earth
	case (72339):
		return sigil_strings[10]; // Concentration
	case (24549):
	case (24550):
	case (24551):
		return sigil_strings[11]; // Water
	case (74326):
	case (75623):
		return sigil_strings[12]; // Transference
	case (24603):
	case (24604):
	case (24605):
		return sigil_strings[13]; // Geomancy
	case (24862):
	case (24608):
	case (24609):
		return sigil_strings[14]; // Doom
	case (24573):
	case (24574):
	case (24575):
		return sigil_strings[15]; // Bloodlust
	case (24616):
	case (24617):
	case (24618):
		return sigil_strings[16]; // Accuracy
	case (24598):
	case (24599):
		return sigil_strings[17]; // Leeching
	case (24637):
	case (24638):
	case (24639):
		return sigil_strings[18]; // Paralayzation
	case (24595):
	case (24596):
	case (24597):
		return sigil_strings[19]; // Hydro
	case (24576):
	case (24577):
	case (24578):
		return sigil_strings[20]; // Corruption
	case (24673):
	case (24674):
	case (24675):
		return sigil_strings[21]; // Smothering
	case (68436):
	case (68439):
		return sigil_strings[22]; // Mischief
	case (36053):
		return sigil_strings[23]; // Night
	case (24562):
	case (24563):
	case (24564):
		return sigil_strings[24]; // Strength
	case (49457):
		return sigil_strings[25]; // Momentum
	case (24865):
		return sigil_strings[26]; // Celerity
	case (24581):
	case (24582):
		return sigil_strings[27]; // Life
	case (24785):
		return sigil_strings[28]; // Golemancer
	case (24565):
	case (24566):
	case (24567):
		return sigil_strings[29]; // Frailty
	case (24571):
		return sigil_strings[30]; // Purity
	case (67340):
		return sigil_strings[31]; // Cleansing
	case (38294):
		return sigil_strings[32]; // Generosity
	case (24593):
	case (24594):
		return sigil_strings[33]; // Restoration
	case (24622):
	case (24623):
	case (24624):
		return sigil_strings[34]; // Smoldering
	case (71130):
		return sigil_strings[35]; // Ruthlessness
	case (24610):
	case (24611):
	case (24612):
		return sigil_strings[36]; // Agony
	case (24641):
	case (24642):
		return sigil_strings[37]; // Undead Slaying
	case (24561):
		return sigil_strings[38]; // Rage
	case (24678):
		return sigil_strings[39]; // Justice
	case (24628):
	case (24629):
	case (24630):
		return sigil_strings[40]; // Chilling
	case (24646):
	case (24647):
	case (24648):
		return sigil_strings[41]; // Grawl Slaying
	case (24670):
	case (24671):
	case (24672):
		return sigil_strings[42]; // Mad Scientist
	case (24682):
	case (24683):
	case (24684):
		return sigil_strings[43]; // Sorrow
	case (24587):
	case (24588):
	case (24589):
		return sigil_strings[44]; // Speed
	case (24766):
	case (24767):
	case (24768):
		return sigil_strings[45]; // Dwayna
	case (24655):
	case (24656):
	case (24657):
		return sigil_strings[46]; // Ogre Slaying
	case (24600):
		return sigil_strings[47]; // Intelligence
	case (24601):
		return sigil_strings[48]; // Battle
	case (48911):
		return sigil_strings[49]; // Torment
	case (24591):
		return sigil_strings[50]; // Luck
	case (67913):
		return sigil_strings[51]; // Blight
	case (67912):
		return sigil_strings[52]; // Defender
	case (44945):
	case (44946):
	case (44947):
		return sigil_strings[53]; // Renewal
	case (24866):
	case (24867):
	case (24868):
		return sigil_strings[54]; // Impact
	case (24579):
	case (24580):
		return sigil_strings[55]; // Perception
	case (24572):
		return sigil_strings[56]; // Nullification
	case (24807):
	case (24808):
	case (24809):
		return sigil_strings[57]; // Ghost Slaying
	case (24679):
	case (24680):
	case (24681):
		return sigil_strings[58]; // Dreams
	case (24634):
	case (24635):
	case (24636):
		return sigil_strings[59]; // Debility
	case (24631):
	case (24632):
	case (24633):
		return sigil_strings[60]; // Venom
	case (24619):
	case (24620):
	case (24621):
		return sigil_strings[61]; // Peril
	case (24592):
		return sigil_strings[62]; // Stamina
	case (24667):
		return sigil_strings[63]; // Wrath
	case (24664):
	case (24665):
	case (24666):
		return sigil_strings[64]; // Demon Slaying
	case (72872):
		return sigil_strings[65]; // Absorption
	case (24584):
	case (24585):
	case (24586):
		return sigil_strings[66]; // Benevolence
	case (24625):
	case (24626):
	case (24627):
		return sigil_strings[67]; // Hobbling
	case (67341):
		return sigil_strings[68]; // Cruelty
	case (24658):
	case (24659):
	case (24660):
		return sigil_strings[69]; // Serpent Slaying
	case (24583):
		return sigil_strings[70]; // Demon Summoning
	case (67343):
		return sigil_strings[71]; // Incapacitation
	case (70825):
		return sigil_strings[72]; // Draining

	default:
		return jewel_name_from_id(id);
	};
}


const char *spec_name_from_id(u32 spec)
{
	static char* const spec_strings[] = {
		"NONE",
		"Dueling",
		"Death Magic",
		"Invoation",
		"Strength",
		"Druid",
		"Explosives",
		"DareDevil",
		"Marksmanship",
		"Retribution",
		"Domination",
		"Tactics",
		"Salvation",
		"Valor",
		"Corruption",
		"Devastation",
		"Radiance",
		"Water",
		"Berserker",
		"Blood Magic",
		"Shadow Arts",
		"Tools",
		"Defense",
		"Inspiration",
		"Illusions",
		"Nature Magic",
		"Earth",
		"Dragon Hunter",
		"Deadly Arts",
		"Alchemy",
		"Skirmishing",
		"Fire",
		"Beastmastery",
		"Survival",
		"Reaper",
		"Crit Strikes",
		"Arms",
		"Arcane",
		"Firearms",
		"Curses",
		"Chronomancer",
		"Air",
		"Zeal",
		"Scrapper",
		"Trickery",
		"Chaos",
		"Virtues",
		"Inventions",
		"Tempest",
		"Honor",
		"Soul Reaping",
		"Discipline",
		"Herald",
		"Spite",
		"Acrobatics",
		"INVALID"
	};

	if (spec >= ARRAYSIZE(spec_strings))
		return spec_strings[ARRAYSIZE(spec_strings) - 1];
	return spec_strings[spec];
}

const char *trait_name_from_id(u32 spec, u32 id)
{
	static char idstr[6];

	typedef struct __IdNamePair
	{
		u32 id;
		i8 *name;
	} IdNamePair;

	static IdNamePair const traitDB[SPEC_END][9] = {
		// SPEC None
		{
			{ TRAIT_NONE, "NONE" },
			{ TRAIT_NONE, "NONE2" },
			{ TRAIT_NONE, "NONE3" },
			{ TRAIT_NONE, "NONE4" },
			{ TRAIT_NONE, "NONE5" },
			{ TRAIT_NONE, "NONE6" },
			{ TRAIT_NONE, "NONE7" },
			{ TRAIT_NONE, "NONE8" },
			{ TRAIT_NONE, "NONE9" },
		},
		// Mes Dueling
		{
			{ TRAIT_MES_PHANTASMAL_FURY, "[TOP] Phantasmal Fury" },
			{ TRAIT_MES_DESPERATE_DECOY, "[MID] Desperate Decoy" },
			{ TRAIT_MES_DUELISTS_DISCIPLINE, "[BOT] Duelists Discipline" },
			{ TRAIT_MES_BLINDING_DISSIPATION, "[TOP] Blinding Dissipation" },
			{ TRAIT_MES_EVASIVE_MIRROR, "[MID] Evasive Mirror" },
			{ TRAIT_MES_FENCERS_FINESSE, "[BOT] Fencer's Finesse" },
			{ TRAIT_MES_HARMONIOUS_MANTRAS, "[TOP] Harmonious Mantras" },
			{ TRAIT_MES_MISTRUST, "[MID] Mistrust" },
			{ TRAIT_MES_DECEPTIVE_EVASION, "[BOT] Deceptive Evasion" },
		},
		// Necro Death Magic
		{
			{ TRAIT_NECRO_FLESH_OF_THE_MASTER, "[TOP] Flesh of the Master" },
			{ TRAIT_NECRO_SHROUDED_REMOVAL, "[MID] Shrouded Removal" },
			{ TRAIT_NECRO_PUTRID_DEFENSE, "[BOT] Putrid Defense" },
			{ TRAIT_NECRO_NECROMANTIC_CORRUPTION, "[TOP] Necromantic Corruption" },
			{ TRAIT_NECRO_REAPERS_PROTECTION, "[MID] Reapers Protection" },
			{ TRAIT_NECRO_DEADLY_STRENGTH, "[BOT] Deadly Strength" },
			{ TRAIT_NECRO_DEATH_NOVA, "[TOP] Death Nova" },
			{ TRAIT_NECRO_CORRUPTERS_FAVOR, "[MID] Corrupter's Favor" },
			{ TRAIT_NECRO_UNHOLY_SANCTUARY, "[BOT] Unholy Sanctuary" },
		},
		// Rev Invocation
		{
			{ TRAIT_REV_Cruel_Repercussion, "[TOP] Cruel Repercussion" },
			{ TRAIT_REV_Cleansing_Channel, "[MID] Cleansing Channel" },
			{ TRAIT_REV_Fierce_Infusion, "[BOT] Fierce Infusion" },
			{ TRAIT_REV_Equilibrium, "[TOP] Equilibrium" },
			{ TRAIT_REV_Invigorating_Flow, "[MID] Invigorating Flow" },
			{ TRAIT_REV_Increased_Response, "[BOT] Increased Response" },
			{ TRAIT_REV_Roiling_Mists, "[TOP] Roiling Mists" },
			{ TRAIT_REV_Charged_Mists, "[MID] Charged Mists" },
			{ TRAIT_REV_Shrouding_Mists, "[BOT] Shrouding Mists" },
		},
		// War Strength
		{
			{ TRAIT_WAR_Death_from_Above, "[TOP] Death from Above" },
			{ TRAIT_WAR_Restorative_Strength, "[MID] Restorative Strength" },
			{ TRAIT_WAR_Peak_Performance, "[BOT] Peak Performance" },
			{ TRAIT_WAR_Body_Blow, "[TOP] Body Blow" },
			{ TRAIT_WAR_Forceful_Greatsword, "[MID] Forceful Greatsword" },
			{ TRAIT_WAR_Great_Fortitude, "[BOT] Great Fortitude" },
			{ TRAIT_WAR_Berserkers_Power, "[TOP] Berserker's Power" },
			{ TRAIT_WAR_Distracting_Strikes, "[MID] Distracting Strikes" },
			{ TRAIT_WAR_Axe_Mastery, "[BOT] Axe Mastery" },
		},
		// Ranger Druid
		{
			{ TRAIT_RANGER_Druidic_Clarity, "[TOP] Druidic Clarity" },
			{ TRAIT_RANGER_Cultivated_Synergy, "[MID] Cultivated Synergy" },
			{ TRAIT_RANGER_Primal_Echoes, "[BOT] Primal Echoes" },
			{ TRAIT_RANGER_Celestial_Shadow, "[TOP] Celestial Shadow" },
			{ TRAIT_RANGER_Verdant_Etching, "[MID] Verdant Etching" },
			{ TRAIT_RANGER_Natural_Stride, "[BOT] Natural Stride" },
			{ TRAIT_RANGER_Grace_of_the_Land, "[TOP] Grace of the Land" },
			{ TRAIT_RANGER_Lingering_Light, "[MID] Lingering Light" },
			{ TRAIT_RANGER_Ancient_Seeds, "[BOT] Ancient Seeds" },
		},
		// Engi Explosives
		{
			{ TRAIT_ENGI_Grenadier, "[TOP] Grenadier" },
			{ TRAIT_ENGI_Explosive_Descent, "[MID] Explosive Descent" },
			{ TRAIT_ENGI_Glass_Cannon, "[BOT] Glass Cannon" },
			{ TRAIT_ENGI_Aim_Assisted_Rocket, "[TOP] Aim-Assisted Rocket" },
			{ TRAIT_ENGI_Sharped_Charge, "[MID] Sharped Charge" },
			{ TRAIT_ENGI_Short_Fuse, "[BOT] Short Fuse" },
			{ TRAIT_ENGI_Siege_Rounds, "[TOP] Siege Rounds" },
			{ TRAIT_ENGI_Sharpnel, "[MID] Sharpnel" },
			{ TRAIT_ENGI_Thermobaric_Detonation, "[BOT] Thermobaric Detonation" },
		},
		// Thief Daredevil
		{
			{ TRAIT_THIEF_Havoc_Mastery, "[TOP] Havoc Mastery" },
			{ TRAIT_THIEF_Weakening_Strikes, "[MID] Weakening Strikes" },
			{ TRAIT_THIEF_Brawlers_Tenacity, "[BOT] Brawler's Tenacity" },
			{ TRAIT_THIEF_Staff_Master, "[TOP] Staff Master" },
			{ TRAIT_THIEF_Escapists_Absolution, "[MID] Escapist's Absolution" },
			{ TRAIT_THIEF_Impacting_Disruption, "[BOT] Impacting Disruption" },
			{ TRAIT_THIEF_Lotus_Training, "[TOP] Lotus Training" },
			{ TRAIT_THIEF_Unhindered_Combatant, "[MID] Unhindered Combatant" },
			{ TRAIT_THIEF_Bounding_Dodger, "[BOT] Bounding Dodger" },
		},
		// Ranger Marksmanship
		{
			{ TRAIT_RANGER_Enlargement, "[TOP] Enlargement" },
			{ TRAIT_RANGER_Perdators_Instinct, "[MID] Perdator's Instinct" },
			{ TRAIT_RANGER_Clarion_Bond, "[BOT] Clarion Bond" },
			{ TRAIT_RANGER_Brutish_Seals, "[TOP] Brutish Seals" },
			{ TRAIT_RANGER_Steady_Focus, "[MID] Steady Focus" },
			{ TRAIT_RANGER_Moment_of_Clarity, "[BOT] Moment of Clarity" },
			{ TRAIT_RANGER_Predators_Onslaught, "[TOP] Predator's Onslaught" },
			{ TRAIT_RANGER_Remorseless, "[MID] Remorseless" },
			{ TRAIT_RANGER_Lead_the_Wind, "[BOT] Lead the Wind" },
		},
		// Rev Retri
		{
			{ TRAIT_REV_Planar_Protection, "[TOP] Planar Protection" },
			{ TRAIT_REV_Close_Quarters, "[MID] Close Quarters" },
			{ TRAIT_REV_Improved_Aggression, "[BOT] Improved Aggression" },
			{ TRAIT_REV_Eye_for_an_Eye, "[TOP] Eye for an Eye" },
			{ TRAIT_REV_Retaliatory_Evasion, "[MID] Retaliatory Evasion" },
			{ TRAIT_REV_Dwarven_Battle_Training, "[BOT] Dwarven Battle Training" },
			{ TRAIT_REV_Empowering_Vengeance, "[TOP] Empowering Vengeance" },
			{ TRAIT_REV_Versed_in_Stone, "[MID] Versed in Stone" },
			{ TRAIT_REV_Steadfast_Rejuvenation, "[BOT] Steadfast Rejuvenation" },
		},
		// Mes Domination
		{
			{ TRAIT_MES_Confounding_Suggestions, "[TOP] Confounding Suggestions" },
			{ TRAIT_MES_Empowered_Illusions, "[MID] Empowered Illusions" },
			{ TRAIT_MES_Rending_Shatter, "[BOT] Rending Shatter" },
			{ TRAIT_MES_Shattered_Concentration, "[TOP] Shattered Concentration" },
			{ TRAIT_MES_Blurred_Inscriptions, "[MID] Blurred Inscriptions" },
			{ TRAIT_MES_Furious_Interruption, "[BOT] Furious Interruption" },
			{ TRAIT_MES_Imagined_Burden, "[TOP] Imagined Burden" },
			{ TRAIT_MES_Mental_Anguish, "[MID] Mental Anguish" },
			{ TRAIT_MES_Power_Block, "[BOT] Power Block" },
		},
		// War Tactics
		{
			{ TRAIT_WAR_Leg_Specialist, "[TOP] Leg Specialist" },
			{ TRAIT_WAR_Quick_Breating, "[MID] Quick Breating" },
			{ TRAIT_WAR_Empowered, "[BOT] Empowered" },
			{ TRAIT_WAR_Shrug_it_Off, "[TOP] Shrug it Off" },
			{ TRAIT_WAR_Burning_Arrows, "[MID] Burning Arrows" },
			{ TRAIT_WAR_Empower_Allies, "[BOT] Empower Allies" },
			{ TRAIT_WAR_Powerful_Synergy, "[TOP] Powerful Synergy" },
			{ TRAIT_WAR_Vigorous_Shouts, "[MID] Vigorous Shouts" },
			{ TRAIT_WAR_Phalanx_Strength, "[BOT] Phalanx Strength" },
		},
		// Rev Salvation
		{
			{ TRAIT_REV_Nourishing_Roots, "[TOP] Nourishing Roots" },
			{ TRAIT_REV_Blinding_Truths, "[MID] Blinding Truths" },
			{ TRAIT_REV_Tranquil_Balance, "[BOT] Tranquil Balance" },
			{ TRAIT_REV_Tranquil_Benediction, "[TOP] Tranquil Benediction" },
			{ TRAIT_REV_Eluding_Nullification, "[MID] Eluding Nullification" },
			{ TRAIT_REV_Invoking_Harmony, "[BOT] Invoking Harmony" },
			{ TRAIT_REV_Selfless_Amplification, "[TOP] Selfless Amplification" },
			{ TRAIT_REV_Natural_Abundance, "[MID] Natural Abundance" },
			{ TRAIT_REV_Momentary_Pacification, "[BOT] Momentary Pacification" },
		},
		// Guard Valor
		{
			{ TRAIT_GUARD_Strength_of_the_Fallen, "[TOP] Strength of the Fallen" },
			{ TRAIT_GUARD_Smiters_Boon, "[MID] Smiter's Boon" },
			{ TRAIT_GUARD_Focus_Mastery, "[BOT] Focus Mastery" },
			{ TRAIT_GUARD_Stalwart_Defender, "[TOP] Stalwart Defender" },
			{ TRAIT_GUARD_Strength_in_Numbers, "[MID] Strength in Numbers" },
			{ TRAIT_GUARD_Communal_Defenses, "[BOT] Communal Defenses" },
			{ TRAIT_GUARD_Altruistic_Healing, "[TOP] Altruistic Healing" },
			{ TRAIT_GUARD_Monks_Focus, "[MID] Monk's Focus" },
			{ TRAIT_GUARD_Retributive_Armor, "[BOT] Retributive Armor" },
		},
		// Rev Courruption
		{
			{ TRAIT_REV_Replenishing_Despair, "[TOP] Replenishing Despair" },
			{ TRAIT_REV_Demonic_Defiance, "[MID] Demonic Defiance" },
			{ TRAIT_REV_Venom_Enhancement, "[BOT] Venom Enhancement" },
			{ TRAIT_REV_Bolstered_Anguish, "[TOP] Bolstered Anguish" },
			{ TRAIT_REV_Frigid_Precision, "[MID] Frigid Precision" },
			{ TRAIT_REV_Spontaneous_Destruction, "[BOT] Spontaneous Destruction" },
			{ TRAIT_REV_Diabolic_Inferno, "[TOP] Diabolic Inferno" },
			{ TRAIT_REV_Manaical_Persistence, "[MID] Manaical Persistence" },
			{ TRAIT_REV_Pulsating_Pestilence, "[BOT] Pulsating Pestilence" },
		},
		// Rev Devastation
		{
			{ TRAIT_REV_Ferocious_Strikes, "[TOP] Ferocious Strikes" },
			{ TRAIT_REV_Vicious_Lacerations, "[MID] Vicious Lacerations" },
			{ TRAIT_REV_Malicious_Reprisal, "[BOT] Malicious Reprisal" },
			{ TRAIT_REV_Jade_Echo, "[TOP] Jade Echo" },
			{ TRAIT_REV_Nefarious_Momentum, "[MID] Nefarious Momentum" },
			{ TRAIT_REV_Assassins_Presence, "[BOT] Assassin's Presence" },
			{ TRAIT_REV_Swift_Termination, "[TOP] Swift Termination" },
			{ TRAIT_REV_Dismantle_Fortifications, "[MID] Dismantle Fortifications" },
			{ TRAIT_REV_Assassins_Annhilation, "[BOT] Assassin's Annhilation" },
		},
		// Guard Radiance
		{
			{ TRAIT_GUARD_Inner_Fire, "[TOP] Inner Fire" },
			{ TRAIT_GUARD_Right_Hand_Strength, "[MID] Right-Hand Strength" },
			{ TRAIT_GUARD_Healers_Retribution, "[BOT] Healer's Retribution" },
			{ TRAIT_GUARD_Wrath_of_Justice, "[TOP] Wrath of Justice" },
			{ TRAIT_GUARD_Radiant_Fire, "[MID] Radiant Fire" },
			{ TRAIT_GUARD_Retribution, "[BOT] Retribution" },
			{ TRAIT_GUARD_Amplified_Wrath, "[TOP] Amplified Wrath" },
			{ TRAIT_GUARD_Perfect_Inscriptions, "[MID] Perfect Inscriptions" },
			{ TRAIT_GUARD_Radiant_Retaliation, "[BOT] Radiant Retaliation" },
		},
		// Ele Water
		{
			{ TRAIT_ELE_Soothing_Ice, "[TOP] Soothing Ice" },
			{ TRAIT_ELE_Piercing_Shards, "[MID] Piercing Shards" },
			{ TRAIT_ELE_Stop_Drop_and_Roll, "[BOT] Stop, Drop and Roll" },
			{ TRAIT_ELE_Soothing_Disruption, "[TOP] Soothing Disruption" },
			{ TRAIT_ELE_Cleansing_Wave, "[MID] Cleansing Wave" },
			{ TRAIT_ELE_Aquamancers_Training, "[BOT] Aquamancer's Training" },
			{ TRAIT_ELE_Cleansing_Water, "[TOP] Cleansing Water" },
			{ TRAIT_ELE_Powerful_Aura, "[MID] Powerful Aura" },
			{ TRAIT_ELE_Soothing_Power, "[BOT] Soothing Power" },
		},
		// War Berserker
		{
			{ TRAIT_WAR_Smash_Brawler, "[TOP] Smash Brawler" },
			{ TRAIT_WAR_Last_Blaze, "[MID] Last Blaze" },
			{ TRAIT_WAR_Savage_Instinct, "[BOT] Savage Instinct" },
			{ TRAIT_WAR_Blood_Reaction, "[TOP] Blood Reaction" },
			{ TRAIT_WAR_Heal_the_Soul, "[MID] Heal the Soul" },
			{ TRAIT_WAR_Dead_or_Alive, "[BOT] Dead or Alive" },
			{ TRAIT_WAR_Bloody_Roar, "[TOP] Bloody Roar" },
			{ TRAIT_WAR_King_of_Fires, "[MID] King of Fires" },
			{ TRAIT_WAR_Eternal_Champion, "[BOT] Eternal Champion" },
		},
		// Necro Blood Magic
		{
			{ TRAIT_NEC_Ritual_of_Life, "[TOP] Ritual of Life" },
			{ TRAIT_NEC_Quickening_Thirst, "[MID] Quickening Thirst" },
			{ TRAIT_NEC_Blood_Bond, "[BOT] Blood Bond" },
			{ TRAIT_NEC_Life_from_Death, "[TOP] Life from Death" },
			{ TRAIT_NEC_Banshees_Wail, "[MID] Banshee's Wail" },
			{ TRAIT_NEC_Vampiric_Presence, "[BOT] Vampiric Presence" },
			{ TRAIT_NEC_Vampiric_Rituals, "[TOP] Vampiric Rituals" },
			{ TRAIT_NEC_Unholy_Martyr, "[MID] Unholy Martyr" },
			{ TRAIT_NEC_Transfusion, "[BOT] Transfusion" },
		},
		// Thief Shadow Arts
		{
			{ TRAIT_THIEF_Last_Refuge, "[TOP] Last Refuge" },
			{ TRAIT_THIEF_Concealed_Defeat, "[MID] Concealed Defeat" },
			{ TRAIT_THIEF_Shadows_Embrace, "[BOT] Shadow's Embrace" },
			{ TRAIT_THIEF_Shadow_Protector, "[TOP] Shadow Protector" },
			{ TRAIT_THIEF_Hidden_Thief, "[MID] Hidden Thief" },
			{ TRAIT_THIEF_Leeching_Venoms, "[BOT] Leeching Venoms" },
			{ TRAIT_THIEF_Cloaked_in_Shadows, "[TOP] Cloaked in Shadows" },
			{ TRAIT_THIEF_Shadows_Rejuvenation, "[MID] Shadow's Rejuvenation" },
			{ TRAIT_THIEF_Rending_Shade, "[BOT] Rending Shade" },
		},
		// Engi Tools
		{
			{ TRAIT_ENGI_Static_Discharge, "[TOP] Static Discharge" },
			{ TRAIT_ENGI_Reactive_Lenses, "[MID] Reactive Lenses" },
			{ TRAIT_ENGI_Power_Wrench, "[BOT] Power Wrench" },
			{ TRAIT_ENGI_Streamlined_Kits, "[TOP] Streamlined Kits" },
			{ TRAIT_ENGI_Lock_On, "[MID] Lock On" },
			{ TRAIT_ENGI_Takedown_Round, "[BOT] Takedown Round" },
			{ TRAIT_ENGI_Kinetic_Battery, "[TOP] Kinetic Battery" },
			{ TRAIT_ENGI_Adrenal_Implant, "[MID] Adrenal Implant" },
			{ TRAIT_ENGI_Gadgeteer, "[BOT] Gadgeteer" },
		},
		// War Defense
		{
			{ TRAIT_WAR_Shield_Master, "[TOP] Shield Master" },
			{ TRAIT_WAR_Dogged_March, "[MID] Dogged March" },
			{ TRAIT_WAR_Cull_the_Weak, "[BOT] Cull the Weak" },
			{ TRAIT_WAR_Defy_Pain, "[TOP] Defy Pain" },
			{ TRAIT_WAR_Armored_Attack, "[MID] Armored Attack" },
			{ TRAIT_WAR_Sundering_Mace, "[BOT] Sundering Mace" },
			{ TRAIT_WAR_Last_Stand, "[TOP] Last Stand" },
			{ TRAIT_WAR_Cleansing_Ire, "[MID] Cleansing Ire" },
			{ TRAIT_WAR_Rousing_Resilience, "[BOT] Rousing Resilience" },
		},
		// Mes Inspiration
		{
			{ TRAIT_MES_Medics_Feedback, "[TOP] Medic's Feedback" },
			{ TRAIT_MES_Restorative_Mantras, "[MID] Restorative Mantras" },
			{ TRAIT_MES_Persisting_Images, "[BOT] Persisting Images" },
			{ TRAIT_MES_Wardens_Feedback, "[TOP] Warden's Feedback" },
			{ TRAIT_MES_Restorative_Illusions, "[MID] Restorative Illusions" },
			{ TRAIT_MES_Protected_Phantasms, "[BOT] Protected Phantasms" },
			{ TRAIT_MES_Mental_Defense, "[TOP] Mental Defense" },
			{ TRAIT_MES_Illusionary_Inspiration, "[MID] Illusionary Inspiration" },
			{ TRAIT_MES_Temporal_Enchanter, "[BOT] Temporal Enchanter" },
		},
		// Mes Illusions
		{
			{ TRAIT_MES_Compunding_Power, "[TOP] Compunding Power" },
			{ TRAIT_MES_Persistence_of_Memory, "[MID] Persistence of Memory" },
			{ TRAIT_MES_The_Pledge, "[BOT] The Pledge" },
			{ TRAIT_MES_Shattered_Strength, "[TOP] Shattered Strength" },
			{ TRAIT_MES_Phantasmal_Haste, "[MID] Phantasmal Haste" },
			{ TRAIT_MES_Maim_the_Disillusioned, "[BOT] Maim the Disillusioned" },
			{ TRAIT_MES_Ineptitude, "[TOP] Ineptitude" },
			{ TRAIT_MES_Master_of_Fragmentation, "[MID] Master of Fragmentation" },
			{ TRAIT_MES_Malicious_Sorcery, "[BOT] Malicious Sorcery" },
		},
		// Ranger Nature Magic
		{
			{ TRAIT_RANGER_Bountiful_Hunter, "[TOP] Bountiful Hunter" },
			{ TRAIT_RANGER_Instinctive_Reaction, "[MID] Instinctive Reaction" },
			{ TRAIT_RANGER_Allies_Aid, "[BOT] Allie's Aid" },
			{ TRAIT_RANGER_Evasive_Purity, "[TOP] Evasive Purity" },
			{ TRAIT_RANGER_Vigorous_Training, "[MID] Vigorous Training" },
			{ TRAIT_RANGER_Windborne_Notes, "[BOT] Windborne Notes" },
			{ TRAIT_RANGER_Natures_Vengeance, "[TOP] Nature's Vengeance" },
			{ TRAIT_RANGER_Protective_Ward, "[MID] Protective Ward" },
			{ TRAIT_RANGER_Invigorating_Bond, "[BOT] Invigorating Bond" },
		},
		// Ele Earth
		{
			{ TRAIT_ELE_Earths_Embrace, "[TOP] Earth's Embrace" },
			{ TRAIT_ELE_Serrated_Stones, "[MID] Serrated Stones" },
			{ TRAIT_ELE_Elemental_Shielding, "[BOT] Elemental Shielding" },
			{ TRAIT_ELE_Strength_of_Stone, "[TOP] Strength of Stone" },
			{ TRAIT_ELE_Rock_Solid, "[MID] Rock Solid" },
			{ TRAIT_ELE_Geomancers_Training, "[BOT] Geomancer's Training" },
			{ TRAIT_ELE_Diamond_Skin, "[TOP] Diamond Skin" },
			{ TRAIT_ELE_Written_in_Stone, "[MID] Written in Stone" },
			{ TRAIT_ELE_Stone_Heart, "[BOT] Stone Heart" },
		},
		// Guard DH
		{
			{ TRAIT_GUARD_Piercing_Light, "[TOP] Piercing Light" },
			{ TRAIT_GUARD_Dulled_Senses, "[MID] Dulled Senses" },
			{ TRAIT_GUARD_Soaring_Devastation, "[BOT] Soaring Devastation" },
			{ TRAIT_GUARD_Hunters_Determination, "[TOP] Hunter's Determination" },
			{ TRAIT_GUARD_Zealots_Aggression, "[MID] Zealot's Aggression" },
			{ TRAIT_GUARD_Bulwark, "[BOT] Bulwark" },
			{ TRAIT_GUARD_Hunters_Fortification, "[TOP] Hunter's Fortification" },
			{ TRAIT_GUARD_Heavy_Light, "[MID] Heavy Light" },
			{ TRAIT_GUARD_Big_Game_Hunter, "[BOT] Big Game Hunter" },
		},
		// Thief Deadly Arts
		{
			{ TRAIT_THIEF_Dagger_Training, "[TOP] Dagger Training" },
			{ TRAIT_THIEF_Mug, "[MID] Mug" },
			{ TRAIT_THIEF_Trappers_Respite, "[BOT] Trapper's Respite" },
			{ TRAIT_THIEF_Deadly_Trapper, "[TOP] Deadly Trapper" },
			{ TRAIT_THIEF_Panic_Strikes, "[MID] Panic Strikes" },
			{ TRAIT_THIEF_Revealed_Training, "[BOT] Revealed Training" },
			{ TRAIT_THIEF_Potent_Poison, "[TOP] Potent Poison" },
			{ TRAIT_THIEF_Improvisation, "[MID] Improvisation" },
			{ TRAIT_THIEF_Executioner, "[BOT] Executioner" },
		},
		// Engi Alchemy
		{
			{ TRAIT_ENGI_Invigorating_Speed, "[TOP] Invigorating Speed" },
			{ TRAIT_ENGI_Protection_Injection, "[MID] Protection Injection" },
			{ TRAIT_ENGI_Health_Insurance, "[BOT] Health Insurance" },
			{ TRAIT_ENGI_Inversion_Enzyme, "[TOP] Inversion Enzyme" },
			{ TRAIT_ENGI_Self_Regulating_Defenses, "[MID] Self-Regulating Defenses" },
			{ TRAIT_ENGI_Backpack_Regenrator, "[BOT] Backpack Regenrator" },
			{ TRAIT_ENGI_HGH, "[TOP] HGH" },
			{ TRAIT_ENGI_Stimulant_Supplier, "[MID] Stimulant Supplier" },
			{ TRAIT_ENGI_Iron_Blooded, "[BOT] Iron Blooded" },
		},
		// Ranger Skirmish
		{
			{ TRAIT_RANGER_Sharpened_Edges, "[TOP] Sharpened Edges" },
			{ TRAIT_RANGER_Primal_Reflexes, "[MID] Primal Reflexes" },
			{ TRAIT_RANGER_Trappers_Expertise, "[BOT] Trapper's Expertise" },
			{ TRAIT_RANGER_Spotter, "[TOP] Spotter" },
			{ TRAIT_RANGER_Striders_Defense, "[MID] Strider's Defense" },
			{ TRAIT_RANGER_Hidden_Barbs, "[BOT] Hidden Barbs" },
			{ TRAIT_RANGER_Quick_Draw, "[TOP] Quick Draw" },
			{ TRAIT_RANGER_Light_on_your_Feet, "[MID] Light on your Feet" },
			{ TRAIT_RANGER_Most_Dangerous_Game, "[BOT] Most Dangerous Game" },
		},
		// Ele Fire
		{
			{ TRAIT_ELE_Burning_Precision, "[TOP] Burning Precision" },
			{ TRAIT_ELE_Conjurer, "[MID] Conjurer" },
			{ TRAIT_ELE_Burning_Fire, "[BOT] Burning Fire" },
			{ TRAIT_ELE_Pyromancers_Training, "[TOP] Pyromancer's Training" },
			{ TRAIT_ELE_One_with_Fire, "[MID] One with Fire" },
			{ TRAIT_ELE_Power_Overwhelming, "[BOT] Power Overwhelming" },
			{ TRAIT_ELE_Persisting_Flames, "[TOP] Persisting Flames" },
			{ TRAIT_ELE_Pyromancers_Puissance, "[MID] Pyromancer's Puissance" },
			{ TRAIT_ELE_Blinding_Ashes, "[BOT] Blinding Ashes" },
		},
		// Ranger Beastmastery
		{
			{ TRAIT_RANGER_Go_for_the_Eyes, "[TOP] Go for the Eyes" },
			{ TRAIT_RANGER_Companions_Might, "[MID] Companion's Might" },
			{ TRAIT_RANGER_Resounding_Timbre, "[BOT] Resounding Timbre" },
			{ TRAIT_RANGER_Wilting_Strike, "[TOP] Wilting Strike" },
			{ TRAIT_RANGER_Two_Handed_Training, "[MID] Two-Handed Training" },
			{ TRAIT_RANGER_Natural_Healing, "[BOT] Natural Healing" },
			{ TRAIT_RANGER_Beastly_Warden, "[TOP] Beastly Warden" },
			{ TRAIT_RANGER_Zephyrs_Speed, "[MID] Zephyr's Speed" },
			{ TRAIT_RANGER_Hones_Axes, "[BOT] Hones Axes" },
		},
		// Ranger Wildreness Survival
		{
			{ TRAIT_RANGER_Soften_the_Fall, "[TOP] Soften the Fall" },
			{ TRAIT_RANGER_Oakheart_Salve, "[MID] Oakheart Salve" },
			{ TRAIT_RANGER_Expertise_Training, "[BOT] Expertise Training" },
			{ TRAIT_RANGER_Ambidexterity, "[TOP] Ambidexterity" },
			{ TRAIT_RANGER_Refined_Toxins, "[MID] Refined Toxins" },
			{ TRAIT_RANGER_Shared_Anguish, "[BOT] Shared Anguish" },
			{ TRAIT_RANGER_Empathic_Bond, "[TOP] Empathic Bond" },
			{ TRAIT_RANGER_Wilderness_Knowledge, "[MID] Wilderness Knowledge" },
			{ TRAIT_RANGER_Poison_Master, "[BOT] Poison Master" },
		},
		// Necro Reaper
		{
			{ TRAIT_NEC_Augury_of_Death, "[TOP] Augury of Death" },
			{ TRAIT_NEC_Chilling_Nova, "[MID] Chilling Nova" },
			{ TRAIT_NEC_Relentless_Pursuit, "[BOT] Relentless Pursuit" },
			{ TRAIT_NEC_Soul_Eater, "[TOP] Soul Eater" },
			{ TRAIT_NEC_Chilling_Victory, "[MID] Chilling Victory" },
			{ TRAIT_NEC_Decimate_Defenses, "[BOT] Decimate Defenses" },
			{ TRAIT_NEC_Blighters_Boon, "[TOP] Blighter's Boon" },
			{ TRAIT_NEC_Deathly_Chill, "[MID] Deathly Chill" },
			{ TRAIT_NEC_Reapers_Onslaught, "[BOT] Reaper's Onslaught" },
		},
		// Thief Crit Strikes
		{
			{ TRAIT_THIEF_Side_Strike, "[TOP] Side Strike" },
			{ TRAIT_THIEF_Signets_of_Power, "[MID] Signets of Power" },
			{ TRAIT_THIEF_Flawless_Strike, "[BOT] Flawless Strike" },
			{ TRAIT_THIEF_Sundering_Strikes, "[TOP] Sundering Strikes" },
			{ TRAIT_THIEF_Practiced_Tolerance, "[MID] Practiced Tolerance" },
			{ TRAIT_THIEF_Ankle_Shots, "[BOT] Ankle Shots" },
			{ TRAIT_THIEF_No_Quarter, "[TOP] No Quarter" },
			{ TRAIT_THIEF_Hidden_Killer, "[MID] Hidden Killer" },
			{ TRAIT_THIEF_Invigorating_Precision, "[BOT] Invigorating Precision" },
		},
		// War Arms
		{
			{ TRAIT_WAR_Berserks_Fury, "[TOP] Berserk's Fury" },
			{ TRAIT_WAR_Signet_Mastery, "[MID] Signet Mastery" },
			{ TRAIT_WAR_Opportunist, "[BOT] Opportunist" },
			{ TRAIT_WAR_Unsuspecting_Foe, "[TOP] Unsuspecting Foe" },
			{ TRAIT_WAR_Deep_Strikes, "[MID] Deep Strikes" },
			{ TRAIT_WAR_Blademaster, "[BOT] Blademaster" },
			{ TRAIT_WAR_Burst_Precision, "[TOP] Burst Precision" },
			{ TRAIT_WAR_Furious, "[MID] Furious" },
			{ TRAIT_WAR_Dual_Wielding, "[BOT] Dual Wielding" },
		},
		// Ele Arcane
		{
			{ TRAIT_ELE_Arcane_Precision, "[TOP] Arcane Precision" },
			{ TRAIT_ELE_Renewing_Stamina, "[MID] Renewing Stamina" },
			{ TRAIT_ELE_Arcane_Abatement, "[BOT] Arcane Abatement" },
			{ TRAIT_ELE_Arcane_Resurrection, "[TOP] Arcane Resurrection" },
			{ TRAIT_ELE_Elemental_Contingency, "[MID] Elemental Contingency" },
			{ TRAIT_ELE_Final_Shielding, "[BOT] Final Shielding" },
			{ TRAIT_ELE_Evasive_Arcana, "[TOP] Evasive Arcana" },
			{ TRAIT_ELE_Elemental_Surge, "[MID] Elemental Surge" },
			{ TRAIT_ELE_Bountiful_Power, "[BOT] Bountiful Power" },
		},
		// Engi Firearms
		{
			{ TRAIT_ENGI_Chemical_Rounds, "[TOP] Chemical Rounds" },
			{ TRAIT_ENGI_Heavy_Armor_Exploit, "[MID] Heavy Armor Exploit" },
			{ TRAIT_ENGI_High_Caliber, "[BOT] High Caliber" },
			{ TRAIT_ENGI_Pinpoint_Distribution, "[TOP] Pinpoint Distribution" },
			{ TRAIT_ENGI_Skilled_Marksman, "[MID] Skilled Marksman" },
			{ TRAIT_ENGI_No_Scope, "[BOT] No Scope" },
			{ TRAIT_ENGI_Juggernaut, "[TOP] Juggernaut" },
			{ TRAIT_ENGI_Modified_Ammunition, "[MID] Modified Ammunition" },
			{ TRAIT_ENGI_Incendiary_Powder, "[BOT] Incendiary Powder" },
		},
		// Necro Curses
		{
			{ TRAIT_NEC_Terrifying_Descent, "[TOP] Terrifying Descent" },
			{ TRAIT_NEC_Plague_Sending, "[MID] Plague Sending" },
			{ TRAIT_NEC_Chilling_Darkness, "[BOT] Chilling Darkness" },
			{ TRAIT_NEC_Master_of_Corruption, "[TOP] Master of Corruption" },
			{ TRAIT_NEC_Path_of_Corruption, "[MID] Path of Corruption" },
			{ TRAIT_NEC_Terror, "[BOT] Terror" },
			{ TRAIT_NEC_Weakening_Shroud, "[TOP] Weakening Shroud" },
			{ TRAIT_NEC_Parasitic_Contagion, "[MID] Parasitic Contagion" },
			{ TRAIT_NEC_Lingering_Curse, "[BOT] Lingering Curse" },
		},
		// Mes Chrono
		{
			{ TRAIT_MES_Delayed_Reactions, "[TOP] Delayed Reactions" },
			{ TRAIT_MES_Time_Catches_Up, "[MID] Time Catches Up" },
			{ TRAIT_MES_Alls_Well_That_Ends_Well, "[BOT] All's Well That Ends Well" },
			{ TRAIT_MES_Danger_Time, "[TOP] Danger Time" },
			{ TRAIT_MES_Illusionairy_Reversion, "[MID] Illusionairy Reversion" },
			{ TRAIT_MES_Improved_Alacrity, "[BOT] Improved Alacrity" },
			{ TRAIT_MES_Lost_Time, "[TOP] Lost Time" },
			{ TRAIT_MES_Seize_the_Moment, "[MID] Seize the Moment" },
			{ TRAIT_MES_Chronophantasma, "[BOT] Chronophantasma" },
		},
		// Ele Air
		{
			{ TRAIT_ELE_Zephyrs_Boon, "[TOP] Zephyr's Boon" },
			{ TRAIT_ELE_One_with_Air, "[MID] One with Air" },
			{ TRAIT_ELE_Ferocious_Winds, "[BOT] Ferocious Winds" },
			{ TRAIT_ELE_Inscription, "[TOP] Inscription" },
			{ TRAIT_ELE_Aeromancers_Training, "[MID] Aeromancer's Training" },
			{ TRAIT_ELE_Tempest_Defense, "[BOT] Tempest Defense" },
			{ TRAIT_ELE_Bolt_to_the_Heart, "[TOP] Bolt to the Heart" },
			{ TRAIT_ELE_Fresh_Air, "[MID] Fresh Air" },
			{ TRAIT_ELE_Lightning_Rod, "[BOT] Lightning Rod" },
		},
		// Guard Zeal
		{
			{ TRAIT_GUARD_Wrathful_Spirit, "[TOP] Wrathful Spirit" },
			{ TRAIT_GUARD_Fiery_Wrath, "[MID] Fiery Wrath" },
			{ TRAIT_GUARD_Zealous_Scepter, "[BOT] Zealous Scepter" },
			{ TRAIT_GUARD_Blinding_Jeopardy, "[TOP] Blinding Jeopardy" },
			{ TRAIT_GUARD_Zealous_Blade, "[MID] Zealous Blade" },
			{ TRAIT_GUARD_Kindles_Zeal, "[BOT] Kindles Zeal" },
			{ TRAIT_GUARD_Expeditious_Spirit, "[TOP] Expeditious Spirit" },
			{ TRAIT_GUARD_Shattered_Aegis, "[MID] Shattered Aegis" },
			{ TRAIT_GUARD_Symbolic_Avenger, "[BOT] Symbolic Avenger" },
		},
		// Engi Scrapper
		{
			{ TRAIT_ENGI_Shocking_Speed, "[TOP] Shocking Speed" },
			{ TRAIT_ENGI_Perfectly_Weighted, "[MID] Perfectly Weighted" },
			{ TRAIT_ENGI_Recovery_Matrix, "[BOT] Recovery Matrix" },
			{ TRAIT_ENGI_Rapid_Regenration, "[TOP] Rapid Regenration" },
			{ TRAIT_ENGI_Expert_Examination, "[MID] Expert Examination" },
			{ TRAIT_ENGI_Mass_Momentum, "[BOT] Mass Momentum" },
			{ TRAIT_ENGI_Adaptive_Armor, "[TOP] Adaptive Armor" },
			{ TRAIT_ENGI_Final_Salvo, "[MID] Final Salvo" },
			{ TRAIT_ENGI_Applied_Force, "[BOT] Applied Force" },
		},
		// Thief Trickery
		{
			{ TRAIT_THIEF_Uncatchable, "[TOP] Uncatchable" },
			{ TRAIT_THIEF_Flanking_Strikes, "[MID] Flanking Strikes" },
			{ TRAIT_THIEF_Thrill_of_the_Crime, "[BOT] Thrill of the Crime" },
			{ TRAIT_THIEF_Bountiful_Theft, "[TOP] Bountiful Theft" },
			{ TRAIT_THIEF_Trickster, "[MID] Trickster" },
			{ TRAIT_THIEF_Pressure_Striking, "[BOT] Pressure Striking" },
			{ TRAIT_THIEF_Quick_Pockets, "[TOP] Quick Pockets" },
			{ TRAIT_THIEF_Sleight_of_Hand, "[MID] Sleight of Hand" },
			{ TRAIT_THIEF_Bewildering_Ambush, "[BOT] Bewildering Ambush" },
		},
		// Mes Chaos
		{
			{ TRAIT_MES_Descent_into_Madness, "[TOP] Descent into Madness" },
			{ TRAIT_MES_Illusionary_Defenses, "[MID] Illusionary Defenses" },
			{ TRAIT_MES_Master_of_Manipulation, "[BOT] Master of Manipulation" },
			{ TRAIT_MES_Mirror_of_Anguish, "[TOP] Mirror of Anguish" },
			{ TRAIT_MES_Chaotic_Transference, "[MID] Chaotic Transference" },
			{ TRAIT_MES_Chaotic_Dampening, "[BOT] Chaotic Dampening" },
			{ TRAIT_MES_Chaotic_Interruption, "[TOP] Chaotic Interruption" },
			{ TRAIT_MES_Prismatic_Understanding, "[MID] Prismatic Understanding" },
			{ TRAIT_MES_Bountiful_Disillusionment, "[BOT] Bountiful Disillusionment" },
		},
		// Guard Virtues
		{
			{ TRAIT_GUARD_Unscathed_Contender, "[TOP] Unscathed Contender" },
			{ TRAIT_GUARD_Retaliatory_Subconscious, "[MID] Retaliatory Subconscious" },
			{ TRAIT_GUARD_Master_of_Consecrations, "[BOT] Master of Consecrations" },
			{ TRAIT_GUARD_Virtuous_Solace, "[TOP] Virtuous Solace" },
			{ TRAIT_GUARD_Absolute_Resolution, "[MID] Absolute Resolution" },
			{ TRAIT_GUARD_Glacial_Heart, "[BOT] Glacial Heart" },
			{ TRAIT_GUARD_Permeating_Wrath, "[TOP] Permeating Wrath" },
			{ TRAIT_GUARD_Battle_Presence, "[MID] Battle Presence" },
			{ TRAIT_GUARD_Indomitable_Courage, "[BOT] Indomitable Courage" },
		},
		// Engi Inventions
		{
			{ TRAIT_ENGI_Over_Shield, "[TOP] Over Shield" },
			{ TRAIT_ENGI_Automated_Medical_Response, "[MID] Automated Medical Response" },
			{ TRAIT_ENGI_Autodefense_Bomb_Dispenser, "[BOT] Autodefense Bomb Dispenser" },
			{ TRAIT_ENGI_Experimental_Turrets, "[TOP] Experimental Turrets" },
			{ TRAIT_ENGI_Soothing_Detonation, "[MID] Soothing Detonation" },
			{ TRAIT_ENGI_Mecha_Legs, "[BOT] Mecha Legs" },
			{ TRAIT_ENGI_Advanced_Turrets, "[TOP] Advanced Turrets" },
			{ TRAIT_ENGI_Bunker_Down, "[MID] Bunker Down" },
			{ TRAIT_ENGI_Medical_Disersion_Field, "[BOT] Medical Disersion Field" },
		},
		// Ele Tempest
		{
			{ TRAIT_ELE_Gale_Song, "[TOP] Gale Song" },
			{ TRAIT_ELE_Latent_Stamina, "[MID] Latent Stamina" },
			{ TRAIT_ELE_Unstable_Conduit, "[BOT] Unstable Conduit" },
			{ TRAIT_ELE_Tempestuous_Aria, "[TOP] Tempestuous Aria" },
			{ TRAIT_ELE_Invigorating_Torrents, "[MID] Invigorating Torrents" },
			{ TRAIT_ELE_Harmonious_Conduit, "[BOT] Harmonious Conduit" },
			{ TRAIT_ELE_Imbued_Melodies, "[TOP] Imbued Melodies" },
			{ TRAIT_ELE_Lucid_Singularity, "[MID] Lucid Singularity" },
			{ TRAIT_ELE_Elemental_Bastion, "[BOT] Elemental Bastion" },
		},
		// Guard Honor
		{
			{ TRAIT_GUARD_Invigorated_Bulwark, "[TOP] Invigorated Bulwark" },
			{ TRAIT_GUARD_Protective_Reviver, "[MID] Protective Reviver" },
			{ TRAIT_GUARD_Protectors_Impact, "[BOT] Protector's Impact" },
			{ TRAIT_GUARD_Honorable_Staff, "[TOP] Honorable Staff" },
			{ TRAIT_GUARD_Pure_of_Heart, "[MID] Pure of Heart" },
			{ TRAIT_GUARD_Empowering_Might, "[BOT] Empowering Might" },
			{ TRAIT_GUARD_Pure_of_Voice, "[TOP] Pure of Voice" },
			{ TRAIT_GUARD_Writ_of_Persistence, "[MID] Writ of Persistence" },
			{ TRAIT_GUARD_Force_of_Will, "[BOT] Force of Will" },
		},
		// Necro Soul Reaping
		{
			{ TRAIT_NEC_Unyielding_Blast, "[TOP] Unyielding Blast" },
			{ TRAIT_NEC_Soul_Marks, "[MID] Soul Marks" },
			{ TRAIT_NEC_Speed_of_Shadows, "[BOT] Speed of Shadows" },
			{ TRAIT_NEC_Spectral_Mastery, "[TOP] Spectral Mastery" },
			{ TRAIT_NEC_Vital_Persistence, "[MID] Vital Persistence" },
			{ TRAIT_NEC_Fear_of_Death, "[BOT] Fear of Death" },
			{ TRAIT_NEC_Foot_in_the_Grave, "[TOP] Foot in the Grave" },
			{ TRAIT_NEC_Death_Perception, "[MID] Death Perception" },
			{ TRAIT_NEC_Dhummfire, "[BOT] Dhummfire" },
		},
		// War Discipline
		{
			{ TRAIT_WAR_Crack_Shot, "[TOP] Crack Shot" },
			{ TRAIT_WAR_Warriors_Sprint, "[MID] Warrior's Sprint" },
			{ TRAIT_WAR_Vengeful_Return, "[BOT] Vengeful Return" },
			{ TRAIT_WAR_Inspiring_Battle_Standard, "[TOP] Inspiring Battle Standard" },
			{ TRAIT_WAR_Destruction_of_the_Empowered, "[MID] Destruction of the Empowered" },
			{ TRAIT_WAR_Brawlers_Recovery, "[BOT] Brawler's Recovery" },
			{ TRAIT_WAR_Merciless_Hammer, "[TOP] Merciless Hammer" },
			{ TRAIT_WAR_Heightened_Focus, "[MID] Heightened Focus" },
			{ TRAIT_WAR_Burst_Mastery, "[BOT] Burst Mastery" },
		},
		// Rev Herald
		{
			{ TRAIT_REV_Swift_Gale, "[TOP] Swift Gale" },
			{ TRAIT_REV_Radiant_Revival, "[MID] Radiant Revival" },
			{ TRAIT_REV_Hardening_Persistence, "[BOT] Hardening Persistence" },
			{ TRAIT_REV_Bolster_Fortifications, "[TOP] Bolster Fortifications" },
			{ TRAIT_REV_Shared_Empowerment, "[MID] Shared Empowerment" },
			{ TRAIT_REV_Harmonize_Continuity, "[BOT] Harmonize Continuity" },
			{ TRAIT_REV_Elders_Force, "[TOP] Elder's Force" },
			{ TRAIT_REV_Soothing_Bastion, "[MID] Soothing Bastion" },
			{ TRAIT_REV_Enhanced_Bulwark, "[BOT] Enhanced Bulwark" },
		},
		// Necro Spite
		{
			{ TRAIT_NEC_Spiteful_Talisman, "[TOP] Spiteful Talisman" },
			{ TRAIT_NEC_Spiteful_Renewal, "[MID] Spiteful Renewal" },
			{ TRAIT_NEC_Bitter_Chill, "[BOT] Bitter Chill" },
			{ TRAIT_NEC_Chill_of_Death, "[TOP] Chill of Death" },
			{ TRAIT_NEC_Rending_Shroud, "[MID] Rending Shroud" },
			{ TRAIT_NEC_Unholy_Fervor, "[BOT] Unholy Fervor" },
			{ TRAIT_NEC_Signets_of_Suffering, "[TOP] Signets of Suffering" },
			{ TRAIT_NEC_Close_to_Death, "[MID] Close to Death" },
			{ TRAIT_NEC_Spitful_Spirit, "[BOT] Spitful Spirit" },
		},
		// Thief Acro
		{
			{ TRAIT_THIEF_Instant_Reflexes, "[TOP] Instant Reflexes" },
			{ TRAIT_THIEF_Vigorous_Recovery, "[MID] Vigorous_Recovery" },
			{ TRAIT_THIEF_Pain_Response, "[BOT] Pain Response" },
			{ TRAIT_THIEF_Guarded_Initiation, "[TOP] Guarded Initiation" },
			{ TRAIT_THIEF_Swindlers_Equilibrium, "[MID] Swindler's Equilibrium" },
			{ TRAIT_THIEF_Hard_to_Catch, "[BOT] Hard to Catch" },
			{ TRAIT_THIEF_Assassins_Reward, "[TOP] Assassin's Reward" },
			{ TRAIT_THIEF_Upper_Hand, "[MID] Upper Hand" },
			{ TRAIT_THIEF_Dont_Stop, "[BOT] Dont Stop" },
		},
	};


	if (id == TRAIT_NONE)
		return traitDB[SPEC_NONE][0].name;

	if (spec > 0 && spec < SPEC_END) {
		const IdNamePair *specTraits = traitDB[spec];
		for (u32 i = 0; i < 9; i++)
		{
			if (specTraits[i].id == id)
				return specTraits[i].name;
		}
	}

	wsprintfA(&idstr[0], "%06d", id);
	return idstr;
}