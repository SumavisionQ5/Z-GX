/*  Copyright 2008 Theo Berkau
    This file is part of Yabause.

    Yabause is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    Yabause is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Yabause; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
*/

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <malloc.h>
#include <wiiuse/wpad.h>
#include <ogcsys.h>
#include <gccore.h>
#include <fat.h>
#include <sdcard/wiisd_io.h>
#include <dirent.h>


//#include <di/di.h>
#include "osd/osd.h"
#include "osd/gui.h"
#include "gui/gui.h"
#include "sgx/svi.h"

#include "cart.h"
#include "cs2.h"
#include "snd.h"
#include "m68kcore.h"
#include "peripheral.h"
#include "vdp2.h"
#include "scsp.h"

#include "memory.h"

/* Wii includes */
//#include "syswii.h"


int CoreExec(void);

void InitMenu(void);
void Check_Bios_Exist(void);
void Check_SD_Init(void);
void Check_Read_Dir(void);
void scroll_filelist(s8 delta);
void print_filelist(void);
void scroll_cartlist(s8 delta);
void print_cartlist(void);
void scroll_settinglist(s8 delta);
void print_settinglist(void);

/* Constants */

static int IsPal = 0;
volatile int exec_emu = 0;
volatile int reset_emu = 0;
int running=1;

#ifdef AUTOLOADPLUGIN
u8 autoload = 0;
u8 returnhomebrew = 0;
//#include "homebrew.h"
//#define TITLE_ID(x,y) (((u64)(x) << 32) | (y))
char Exit_Dol_File[1024];
#endif

PerInterface_struct *PERCoreList[] = {
&PERDummy,
NULL
};

CDInterface *CDCoreList[] = {
&DummyCD,
&ISOCD,
NULL
};

#ifdef SCSP_PLUGIN
SCSPInterface_struct *SCSCoreList[] = {
&SCSDummy,
&SCSScsp2,
&SCSScsp2,
NULL
};
#endif


//XXX: Change the directory.
char games_dir[128];
char saves_dir[128];
char carts_dir[128];
static char biospath[32];
char prev_itemnum[512];
static char buppath[512];
char settingpath[512];
static char isofilename[512]="";

enum MessageId {
	MESSAGE_NO_FOLDER,
	MESSAGE_NO_BIOS,
	MESSAGE_NO_GAMES,
	MESSAGE_GAME_SAVED,
	MESSAGE_GAME_NOT_SAVED,
};


const String gui_messages[8] = {
	{30," Could not "
		"open games "
		"  folder"},
	{18,"No BIOS.bin"
		"  found"},
	{19,"No games   "
		" found"},
	{11,"Game saved!"},
	{20,"Could not  "
		"save game"},
};

extern int vdp2width, vdp2height;
int wii_width, wii_height;
extern int wii_ir_x;
extern int wii_ir_y;
extern int wii_ir_valid;

#define FILES_PER_PAGE	10

struct file *filelist = NULL;
u32 nb_files = 0;
#define CART_TYPE_NUM 11
#define SETTING_NUM 14

void gotoxy(int x, int y);
void OnScreenDebugMessage(char *string, ...);
void SetMenuItem();
void DoMenu();
int GameExec();
int LoadCue();
int Settings();
int CartSet();
int CartSetExec();
int BiosWith();
int BiosOnlySet();
int FrameskipOff();

void TexCopy_LoRes(u32 w, u32 h);


static s32 selected = 0, start = 0;
int cart_enabled = 0; //toggle cartucho RAM 4MB desde el menu (Z)
int p240_pref = 0; //preferencia 240p (refleja si existe 240p.txt)
// ===== Menu de opciones =====
int opt_in_menu = 0;      // 0 = lista de juegos, 1 = menu de opciones
int opt_cursor = 0;       // opcion seleccionada en el menu
int opt_scanline = 0;     // 1 = filtro scanline ON
int opt_lightgun = 0;     // 1 = lightgun ON
int opt_bezel = 0;        // 1 = bezel (marco) ON
int opt_fps = 1;          // 1 = mostrar FPS (default ON, como estaban fijos antes)
#define OPT_COUNT 8        // cantidad de opciones en el menu
char g_device_path[16] = "sd:/"; //ruta del dispositivo activo (sd:/ o usb:/), para 240p y covers
static s32 selectedcart = 7;
static int bioswith = 0;
static int frameskipoff = 0;
int specialcoloron = 1;
int eachbackupramon = 1;
int threadingscsp2on = 1;
int eachsettingon = 1;

int menuselect=1;
int setmenuselect=0;
int sounddriverselect=2;
int videodriverselect=2;
int m68kdriverselect=1;
int scspdriverselect=2;
int configurebuttonsselect=0;
int WIIconfigurebuttonsselect=0;
int CLAconfigurebuttonsselect=0;
int GCCconfigurebuttonsselect=0;
int settimingselect=0;
int aboutmenuselect=6;


extern int smpcperipheraltiming;
extern int smpcothertiming;
extern int declinenum;
extern int dividenumclock;

static BOOL flag_mount = FALSE;

GXTexObj tex_lores_fb;

extern int fpstoggle;
extern int fps;

void fat_remount()
{
   if(flag_mount)
   {
      fatUnmount("fat:/");
      flag_mount = FALSE;
   }
   fatInitDefault();
   flag_mount = TRUE;
}

void reset()
{
   reset_emu = 1;
}

void powerdown()
{
	exec_emu = 1;
	//SYS_ResetSystem(SYS_POWEROFF, 0, 0);  //Shutdown wii
	//SYS_ResetSystem(SYS_RETURNTOMENU, 0, 0); // Goto wii menu
}

#define GUI_FILENAMES_MAX		15


GuiItems filename_items = { 0, 0, 0, GUI_FILENAMES_MAX, 0, 10, NULL };
char* game_name_strings;
u32 games_filecount;
u32 games_cursor;

static int fn_compare(const void *x, const void *y) {
        return strcasecmp(((String *)x)->data, ((String *)y)->data);
}

s32 games_LoadList()
{

	u32 str_pos = 0;
	struct dirent *entry;
	DIR *dp = opendir(games_dir);
	if (!dp) {
		gui_SetMessage(gui_messages[MESSAGE_NO_FOLDER], 0xffff);
		return -1;
	}
	//Allocate memory for filename storing
	game_name_strings = (char*) calloc(0x10000, sizeof(*game_name_strings));	//64KB for now
	filename_items.item = (String*) calloc(1024, sizeof(*filename_items.item));	//1024 entries
	games_filecount = 0;

	while ((entry = readdir(dp))) {
		u32 len = strlen(entry->d_name) + 1;
		if (len > 5 && (!strcasecmp(entry->d_name + len - 5, ".cue") || !strcasecmp(entry->d_name + len - 5, ".chd"))) {
			char *str_dst = &game_name_strings[str_pos];
			strcpy(str_dst, entry->d_name);
			filename_items.item[games_filecount].len = len;
			filename_items.item[games_filecount].data = str_dst;
			games_filecount++;
			str_pos += len;
		}
	}
	filename_items.count = games_filecount;
	if (!filename_items.count) {
		closedir(dp);
		gui_SetMessage(gui_messages[MESSAGE_NO_GAMES], 0xffff);
		return -1;
	}

	qsort(filename_items.item, games_filecount, sizeof(*filename_items.item), fn_compare);
	closedir(dp);
	return 0;
}



#define MENU_BOX_SELECT		0x3

u32 menu_select_index = 0;
u32 axis_button = 0;
u32 axis_button_prev = 0;
u32 menu_num_items = 4;
u32 wait_for_sync = 1;



void menu_Init(void)
{
	//filename_list_init();
	axis_button = 0;
	axis_button_prev = 0;
	wait_for_sync = 1;
}

// ===== Guardar/cargar config del menu de opciones =====
void options_Save(void)
{
	char path[48];
	sprintf(path, "%sZGX/config.txt", g_device_path);
	FILE *f = fopen(path, "wb");
	if (f) {
		fprintf(f, "fps=%d\n", opt_fps);
		fprintf(f, "cart=%d\n", cart_enabled);
		fprintf(f, "frameskip=%d\n", frameskipoff);
		fprintf(f, "scanline=%d\n", opt_scanline);
		fprintf(f, "bezel=%d\n", opt_bezel);
		fprintf(f, "lightgun=%d\n", opt_lightgun);
		fclose(f);
	}
}
void options_Load(void)
{
	char path[48];
	sprintf(path, "%sZGX/config.txt", g_device_path);
	FILE *f = fopen(path, "rb");
	if (f) {
		char line[32]; int v;
		while (fgets(line, sizeof(line), f)) {
			if (sscanf(line, "fps=%d", &v) == 1) opt_fps = v;
			else if (sscanf(line, "cart=%d", &v) == 1) cart_enabled = v;
			else if (sscanf(line, "frameskip=%d", &v) == 1) frameskipoff = v;
			else if (sscanf(line, "scanline=%d", &v) == 1) opt_scanline = v;
			else if (sscanf(line, "bezel=%d", &v) == 1) opt_bezel = v;
			else if (sscanf(line, "lightgun=%d", &v) == 1) opt_lightgun = v;
		}
		fclose(f);
	}
}
// Devuelve el texto de cada opcion en un buffer
static void opt_GetLine(int idx, char *buf)
{
	switch (idx) {
		case 0: sprintf(buf, "Show FPS:    %s", opt_fps ? "ON" : "OFF"); break;
		case 1: sprintf(buf, "240p Video:  %s", p240_pref ? "ON" : "OFF"); break;
		case 2: sprintf(buf, "4MB Cart:    %s", cart_enabled ? "ON" : "OFF"); break;
		case 3: sprintf(buf, "Frameskip:   %s", frameskipoff ? "ON" : "OFF"); break;
		case 4: sprintf(buf, "Scanlines:   %s", opt_scanline ? "ON" : "OFF"); break;
		case 5: sprintf(buf, "Bezel:       %s", opt_bezel ? "ON" : "OFF"); break;
		case 6: sprintf(buf, "Lightgun:    %s", opt_lightgun ? "ON" : "OFF"); break;
		case 7: sprintf(buf, "BIOS:        (soon)"); break;
	}
}
// Cambia el valor de la opcion seleccionada
static void opt_Toggle(int idx)
{
	switch (idx) {
		case 0: opt_fps ^= 1; break;
		case 1: {
			char p[48]; sprintf(p, "%sZGX/240p.txt", g_device_path);
			p240_pref ^= 1;
			if (p240_pref) { FILE*pf=fopen(p,"wb"); if(pf)fclose(pf); } else remove(p);
		} break;
		case 2: cart_enabled ^= 1; break;
		case 3: frameskipoff ^= 1; break;
		case 4: opt_scanline ^= 1; break;
		case 5: opt_bezel ^= 1; break;
		case 6: opt_lightgun ^= 1; break;
		case 7: break; // BIOS placeholder
	}
	options_Save();
}

u32 menu_Handle(void)
{
	u32 buttons;
	per_updatePads();
	buttons = PER_BUTTONS_DOWN(0);
	// ===== Menu de opciones (START+Y para entrar y salir - universal: GC/Wiimote/Classic) =====
	{
		static u32 _spv = 0;
		u32 _held = PER_BUTTONS_HELD(0);
		u32 _sn = ((_held & PAD_DI_STR) && (_held & PAD_DI_Y)) ? 1 : 0;
		if (_sn && !_spv) { opt_in_menu ^= 1; if (opt_in_menu) opt_cursor = 0; }
		_spv = _sn;
	}
	if (opt_in_menu) {
		u32 held = PER_BUTTONS_HELD(0);
		static int ohf = 0;
		u32 ob = buttons;
		if (held & (PAD_DI_UP | PAD_DI_DOWN)) { ohf++; if (ohf > 30 && (ohf % 4 == 0)) ob |= (held & (PAD_DI_UP | PAD_DI_DOWN)); }
		else ohf = 0;
		u32 oa = PAD_DI_C;
		if (perpad[0].type == PAD_TYPE_GCPAD) oa = PAD_DI_B;
		else if (perpad[0].type == PAD_TYPE_N64PAD) oa = PAD_DI_A;
		if (ob & PAD_DI_UP)    { if (opt_cursor == 0) opt_cursor = OPT_COUNT - 1; else opt_cursor--; }
		else if (ob & PAD_DI_DOWN)  { if (opt_cursor == OPT_COUNT - 1) opt_cursor = 0; else opt_cursor++; }
		else if (ob & (oa | PAD_DI_LEFT | PAD_DI_RIGHT)) { opt_Toggle(opt_cursor); }
		// dibujar el menu de opciones reusando gui_Draw (hace el setup GX correcto)
		{
			static char optbuf[OPT_COUNT][52];
			static String optstr[OPT_COUNT];
			static GuiItems optitems;
			int i;
			for (i = 0; i < OPT_COUNT; i++) {
				char tmp[48];
				opt_GetLine(i, tmp);
				sprintf(optbuf[i], "%s", tmp);
				optstr[i].data = (u8*)optbuf[i];
				optstr[i].len = strlen(optbuf[i]);
			}
			optitems.cursor = opt_cursor;
			optitems.count = OPT_COUNT;
			optitems.disp_offset = 0;
			optitems.disp_count = OPT_COUNT;
			optitems.x = 0;
			optitems.y = 10;
			optitems.item = optstr;
			gui_Draw(&optitems);
		}
		SVI_CopyFrame();
		SVI_SwapBuffers(1);
		return GUI_RET_NONE;
	}
	{ static u32 _cpv=0; u32 _cn=((PER_BUTTONS_HELD(0)&(PAD_DI_L|PAD_DI_R))==(PAD_DI_L|PAD_DI_R)); if(_cn && !_cpv) cart_enabled^=1; _cpv=_cn; } //TOGGLE cart 4MB con L+R
	{ static u32 _ppv=0; u32 _pn=((PER_BUTTONS_HELD(0)&(PAD_DI_Z|PAD_DI_Y))==(PAD_DI_Z|PAD_DI_Y)); if(_pn && !_ppv) { char _p2p[32]; sprintf(_p2p, "%sZGX/240p.txt", g_device_path); p240_pref^=1; if(p240_pref){ FILE*_pf=fopen(_p2p,"wb"); if(_pf)fclose(_pf); } else { remove(_p2p); } } _ppv=_pn; } //TOGGLE 240p con Z+Y (usa g_device_path)
	{
		u32 held = PER_BUTTONS_HELD(0);
		static int hold_frames = 0;
		if (held & (PAD_DI_UP | PAD_DI_DOWN)) {
			hold_frames++;
			if (hold_frames > 30 && (hold_frames % 3 == 0)) {
				buttons |= (held & (PAD_DI_UP | PAD_DI_DOWN));
			}
		} else {
			hold_frames = 0;
		}
	}
	u32 btn_a = PAD_DI_C;
	u32 btn_b = PAD_DI_B;
	if (perpad[0].type == PAD_TYPE_GCPAD) {
		btn_a = PAD_DI_B;
		btn_b = PAD_DI_A;
	} else if (perpad[0].type == PAD_TYPE_N64PAD){
		btn_a = PAD_DI_A;
		btn_b = PAD_DI_X;
	}

	if(buttons & PAD_DI_DOWN) {
		if (filename_items.cursor == filename_items.count - 1) {
			filename_items.cursor = 0;
			filename_items.disp_offset = 0;
		}
		else {
			u32 disp_end = filename_items.disp_offset + filename_items.disp_count;
			if ((disp_end == filename_items.cursor + 5) & (disp_end < filename_items.count) ) {
				filename_items.disp_offset = (filename_items.cursor + 6) - filename_items.disp_count;
			}
			filename_items.cursor++;
		}
	}
	else if(buttons & PAD_DI_UP) {
		if (filename_items.cursor == 0) {
			filename_items.cursor = filename_items.count - 1;
			filename_items.disp_offset = (filename_items.count - filename_items.disp_count);
			filename_items.disp_offset = (filename_items.disp_offset > filename_items.count ? 0 : filename_items.disp_offset);
		}
		else {
			if ((filename_items.disp_offset + 5 == filename_items.cursor) & (filename_items.disp_offset > 0)) {
				filename_items.disp_offset = (filename_items.cursor - 6);
			}
			filename_items.cursor--;
		}
	}
	else if(buttons & PAD_DI_LEFT) {
		if (filename_items.cursor >= filename_items.disp_count)
			filename_items.cursor -= filename_items.disp_count;
		else
			filename_items.cursor = 0;
		if (filename_items.disp_offset >= filename_items.disp_count)
			filename_items.disp_offset -= filename_items.disp_count;
		else
			filename_items.disp_offset = 0;
	}
	else if(buttons & PAD_DI_RIGHT) {
		filename_items.cursor += filename_items.disp_count;
		if (filename_items.cursor >= filename_items.count)
			filename_items.cursor = filename_items.count - 1;
		if (filename_items.disp_offset + filename_items.disp_count < filename_items.count)
			filename_items.disp_offset += filename_items.disp_count;
	}
	else if (buttons & btn_a) {	//Uses the C button as A button
		if (filename_items.count && filename_items.cursor < filename_items.count) {
			//FPS controlado por el menu de opciones (opt_fps)
			if (opt_fps) yabsys.flags |= SYS_FLAGS_SHOW_FPS;
			else yabsys.flags &= ~SYS_FLAGS_SHOW_FPS;
			//Set up file name
			sprintf(isofilename, "%s/%s", games_dir, filename_items.item[filename_items.cursor].data);
			return GUI_RET_SELECT;
		}
	}
	else if (buttons & btn_b) {	//Uses the B button as B button
		//XXX: ask to quit?
		mem_Deinit();
		exit(0);
		//SYS_ResetSystem(SYS_SHUTDOWN, 0, 0);
	}

	gui_Draw(&filename_items);
	SVI_CopyFrame();
	SVI_SwapBuffers(1);
	return GUI_RET_NONE;
}

int ResetSettings(void)
{
   bioswith = 0;
   selectedcart = 7;
   videodriverselect = 2;
   sounddriverselect = 2;
   m68kdriverselect = 1;
   scspdriverselect = 2;
   //selected;
   frameskipoff = 0;
   specialcoloron = 1;
   smpcperipheraltiming = 1000;
   smpcothertiming = 1050;
   eachbackupramon = 1;
   declinenum = 15;
   dividenumclock = 1;
   threadingscsp2on = 0;

   SetMenuItem();

   menuselect=1;
   return 0;
}


//Usage
// sega-gx <game_filepath> <*saves_folder>
int main(int argc, char **argv)
{
	u32 gui_value = GUI_RET_NONE;
	char *device_path = NULL;
	L2Enhance();
	SYS_SetResetCallback(reset);
	SYS_SetPowerCallback(powerdown);


	usleep(500000);
	//Autoload the gamefile
	if (argc > 1) {
		// 1) Montar el dispositivo PRIMERO (antes de escribir cualquier archivo)
		if ((argv[1][0]=='u' || argv[1][0]=='U') && fatMountSimple("usb", &__io_usbstorage)) {
			device_path = "usb:/";
		} else if (fatMountSimple("sd", &__io_wiisd)) {
			device_path = "sd:/";
		} else if (fatMountSimple("usb", &__io_usbstorage)) {
			device_path = "usb:/";
		}
		if (!device_path) device_path = "sd:/"; // fallback
		strcpy(g_device_path, device_path);
		// 3) Armar el path del juego
		gui_value = GUI_RET_SELECT;
		if (argc > 2) { sprintf(isofilename, "%s/%s", argv[1], argv[2]); } else { strcpy(isofilename, argv[1]); }
		// 4) Preferencia 240p
		{ char _p2r[48]; sprintf(_p2r, "%sZGX/240p.txt", g_device_path); FILE *_p2f = fopen(_p2r, "rb"); if (_p2f) { p240_pref = 1; fclose(_p2f); } }
		options_Load(); //cargar config tambien en el camino WiiFlow
	} else {
		if(fatMountSimple("sd", &__io_wiisd)) {
			device_path = "sd:/";
		} else if (fatMountSimple("usb", &__io_usbstorage)){
			device_path = "usb:/";
		} else {
			//Device not found notice
		}
		if (device_path) strcpy(g_device_path, device_path); //guardar ruta del dispositivo para 240p/covers
		//Only load gamelist
		sprintf(games_dir, "%s%s", device_path, "ZGX/games");
		games_LoadList();
	{ char _p2r[32]; sprintf(_p2r, "%sZGX/240p.txt", g_device_path); FILE *_p2f = fopen(_p2r, "rb"); if (_p2f) { p240_pref = 1; fclose(_p2f); } } //sincronizar indicador 240p (usa g_device_path)
	options_Load(); //cargar config del menu de opciones (sonido, fps, cart, etc.)
	}

	//Copy the routes
	sprintf(biospath, "%s%s", device_path, "ZGX/bios/bios.bin");
	sprintf(saves_dir, "%s%s", device_path, "ZGX/saves");

	bioswith = 1;			//bioswith
	selectedcart = 7;		//cartridge
	videodriverselect = 2;	//videodriver
	sounddriverselect = 0;	//sounddriver
	m68kdriverselect = 1;	//m68kdriver
	scspdriverselect = 2;	//scsodriver
	selected = 0;			//fileselected
	if(selected >= FILES_PER_PAGE) start = selected - FILES_PER_PAGE + 1;
	frameskipoff = 1;	//frameskipoff
	specialcoloron = 1;	//specialcoloron
	smpcperipheraltiming = 1000; //smpcperipheraltiming
	smpcothertiming = 1050; //smpcothertiming
	eachbackupramon = 1; //eachbackupramon
	declinenum = 10; //declinenum //STANDARD
	dividenumclock = 1; //dividenumclock
	threadingscsp2on = 0; //threadingscsp2on	//No threading

	SVI_Init();
	mem_allocate();
	snd_Init();
	menu_Init();
	per_Init();

	#ifdef TEST_GUI
	while(1) {
		per_updatePads();
		u32 buttons = PER_BUTTONS_DOWN(0);
		u32 btn_a = PAD_DI_C;
		if (buttons & btn_a) {
			return 0;
		}
		gui_Test();
		SVI_CopyFrame();
		SVI_SwapBuffers(1);
	}
	return 0;
	#else
	gui_Init();
	#endif

	osd_ProfAddCounter(PROF_SH2M, "SH2M");
	osd_ProfAddCounter(PROF_SH2S, "SH2S");
	osd_ProfAddCounter(PROF_M68K, "M68K");
	osd_ProfAddCounter(PROF_SCSP, "SCSP");
	osd_ProfAddCounter(PROF_SCU, "SCU");
	osd_ProfAddCounter(PROF_SMPC, "SMPC");
	osd_ProfAddCounter(PROF_VDP1, "VDP1");
	osd_ProfAddCounter(PROF_VDP2, "VDP2");
	osd_ProfAddCounter(PROF_CDB, "CDB");
	// Solo rearmar isofilename desde la lista si NO venimos de autoboot/WiiFlow (argc<=1).
	// Si venimos de WiiFlow (argc>1), isofilename ya tiene el path correcto del juego.
	if (argc <= 1) {
		sprintf(isofilename, "%s/%s", games_dir, filename_items.item[filename_items.cursor].data);
	}

	while(1) {
		if (gui_value == GUI_RET_SELECT) {
			CoreExec();
		}
		gui_value = menu_Handle();
	}

	return 0;
}


int CoreExec()
{
	yabauseinit_struct yinit;
	int ret;
	FILE *fp;
	//XXX: wait for Vsync is off...
	wait_for_sync = 0;
	// aplicar opcion de FPS del menu
	if (opt_fps) yabsys.flags |= SYS_FLAGS_SHOW_FPS;
	else yabsys.flags &= ~SYS_FLAGS_SHOW_FPS;

	WPAD_SetDataFormat(WPAD_CHAN_ALL,WPAD_FMT_BTNS_ACC_IR);
	WPAD_SetVRes(WPAD_CHAN_ALL, 640, 480);

	memset(&yinit, 0, sizeof(yabauseinit_struct));
	//yinit.percoretype = PERCORE_WIICLASSIC;
	yinit.sh2coretype = 0;
	yinit.vidcoretype = videodriverselect;
	yinit.scspcoretype = scspdriverselect;
	yinit.sndcoretype = sounddriverselect;
	yinit.cdcoretype = CDCORE_ISO;
	yinit.m68kcoretype = m68kdriverselect;
	yinit.carttype = cart_enabled ? 2 : 0;
	yinit.regionid = REGION_AUTODETECT;
	if (!bioswith || ((fp = fopen(biospath, "rb")) == NULL)) {
		yinit.biospath = NULL;
		bioswith = 0;
	} else {
		yinit.biospath = biospath;
		fclose(fp);
	}
#if 1
	yinit.cdpath = isofilename;
#else
	yinit.cdpath = NULL;
#endif
	strcpy(buppath, saves_dir);
	yinit.buppath = buppath;
	yinit.mpegpath = NULL;
	yinit.cartpath = NULL;
	yinit.netlinksetting = NULL;
	yinit.videoformattype = IsPal;
	yinit.clocksync = 0;
	yinit.basetime = 0;
	yinit.usethreads = threadingscsp2on;

	// Hijack the fps display
	//VIDSoft.OnScreenDebugMessage = OnScreenDebugMessage;
	exec_emu = 0;

	VIDEO_SetBlack(1);
	SVI_SetResolution(0x00D2);
		SVI_ClearXFB(); //limpiar XFB al cargar cada juego (evita basura del anterior al encadenar)
	VIDEO_WaitVSync();

	if ((ret = YabauseInit(&yinit)) == 0) {
		ScspSetFrameAccurate(1);
		YabauseSetDecilineMode(1);
		VIDEO_SetBlack(0);

		while(!exec_emu) {
			u32 result = 0;
			if (per_updatePads()) {
				exec_emu = 1;
				result = 0;
			} else {
				char msg[128] = {0};
				sprintf(msg, "MEM1 used: %d, available: %d", 0x01800000 - SYS_GetArena1Size(), SYS_GetArena1Size());
				osd_MsgAdd(20, 28, 0xFFFFFFFF, msg);
				result = YabauseExec();
			}
			//XXX: recover memory used...
			if (result != 0) {
				return -1;
			}
			if (reset_emu) {
				YabauseReset();
				reset_emu = 0;
				SYS_SetResetCallback(reset);
			}
		}
		VIDEO_SetBlack(1);
		if(strlen(cdip->itemnum)!=0) {
			strcpy(prev_itemnum, cdip->itemnum);
		}
		YabauseDeInit();
#ifdef AUTOLOADPLUGIN
		if(autoload)
			autoload=0;
#endif
		exec_emu=0;
		reset_emu=0;
	}
	SVI_SetResolution(0x80D2);
	VIDEO_SetBlack(0);
	VIDEO_WaitVSync();
	wait_for_sync = 1;
	return 0;
}

void YuiErrorMsg(const char *string)
{
   if (strncmp(string, "Master SH2 invalid opcode", 25) == 0)
   {
      if (!running)
         return;
      running = 0;
      printf("%s\n", string);
   }
}
