/* The game's original data for menus, screens, shop, text, fonts and the window (src\ui).
 * Ordinary typed C objects (their types: data.h; records of several values: records.h); no object depends on where another one lives. */
#include "../rr.h"
#include "data.h"
extern char RRT_SoundMix[]; __pragma(comment(linker, "/alternatename:_RRT_SoundMix=SoundMix"))
extern char RRT_StartupSpritesLoad[]; __pragma(comment(linker, "/alternatename:_RRT_StartupSpritesLoad=StartupSpritesLoad"))
extern char RRT_PlayerAiModeAttack[]; __pragma(comment(linker, "/alternatename:_RRT_PlayerAiModeAttack=PlayerAiModeAttack"))
extern char RRT_PlayerAiModeChase[]; __pragma(comment(linker, "/alternatename:_RRT_PlayerAiModeChase=PlayerAiModeChase"))
extern char RRT_CheatNamesApply[]; __pragma(comment(linker, "/alternatename:_RRT_CheatNamesApply=CheatNamesApply"))
extern char RRT_CheatsEnable[]; __pragma(comment(linker, "/alternatename:_RRT_CheatsEnable=CheatsEnable"))
extern char RRT_CheatsDisable[]; __pragma(comment(linker, "/alternatename:_RRT_CheatsDisable=CheatsDisable"))
extern char RRT_ChaserDrop[]; __pragma(comment(linker, "/alternatename:_RRT_ChaserDrop=ChaserDrop"))
extern char RRT_UnusedControlRowAction[]; __pragma(comment(linker, "/alternatename:_RRT_UnusedControlRowAction=UnusedControlRowAction"))
extern char RRT_ConfigureInputOpen[]; __pragma(comment(linker, "/alternatename:_RRT_ConfigureInputOpen=ConfigureInputOpen"))
extern char RRT_UnusedControlRowAction2[]; __pragma(comment(linker, "/alternatename:_RRT_UnusedControlRowAction2=UnusedControlRowAction2"))
extern char RRT_MenuStartPressed[]; __pragma(comment(linker, "/alternatename:_RRT_MenuStartPressed=MenuStartPressed"))
extern char RRT_MenuItemAction[]; __pragma(comment(linker, "/alternatename:_RRT_MenuItemAction=MenuItemAction"))
extern char RRT_CharacterSelect[]; __pragma(comment(linker, "/alternatename:_RRT_CharacterSelect=CharacterSelect"))
extern char RRT_MenuNextScreen[]; __pragma(comment(linker, "/alternatename:_RRT_MenuNextScreen=MenuNextScreen"))
extern char RRT_ResultPageStep[]; __pragma(comment(linker, "/alternatename:_RRT_ResultPageStep=ResultPageStep"))
extern char RRT_ResultPageNext[]; __pragma(comment(linker, "/alternatename:_RRT_ResultPageNext=ResultPageNext"))
extern char RRT_LevelPageOpen[]; __pragma(comment(linker, "/alternatename:_RRT_LevelPageOpen=LevelPageOpen"))
extern char RRT_ChoiceScreenPick[]; __pragma(comment(linker, "/alternatename:_RRT_ChoiceScreenPick=ChoiceScreenPick"))
extern char RRT_BikeShopBuy[]; __pragma(comment(linker, "/alternatename:_RRT_BikeShopBuy=BikeShopBuy"))
extern char RRT_BikeShopTradeIn[]; __pragma(comment(linker, "/alternatename:_RRT_BikeShopTradeIn=BikeShopTradeIn"))
extern char RRT_ShopPageAFlag[]; __pragma(comment(linker, "/alternatename:_RRT_ShopPageAFlag=ShopPageAFlag"))
extern char RRT_BikeClassSelect[]; __pragma(comment(linker, "/alternatename:_RRT_BikeClassSelect=BikeClassSelect"))
extern char RRT_GameScreenLeave[]; __pragma(comment(linker, "/alternatename:_RRT_GameScreenLeave=GameScreenLeave"))
extern char RRT_UnusedNetOptionsClose[]; __pragma(comment(linker, "/alternatename:_RRT_UnusedNetOptionsClose=UnusedNetOptionsClose"))
extern char RRT_UnusedNetOptionsAction[]; __pragma(comment(linker, "/alternatename:_RRT_UnusedNetOptionsAction=UnusedNetOptionsAction"))
extern char RRT_UnusedNetFlagsUpdate[]; __pragma(comment(linker, "/alternatename:_RRT_UnusedNetFlagsUpdate=UnusedNetFlagsUpdate"))
extern char RRT_UnusedNetOptionItemA[]; __pragma(comment(linker, "/alternatename:_RRT_UnusedNetOptionItemA=UnusedNetOptionItemA"))
extern char RRT_UnusedNetOptionItemB[]; __pragma(comment(linker, "/alternatename:_RRT_UnusedNetOptionItemB=UnusedNetOptionItemB"))
extern char RRT_UnusedNetTick[]; __pragma(comment(linker, "/alternatename:_RRT_UnusedNetTick=UnusedNetTick"))
extern char RRT_ThrashModePick[]; __pragma(comment(linker, "/alternatename:_RRT_ThrashModePick=ThrashModePick"))
extern char RRT_BigGameModePick[]; __pragma(comment(linker, "/alternatename:_RRT_BigGameModePick=BigGameModePick"))
extern char RRT_GameLoad[]; __pragma(comment(linker, "/alternatename:_RRT_GameLoad=GameLoad"))
extern char RRT_RestroomBlankItem[]; __pragma(comment(linker, "/alternatename:_RRT_RestroomBlankItem=RestroomBlankItem"))
extern char RRT_GameSave[]; __pragma(comment(linker, "/alternatename:_RRT_GameSave=GameSave"))
extern char RRT_SelectCharacterOpen[]; __pragma(comment(linker, "/alternatename:_RRT_SelectCharacterOpen=SelectCharacterOpen"))
extern char RRT_MenuDoneAction[]; __pragma(comment(linker, "/alternatename:_RRT_MenuDoneAction=MenuDoneAction"))
extern char RRT_OptionPageClose[]; __pragma(comment(linker, "/alternatename:_RRT_OptionPageClose=OptionPageClose"))
extern char RRT_GameModePageOpen[]; __pragma(comment(linker, "/alternatename:_RRT_GameModePageOpen=GameModePageOpen"))
extern char RRT_LevelPageOpenList[]; __pragma(comment(linker, "/alternatename:_RRT_LevelPageOpenList=LevelPageOpenList"))
extern char RRT_PlayerNameStore[]; __pragma(comment(linker, "/alternatename:_RRT_PlayerNameStore=PlayerNameStore"))
extern char RRT_PlayerNamePageOpen[]; __pragma(comment(linker, "/alternatename:_RRT_PlayerNamePageOpen=PlayerNamePageOpen"))
extern char RRT_JukeboxPageItem[]; __pragma(comment(linker, "/alternatename:_RRT_JukeboxPageItem=JukeboxPageItem"))
extern char RRT_JukeboxSongPick[]; __pragma(comment(linker, "/alternatename:_RRT_JukeboxSongPick=JukeboxSongPick"))
extern char RRT_OptionPageSelect[]; __pragma(comment(linker, "/alternatename:_RRT_OptionPageSelect=OptionPageSelect"))
extern char RRT_OptionPageNext[]; __pragma(comment(linker, "/alternatename:_RRT_OptionPageNext=OptionPageNext"))
extern char RRT_JukeboxPageOpen[]; __pragma(comment(linker, "/alternatename:_RRT_JukeboxPageOpen=JukeboxPageOpen"))
extern char RRT_ResolutionPageOpen[]; __pragma(comment(linker, "/alternatename:_RRT_ResolutionPageOpen=ResolutionPageOpen"))
extern char RRT_BikersPageOpen[]; __pragma(comment(linker, "/alternatename:_RRT_BikersPageOpen=BikersPageOpen"))
extern char RRT_VideoModePageOpen[]; __pragma(comment(linker, "/alternatename:_RRT_VideoModePageOpen=VideoModePageOpen"))
char s_Matte[8] = "Matte";
char s_Video[8] = "Video/";
char s_dotAVI[8] = ".AVI";
char s_pctspctd[8] = "%s%d";
Rec_cheatsSolo rec_cheatsSolo = {
    /* tbl_cheatsSolo */ { (unsigned)RRT_CheatsEnable, { 0,0,0,0 }, (unsigned)((unsigned char *)&s_avivideo + 192), { 0,0,0,0 }, (unsigned)RRT_CheatsDisable, { 0,0,0,0 } },
    /* g_cheatTable */ 0,
    /* tbl_cheatHandlers */ {
        0,(unsigned)RRT_ChaserDrop,0,(unsigned)((unsigned char *)&s_avivideo + 164),0,(unsigned)RRT_ChaserDrop,0,
        (unsigned)((unsigned char *)&s_avivideo + 148),0,(unsigned)RRT_StartupSpritesLoad,0,(unsigned)((unsigned char *)&s_avivideo + 132),0,
        (unsigned)RRT_PlayerAiModeAttack,0,(unsigned)((unsigned char *)&s_avivideo + 116),0,(unsigned)RRT_PlayerAiModeChase,0,
        (unsigned)((unsigned char *)&s_avivideo + 96)
    }
};
struct L_tbl_cheats tbl_cheats = { { 0,0,0,0 }, (unsigned)&g_songMessages.tbl_cheatFlags, (unsigned)((unsigned char *)&s_avivideo + 84), { 0,0,0,0,0,0,0,0 }, (unsigned)&g_demoMode, (unsigned)((unsigned char *)&s_avivideo + 60), { 0,0,0,0 }, (unsigned)RRT_CheatNamesApply, (unsigned)&g_useNamesB, (unsigned)((unsigned char *)&s_avivideo + 48), { 0,0,0,0 }, (unsigned)RRT_CheatNamesApply, (unsigned)&g_songState.g_useNamesA, (unsigned)((unsigned char *)&s_avivideo + 32), { 0,0,0,0 }, (unsigned)RRT_CheatNamesApply, (unsigned)&g_songState.g_decompState, (unsigned)((unsigned char *)&s_avivideo + 12), { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } };
Rec_laneBikeIds rec_laneBikeIds = {
    /* tbl_laneBikeIds */ {
        191,0,0,0,109,1,0,0,104,1,0,0,108,1,0,0,106,1,0,0,105,1,0,0,107,1,0,0,103,1,0,0,102,1
    },
    /* g_copVariant */ 0,
    /* tbl_trackIds */ {0},
    /* tbl_thrashBikes */ {
        1,0,12,6,9,1,3,12,8,7,2,14,10,9,7,4,14,6,5,7,4,3,12,8,7,1,3,10,5,7,1,3,12,8,7,2,14,6,9,7,1,3,12,8,7,4,0,13,6,9,1,3,12,8,7,4,0,12,6,9,1,3,12,8,
        7,4,0,13,6,9,1,3,12,8,7,4,0,13,6,9
    }
};
Rec_controlRows rec_controlRows = {
    /* tbl_controlRows */ {0},
    /* g_controlRowsLink */ {
        0x100,0,0x78,0xf0,0x1e,0x10b0000,0xb01,0,0x1,(unsigned)RRT_UnusedControlRowAction,0,0,0,0,0x101,0,0x96,0xf0,0x1e,0x2000101,0x2,0,0x1,
        (unsigned)RRT_UnusedControlRowAction,0,0,0,0,0x102,0,0xb4,0xf0,0x1e,0x3010202,0x103,0,0x1,(unsigned)RRT_UnusedControlRowAction,0,0,0,0,0x103,0,0xd2,
        0xf0,0x1e,0x4020303,0x204,0,0x1,(unsigned)RRT_UnusedControlRowAction,0,0,0,0,0x104,0,0xf0,0xf0,0x1e,0x5030404,0x305,0,0x1,
        (unsigned)RRT_UnusedControlRowAction,0,0,0,0,0x105,0,0x10e,0xf0,0x1e,0x6040505,0x406,0,0x1,(unsigned)RRT_UnusedControlRowAction,0,0,0,0,0x106,0,
        0x12c,0xf0,0x1e,0x7050606,0x507,0,0x1,(unsigned)RRT_UnusedControlRowAction,0,0,0,0,0x107,0,0x14a,0xf0,0x1e,0x8060707,0x608,0,0x1,
        (unsigned)RRT_UnusedControlRowAction,0,0,0,0,0x108,0,0x168,0xf0,0x1e,0x9070808,0x709,0,0x1,(unsigned)RRT_UnusedControlRowAction,0,0,0,0,0x17e,0,
        0x186,0xf0,0x1e,0xa080909,0x80a,0,0x1,(unsigned)RRT_UnusedControlRowAction2,0,0,0,0,0x17f,0,0x1a4,0xf0,0x1e,0xb090a0a,0x90b,0,0x1,
        (unsigned)RRT_UnusedControlRowAction2,0,0,0,0x10,0x56,0,0x1c2,0xf0,0x1e,0xa0b0b,0xa00,0x8000,0x1,0,0,0,0,0,0,0,0x72,0xf0,0x6,0,0,0,
        0,0,0,0,0,0,0xa0,0x104,0x78,0x168,0x1e,0,0,0x10000,0,0,0,0,0,0,0xa1,0x104,0x96,0x168,0x1e,0x1010101,0x101,0x10000,
        0,0,0,0,0,0,0xa2,0x104,0xb4,0x168,0x1e,0x2020202,0x202,0x10000,0,0,0,0,0,0,0xa3,0x104,0xd2,0x168,0x1e,0x3030303,
        0x303,0x10000,0,0,0,0,0,0,0xa4,0x104,0xf0,0x168,0x1e,0x4040404,0x404,0x10000,0,0,0,0,0,0,0xa5,0x104,0x10e,0x168,
        0x1e,0x5050505,0x505,0x10000,0,0,0,0,0,0,0xa6,0x104,0x12c,0x168,0x1e,0x6060606,0x606,0x10000,0,0,0,0,0,0,0xa7,0x104,
        0x14a,0x168,0x1e,0x7070707,0x707,0x10000,0,0,0,0,0,0,0xa8,0x104,0x168,0x168,0x1e,0x8080808,0x808,0x10000,0,0,0,0
    }
};
int g_controlsMenuSel = 22;
Rec_keyBindings rec_keyBindings = {
    /* tbl_keyBindings */ 0,
    /* tbl_keyBindingsKey */ {
        -108,0,-106,0,-109,0,-124,0,110,0,-123,0,13,0,32,0,100,0,-107,0,-127
    },
    /* tbl_keyBindingsEnd */ {
        0,-108,0,-106,0,-109,0,-107,37,0,39,0,40,0,38,0,0,0,1,1
    }
};
unsigned tbl_keyNamesDefault[66] = {
    0,0,0x908,0xd80,0x929190,0,0x1b000000,0,0x83828120,0x95949384,0x96,0x868500,0x33323130,0x37363534,0x3938,0,0x63626100,0x67666564,0x6b6a6968,
    0x6f6e6d6c,0x73727170,0x77767574,0x7a7978,0,0x82968385,0x84958093,0x2b2a8194,0x2f862d00,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x3d3b0000,0x2f2e2d2c,
    0x60,0,0,0,0,0,0x5b000000,0x275d5c
};
unsigned tbl_keyNames7[66] = {
    0,0,0x908,0xd80,0x929190,0,0x1b000000,0,0x83828120,0x95949384,0x96,0x868500,0x33323130,0x37363534,0x3938,0,0x63626100,0x67666564,0x6b6a6968,
    0x6f6e6d6c,0x73727170,0x77767574,0x7a7978,0,0x82968385,0x84958093,0x2b2a8194,0xf7862d00,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x2bfc0000,0x232e2d2c,
    0xf6,0,0,0,0,0,0xdf000000,0xe4b45e,0x3c0000
};
unsigned tbl_keyNames12[66] = {
    0,0,0x908,0xd80,0x929190,0,0x1b000000,0,0x83828120,0x95949384,0x96,0x868500,0x33323130,0x37363534,0x3938,0,0x63626100,0x67666564,0x6b6a6968,
    0x6f6e6d6c,0x73727170,0x77767574,0x7a7978,0,0x82968385,0x84958093,0x2b2a8194,0x2f862d00,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x3d240000,0x3a3b2d2c,
    0x25,0,0,0,0,0,0x29000000,0x21b25eb5,0x3c0000
};
unsigned tbl_keyNames16[66] = {
    0,0,0x908,0xd80,0x929190,0,0x1b000000,0,0x83828120,0x95949384,0x96,0x868500,0x33323130,0x37363534,0x3938,0,0x63626100,0x67666564,0x6b6a6968,
    0x6f6e6d6c,0x73727170,0x77767574,0x7a7978,0,0x82968385,0x84958093,0x2b2a8194,0x2f862d00,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x5d5b0000,0xf92e2d2c,
    0x60,0,0,0,0,0,0xb4000000,0x23ec5c,0x3c0000
};
unsigned tbl_keyNames10[66] = {
    0,0,0x908,0xd80,0x929190,0,0x1b000000,0,0x83828120,0x95949384,0x96,0x868500,0x33323130,0x37363534,0x3938,0,0x63626100,0x67666564,0x6b6a6968,
    0x6f6e6d6c,0x73727170,0x77767574,0x7a7978,0,0x82968385,0x84958093,0x2b2a8194,0x2f862d00,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x5d5b0000,0x7d2e2d2c,
    0xf1,0,0,0,0,0,0xb4000000,0x7ba15c,0x3c0000
};
unsigned tbl_keyNames11[78] = {
    0,0,0x908,0xd80,0x929190,0,0x1b000000,0,0x83828120,0x95949384,0x96,0x868500,0x33323130,0x37363534,0x3938,0,0x63626100,0x67666564,0x6b6a6968,
    0x6f6e6d6c,0x73727170,0x77767574,0x7a7978,0,0x82968385,0x84958093,0x2b2a8194,0xf7862d00,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0x5c7e0000,0x602e2d2c,
    0xf6,0,0,0,0,0,0xb4000000,0xe4e5a7,0x3c0000,0,0,0,0,0,0,0,0,0,0x96009400,0x84009300,0x85006e00,0x20000d00,0x95006400,0x8100,0x270025,0x93000028,
    0x16e00,0x20000002,0x269400,0x9500
};
Rec_comm_datamodem rec_comm_datamodem = {
    /* s_comm_datamodem */ "comm/datamodem",
    /* s_TapiComm */ "TapiComm",
    /* s_helpFile */ {0},
    /* s_CreateCallParams */ "CreateCallParams: ",
    /* s_lineGetID */ "lineGetID: ",
    /* s_lineGetDevCaps */ "lineGetDevCaps",
    /* s_lineGetAddressStatus */ "lineGetAddressStatus",
    /* s_lineGetCallStatus */ "lineGetCallStatus",
    /* s_lineGetAddressCaps */ "lineGetAddressCaps",
    /* s_CONTROL_EXE_MODEM_CPL_ADD */ {
        67,79,78,84,82,79,76,46,69,88,69,32,77,79,68,69,77,46,67,80,76,44,44,65,68,68,0,0,65,110,115,119,101,114
    }
};
Rec_comm rec_comm = {
    /* s_comm */ "comm",
    /* s_VerifyUsableLine_lineGetID */ "VerifyUsableLine:lineGetID: ",
    /* s_lpt */ "lpt",
    /* s_Invalid_Location_Selected */ "Invalid Location Selected",
    /* s_FillLocationInfo */ "FillLocationInfo",
    /* s_pcts */ " (%s)",
    /* s_lineTranslateOutput */ "lineTranslateOutput",
    /* s_Redial */ "Redial",
    /* s_Country */ "Country",
    /* s_Number */ "Number",
    /* s_AreaCode */ "AreaCode",
    /* s_Name */ "Name",
    /* s_RoadRash */ "RoadRash",
    /* s_Entrypct04d */ "Entry%04d",
    /* s_Phonebook */ "Phonebook",
    /* s_pcts_pctlu */ "%s (%lu)",
    /* s_FillCountryCodeList */ "FillCountryCodeList",
    /* g_directSound */ 0,
    /* g_soundInit */ {0},
    /* s_Error */ "Error",
    /* s_Could_not_set_format */ "Could not set format",
    /* s_Error_loading_wavefile */ "Error loading wavefile",
    /* s_File_corrupt_or_doesn_t_exist */ "File corrupt or doesn't exist",
    /* g_vxdHandle */ (HANDLE)0xffffffff,
    /* g_staticPool */ {
        1
    },
    /* s_Lalloc_VXD */ "\\\\.\\Lalloc.VXD",
    /* g_streamList */ 0,
    /* s_pcts_2 */ {
        37,115,124,0,0,0,0,0,0,0,0,0,0,0,-16,63
    }
};
char s_Bikes_Super_Diablo[20] = "Bikes/Super/Diablo";
char s_Bikes_Super_Banzai[20] = "Bikes/Super/Banzai";
char s_Bikes_Sport_Perro[20] = "Bikes/Sport/Perro";
char s_Bikes_Sport_DMG[16] = "Bikes/Sport/DMG";
char s_Bikes_Sport_Stiletto[24] = "Bikes/Sport/Stiletto";
char s_Bikes_Sport_Kamikaze[24] = "Bikes/Sport/Kamikaze";
char s_Bikes_Sport_Diablo[20] = "Bikes/Sport/Diablo";
char s_Bikes_Rat_Corsair[20] = "Bikes/Rat/Corsair";
char s_Bikes_Rat_Perro[16] = "Bikes/Rat/Perro";
char s_Bikes_Rat_Killer[20] = "Bikes/Rat/Killer";
char s_Bikes_Rat_Kamikaze[20] = "Bikes/Rat/Kamikaze";
char s_Bikes_Rat_Banzai[20] = "Bikes/Rat/Banzai";
char s_Panzer[8] = "Panzer";
char s_Olleys[8] = "Olleys";
char s_Chars_ManoMano[16] = "Chars/ManoMano";
char s_Main[8] = "Main";
char s_Credits[8] = "Credits";
char s_Showroom_Super_Bikes[24] = "Showroom/Super/Bikes";
char s_Showroom_Sport_Bikes[24] = "Showroom/Sport/Bikes";
char s_Showroom_Rat_Bikes[20] = "Showroom/Rat/Bikes";
char s_Showroom_Super_Kamikaze[24] = "Showroom/Super/Kamikaze";
char s_Showroom_Super_Corsair[24] = "Showroom/Super/Corsair";
char s_Showroom_Super_Stiletto[24] = "Showroom/Super/Stiletto";
char s_Showroom_Super_Diablo[24] = "Showroom/Super/Diablo";
char s_Showroom_Super_Banzai[24] = "Showroom/Super/Banzai";
char s_Showroom_Sport_Perro[24] = "Showroom/Sport/Perro";
char s_Showroom_Sport_DMG[20] = "Showroom/Sport/DMG";
char s_Showroom_Sport_Stiletto[24] = "Showroom/Sport/Stiletto";
char s_Showroom_Sport_Kamikaze[24] = "Showroom/Sport/Kamikaze";
char s_Showroom_Sport_Diablo[24] = "Showroom/Sport/Diablo";
char s_Showroom_Rat_Corsair[24] = "Showroom/Rat/Corsair";
char s_Showroom_Rat_Perro[20] = "Showroom/Rat/Perro";
char s_Showroom_Rat_Killer[20] = "Showroom/Rat/Killer";
char s_Showroom_Rat_Kamikaze[24] = "Showroom/Rat/Kamikaze";
char s_Showroom_Rat_Banzai[20] = "Showroom/Rat/Banzai";
char s_Street_Home[12] = "Street/Home";
char s_Street_Olley[16] = "Street/Olley";
char s_Street_Klub[12] = "Street/Klub";
char s_Chars_Mike_N[16] = "Chars/Mike-N";
char s_Chars_Jim_N[12] = "Chars/Jim-N";
char s_Chars_Rhonda_N[16] = "Chars/Rhonda-N";
char s_Chars_Pearl_N[16] = "Chars/Pearl-N";
char s_Chars_Jon_N[12] = "Chars/Jon-N";
char s_Chars_Cydney_N[16] = "Chars/Cydney-N";
char s_Chars_Bose_N[16] = "Chars/Bose-N";
char s_Chars_Axle_N[16] = "Chars/Axle-N";
char s_Postcard_RestB[16] = "Postcard/RestB";
char s_Postcard_RestA[16] = "Postcard/RestA";
char s_Postcard_Klub[16] = "Postcard/Klub";
char s_Postcard_PCH[16] = "Postcard/PCH";
char s_Postcard_QPCH[16] = "Postcard/QPCH";
char s_Postcard_Napa[16] = "Postcard/Napa";
char s_Postcard_QNapa[16] = "Postcard/QNapa";
char s_Postcard_Penin[16] = "Postcard/Penin";
char s_Postcard_QPenin[16] = "Postcard/QPenin";
char s_Postcard_Sierra[16] = "Postcard/Sierra";
char s_Postcard_SierraR[20] = "Postcard/SierraR";
char s_Postcard_QSierra[20] = "Postcard/QSierra";
char s_Postcard_QSierraR[20] = "Postcard/QSierraR";
char s_Postcard_City[16] = "Postcard/City";
char s_Postcard_QCity[16] = "Postcard/QCity";
int g_shopBike;
int tbl_shopSlots[4];
int g_scrollSpeed = 1;
int tbl_levelValues[26] = {
    53,84,115,135,163,53,79,98,129,171,53,80,110,150,168,50,76,106,128,185,55,81,110,126,161
};
unsigned char tbl_bikePrices[64] = {
    143,17,0,0,177,12,0,0,169,13,0,0,113,21,0,0,183,11,0,0,46,117,0,0,55,74,0,0,64,156,0,0,29,85,0,0,72,136,0,0,228,53,0,0,235,65,0,0,212,46,0,0,239,
    35,0,0,82,27
};
Rec_startCash rec_startCash = {
    /* tbl_startCash */ 500,
    /* tbl_cash */ {
        200,500,1000,400,750,800,600,500
    }
};
struct L_tbl_gameModeMenu tbl_gameModeMenu = { { 0,0,0,0,10,0,0,0,2,0,0,0,0,0,0,0,164,1,0,0,160,0,0,0,60,0,0,0,3,1,0,0,1,3,0,0,4,0,0,0,1,0,0,0 }, (unsigned)RRT_ThrashModePick, { 3,0,0,0,80,72,0,0,0,0,0,0,14,0,0,0,4,0,0,0,160,0,0,0,164,1,0,0,160,0,0,0,60,0,0,0,0,2,1,1,2,0,0,0,4,0,0,0,1,0,0,0 }, (unsigned)RRT_BigGameModePick, { 5,0,0,0,80,72,0,0,0,0,0,0,11,0,0,0,13,0,0,0,64,1,0,0,164,1,0,0,160,0,0,0,60,0,0,0,1,3,2,2,3,1,0,0,4,0,0,0,1,0,0,0 }, (unsigned)RRT_UnusedNetFlagsUpdate, { 14,0,0,0,80,72,0,0,0,0,0,0,16,0,0,0,6,0,0,0,224,1,0,0,164,1,0,0,160,0,0,0,60,0,0,0,2,0,3,3,0,2,0,0,4,128,0,0,1,0,0,0,0,0,0,0,7,0,0,0,80,72,0,0 } };
int tbl_menuItems1[14] = {
    0,25,48,426,450,214,30,67371267,1024,34821,1,0,49,18512
};
int tbl_menuItems2[14] = {
    0,16,6,426,450,214,30,67371267,1024,34821,1,0,7,18512
};
Rec_trackMenuItems rec_trackMenuItems = {
    /* tbl_trackMenuItems */ { { 0,0,0,0,11,0,0,0,34,1,0,0,170,1,0,0,194,1,0,0,214,0,0,0,30,0,0,0,3,1,4,4,0,4,0,0,5,8,0,0,1,0,0,0 }, (unsigned)RRT_UnusedNetTick, { 40,1,0,0,80,72,0,0,0,0,0,0,1,0,0,0,16,0,0,0,0,0,0,0,164,1,0,0,213,0,0,0,30,0,0,0,4,2,1,1,1,5,0,0,21,72,0,0,1,0,0,0 }, (unsigned)RRT_ChoiceScreenPick, { 1,0,0,0,80,72,0,0,0,0,0,0,2,0,0,0,17,0,0,0,0,0,0,0,194,1,0,0,213,0,0,0,30,0,0,0,5,3,0,0,2,0,0,0,21,72,0,0,1,0,0,0 }, (unsigned)RRT_ChoiceScreenPick, { 1,0,0,0,80,72,0,0,0,0,0,0,3,0,0,0,18,0,0,0,213,0,0,0,164,1,0,0,213,0,0,0,30,0,0,0,0,4,3,3,3,1,0,0,21,72,0,0,1,0,0,0 }, (unsigned)RRT_ChoiceScreenPick, { 1,0,0,0,80,72,0,0,0,0,0,0,4,0,0,0,19,0,0,0,213,0,0,0,194,1,0,0,213,0,0,0,30,0,0,0,1,5,2,2,4,2,0,0,21,72,0,0,1,0,0,0 }, (unsigned)RRT_ChoiceScreenPick, { 1,0,0,0,80,72,0,0,0,0,0,0,5,0,0,0,20,0,0,0,170,1,0,0,164,1,0,0,214,0,0,0,30,0,0,0,2,0,5,5,5,3,0,0,21,72,0,0,1,0,0,0 }, (unsigned)RRT_ChoiceScreenPick, { 1,0,0,0,80,72,0,0 } },
    /* tbl_menuItemTemplate */ {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0x20,0,0x192,0xd5,0x18,0x1020306,0x701,5,1,(unsigned)RRT_CharacterSelect,0xa0,0x4852,0,2,0x21,0,0x1aa,0xd5,0x18,
        0x2000407,2,5,1,(unsigned)RRT_CharacterSelect,0xa1,0x4852,0,3,0x22,0,0x1c2,0xd5,0x1e,0x10505,0x103,5,1,(unsigned)RRT_CharacterSelect,0xa2,0x4852,0,4,0x23,
        0xd5,0x192,0xd5,0x18,0x4050600,0x204,5,1,(unsigned)RRT_CharacterSelect,0xa3,0x4852,0,5,0x24,0xd5,0x1aa,0xd5,0x18,0x5030701,0x305,5,1,
        (unsigned)RRT_CharacterSelect,0xa4,0x4850,0,6,0x25,0xd5,0x1c2,0xd5,0x1e,0x3040202,0x406,5,1,(unsigned)RRT_CharacterSelect,0xa5,0x4850,0,7,0x26,0x1aa,0x192,
        0xd6,0x18,0x7070003,0x507,5,1,(unsigned)RRT_CharacterSelect,0xa6,0x4850,0,8,0x27,0x1aa,0x1aa,0xd6,0x18,0x6060104,0x600,5,1,(unsigned)RRT_CharacterSelect,
        0xa7,0x4850
    },
    /* tbl_menuPrevPage */ {
        0,0,0x1aa,0x1c2,0xd6,0x1e,0,0,0x8010,0,(unsigned)RRT_MenuItemAction,0,0,0,0x19,0x30,0,0x1a4,0xd5,0x3c,0x102,0x201,0x805,1,0,0x31,0x4850,0,0x14,0x32,
        0xd5,0x1a4,0xd5,0x3c,0x1010200,2,0x805,1,0,0x33,0x4850,0,6,8,0x1aa,0x1a4,0xd6,0x3c,0x2020001,0x100,0x805,1,(unsigned)RRT_MenuStartPressed,9,0x4850,0,
        0x1a,0x34,0x50,0x1a4,0xc8,0x1e,0x2020101,0x301,0x805,1,(unsigned)RRT_ResultPageStep,0x35,0x4850,0,0x10,6,0x168,0x1a4,0xc8,0x1e,0x3030000,2,0x805,1,
        0,7,0x4850,0,0x1b,0x36,0x50,0x1c2,0xc8,0x1e,0x303,0x103,0x805,1,0,0x37,0x4850,0,0xf,0x38,0x168,0x1c2,0xc8,0x1e,0x1010202,0x200,0x8805,1,0,
        0x39,0x4850,0,0,0,0,0x1a4,0x50,0x3c,0,0,0,0,0,0,0,0,0,0,0x118,0x1a4,0x50,0x3c,0,0,0,0,0,0,0,0,0,0,0x230,0x1a4,0x50,0x3c,0,0,0,0,0,0,0,0,0x15,
        0x3a,0x50,0x1a4,0xc8,0x1e,0x2020101,0x301,5,1,0,0x3b,0x4850,0,0x17,0x3e,0x168,0x1a4,0xc8,0x1e,0x3030000,2,5,1,0,0x3f,0x4850,0,0x16,0x3c,0x50,
        0x1c2,0xc8,0x1e,0x303,0x103,5,1,0,0x3d,0x4850,0,0xf,0x38,0x168,0x1c2,0xc8,0x1e,0x1010202,0x200,0x8005,1,0,0x39,0x4850,0,0,0,0,0x1a4,0x50,0x3c,
        0,0,0,0,0,0,0,0,0,0,0x118,0x1a4,0x50,0x3c,0,0,0,0,0,0,0,0,0,0,0x230,0x1a4,0x50,0x3c,0,0,0,0,0,0,0,0,0x18,0x60,0,0x1a4,0xd5,0x1e,0x3030102,
        0x501,0x2112,1,(unsigned)RRT_BikeClassSelect,0x80,0x4852,0,0x18,0x62,0xd5,0x1a4,0xd5,0x1e,0x4040200,2,0x2112,1,(unsigned)RRT_BikeClassSelect,0x82,0x4852,0,
        0x18,0x64,0x1aa,0x1a4,0xd6,0x1e,0x5050001,0x103,0x2112,1,(unsigned)RRT_BikeClassSelect,0x84,0x4852,0,0x18,0x61,0,0x1c2,0xd5,0x1e,0x405,0x204,0x2112,
        1,(unsigned)RRT_BikeClassSelect,0x81,0x4850,0,0x18,0x63,0xd5,0x1c2,0xd5,0x1e,0x1010503,0x305,0x2112,1,(unsigned)RRT_BikeClassSelect,0x83,0x4850,0,0x14,0x5a,
        0x1aa,0x1c2,0xd6,0x1e,0x2020304,0x400,0x8102,1,0,0,0x4850,0,0x18,0x65,0,0x1a4,0xd5,0x1e,0x3030102,0x501,0x2112,1,(unsigned)RRT_BikeClassSelect,0x85,
        0x4850,0,0x18,0x66,0xd5,0x1a4,0xd5,0x1e,0x4040200,2,0x2112,1,(unsigned)RRT_BikeClassSelect,0x86,0x4852,0,0x18,0x67,0x1aa,0x1a4,0xd6,0x1e,0x5050001,
        0x103,0x2112,1,(unsigned)RRT_BikeClassSelect,0x87,0x4852,0,0x18,0x68,0,0x1c2,0xd5,0x1e,0x405,0x204,0x2112,1,(unsigned)RRT_BikeClassSelect,0x88,0x4852,0,
        0x18,0x69,0xd5,0x1c2,0xd5,0x1e,0x1010503,0x305,0x2112,1,(unsigned)RRT_BikeClassSelect,0x89,0x4850,0,0x14,0x5a,0x1aa,0x1c2,0xd6,0x1e,0x2020304,0x400,
        0x8102,1,0,0,0x4850,0,0x18,0x6a,0,0x1a4,0xd5,0x1e,0x3030102,0x501,0x2112,1,(unsigned)RRT_BikeClassSelect,0x8a,0x4852,0,0x18,0x6b,0xd5,0x1a4,0xd5,
        0x1e,0x4040200,2,0x2112,1,(unsigned)RRT_BikeClassSelect,0x8b,0x4852,0,0x18,0x6c,0x1aa,0x1a4,0xd6,0x1e,0x5050001,0x103,0x2112,1,
        (unsigned)RRT_BikeClassSelect,0x8c,0x4850,0,0x18,0x6d,0,0x1c2,0xd5,0x1e,0x405,0x204,0x2112,1,(unsigned)RRT_BikeClassSelect,0x8d,0x4850,0,0x18,0x6e,0xd5,
        0x1c2,0xd5,0x1e,0x1010503,0x305,0x2112,1,(unsigned)RRT_BikeClassSelect,0x8e,0x4852,0,0x14,0x5a,0x1aa,0x1c2,0xd6,0x1e,0x2020304,0x400,0x8102,1,0,0,
        0x4850
    },
    /* tbl_shopPageA */ { 0x14,0xd9,0x50,0x1c2,0xc8,0x1e,0x101,0x101,0x184 },
    /* tbl_shopPageA_flag */ { 1,(unsigned)RRT_BikeShopTradeIn,1 },
    /* tbl_shopPageA_style */ { 0x4852,0 },
    /* tbl_shopPageB */ { 0x14,0xda,0x168,0x1c2,0xc8,0x1e,0x1010000,0 },
    /* tbl_shopPageB_style */ { 0x8184,1,(unsigned)RRT_ShopPageAFlag },
    /* tbl_shopPageC_flag */ 1,
    /* tbl_shopPageC */ {
        0x4852,0,0,0,0x50,0x1a4,0xc8,0x1e,0,0,0,0,0,0,0,0,0,0,0x168,0x1a4,0xc8,0x1e,0,0,0,0,0,0,0,0,0,0,0,0x1a4,0x50,0x3c,0,0,0,0,0,0,0,0,0,0,0x118,0x1a4,0x50,
        0x3c,0,0,0,0,0,0,0,0,0,0,0x230,0x1a4,0x50,0x3c,0,0,0,0,0,0,0
    },
    /* tbl_messagePage */ {0},
    /* tbl_resultPage */ { 0x19,0,0,0x19a,0x280,0x46,0,0,0x24,1,(unsigned)RRT_ResultPageStep },
    /* g_resultTextId */ 0,
    /* tbl_resultPageText */ { 0x4850,0,0x19,0,0,0,0,0,0,0,0x8020,1,(unsigned)RRT_ResultPageNext,0,0 }
};
struct L_tbl_creditsPage tbl_creditsPage = { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,32,128,0,0,1,0,0,0 }, (unsigned)RRT_GameScreenLeave, { 0,0,0,0,0,0,0,0 } };
Rec_demoPage rec_demoPage = {
    /* tbl_demoPage */ {0},
    /* g_raceTrack */ {
        0,0xcf,0,0x19a,0x280,0x46,0,0,0,1,0,0,0x4850,0,0x21,0xc0,0,0x19a,0x280,0x46,0,0,0x8000,1,(unsigned)RRT_BikeShopBuy,0,0x4850,0,0x21,0xc3,0,0x19a,
        0x280,0x46,0,0,0x8000,1,(unsigned)RRT_BikeShopBuy,0,0x4850,0,0x22
    },
    /* tbl_trackPage */ {
        1,0,0x19a,0x280,0x46,0,0,0x8000,1,(unsigned)RRT_LevelPageOpen,0,0x4850,0,0x22,0xc7,0,0x19a,0x280,0x46,0,0,0x8000,1,(unsigned)RRT_LevelPageOpen,0,0x4850
    },
    /* tbl_levelPage */ { 1,0,0x19a,0x280,0x46,0,0,0x8000,1,(unsigned)RRT_MenuNextScreen,0,0x4850 }
};
Rec_levelPageItems rec_levelPageItems = {
    /* tbl_levelPageItems */ {
        0,0,160,0,112,640,24,0,1
    },
    /* tbl_levelPageItemsSel */ { { 0,0,0,0 }, (unsigned)RRT_MenuNextScreen, { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,161,0,0,0,0,0,0,0,135,0,0,0,128,2,0,0,24,0,0,0,1,1,1,1,2,1,0,0,0,0,0,0,0,0,0,0 }, (unsigned)RRT_MenuNextScreen, { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,162,0,0,0,0,0,0,0,158,0,0,0,128,2,0,0,24,0,0,0,2,2,2,2,3,2,0,0,0,0,0,0,0,0,0,0 }, (unsigned)RRT_MenuNextScreen, { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,163,0,0,0,0,0,0,0,181,0,0,0,128,2,0,0,24,0,0,0,3,3,3,3,4,3,0,0,0,0,0,0,0,0,0,0 }, (unsigned)RRT_MenuNextScreen, { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,164,0,0,0,0,0,0,0,204,0,0,0,128,2,0,0,24,0,0,0,4,4,4,4,5,4,0,0,0,0,0,0,0,0,0,0 }, (unsigned)RRT_MenuNextScreen, { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,165,0,0,0,0,0,0,0,227,0,0,0,128,2,0,0,24,0,0,0,5,5,5,5,6,5,0,0,0,0,0,0,0,0,0,0 }, (unsigned)RRT_MenuNextScreen, { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,166,0,0,0,0,0,0,0,250,0,0,0,128,2,0,0,24,0,0,0,6,6,6,6,7,6,0,0,0,0,0,0,0,0,0,0 }, (unsigned)RRT_MenuNextScreen, { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,167,0,0,0,0,0,0,0,17,1,0,0,128,2,0,0,24,0,0,0,7,7,7,7,8,7,0,0,0,0,0,0,0,0,0,0 }, (unsigned)RRT_MenuNextScreen, { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,168,0,0,0,0,0,0,0,40,1,0,0,128,2,0,0,24,0,0,0,8,8,8,8,9,8,0,0,0,0,0,0,0,0,0,0 }, (unsigned)RRT_MenuNextScreen, { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,169,0,0,0,0,0,0,0,63,1,0,0,128,2,0,0,24,0,0,0,9,9,9,9,10,9,0,0,0,0,0,0,0,0,0,0 }, (unsigned)RRT_MenuNextScreen, { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,170,0,0,0,0,0,0,0,86,1,0,0,128,2,0,0,24,0,0,0,10,10,10,10,11,10,0,0,0,0,0,0,0,0,0,0 }, (unsigned)RRT_MenuNextScreen, { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,171,0,0,0,0,0,0,0,109,1,0,0,128,2,0,0,24,0,0,0,11,11,11,11,12,11,0,0,0,0,0,0,0,0,0,0 }, (unsigned)RRT_MenuNextScreen, { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,172,0,0,0,0,0,0,0,132,1,0,0,128,2,0,0,24,0,0,0,12,12,12,12,13,12,0,0,0,0,0,0,0,0,0,0 }, (unsigned)RRT_MenuNextScreen, { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,173,0,0,0,0,0,0,0,155,1,0,0,128,2,0,0,24,0,0,0,13,13,13,13,14,13,0,0,0,0,0,0,0,0,0,0 }, (unsigned)RRT_MenuNextScreen, { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,174,0,0,0,0,0,0,0,178,1,0,0,128,2,0,0,24,0,0,0,14,14,14,14,0,14,0,0,0,0,0,0,0,0,0,0 }, (unsigned)RRT_MenuNextScreen, { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,200,0,0,0,0,0,0,0,154,1,0,0,128,2,0,0,70,0,0,0,0,0,0,0,0,0,0,0,0,128,0,0,1,0,0,0 }, (unsigned)RRT_MenuNextScreen, { 0,0,0,0,80,72,0,0 } }
};
MenuScreens g_menuScreens = {
    /* g_menuScreenFlag */ {0},
    /* g_lastTextId */ 0,
    /* g_lastTextFlags */ 0,
    /* tbl_screenFocus */ { { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0 }, (unsigned)&tbl_creditsPage, { 0,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0 }, (unsigned)&rec_demoPage.tbl_demoPage },
    /* tbl_screenGameMode */ {
        0xce,0x4851,0,4,(unsigned)&tbl_gameModeMenu,0x46,0x4850,0,6,(unsigned)((unsigned char *)&rec_trackMenuItems.tbl_trackMenuItems + 56)
    },
    /* tbl_screenRestroom */ {
        0,(unsigned)((unsigned char *)&tbl_netOptions + 224),0xd,0x4850,0,6,(unsigned)((unsigned char *)&rec_trackMenuItems.tbl_trackMenuItems + 56)
    },
    /* tbl_screenOptions */ {
        0,(unsigned)((unsigned char *)&g_restroomDefault + 8),0xdc,0x4850,0,9,
        (unsigned)((unsigned char *)&rec_trackMenuItems.tbl_menuItemTemplate + 56),0,0,0,3,
        (unsigned)((unsigned char *)&rec_trackMenuItems.tbl_menuPrevPage + 52)
    },
    /* tbl_screenA */ {
        0,(unsigned)&tbl_menuEnable,6,0x4850
    },
    /* tbl_screenB */ {
        0,(unsigned)&rec_optItems.tbl_optItems,0x50,0x4850
    },
    /* tbl_screenControls */ {
        0,(unsigned)&rec_controlRows.tbl_controlRows,0x54,0x4850
    },
    /* tbl_screenC */ {
        0,(unsigned)((unsigned char *)&tbl_optVideoMode + 224),0x18a,0x4850,0,7,
        (unsigned)((unsigned char *)&rec_trackMenuItems.tbl_menuPrevPage + 612),0,0,0,6,
        (unsigned)((unsigned char *)&rec_trackMenuItems.tbl_menuPrevPage + 1004),0,0,0,6,
        (unsigned)((unsigned char *)&rec_trackMenuItems.tbl_menuPrevPage + 1340),0,0,0,6,
        (unsigned)((unsigned char *)&rec_trackMenuItems.tbl_menuPrevPage + 1676),0,0,0,7,
        (unsigned)((unsigned char *)&rec_trackMenuItems.tbl_menuPrevPage + 2012)
    },
    /* g_selectedItem */ 0,
    /* g_selectedStyle */ 0,
    /* tbl_screenResults */ {
        0,7,(unsigned)((unsigned char *)&rec_trackMenuItems.tbl_menuPrevPage + 220),0,0,0,2,(unsigned)&rec_trackMenuItems.tbl_messagePage,0,0,0,6,
        (unsigned)((unsigned char *)&rec_trackMenuItems.tbl_trackMenuItems + 56),0,0,0,1,(unsigned)((unsigned char *)&rec_demoPage.g_raceTrack + 52),
        0xb0,0x4851,0,1,(unsigned)((unsigned char *)&rec_demoPage.g_raceTrack + 108),0xb1,0x4851,0,1,
        (unsigned)((unsigned char *)&rec_demoPage.g_raceTrack + 164)
    },
    /* tbl_screenPostRace */ {
        0xb2,0x4851,0,1,(unsigned)((unsigned char *)&rec_demoPage.tbl_trackPage + 48),0xb3,0x4851,0,1,(unsigned)((unsigned char *)&rec_levelPageItems.tbl_levelPageItemsSel + 800),
        0xb4,0x4851,0,1,(unsigned)((unsigned char *)&rec_demoPage.tbl_trackPage + 104)
    },
    /* tbl_screenLevel */ {
        0xf,(unsigned)&rec_levelPageItems.tbl_levelPageItems,0xb7,0x4850
    },
    /* tbl_screenCredits */ {
        0,1,(unsigned)&tbl_creditsPage
    }
};
Rec_menuScreen rec_menuScreen = {
    /* g_menuScreen */ {0},
    /* g_drawTextParams */ {
        20,0,0,0,4
    },
    /* tbl_resultTexts */ {
        82,72,0,0,82,72,0,0,82,72,0,0,80,72,0,0,82,72,0,0,82,72,0,0,82,72,0,0,82,72,0,0,80,72,0,0,82,72,0,0,82,72,0,0,82,72,0,0,82,72,0,0,80,72,0,0,
        82,72,0,0,80,72,0,0,80,72,0,0,82,72,0,0,80,72,0,0,80,72,0,0,82,72,0,0,82,72,0,0,80,72,0,0,82,72
    },
    /* tbl_resultIds */ {
        -6,1,-4,1,-2,1,-5,1,-3,1,-1,1,0,2,1,2,2,2,3,2,4,2,5,2
    },
    /* tbl_resultRows */ {
        5
    },
    /* tbl_resultRows2 */ {
        4,0,6,2,8,2,10,2,7,2,9,2,11,2,12,2,13,2,14,2,15,2,16,2,17,2,4,0,1,0,18,2,20,2,22,2,19,2,21,2,23,2,24,2,25,2,26,2,27,2,28,2,29,2,7,0,2,0,30,2,
        32,2,34,2,31,2,33,2,35,2,36,2,37,2,38,2,39,2,40,2,41,2,2,0,3,0,42,2,44,2,46,2,43,2,45,2,47,2,48,2,49,2,50,2,51,2,52,2,53,2,1,0,6,0,54,2,56,2,
        58,2,55,2,57,2,59,2,60,2,61,2,62,2,65,2,64,2,63,2,8,0,7,0,66,2,68,2,70,2,67,2,69,2,71,2,72,2,73,2,74,2,75,2,76,2,77,2,6,0,8,0,78,2,80,2,82,2,
        79,2,81,2,83,2,84,2,85,2,86,2,87,2,88,2,89,2,3,0,5,0,90,2,92,2,94,2,91,2,93,2,95,2,96,2,97,2,98,2,99,2,100,2,101,2,5,0,4,0,102,2,104,2,106,2,
        103,2,105,2,107,2,108,2,109,2,110,2,111,2,112,2,113,2,4,0,1,0,114,2,116,2,118,2,115,2,117,2,119,2,120,2,121,2,122,2,123,2,124,2,125,2,7,0,2,0,
        -126,2,-128,2,126,2,-125,2,-127,2,127,2,-124,2,-123,2,-122,2,-121,2,-120,2,-119,2,2,0,3,0,-118,2,-116,2,-114,2,-117,2,-115,2,-113,2,-112,2,
        -111,2,-110,2,-109,2,-108,2,-107,2,1,0,6,0,-106,2,-104,2,-102,2,-105,2,-103,2,-101,2,-100,2,-99,2,-98,2,-97,2,-96,2,-95,2,8,0,7,0,-94,2,-92,2,
        -90,2,-93,2,-91,2,-89,2,-88,2,-87,2,-86,2,-85,2,-84,2,-83,2,6,0,8,0,-82,2,-80,2,-78,2,-81,2,-79,2,-77,2,-76,2,-75,2,-74,2,-73,2,-72,2,-71,2,3,
        0,5,0,-70,2,-68,2,-66,2,-69,2,-67,2,-65,2,-64,2,-63,2,-62,2,-61,2,-60,2,-59,2,5,0,4,0,-58,2,-56,2,-54,2,-57,2,-55,2,-53,2,-52,2,-51,2,-50,2,
        -49,2,-48,2,-47,2,4,0,1,0,-46,2,-44,2,-42,2,-45,2,-43,2,-41,2,-40,2,-39,2,-38,2,-37,2,-36,2,-35,2,7,0,2,0,-34,2,-32,2,-30,2,-33,2,-31,2,-29,2,
        -28,2,-27,2,-26,2,-25,2,-24,2,-23,2,2,0,3,0,-22,2,-20,2,-18,2,-21,2,-19,2,-17,2,-16,2,-15,2,-14,2,-13,2,-12,2,-11,2,1,0,6,0,-10,2,-8,2,-6,2,
        -9,2,-7,2,-5,2,-4,2,-3,2,-2,2,-1,2,0,3,1,3,8,0,7,0,2,3,4,3,6,3,3,3,5,3,7,3,8,3,9,3,10,3,11,3,12,3,13,3,6,0,8,0,14,3,16,3,18,3,15,3,17,3,19,3,
        20,3,21,3,22,3,23,3,24,3,25,3,3,0,5,0,26,3,28,3,30,3,27,3,29,3,31,3,32,3,33,3,34,3,35,3,36,3,37,3,5,0,4,0,38,3,40,3,42,3,39,3,41,3,43,3,44,3,
        45,3,46,3,47,3,48,3,49,3,4,0,1,0,50,3,52,3,54,3,51,3,53,3,55,3,56,3,57,3,58,3,59,3,60,3,61,3,7,0,2,0,62,3,64,3,66,3,63,3,65,3,67,3,68,3,69,3,
        70,3,71,3,72,3,73,3,2,0,3,0,74,3,76,3,78,3,75,3,77,3,79,3,80,3,81,3,82,3,83,3,84,3,85,3,1,0,6,0,86,3,88,3,90,3,87,3,89,3,91,3,92,3,93,3,94,3,
        95,3,96,3,97,3,8,0,7,0,98,3,100,3,102,3,99,3,101,3,103,3,104,3,105,3,106,3,107,3,108,3,109,3,6,0,8,0,110,3,112,3,114,3,111,3,113,3,115,3,116,
        3,117,3,118,3,119,3,120,3,121,3,3,0,5,0,122,3,124,3,126,3,123,3,125,3,127,3,-128,3,-127,3,-126,3,-125,3,-124,3,-123,3,5,0,4,0,-122,3,-120,3,
        -118,3,-121,3,-119,3,-117,3,-116,3,-115,3,-114,3,-113,3,-112,3,-111,3,4,0,1,0,-110,3,-108,3,-106,3,-109,3,-107,3,-105,3,-104,3,-103,3,-102,3,
        -101,3,-100,3,-99,3,7,0,2,0,-98,3,-96,3,-94,3,-97,3,-95,3,-93,3,-92,3,-91,3,-90,3,-89,3,-88,3,-87,3,2,0,3,0,-86,3,-84,3,-82,3,-85,3,-83,3,-81,
        3,-80,3,-79,3,-78,3,-77,3,-76,3,-75,3,1,0,6,0,-74,3,-72,3,-70,3,-73,3,-71,3,-69,3,-68,3,-67,3,-66,3,-65,3,-64,3,-63,3,8,0,7,0,-62,3,-60,3,-58,
        3,-61,3,-59,3,-57,3,-56,3,-55,3,-54,3,-53,3,-52,3,-51,3,6,0,8,0,-50,3,-48,3,-46,3,-49,3,-47,3,-45,3,-44,3,-43,3,-42,3,-41,3,-40,3,-39,3,3,0,5
    },
    /* g_resultCount */ 0,
    /* tbl_resultTags */ {
        1,0,1,2
    },
    /* tbl_prizes */ {
        1000
    },
    /* tbl_prizesEnabled */ {
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-18,2,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        -12,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,-112,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,44,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-6,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-56,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-96,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-126,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,100,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,70,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,50,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,30,0,0,0,0,0,0,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,20,0,
        0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
        0,0,0,10
    }
};
EditTextRec g_editTextRec = {
    /* g_editTextBefore */ 0,
    /* s_dash */ 45,
    /* s_blank */ "-----------"
};
HWND g_buttonSpace;
HWND g_buttonEnter;
HWND g_buttonEscape;
char g_paused;
char g_windowActive;
int g_idleTicks;
int g_demoCycle;
char g_toggleFlag;
int g_demoTrack;
char *g_editCursor;
char g_editing;
char g_editDone;
char g_screenRect[16] = {
    0,0,0,0,0,0,0,0,-128,2,0,0,-32,1
};
unsigned char g_creditsTextParams[20] = {
    20,0,0,0,35,0,0,0,40
};
char s_ego_pctd_pctd[12] = "ego %d/%d\n";
char s_level_pctd_bike_pctd[20] = "level %d bike %d\n";
char s_cursorChar[4] = {
    -24
};
char s_hotkeyEscapes[8] = {
    1,2,3,4
};
char s_pctdpctcpctd[8] = "%d%c%d";
char s_pcts_pctspctspctspcts_pctu_pctspcts_pcts[36] = "  %s     %s%s%s%s $%u   %s%s %s  ";
char s_pcts_pcts_2[32] = "     %s              %s      ";
char s_pcts_pcts_pctspcts_pctu[36] = "     %s     %s     %s%s  $%u     ";
char s_FNH[4] = "FNH";
char s_pctd_pcts[8] = "%d:%s";
char s_pct02dpctcpctd[12] = "%02d%c%d";
char s_pct2d_pct4d_pct_s_pcts[20] = "%2d)  $%4d  %*s  %s";
char s_pct2d_pct_s_pcts[16] = "%2d)  %*s  %s";
char s_Start[8] = "Start";
char s_Super_Kamikaze[16] = "Super/Kamikaze";
char s_Super_Corsair[16] = "Super/Corsair";
char s_Super_Stiletto[16] = "Super/Stiletto";
char s_Super_Diablo[16] = "Super/Diablo";
char s_Super_Banzai[16] = "Super/Banzai";
char s_Sport_Perro[12] = "Sport/Perro";
char s_Sport_DMG[12] = "Sport/DMG";
char s_Sport_Stiletto[16] = "Sport/Stiletto";
char s_Sport_Kamikaze[16] = "Sport/Kamikaze";
char s_Sport_Diablo[16] = "Sport/Diablo";
char s_Rat_Corsair[12] = "Rat/Corsair";
char s_Rat_Perro[12] = "Rat/Perro";
char s_Rat_Killer[12] = "Rat/Killer";
char s_Rat_Kamikaze[16] = "Rat/Kamikaze";
char s_Rat_Banzai[12] = "Rat/Banzai";
FontNames s_fontNames = {
    /* s_Duel */ "Duel",
    /* s_Jessie */ "Jessie",
    /* s_Wingdings */ "Wingdings",
    /* s_Arial */ "Arial",
    /* s_New_Courier_Bold */ "New Courier Bold",
    /* s_MS_Sans_Serif */ "MS Sans Serif",
    /* s_Futura_Condensed_Demi_Regular */ "Futura Condensed Demi Regular",
    /* s_Futura_Condensed_Bold */ "Futura Condensed Bold",
    /* s_Futura_Condensed_Regular */ "Futura Condensed Regular",
    /* s_Badloc */ "Badloc"
};
char s_RashInfo[12] = "RashInfo";
int g_stringBank = -1;
char s_Text[8] = "Text/";
char s_dotdll[8] = ".dll";
char s_pctd_pctd[8] = "%d %d";
char s_Badloc_pcts[12] = "Badloc: %s";
char s_Futura_Bold_pcts[16] = "Futura Bold: %s";
char s_Futura_Demi_pcts[16] = "Futura Demi: %s";
char s_Futura_Regular_pcts[20] = "Futura Regular: %s";
char s_FutR[8] = "FutR";
char s_FutD[8] = "FutD";
char s_FutB[28] = {
    70,117,116,66,0,0,0,0,67,104,97,116,70,120
};
unsigned char tbl_debugText[12] = {
    0,0,0,0,0,0,0,0,119,1
};
char tbl_debugPage[56] = {
    0,0,0,0,0,0,0,0,-17,0,0,0,60,1,0,0,92,0,0,0,68,1,0,0,22
};
int g_menuStep = 2;
unsigned tbl_netOptions[140] = {
    0,0,0,0x13c,0x5a,0xf0,0x1e,0,0,0x8030,0,(unsigned)RRT_UnusedNetOptionsClose,0,0x4850,0,1,0x184,0x13c,0x7e,0xf0,0x1e,0x2030101,0x302,0x10,1,
    (unsigned)RRT_UnusedNetOptionsClose,0,0x4850,0,2,0x185,0x13c,0xa2,0xf0,0x1e,0x3010202,0x103,0x10,1,(unsigned)RRT_UnusedNetOptionsClose,0,0x4850,0,3,0x186,0x13c,0xc6,0xf0,
    0x1e,0x1020303,0x201,0x10,1,(unsigned)RRT_UnusedNetOptionsClose,0,0x4850,0,0,0x121,0,0x194,0x140,0x19,0x1020303,0x501,5,1,(unsigned)RRT_UnusedNetOptionsAction,0x127,0x4850,
    0,0,0x120,0,0x1ad,0x140,0x19,0x2000404,2,5,1,0,0x126,0x4850,0,0xc,0x36,0,0x1c6,0x140,0x1a,0x10505,0x103,0x20805,0,0,0x37,0x4850,0,0xd,0xdc,0x140,
    0x194,0x140,0x19,0x4050000,0x204,0x20805,1,0,0xdd,0x4850,0,0x10,6,0x140,0x1ad,0x140,0x19,0x5030101,0x305,0x28805,1,0,7,0x4850,0,0,0x122,0x140,
    0x1c6,0x140,0x1a,0x3040202,0x400,0x20805,0,(unsigned)RRT_UnusedNetTick,0x128,0x4850
};
unsigned g_restroomDefault[254] = {
    6,0,0,0xd,0x110,0,0x78,0x64,0x28,0x1080000,0x801,0,1,(unsigned)RRT_UnusedNetOptionItemA,0,0,0,0xd,0x111,0,0xa0,0x64,0x28,0x2000101,2,0,1,
    (unsigned)RRT_UnusedNetOptionItemA,0,0,0,0xd,0x112,0,0xc8,0x64,0x28,0x3010202,0x103,0,1,(unsigned)RRT_UnusedNetOptionItemA,0,0,0,0xd,0x113,0,0xf0,0x64,0x28,0x4020303,
    0x204,0,1,(unsigned)RRT_UnusedNetOptionItemA,0,0,0,0xd,0x114,0,0x118,0x64,0x28,0x5030404,0x305,0,1,(unsigned)RRT_UnusedNetOptionItemA,0,0,0,0xd,0x115,0,0x140,0x64,0x28,
    0x6040505,0x406,0,1,(unsigned)RRT_UnusedNetOptionItemA,0,0,0,0xd,0x116,0,0x168,0x64,0x28,0x7050606,0x507,0,1,(unsigned)RRT_UnusedNetOptionItemA,0,0,0,0xd,0x117,0,0x190,
    0x64,0x28,0x8060707,0x608,0,1,(unsigned)RRT_UnusedNetOptionItemA,0,0,0,0xb,0x56,0,0x1b8,0x64,0x28,0x70808,0x700,0x8000,1,0,0,0,0,0,0,0,0x72,0x64,6,0,0,0,0,0,
    0,0,0,0xd,0xa0,0x78,0x78,0x1f4,0x28,0,0,0,0,(unsigned)RRT_UnusedNetOptionItemB,0,0,0,0xd,0xa1,0x78,0xa0,0x1f4,0x28,0x1010101,0x101,0,0,(unsigned)RRT_UnusedNetOptionItemB,
    0,0,0,0xd,0xa2,0x78,0xc8,0x1f4,0x28,0x2020202,0x202,0,0,(unsigned)RRT_UnusedNetOptionItemB,0,0,0,0xd,0xa3,0x78,0xf0,0x1f4,0x28,0x3030303,0x303,0,0,
    (unsigned)RRT_UnusedNetOptionItemB,0,0,0,0xd,0xa4,0x78,0x118,0x1f4,0x28,0x4040404,0x404,0,0,(unsigned)RRT_UnusedNetOptionItemB,0,0,0,0xd,0xa5,0x78,0x140,0x1f4,0x28,
    0x5050505,0x505,0,0,(unsigned)RRT_UnusedNetOptionItemB,0,0,0,0xd,0xa6,0x78,0x168,0x1f4,0x28,0x6060606,0x606,0,0,(unsigned)RRT_UnusedNetOptionItemB,0,0,0,0xd,0xa7,0x78,
    0x190,0x1f4,0x28,0x7070707,0x707,0,0,(unsigned)RRT_UnusedNetOptionItemB
};
int g_optionsDefault = 18;
char s_newline[4] = {
    10
};
char s_NNNNYNYY[12] = "NNNNYNYY";
char s_NNNNYYYY[12] = "NNNNYYYY";
char s_YNNYYNYY[12] = "YNNYYNYY";
char s_YNYYYNYY[12] = "YNYYYNYY";
char s_YYYYYNYY[12] = "YYYYYNYY";
char s_YNNYYYYN[14] = {
    89,78,78,89,89,89,89,78,0,0,0,0,89,78
};
char *g_menuEnableDefault = (char *)((unsigned)((unsigned char *)&s_YNNYYYYN + 12));
char *g_menuEnablePage1 = (char *)((unsigned)&s_YNNYYYYN);
char *g_menuEnableSavedPlayer = (char *)((unsigned)&s_YYYYYNYY);
char *g_menuEnableUnsavedPlayer = (char *)((unsigned)&s_YNYYYNYY);
char *g_menuEnableNoCharacter = (char *)((unsigned)&s_YNNYYNYY);
char *g_menuEnablePage3 = (char *)((unsigned)((unsigned char *)&s_YNNYYYYN + 12));
char *g_menuYesNo6 = (char *)((unsigned)&s_NNNNYYYY);
char *g_menuYesNo7 = (char *)((unsigned)&s_NNNNYNYY);
char g_fileDialogNoDir;
struct L_tbl_optGameMode tbl_optGameMode = { { 0,0,0,0,0,0,0,0,0,0,0,0,60,1,0,0,8,1,0,0,4,1,0,0,30,0,0,0,0,0,0,0,0,0,0,0,48,128,0,0,0,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0,0,0,0,0,1,0,0,0,2,0,0,0,60,1,0,0,44,1,0,0,4,1,0,0,30,0,0,0,1,1,3,2,2,3,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0,0,0,0,0,2,0,0,0,4,0,0,0,60,1,0,0,80,1,0,0,4,1,0,0,30,0,0,0,2,2,1,3,3,1,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0,0,0,0,0,3,0,0,0,13,0,0,0,60,1,0,0,116,1,0,0,4,1,0,0,30,0,0,0,3,3,2,1,1,2,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0 } };
struct L_tbl_optLevel tbl_optLevel = { { 0,0,0,0,0,0,0,0,0,0,0,0,60,1,0,0,8,1,0,0,4,1,0,0,30,0,0,0,0,0,0,0,0,0,0,0,48,128,0,0,0,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0,0,0,0,0,1,0,0,0,91,0,0,0,60,1,0,0,44,1,0,0,4,1,0,0,30,0,0,0,1,1,5,2,2,5,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0,0,0,0,0,2,0,0,0,92,0,0,0,60,1,0,0,80,1,0,0,4,1,0,0,30,0,0,0,2,2,1,3,3,1,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0,0,0,0,0,3,0,0,0,93,0,0,0,60,1,0,0,116,1,0,0,4,1,0,0,30,0,0,0,3,3,2,4,4,2,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0,0,0,0,0,4,0,0,0,94,0,0,0,60,1,0,0,152,1,0,0,4,1,0,0,30,0,0,0,4,4,3,5,5,3,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0,0,0,0,0,5,0,0,0,95,0,0,0,60,1,0,0,188,1,0,0,4,1,0,0,30,0,0,0,5,5,4,1,1,4,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0 } };
Rec_optPage2 rec_optPage2 = {
    /* tbl_optPage2 */ { { 0,0,0,0,0,0,0,0,0,0,0,0,60,1,0,0,8,1,0,0,4,1,0,0,30,0,0,0,0,0,0,0,0,0,0,0,48,128,0,0,0,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0 } },
    /* g_nameEditBuf */ {
        0,0,0,0,0,0,0,0,0xb8,0,0,0,60,1,0,0,26,1
    },
    /* tbl_optNameEdit */ { 0,0x1c,0x1010101,0x101,0x10,1,(unsigned)RRT_PlayerNameStore,0,0x4850 }
};
struct L_tbl_menuEnable tbl_menuEnable = { { 0,0,0,0,0,0,0,0,64,0,0,0,0,0,0,0,114,0,0,0,0,0,0,0,28,0,0,0,0,0,12,1,1,12,0,0,12,0,0,0,1,0,0,0 }, (unsigned)RRT_GameLoad, { 65,0,0,0,82,72,0,0,0,0,0,0,0,0,0,0,66,0,0,0,0,0,0,0,142,0,0,0,0,0,0,0,28,0,0,0,1,1,0,2,2,0,0,0,12,2,0,0,0,0,0,0 }, (unsigned)RRT_GameSave, { 67,0,0,0,82,72,0,0,0,0,0,0,0,0,0,0,68,0,0,0,0,0,0,0,170,0,0,0,0,0,0,0,28,0,0,0,2,2,1,3,3,1,0,0,12,0,0,0,0,0,0,0 }, (unsigned)RRT_RestroomBlankItem, { 69,0,0,0,82,72,0,0,0,0,0,0,0,0,0,0,70,0,0,0,0,0,0,0,198,0,0,0,0,0,0,0,28,0,0,0,3,3,2,4,4,2,0,0,12,0,0,0,1,0,0,0 }, (unsigned)RRT_GameModePageOpen, { 71,0,0,0,82,72,0,0,0,0,0,0,19,0,0,0,138,1,0,0,0,0,0,0,226,0,0,0,0,0,0,0,28,0,0,0,4,4,3,5,5,3,0,0,12,0,0,0,1,0,0,0,0,0,0,0,139,1,0,0,82,72,0,0,0,0,0,0,0,0,0,0,74,0,0,0,0,0,0,0,254,0,0,0,0,0,0,0,28,0,0,0,5,5,4,6,6,4,0,0,12,0,0,0,0,0,0,0 }, (unsigned)RRT_LevelPageOpenList, { 75,0,0,0,82,72,0,0,0,0,0,0,0,0,0,0,76,0,0,0,0,0,0,0,26,1,0,0,0,0,0,0,28,0,0,0,6,6,5,7,7,5,0,0,12,0,0,0,1,0,0,0 }, (unsigned)RRT_PlayerNamePageOpen, { 77,0,0,0,82,72,0,0,0,0,0,0,14,0,0,0,78,0,0,0,0,0,0,0,54,1,0,0,0,0,0,0,28,0,0,0,7,7,6,8,8,6,0,0,12,0,0,0,0,0,0,0 }, (unsigned)RRT_SelectCharacterOpen, { 79,0,0,0,82,72,0,0,0,0,0,0,17,0,0,0,80,0,0,0,0,0,0,0,82,1,0,0,0,0,0,0,28,0,0,0,8,8,7,9,9,7,0,0,12,4,0,0,1,0,0,0 }, (unsigned)RRT_JukeboxPageItem, { 81,0,0,0,82,72,0,0,0,0,0,0,0,0,0,0,82,0,0,0,0,0,0,0,110,1,0,0,0,0,0,0,28,0,0,0,9,9,8,10,10,8,0,0,12,0,0,0,1,0,0,0 }, (unsigned)RRT_SoundMix, { 83,0,0,0,82,72,0,0,0,0,0,0,18,0,0,0,84,0,0,0,0,0,0,0,138,1,0,0,0,0,0,0,28,0,0,0,10,10,9,11,11,9,0,0,12,0,0,0,1,0,0,0 }, (unsigned)RRT_ConfigureInputOpen, { 85,0,0,0,82,72,0,0,0,0,0,0,0,0,0,0,86,0,0,0,0,0,0,0,166,1,0,0,0,0,0,0,28,0,0,0,11,11,10,12,12,10,0,0,12,128,0,0,1,0,0,0 }, (unsigned)RRT_MenuDoneAction, { 87,0,0,0,82,72,0,0,0,0,0,0,6,0,0,0,8,0,0,0,0,0,0,0,194,1,0,0,0,0,0,0,30,0,0,0,12,12,11,0,0,11,0,0,12,0,0,0,1,0,0,0 }, (unsigned)RRT_MenuStartPressed, { 9,0,0,0,82,72,0,0 } };
struct L_tbl_optSub0 tbl_optSub0 = { { 0,0,0,0,0,0,0,0,0,0,0,0,60,1,0,0,8,1,0,0,4,1,0,0,30,0,0,0,0,0,0,0,0,0,0,0,48,128,0,0,0,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0,0,0,0,0,225,0,0,0,241,0,0,0,60,1,0,0,44,1,0,0,4,1,0,0,30,0,0,0,1,1,2,2,2,2,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_JukeboxSongPick, { 0,0,0,0,80,72,0,0,0,0,0,0,225,0,0,0,242,0,0,0,60,1,0,0,80,1,0,0,4,1,0,0,30,0,0,0,2,2,1,1,1,1,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_JukeboxSongPick, { 0,0,0,0,80,72,0,0 } };
struct L_tbl_optSub1 tbl_optSub1 = { { 0,0,0,0,0,0,0,0,0,0,0,0,60,1,0,0,8,1,0,0,4,1,0,0,30,0,0,0,0,0,0,0,0,0,0,0,48,128,0,0,0,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0,0,0,0,0,226,0,0,0,243,0,0,0,60,1,0,0,44,1,0,0,4,1,0,0,30,0,0,0,1,1,1,1,1,1,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_JukeboxSongPick, { 0,0,0,0,80,72,0,0 } };
struct L_tbl_optSub2 tbl_optSub2 = { { 0,0,0,0,0,0,0,0,0,0,0,0,60,1,0,0,8,1,0,0,4,1,0,0,30,0,0,0,0,0,0,0,0,0,0,0,48,128,0,0,0,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0,0,0,0,0,227,0,0,0,244,0,0,0,60,1,0,0,44,1,0,0,4,1,0,0,30,0,0,0,1,1,3,2,2,3,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_JukeboxSongPick, { 0,0,0,0,80,72,0,0,0,0,0,0,227,0,0,0,245,0,0,0,60,1,0,0,80,1,0,0,4,1,0,0,30,0,0,0,2,2,1,3,3,1,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_JukeboxSongPick, { 0,0,0,0,80,72,0,0,0,0,0,0,227,0,0,0,246,0,0,0,60,1,0,0,116,1,0,0,4,1,0,0,30,0,0,0,3,3,2,1,1,2,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_JukeboxSongPick, { 0,0,0,0,80,72,0,0 } };
struct L_tbl_optSub3 tbl_optSub3 = { { 0,0,0,0,0,0,0,0,0,0,0,0,60,1,0,0,8,1,0,0,4,1,0,0,30,0,0,0,0,0,0,0,0,0,0,0,48,128,0,0,0,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0,0,0,0,0,228,0,0,0,247,0,0,0,60,1,0,0,44,1,0,0,4,1,0,0,30,0,0,0,1,1,4,2,2,4,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_JukeboxSongPick, { 0,0,0,0,80,72,0,0,0,0,0,0,228,0,0,0,248,0,0,0,60,1,0,0,80,1,0,0,4,1,0,0,30,0,0,0,2,2,1,3,3,1,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_JukeboxSongPick, { 0,0,0,0,80,72,0,0,0,0,0,0,228,0,0,0,249,0,0,0,60,1,0,0,116,1,0,0,4,1,0,0,30,0,0,0,3,3,2,4,4,2,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_JukeboxSongPick, { 0,0,0,0,80,72,0,0,0,0,0,0,228,0,0,0,250,0,0,0,60,1,0,0,152,1,0,0,4,1,0,0,30,0,0,0,4,4,3,1,1,3,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_JukeboxSongPick, { 0,0,0,0,80,72,0,0 } };
struct L_tbl_optSub4 tbl_optSub4 = { { 0,0,0,0,0,0,0,0,0,0,0,0,60,1,0,0,8,1,0,0,4,1,0,0,30,0,0,0,0,0,0,0,0,0,0,0,48,128,0,0,0,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0,0,0,0,0,229,0,0,0,251,0,0,0,60,1,0,0,44,1,0,0,4,1,0,0,30,0,0,0,1,1,2,2,2,2,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_JukeboxSongPick, { 0,0,0,0,80,72,0,0,0,0,0,0,229,0,0,0,252,0,0,0,60,1,0,0,80,1,0,0,4,1,0,0,30,0,0,0,2,2,1,1,1,1,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_JukeboxSongPick, { 0,0,0,0,80,72,0,0 } };
struct L_tbl_optSub5 tbl_optSub5 = { { 0,0,0,0,0,0,0,0,0,0,0,0,60,1,0,0,8,1,0,0,4,1,0,0,30,0,0,0,0,0,0,0,0,0,0,0,48,128,0,0,0,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0,0,0,0,0,230,0,0,0,253,0,0,0,60,1,0,0,44,1,0,0,4,1,0,0,30,0,0,0,1,1,2,2,2,2,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_JukeboxSongPick, { 0,0,0,0,80,72,0,0,0,0,0,0,230,0,0,0,254,0,0,0,60,1,0,0,80,1,0,0,4,1,0,0,30,0,0,0,2,2,1,1,1,1,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_JukeboxSongPick, { 0,0,0,0,80,72,0,0 } };
unsigned tbl_musicVideos[2] = {
    (unsigned)&s_fontNames.s_Jessie,(unsigned)&s_fontNames.s_Duel
};
struct L_tbl_optJukebox tbl_optJukebox = { { 0,0,0,0,0,0,0,0,0,0,0,0,60,1,0,0,8,1,0,0,4,1,0,0,30,0,0,0,0,0,0,0,0,0,0,0,48,128,0,0,0,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0,0,0,0,0,0,0,0,0,160,0,0,0,60,1,0,0,44,1,0,0,4,1,0,0,30,0,0,0,1,1,2,2,2,2,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_OptionPageNext, { 0,0,0,0,80,72,0,0,0,0,0,0,1,0,0,0,161,0,0,0,60,1,0,0,80,1,0,0,4,1,0,0,30,0,0,0,2,2,1,1,1,1,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_OptionPageNext, { 0,0,0,0,80,72,0,0 } };
Rec_optItems rec_optItems = {
    /* tbl_optItems */ { { 0,0,0,0,0,0,0,0,225,0,0,0,0,0,0,0,134,0,0,0,0,0,0,0,42,0,0,0,0,0,7,1,1,7,0,0,12,0,0,0,1,0,0,0 }, (unsigned)RRT_OptionPageSelect, { 231,0,0,0,82,72,0,0,0,0,0,0,0,0,0,0,226,0,0,0,0,0,0,0,176,0,0,0,0,0,0,0,42,0,0,0,1,1,0,2,2,0,0,0,12,0,0,0,1,0,0,0 }, (unsigned)RRT_OptionPageSelect, { 232,0,0,0,82,72,0,0,0,0,0,0,0,0,0,0,227,0,0,0,0,0,0,0,218,0,0,0,0,0,0,0,42,0,0,0,2,2,1,3,3,1,0,0,12,0,0,0,1,0,0,0 }, (unsigned)RRT_OptionPageSelect, { 233,0,0,0,82,72,0,0,0,0,0,0,0,0,0,0,228,0,0,0,0,0,0,0,4,1,0,0,0,0,0,0,42,0,0,0,3,3,2,4,4,2,0,0,12,0,0,0,1,0,0,0 }, (unsigned)RRT_OptionPageSelect, { 234,0,0,0,82,72,0,0,0,0,0,0,0,0,0,0,229,0,0,0,0,0,0,0,46,1,0,0,0,0,0,0,42,0,0,0,4,4,3,5,5,3,0,0,12,0,0,0,1,0,0,0 }, (unsigned)RRT_OptionPageSelect, { 235,0,0,0,82,72,0,0,0,0,0,0,0,0,0,0,230,0,0,0,0,0,0,0,88,1,0,0,0,0,0,0,42,0,0,0,5,5,4,6,6,4,0,0,12,0,0,0,1,0,0,0 }, (unsigned)RRT_OptionPageSelect, { 236,0,0,0,82,72,0,0,0,0,0,0,0,0,0,0,175,1,0,0,0,0,0,0,130,1,0,0,0,0,0,0,42,0,0,0,6,6,5,7,7,5,0,0,12,0,0,0 } },
    /* tbl_optNetwork */ {
        0x1,(unsigned)RRT_JukeboxPageOpen,0x1f0,0x4852,0,0x10,0x56,0,0x1ac,0,0x34,0x60707,0x600,0x800c,0x1,0,0x57,0x4852,0,0,0,0,0x72,
        0,0x14,0,0,0,0,0,0,0
    }
};
struct L_tbl_optResolution tbl_optResolution = { { 0,0,0,0,0,0,0,0,0,0,0,0,60,1,0,0,8,1,0,0,4,1,0,0,30,0,0,0,0,0,0,0,0,0,0,0,48,128,0,0,0,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0,0,0,0,0,1,0,0,0,88,0,0,0,60,1,0,0,44,1,0,0,4,1,0,0,30,0,0,0,1,1,3,2,2,3,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0,0,0,0,0,2,0,0,0,129,1,0,0,60,1,0,0,80,1,0,0,4,1,0,0,30,0,0,0,2,2,1,3,3,1,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0,0,0,0,0,3,0,0,0,89,0,0,0,60,1,0,0,116,1,0,0,4,1,0,0,30,0,0,0,3,3,2,1,1,2,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0 } };
struct L_tbl_optBikers tbl_optBikers = { { 0,0,0,0,0,0,0,0,0,0,0,0,60,1,0,0,8,1,0,0,4,1,0,0,30,0,0,0,0,0,0,0,0,0,0,0,48,128,0,0,0,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0,0,0,0,0,1,0,0,0,144,1,0,0,60,1,0,0,44,1,0,0,4,1,0,0,30,0,0,0,1,1,2,2,2,2,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0,0,0,0,0,2,0,0,0,145,1,0,0,60,1,0,0,80,1,0,0,4,1,0,0,30,0,0,0,2,2,1,1,1,1,0,0,16,0,0,0 } };
char g_hiResBikersFound = 1;
struct L_tbl_optVideoMode tbl_optVideoMode = { { 0,0,0,0,0,0,0,0,0,0,0,0,60,1,0,0,8,1,0,0,4,1,0,0,30,0,0,0,0,0,0,0,0,0,0,0,48,128,0,0,0,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0,0,0,0,0,1,0,0,0,177,1,0,0,60,1,0,0,44,1,0,0,4,1,0,0,30,0,0,0,1,1,3,2,2,3,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0,0,0,0,0,2,0,0,0,178,1,0,0,60,1,0,0,80,1,0,0,4,1,0,0,30,0,0,0,2,2,1,3,3,1,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0,0,0,0,0,3,0,0,0,179,1,0,0,60,1,0,0,116,1,0,0,4,1,0,0,30,0,0,0,3,3,2,1,1,2,0,0,16,0,0,0,1,0,0,0 }, (unsigned)RRT_OptionPageClose, { 0,0,0,0,80,72,0,0,0,0,0,0,0,0,0,0,72,0,0,0,0,0,0,0,134,0,0,0,180,0,0,0,48,0,0,0,0,0,6,1,1,6,0,0,12,0,0,0,1,0,0,0 }, (unsigned)RRT_ResolutionPageOpen, { 73,0,0,0,82,72,0,0,0,0,0,0,0,0,0,0,140,1,0,0,0,0,0,0,182,0,0,0,180,0,0,0,48,0,0,0,1,1,0,2,2,0,0,0,12,0,0,0,1,0,0,0 }, (unsigned)RRT_BikersPageOpen, { 141,1,0,0,82,72,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,230,0,0,0,180,0,0,0,48,0,0,0,2,2,1,3,3,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,175,1,0,0,0,0,0,0,22,1,0,0,180,0,0,0,48,0,0,0,3,3,2,4,4,2,0,0,12,0,0,0,1,0,0,0 }, (unsigned)RRT_VideoModePageOpen, { 176,1,0,0,82,72,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,70,1,0,0,180,0,0,0,48,0,0,0,4,4,3,5,5,3,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,118,1,0,0,180,0,0,0,48,0,0,0,5,5,4,6,6,4,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,16,0,0,0,86,0,0,0,0,0,0,0,166,1,0,0,180,0,0,0,58,0,0,0,6,6,5,0,0,5,0,0,12,128,0,0,1,0,0,0,0,0,0,0,87,0,0,0,82,72,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,114,0,0,0,180,0,0,0,20,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } };
int g_screenADefault = 13;
int g_screenBDefault = 9;
int g_screenCDefault = 8;
char s_RRS[4] = "RRS";
char s_pcts_colon_pcts[12] = {
    37,115,32,58,32,-109,37,115,-108
};
char s_HiBob[8] = "HiBob";
char g_chatState[888] = {
    2,0,0,0,0,0,0,0,2,-128,0,0,0,0,0,0,-34,0,0,0,86,0,0,0,-16,0,0,0,0,-128,0,0,-20,0,0,0,-34,0,0,0,64,1,0,0,-16,0,0,0,0,0,0,0,-42,0,0,0,-61,0,0,0,64,
    1,0,0,-16,0,0,0,0,0,0,0,-62,0,0,0,-36,0,0,0,0,0,18,0,0,0,12,0,0,0,0,0,124,0,0,0,-32,0,0,0,0,0,22,0,0,0,18,0,1,0,0,0,10,0,0,0,-57,0,0,0,64,1,0,0,
    -16,0,0,0,2,0,0,0,0,0,0,0,-31,0,0,0,-82,0,0,0,-16,0,0,0,0,0,0,0,-119,0,0,0,-69,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,66,0,0,0,-45,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,-110,0,0,0,-66,0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,-28,0,0,0,-45,0,0,0,0,0,0,0,0,0,0,0,2,-128,0,0,0,0,0,0,-54,0,0,0,79,0,0,0,-16,0,0,0,0,-128,0,0,
    -17,0,0,0,-54,0,0,0,64,1,0,0,-16,0,0,0,0,0,0,0,30,1,0,0,-40,0,0,0,64,1,0,0,-16,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,-38,
    0,0,0,120,0,0,0,-16,0,0,0,1,0,0,0,0,0,0,0,-54,0,0,0,64,1,0,0,-16,0,0,0,2,0,0,0,0,0,0,0,-38,0,0,0,-44,0,0,0,-16,0,0,0,0,0,0,0,-122,0,0,0,-24,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,47,0,0,0,-36,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-113,0,0,0,-36,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-17,0,0,0,-36,0,0,0,0,0,0,0,0,0,0,
    0,2,-128,0,0,0,0,0,0,-66,1,0,0,-88,0,0,0,-32,1,0,0,0,-128,0,0,-40,1,0,0,-66,1,0,0,-128,2,0,0,-32,1,0,0,0,0,0,0,-76,1,0,0,-118,1,0,0,-128,2,0,0,
    -32,1,0,0,0,0,0,0,-124,1,0,0,-71,1,0,0,0,0,28,0,0,0,20,0,0,0,0,0,-6,0,0,0,-63,1,0,0,0,0,44,0,0,0,28,0,1,0,0,0,16,0,0,0,-113,1,0,0,-128,2,0,0,-32,
    1,0,0,2,0,0,0,0,0,0,0,-60,1,0,0,89,1,0,0,-32,1,0,0,0,0,0,0,15,1,0,0,118,1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,-122,0,0,0,-88,1,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,36,1,0,0,124,1,0,0,0,0,0,0,0,0,0,0,1,0,0,0,-56,1,0,0,-88,1,0,0,0,0,0,0,0,0,0,0,2,-128,0,0,0,0,0,0,-108,1,0,0,-98,0,0,0,-32,1,0,0,0,-128,0,0,-34,
    1,0,0,-108,1,0,0,-128,2,0,0,-32,1,0,0,0,0,0,0,58,2,0,0,-80,1,0,0,-128,2,0,0,-32,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,2,0,0,0,0,0,0,0,-76,
    1,0,0,-16,0,0,0,-32,1,0,0,1,0,0,0,0,0,0,0,-108,1,0,0,-128,2,0,0,-32,1,0,0,2,0,0,0,0,0,0,0,-76,1,0,0,-88,1,0,0,-32,1,0,0,0,0,0,0,12,1,0,0,-48,1,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,94,0,0,0,-72,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,30,1,0,0,-72,1,0,0,0,0,0,0,0,0,0,0,0,0,0,0,-34,1,0,0,-72,1
};
char s_pcts_4[12] = "        %s";
char s_pct3dK_pctd_pctd[20] = "        %3dK %d %d";
char s_pcts_pcts_3[8] = " -%s %s";
char s_pcts_pct3dK_pctd_pctd[16] = " -%s %3dK %d %d";
char s_pcts_pcts_4[8] = " +%s %s";
char s_pcts_pct3dK_pctd_pctd_2[16] = " +%s %3dK %d %d";
char s_pctd_pctd_2[8] = "%d:%d";
char s_hudFmtA[4] = {
    -23
};
char s_hudFmtB[4] = {
    -22
};
char s_pctdpctcpct03d[16] = "%d%c%03d";
int g_musicGroup;
int g_configBlock;
int g_videoMode = 2;
char g_altArtwork[4];
char g_unsavedChanges[4];
int g_gameMode;
HINSTANCE g_textModule;
int g_mainWindow;
char g_musicBusy[8];
int g_surfaceA;
int g_surfaceB;
int *g_screenSurface;
int *g_primarySurface;
char g_dialogOnce = 1;
char s_NoVideo[8] = "NoVideo";
char s_Intro[8] = "Intro";
char s_PapyLogo[12] = "PapyLogo";
char s_StudLogo[12] = "StudLogo";
char s_Wreck[8] = "Wreck";
char s_Busted[8] = "Busted";
char s_Lose[8] = "Lose";
char s_Win[4] = "Win";
char s_Level[8] = "Level";
char s_FinalWin[12] = "FinalWin";
unsigned g_billboardFlags = 0x10;
unsigned tbl_controlBits[38] = {
    1,0,2,1,4,0,8,1,0x10,1,0x20,1,0x40,1,0x80,1,0x100,1,0x200,0,0x400,1,0x800,1,0x1000,1,0x4000,1,0x2000,1,0x800,1,0x1000,1,0x4000,1,0x2000,1
};
char g_inputInit;
char s_1_0A_11_18_56_Fri_08_23_96[32] = "1.0A\n(11:18:56 Fri 08-23-96)";
int tbl_versionInfo = (int)((unsigned)&s_1_0A_11_18_56_Fri_08_23_96);
unsigned g_ctypeTable[131] = {
    (unsigned)((unsigned char *)&g_ctypeTable + 10),(unsigned)((unsigned char *)&g_ctypeTable + 10),0x200000,0x200020,0x200020,0x200020,0x200020,
    0x280028,0x280028,0x200028,0x200020,0x200020,0x200020,0x200020,0x200020,0x200020,0x200020,0x200020,0x480020,0x100010,0x100010,0x100010,0x100010,
    0x100010,0x100010,0x100010,0x840010,0x840084,0x840084,0x840084,0x840084,0x100084,0x100010,0x100010,0x100010,0x810081,0x810081,0x810081,0x10001,
    0x10001,0x10001,0x10001,0x10001,0x10001,0x10001,0x10001,0x10001,0x10001,0x100010,0x100010,0x100010,0x820082,0x820082,0x820082,0x20002,0x20002,
    0x20002,0x20002,0x20002,0x20002,0x20002,0x20002,0x20002,0x20002,0x100010,0x100010,0x20
};
int g_mbCurMax = 1;
HINSTANCE g_hInstance;
char tbl_bindingTexts[576];
int g_controlRow;
char g_riderNameBuf[80];
char g_riderNameBuf2[32];
char g_riderNameBuf3[80];
unsigned char g_pitSlotsUsed;
char g_pitMessageCycle;
char g_pitMessageBuf[264];
char g_raceMessageBuf[264];
char g_keyCtrl;
int g_statusWindow;
char g_statusHotkey;
HFONT g_fontMenu;
HFONT g_fontText;
int g_focusBase[2];
unsigned tbl_controlWindows[26];
HWND g_hoverWindow;
HRGN g_updateRegion;
char g_keyAlt;
HWND g_savedFocus;
char g_statusText[128];
Rec_cursorPos rec_cursorPos = {
    /* g_cursorPos */ {0},
    /* g_cursorPosY */ 0
};
char g_menuRepaint;
HHOOK g_keyboardHook;
char g_creditKeys[8];
Rec_regionData rec_regionData = {
    /* g_regionData */ {0},
    /* g_dirtyRectCount */ {0},
    /* tbl_dirtyRects */ {0}
};
int g_pageMark;
HFONT g_fontSmall;
HFONT g_fontCourier;
HFONT g_fontFutura;
HFONT g_fontFuturaDemi;
HFONT g_fontBadloc;
int g_lastControl;
char g_tabLock;
int *g_currentPage;
HFONT g_fontDialog;
char g_itemHotkey;
HFONT g_fontTitle;
char g_lineBuffer[2048];
char g_keyShift;
char g_creditKeysWanted;
HWND g_clickedWindow;
Rec_menuTextBuf rec_menuTextBuf = {
    /* g_menuTextBuf */ {0},
    /* tbl_keyPages */ {0}
};
int g_controlCount;
HANDLE g_clipRegion;
int tbl_controlRecords[26];
int g_creditsLines;
void *g_creditsText;
Rec_slotCount rec_slotCount = {
    /* g_slotCount */ 0,
    /* tbl_slotTexts */ {0}
};
int g_bigGameHub;
char g_stringBuf[256];
char g_numberBuf[16];
char g_decimalSep[8];
char g_metricFlag[8];
char g_weightFmt[32];
char g_lengthFmt[32];
char g_lengthBuf[32];
char g_weightBuf[40];
int g_tipCycle;
char tbl_playerSlots[244];
HWND g_dialogWindow;
char g_scoreTextB[32];
unsigned char g_openFileName[80];
char g_scoreTextA[32];
int g_optionCleanup;
unsigned tbl_hudPens[2];
char g_hudMessage[40];
char g_hudRiderName[40];
int g_hudHeading;
int g_hudSpeed;
int g_hudSpeedFrac;
char g_hudLap;
char g_hudPosition;
unsigned char g_hudFlashTick;
char g_hudBarA;
char g_hudBarB;
char g_hudBarC[7];
int g_hudShadow;
int g_inputRepeat;
int g_nearestDist[363];
HFONT g_fontCourierSmall;
HFONT g_hudFont;
