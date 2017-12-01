#ifndef PLATFORM_4_LIVE_FFMPEG
#define PLATFORM_4_LIVE_FFMPEG

#ifdef __ANDROID__

extern "C" {
#include "./../../3rdparty/ffmpeg/include/libavutil/opt.h"
#include "./../../3rdparty/ffmpeg/include/libavutil/mathematics.h"
#include "./../../3rdparty/ffmpeg/include/libavformat/avformat.h"
#include "./../../3rdparty/ffmpeg/include/libswscale/swscale.h"
#include "./../../3rdparty/ffmpeg/include/libswresample/swresample.h"
#include "./../../3rdparty/ffmpeg/include/libavutil/imgutils.h"
#include "./../../3rdparty/ffmpeg/include/libavutil/samplefmt.h"
#include "./../../3rdparty/ffmpeg/include/libavutil/timestamp.h"
#include "./../../3rdparty/ffmpeg/include/libavcodec/avcodec.h"
#include "./../../3rdparty/ffmpeg/include/libavfilter/avfiltergraph.h"
#include "./../../3rdparty/ffmpeg/include/libavfilter/avcodec.h"
#include "./../../3rdparty/ffmpeg/include/libavfilter/buffersink.h"
#include "./../../3rdparty/ffmpeg/include/libavfilter/buffersrc.h"
#include "./../../3rdparty/ffmpeg/include/libavutil/avutil.h"
#include "./../../3rdparty/ffmpeg/include/libswscale/swscale.h"
}

#elif defined(__APPLE__)	// IOS or OSX
extern "C" {
#include "libavutil/opt.h"
#include "libavutil/mathematics.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "libavutil/imgutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/timestamp.h"
#include "libavcodec/avcodec.h"
#include "libavfilter/avfiltergraph.h"
#include "libavfilter/avcodec.h"
#include "libavfilter/buffersink.h"
#include "libavfilter/buffersrc.h"
#include "libavutil/avutil.h"
#include "libswscale/swscale.h"
}
#endif

#endif	// PLATFORM_4_LIVE_FFMPEG
