extern "C" {
#include <libavformat/avformat.h>
#include <SDL2/SDL.h>
}

using namespace std;

#include "avformat_use.h"
#include "sdl_use.h"
#include "audio_use.h"

// 成功
int main() {
    const char *url = "http://clips.vorwaerts-gmbh.de/big_buck_bunny.mp4";
//    separate_video(url);
//    use_sdl();
//    show();
    sdl_audio::playAudio(url);
    return 0;
}