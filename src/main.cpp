

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

bool use16bit = false;
bool song_init = false;

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




int main(int argc, char *argv[])
{

	bool           b_ret    = false;

	pxtnERR        pxtn_err = pxtnERR_VOID;
	char          path_src[ 255 ] = {0};

	vector<string> extensionList = {"ptcop"};

	string name;

	int timer = 0;



	consoleDemoInit();
	nitroFSInit(NULL);

	soundEnable();
	printf("sup\n");
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


	while (true)
	{
		if(!song_init)
		{
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
		if(keys & KEY_START) pxtn->moo_set_fade(-1, 1);
		if(keys & KEY_SELECT) pxtn->moo_set_fade(1, 1);
		if(keys & KEY_B) {pxtn->moo_set_fade(-1, 1);  timer++;  }
		if(timer && timer++ > 60) {song_init = false; timer = 0; killAllSounds();}

		if(keys2 & KEY_A) req_size = 22*8;
		else req_size = 22*4;

		if(keys & KEY_LEFT) 
		{
			pxtn->_moo_smp_count -= 75000; 
			if(pxtn->_moo_smp_count < 0) pxtn->_moo_smp_count = 0;
			pxtn->_moo_p_eve     = pxtn->evels->get_Records();
		}
		if(keys & KEY_RIGHT) pxtn->_moo_smp_count += 75000;
		


		printf("Hold A to fast forward\nPress B to exit\n");

		printf("\nPress Left and Right to seek\n");
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
