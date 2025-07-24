#pragma once

#include <Arduino.h>
#include "config.h"
/**
*
* I2S audio helper class for ESP32 family
* I2S port 0 assumed
* by copych 2024-2025
* https://gitverse.com/copych
*
*/

#define   AUDIO_CHANNEL_NUM     2     // number of channels, some bugs are present for 1
#define   CHANNEL_SAMPLE_BITS   (CHANNEL_SAMPLE_BYTES * 8)
#define   WHOLE_SAMPLE_BYTES    (CHANNEL_SAMPLE_BYTES * AUDIO_CHANNEL_NUM)
#define   WHOLE_SAMPLE_BITS     (CHANNEL_SAMPLE_BYTES * AUDIO_CHANNEL_NUM * 8)

#if ESP_ARDUINO_VERSION_MAJOR < 3
  // ESP Arduino cores prior to 3.0.0
  #include "driver/i2s.h"
#else
  #define USE_V3
  // ESP Arduino cores 3.0.0 and up
  #include "driver/i2s_std.h"
#endif

// converting between float and int here assumes that float signal is normalized within -1.0 .. 1.0 range
// use the included fclamp() to fix it if needed
//inline float                    fclamp(float smp) { if (smp>1.0f) return 1.0f; if (smp<-1.0f) return -1.0f; return smp; }

class IRAM_ATTR I2S_Audio {
  
  public:
    enum eI2sMode                     { MODE_IN, MODE_OUT, MODE_IN_OUT, MODE_COUNT };
    enum eChannel                     { CH_LEFT, CH_RIGHT, CH_COUNT };
    I2S_Audio()                       {_i2s_mode = MODE_IN_OUT; }
    I2S_Audio(eI2sMode m_)            {_i2s_mode = m_; }
    ~I2S_Audio()                      { deInit(); }
 

#ifdef USE_V3
  const i2s_data_bit_width_t  chn_bit_width = (i2s_data_bit_width_t)CHANNEL_SAMPLE_BITS;
#else
  const i2s_bits_per_sample_t chn_bit_width = (i2s_bits_per_sample_t)CHANNEL_SAMPLE_BITS;
#endif
    
#if (CHANNEL_SAMPLE_BYTES == 1) // not working
    #define BUF_TYPE int16_t
    const float                 int_to_float = 1.0f/32768.0f;
    const float                 float_to_int = 32767.0f;
    const uint32_t              mclk_multiplier = 256;
#elif (CHANNEL_SAMPLE_BYTES == 2)
    #define BUF_TYPE int16_t
    const float                 int_to_float = 1.0f/32768.0f;
    const float                 float_to_int = 32767.0f;
    const uint32_t              mclk_multiplier = 256;
#elif (CHANNEL_SAMPLE_BYTES == 3) // not working
    #define BUF_TYPE int32_t
    const float                 int_to_float = 1.0f/8388608.0f;
    const float                 float_to_int = 8388607.0f;
    const uint32_t              mclk_multiplier = 384;
#elif (CHANNEL_SAMPLE_BYTES == 4)
    #define BUF_TYPE int32_t
    const float                 int_to_float = 1.0f/2147483648.0f;
    const float                 float_to_int = 2147483520.0f;
    const uint32_t              mclk_multiplier = 256;
#endif

    void                        init(eI2sMode i2s_mode = MODE_IN_OUT);
    void                        deInit();
    inline void                 setMode(eI2sMode i2s_mode)        {_i2s_mode = (eI2sMode)constrain((int)i2s_mode, 0, (int)MODE_COUNT-1); }
    inline eI2sMode             getMode()                         { return _i2s_mode; }
    inline void                 setSampleRate(int sr)             {_sample_rate = constrain(sr, 0, 192000); }
    inline int32_t              getSampleRate()                   { return _sample_rate; }
    
    // functions that read/write the whole built-in buffers
    void                        readBuffer()                      { readBuffer(_input_buf); }
    void                        writeBuffer()                     { writeBuffer(_output_buf); }

    void                        writeBuffers(float* L, float* R);


    // functions that read/write the whole custom buffers supplied via pointer argument
    void                        readBuffer(BUF_TYPE* buf);
    void                        writeBuffer(BUF_TYPE* buf);
    
    // functions that convert samples between normalized float and native I2S PCM data
    inline float                convertInSample(BUF_TYPE smp)     { return smp * int_to_float; }
    inline int32_t              convertOutSample(float smp)       { return smp * float_to_int; }
    
    // functions that read/write a single channel sample from/to custom buffers supplied via pointer argument
    inline float                readSample(int n, int chan, BUF_TYPE* buf)             { return convertInSample(buf[_channel_num * n + chan]); }
    inline void                 writeSample(float smp, int n, int chan, BUF_TYPE* buf) { buf[_channel_num * n + chan] = convertOutSample(smp); }
    
    // functions that read/write a single channel sample from/to built-in buffers
    inline float                readSample(int n, int chan)             { return convertInSample(_input_buf[_channel_num * n + chan]); }
    inline void                 writeSample(float smp, int n, int chan) { _output_buf[_channel_num * n + chan] = convertOutSample(smp); }
    
    /** functions for unattended reading/writing:
     * they read/write samples for all channels via pointers/references and advances the current buffer position
     * also it invokes buffer read/write on empty/full buffer, so you can use just these few functions to deal with I2S.
     * Note that other functions don't modify current buffer positions, so you'd better not to mix them
    */
    void                        getSamples(float* sampleLeft, float* sampleRight);                  // default built-in buffer
    void                        putSamples(float* sampleLeft, float* sampleRight);                  // default built-in buffer
    
    void                        getSamples(float& sampleLeft, float& sampleRight);                  // default built-in buffer
    void                        putSamples(float& sampleLeft, float& sampleRight);                  // default built-in buffer
    
    void                        getSamples(float* sampleLeft, float* sampleRight, BUF_TYPE* buf );  // custom external buffer
    void                        putSamples(float* sampleLeft, float* sampleRight, BUF_TYPE* buf );  // custom external buffer
    
    void                        getSamples(float& sampleLeft, float& sampleRight, BUF_TYPE* buf );  // custom external buffer
    void                        putSamples(float& sampleLeft, float& sampleRight, BUF_TYPE* buf );  // custom external buffer
        
    
    // a few more get functions
    inline BUF_TYPE*		getInputBufPointer()			  { return _input_buf; }
    inline BUF_TYPE*		getOutputBufPointer()			  { return _output_buf; }
    inline int					getBufSizeBytes()				    { return _buffer_len * WHOLE_SAMPLE_BYTES; }
    inline int					getBufLenSmp()					    { return _buffer_len; }
    inline int					getChanNum()					      { return _channel_num; }
    inline int					getChanBytes()					    { return CHANNEL_SAMPLE_BYTES; }
    inline int					getReadSamplesRemain()			{ return _read_remain_smp; }
    inline int					getWriteSamplesRemain()			{ return _write_remain_smp; }
    
  protected:

#ifdef USE_V3
    i2s_chan_handle_t tx_handle;
    i2s_chan_handle_t rx_handle;
#endif

    BUF_TYPE*                   allocateBuffer(const char* name);
    size_t                      _buffer_size                      = AUDIO_CHANNEL_NUM * DMA_BUFFER_LEN * sizeof(BUF_TYPE);
    eI2sMode                    _i2s_mode                         ;
    const i2s_port_t            _i2s_port                         = I2S_NUM_0; // i2s port number
    BUF_TYPE*                   _input_buf                        = nullptr;
    BUF_TYPE*                   _output_buf                       = nullptr;
    uint32_t                    _sample_rate                      = SAMPLE_RATE;
    const int32_t               _buffer_len                       = DMA_BUFFER_LEN;
    const int32_t               _buffer_num                       = DMA_BUFFER_NUM;
    const int32_t               _channel_num                      = AUDIO_CHANNEL_NUM;
    int32_t                     _read_remain_smp                  = 0;
    int32_t                     _write_remain_smp                 = 0;
	  uint32_t 					          _chan							                = 0; // mono channel 0/1
    uint32_t                    _malloc_caps                      = 0;
};
