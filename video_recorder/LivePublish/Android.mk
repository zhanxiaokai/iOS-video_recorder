LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm
LOCAL_CFLAGS := -DHAVE_CONFIG_H -DFPM_ARM -ffast-math -O3
LOCAL_STATIC_LIBRARIES := libcommontool

LOCAL_C_INCLUDES += \
    $(LOCAL_PATH)/../3rdparty/ffmpeg/include 
    
LOCAL_SRC_FILES = \
./common/live_thread.cpp \
./common/publisher_statistics.cpp \
./common/publisher_rate_feed_back.cpp \
./common/live_packet_pool.cpp \
./common/live_audio_packet_pool.cpp \
./common/live_video_packet_queue.cpp \
./common/live_audio_packet_queue.cpp \
./audio_encoder/audio_encoder_adapter.cpp \
./audio_encoder/audio_encoder.cpp \
./publisher/recording_h264_publisher.cpp \
./publisher/recording_publisher.cpp \
./consumer/video_consumer_thread.cpp \
./common/live_video_quality_agent.cpp 

LOCAL_MODULE := liblivecore
include $(BUILD_STATIC_LIBRARY)
