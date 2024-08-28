#include <stdio.h>
#include <string.h>

#include "SDL.h"
#include <vorbis/codec.h>

#include "fileop.h"

#include "audio.h"

// volumes are between 0 and 255

struct audio_vars_s {
    SDL_AudioSpec *obtained;
    
    int initialized;
    
    int music_state; // 0=stopped 1=changing 2=playing
    char *music_name;
    int music_volume;
    int music_fade_volume;
    int music_loop;
    fileop_handle *music_fp;
    
    int sfx_volume;
    
    int global_volume;
    
    int counter;
    
} audio_vars;


struct audio_ogg_s {
    ogg_sync_state oy;
    ogg_page og;
    ogg_stream_state os;
    
    fileop_handle *fp;
    
    int initialized;
} audio_ogg;


int audio_ogg_read(void)
{
    char *buffer;
    int bytes;
    buffer = ogg_sync_buffer(&audio_ogg.oy, 4096);
    bytes = fileop_read(buffer,4096,audio_ogg.fp);
    ogg_sync_wrote(&audio_ogg.oy, bytes);
    return bytes;
}

int audio_ogg_reset(void)
{
    int bytes;
    int result;
    
    if(audio_ogg.initialized==1)
    {
        // shutdown old shit
        ogg_sync_clear(&audio_ogg.oy);
        if(audio_ogg.fp != NULL)
            fileop_close(audio_ogg.fp);
        
    }
    
    // initializes sync buffer oy for use
    ogg_sync_init(&audio_ogg.oy);
    
    audio_ogg.fp = fileop_open(audio_vars.music_name, "rb");
    if(audio_ogg.fp == NULL)
    {
        fprintf(stderr, "audio_ogg_reset(): Warning, can't really open .ogg file maan\n");
        return 1;
    }
    
    bytes=audio_ogg_read();
    
    result=ogg_sync_pageout(&audio_ogg.oy, &audio_ogg.og);
    
    if(result<=0)
    {
        ogg_sync_clear(&audio_ogg.oy);
        fprintf(stderr, "audio_ogg_reset(): Pik..\n");
        return 1;
    }
    
    ogg_stream_init(&audio_ogg.os, ogg_page_serialno(&audio_ogg.og));
    
    
    
    
    
    
    
    audio_ogg.initialized=1;
    return 0;
}

// DJ audio_tick
void audio_tick(void)
{
    // fade music out?
    if(audio_vars.music_state==1 && audio_vars.music_fade_volume>0)
        audio_vars.music_fade_volume--;

    // fade music in?
    if(audio_vars.music_state==2 && audio_vars.music_fade_volume<255)
        audio_vars.music_fade_volume++;
    
    // change music?
    if(audio_vars.music_state==1 && audio_vars.music_fade_volume==0)
    {
        audio_vars.music_state=2;
    }
}

void audio_callback(void *userdata, Uint8 *stream, int len)
{
    int i;
    
    // wee.. hear the silence
    for(i=0;i<len/4;i++)
    {
        stream[i*4]=0;          // lsb 0
        stream[i*4+1]=128;      // msb 0
        stream[i*4+2]=0;        // lsb 1
        stream[i*4+3]=128;      // msb 1
    }
}

int audio_init(void)
{
    SDL_AudioSpec *desired;
    int i;
    
    desired = new SDL_AudioSpec;
    
    desired->freq=44100;
    desired->format=AUDIO_U16SYS;
    desired->channels=2;
    desired->samples=2048;
    desired->callback=audio_callback;
    desired->userdata=NULL;
    
    i=SDL_OpenAudio(desired, audio_vars.obtained);
    if(i!=0)
    {
        audio_vars.initialized=0;
        return 1;
    }
    
    if(audio_vars.obtained == NULL)
    {
        audio_vars.obtained = desired;
    }
    else
    {
        fprintf(stderr, "audio_init(): Warning - couldn't initialize audio with desired configuration\n");
        audio_vars.initialized=0;
        return 1;        
    }
    
    SDL_PauseAudio(0);
    
    audio_vars.initialized=1;
    
    audio_vars.music_state=0;
    audio_vars.music_name=NULL;
    audio_vars.music_volume=0;
    audio_vars.music_fade_volume=0;
    audio_vars.music_fp=NULL;
    
    audio_vars.sfx_volume=0;
    
    audio_vars.global_volume=0;   

    audio_vars.counter=0;
    
    audio_ogg.initialized=0;
        
    return 0;    
}

void audio_shutdown(void)
{
    if(audio_vars.obtained != NULL)
        delete audio_vars.obtained;
    
    SDL_CloseAudio();
    
    audio_vars.initialized=0;
}

void audio_change_music(const char *filename)
{
    audio_vars.music_state=1;
    
    if(audio_vars.music_name!=NULL)
        delete audio_vars.music_name;
    
    audio_vars.music_name = new char[strlen(filename)+1];
    
    strcpy(audio_vars.music_name, filename);    
}

void audio_set_volume(int global, int sfx, int music)
{
    if(global != -1)
        audio_vars.global_volume=global;
    
    if(sfx != -1)
        audio_vars.sfx_volume=sfx;
    
    if(music != -1)
        audio_vars.music_volume=music;
}





