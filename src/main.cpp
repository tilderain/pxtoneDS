

#include <stdio.h>
#include <cstdint>

#include <vector>

#include "nds.h"
#include "filesystem.h"


using namespace std;

#include "./pxtone/pxtnService.h"
#include "./pxtone/pxtnError.h"

#include <malloc.h>

#include "./pxtone/soundFifo.h"

#include "file_browse.h"

#include "gl2d.h"
#include "fat.h"
#include "pxtoneicon.h"

bool use16bit = false;
bool song_init = false;
bool enableVisualization = true;

u8 unit_vols[64] = {10};
u32 unit_freq[64] = {10};

#define _CHANNEL_NUM           1
#define _SAMPLE_PER_SECOND 11025
#define _BUFFER_PER_SEC    (0.3f)

pxtnService*   pxtn     = NULL ;

int req_size = 22*4;

void Timer_1ms()
{
		if(song_init)
			pxtn->Moo( NULL, req_size);
}

static bool _load_ptcop( pxtnService* pxtn, const char* path_src, pxtnERR* p_pxtn_err )
{
	bool           b_ret     = false;
	pxtnDescriptor desc;
	pxtnERR        pxtn_err  = pxtnERR_VOID;
	FILE*          fp        = NULL;
	int32_t        event_num =    0;

	if( !( fp = fopen( path_src, "rb" ) ) ) goto term;
	if( !desc.set_file_r( fp ) ) goto term;

	pxtn_err = pxtn->read       ( &desc ); if( pxtn_err != pxtnOK ) goto term;
	pxtn_err = pxtn->tones_ready(       ); if( pxtn_err != pxtnOK ) goto term;

	b_ret = true;
term:

	if( fp     ) fclose( fp );
	if( !b_ret ) pxtn->evels->Release();

	if( p_pxtn_err ) *p_pxtn_err = pxtn_err;

	return b_ret;
}

static bool _sampling_func( void *user, void *buf, int *p_res_size, int *p_req_size )
{
	pxtnService* pxtn = static_cast<pxtnService*>( user );

	if( !pxtn->Moo( buf, *p_req_size ) ) return false;
	if( p_res_size ) *p_res_size = *p_req_size;

	return true;
}


int gAtlas16Color1;
int gAtlas16Color2;

int gAtlas256Color;

int gTextureLoaded = 0;

int gTextureWidth1024 = TEXTURE_SIZE_1024;
int gTextureHeight512 = TEXTURE_SIZE_512;

int gTextureWidth512 = TEXTURE_SIZE_512;
int gTextureHeight256 = TEXTURE_SIZE_256;

void* gCurrentPalette;


int main(int argc, char *argv[])
{

	bool           b_ret    = false;

	pxtnERR        pxtn_err = pxtnERR_VOID;
	char          path_src[ 255 ] = {0};

	vector<string> extensionList = {"ptcop"};

	string name;

	int timer = 0;

	u16* sprite_gfx_mem = oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_256Color);



	consoleDemoInit();
#ifndef NITROFS
	fatInitDefault();
#else
	nitroFSInit(NULL);
#endif

	soundEnable();
	printf("Pxtone\n");
	// INIT PXTONE.
	pxtn = new pxtnService();
	pxtn_err = pxtn->init(); if( pxtn_err != pxtnOK ) goto term;
	if( !pxtn->set_destination_quality( _CHANNEL_NUM, _SAMPLE_PER_SECOND ) ) goto term;
	
	printf("pxtone INIT\n");

	TIMER2_CR = 0;
	TIMER2_DATA = TIMER_FREQ_256(250); //1000ms
	TIMER2_CR = TIMER_ENABLE | ClockDivider_256 | TIMER_IRQ_REQ; 
	irqEnable(IRQ_TIMER2);
	irqSet(IRQ_TIMER2, Timer_1ms);






	///GFX

	videoSetMode( MODE_0_3D );

    vramSetBankA( VRAM_A_MAIN_SPRITE );     

	oamInit(&oamMain, SpriteMapping_1D_128, false);

	dmaCopy(pxtoneiconPal, SPRITE_PALETTE, 512);
	
	glScreen2D();
	glEnable(GL_TEXTURE_2D);

	glBegin2D();

	consoleDemoInit();

	dmaCopy(pxtoneiconTiles, sprite_gfx_mem, 32*32);


	while (true)
	{
		glBegin2D();
		oamSet(&oamMain, 0, 112, 32, 0, 0, SpriteSize_32x32, SpriteColorFormat_256Color, 
			sprite_gfx_mem, -1, false, false, false, false, false);
		oamUpdate(&oamMain);
		if(!song_init)
		{
			memset(unit_vols, 1, 64);
			name = browseForFile(extensionList);

			// SELECT MUSIC FILE.
			strcpy(path_src, name.c_str());
			// LOAD MUSIC FILE.
			if( !_load_ptcop( pxtn, path_src, &pxtn_err ) ) goto term;

			// PREPARATION PLAYING MUSIC.
			{
				int32_t smp_total = pxtn->moo_get_total_sample();

				pxtnVOMITPREPARATION prep = {0};
				prep.flags          |= pxtnVOMITPREPFLAG_loop;
				prep.start_pos_float =     0;
				prep.master_volume   = 0.80f;

				if( !pxtn->moo_preparation( &prep ) ) goto term;
			}
			song_init = true;

		}

		//printf("lol\n");
		//printf("Memory: %d %d %d\n", mallinfo().arena, mallinfo().uordblks, mallinfo().fordblks);
		consoleClear();
		scanKeys();
		int keys = keysDown();
		int keys2 = keysHeld();
		if(keys & KEY_START) 
			enableVisualization ^= 1;
		if(keys & KEY_B) {pxtn->moo_set_fade(-1, 1);  timer++;  }
		if(timer && timer++ > 60) {song_init = false; timer = 0; killAllSounds();}

		if(keys2 & KEY_A) req_size = 22*8;
		else req_size = 22*4;

		if(keys & KEY_LEFT) 
		{
			pxtn->_moo_smp_count -= 50000; 
			if(pxtn->_moo_smp_count < 0) pxtn->_moo_smp_count = 0;
			pxtn->_moo_p_eve     = pxtn->evels->get_Records();
		}
		if(keys & KEY_RIGHT) pxtn->_moo_smp_count += 50000;
		


		printf("Hold A to fast forward\nPress B to exit\n");

		printf("\nPress Left and Right to seek\n");

		printf("\nPress Start to toggle visual\n");
		
		int progress = (pxtn->_moo_smp_end - pxtn->_moo_smp_count) / 4000 ;
		progress = ((pxtn->_moo_smp_end / 4000) - progress);
		printf("%d", progress);
		glBoxFilled(16, 180, 16 + progress, 180, RGB15(50, 255, 50));

		if(enableVisualization)
		{
			for( int32_t u = 0; u < pxtn->_unit_num; u++ )
			{
				//printf("%d\n", unit_vols[u]);
				glBoxFilledTransparent(16 + (u*16), 165-(unit_freq[u]/5000), 16 + (u*16) + 15, 170-(unit_freq[u]/5000), RGB15(160, 255, 50), (log((int)unit_vols[u]*2.4) * 100)/20);
			}
		}


		glEnd2D();
		glFlush(0);

		swiWaitForVBlank();
	}
	

	// INIT XAudio2.

	// START XAudio2.
	 _sampling_func;

	b_ret = true;
term:
	printf("by\n");
	swiWaitForVBlank();
	SAFE_DELETE( pxtn );
	return 1;
}
