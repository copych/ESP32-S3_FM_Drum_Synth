/*
* I2S_Audio.cpp
* I2S_Audio class by copych
*/
#include "i2s_in_out.h"
#include "esp_log.h"
#include "esp_task_wdt.h" 
static const char* TAG = "I2SAUDIO";

/*
  _malloc_caps hints
      MALLOC_CAP_EXEC     Memory must be able to run executable code
      MALLOC_CAP_32BIT    Memory must allow for aligned 32-bit data accesses
      MALLOC_CAP_8BIT     Memory must allow for 8/16/...-bit data accesses
      MALLOC_CAP_DMA      Memory must be able to be accessed by DMA
      MALLOC_CAP_SPIRAM   Memory must be in SPI RAM
      MALLOC_CAP_INTERNAL Memory must be internal; specifically it should not disappear when flash/spiram cache is switched off
      MALLOC_CAP_DEFAULT  Memory can be returned in a non-capability-specific memory allocation (e.g. malloc(), calloc()) call
      MALLOC_CAP_INVALID  Memory can't be used / list end marker
*/

BUF_TYPE* I2S_Audio::allocateBuffer(const char* name) {
    BUF_TYPE* buf = (BUF_TYPE*)heap_caps_calloc(16, _buffer_size, _malloc_caps);
    if (!buf) {
        ESP_LOGE(TAG, "Couldn't allocate memory for %s buffer", name);
    } else {
        ESP_LOGI(TAG, "%s buffer allocated %d bytes, &=%#010x", name, _buffer_size, buf);
    }
    return buf;
}

void I2S_Audio::init(eI2sMode select_mode) {
	_i2s_mode = select_mode;
	_read_remain_smp = 0;
	_write_remain_smp = 0;
	
#ifndef USE_V3
	i2s_mode_t port_mode;
#else
  #if (CHANNEL_SAMPLE_BYTES == 4)
	_malloc_caps = ( MALLOC_CAP_INTERNAL | MALLOC_CAP_32BIT );
  #else
	_malloc_caps = ( MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT );
  #endif
#endif  

  switch(_i2s_mode) {
    case MODE_IN:
    #ifndef USE_V3
      port_mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
    #endif
      pinMode(I2S_DIN_PIN, INPUT);
      _input_buf = allocateBuffer("_input_buf");
      break;
    case MODE_IN_OUT:
    #ifndef USE_V3
      port_mode = (i2s_mode_t)( I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX );
    #endif
      pinMode(I2S_DOUT_PIN, OUTPUT);
      pinMode(I2S_DIN_PIN, INPUT);
      _input_buf = allocateBuffer("_input_buf");
      _output_buf = allocateBuffer("_output_buf");
      break;
    case MODE_OUT:
    default:
    #ifndef USE_V3
      port_mode = (i2s_mode_t)( I2S_MODE_MASTER | I2S_MODE_TX );
    #endif
      pinMode(I2S_DOUT_PIN, OUTPUT);
      _output_buf = allocateBuffer("_output_buf");

  }
  pinMode(I2S_BCLK_PIN, OUTPUT);
  pinMode(I2S_WCLK_PIN, OUTPUT);

#ifdef USE_V3

  i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(_i2s_port, I2S_ROLE_MASTER);
    chan_cfg.dma_frame_num = DMA_BUFFER_LEN;
    chan_cfg.dma_desc_num = DMA_BUFFER_NUM;
  i2s_new_channel(&chan_cfg, &tx_handle, &rx_handle);
  i2s_std_config_t std_cfg = {
      .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
    //  .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
      .slot_cfg = I2S_STD_PHILIP_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_STEREO),
      .gpio_cfg = {
          .mclk = I2S_GPIO_UNUSED,
          .bclk = (gpio_num_t)I2S_BCLK_PIN,
          .ws = (gpio_num_t)I2S_WCLK_PIN,
          .dout = (gpio_num_t)I2S_DOUT_PIN,
          .din = I2S_GPIO_UNUSED,
          .invert_flags = {
              .mclk_inv = false,
              .bclk_inv = false,
              .ws_inv = false,
          },
      },
  };

  i2s_channel_init_std_mode(tx_handle, &std_cfg);
  i2s_channel_enable(tx_handle);
  
  ESP_LOGI(TAG, "I2S started: BCK %d, WCK %d, DAT %d", I2S_BCLK_PIN, I2S_WCLK_PIN, I2S_DOUT_PIN);
  
  
  
#else
  i2s_config_t i2s_config = {
    .mode = (i2s_mode_t)port_mode,
  //  .mode = (i2s_mode_t)( I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX ),
    .sample_rate = _sample_rate,
    .bits_per_sample = chn_bit_width,
    .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
    .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_STAND_I2S ),
    .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
    .dma_buf_count = DMA_BUFFER_NUM,
    .dma_buf_len = DMA_BUFFER_LEN,
    .use_apll = true,
    .tx_desc_auto_clear = true,
  //  .fixed_mclk = 0
	//	.fixed_mclk = (int)(_sample_rate * mclk_multiplier),
  };

  i2s_pin_config_t i2s_pin_config = {
    .bck_io_num     = I2S_BCLK_PIN,
    .ws_io_num      = I2S_WCLK_PIN,
    .data_out_num   = I2S_DOUT_PIN,
    .data_in_num    = I2S_DIN_PIN
  };

  int err = i2s_driver_install(_i2s_port, &i2s_config, 0, NULL);
  i2s_set_pin(_i2s_port, &i2s_pin_config);
  i2s_zero_dma_buffer(_i2s_port);
  if (err == ESP_OK) {
    ESP_LOGI(TAG, "I2S started OK at BCLK %d, WCLK %d, DIN %d, DOUT %d", I2S_BCLK_PIN, I2S_WCLK_PIN, I2S_DIN_PIN, I2S_DOUT_PIN);
    ESP_LOGI(TAG, "int_to_float %10.10f, float_to_int %10.10f", int_to_float, float_to_int );
  } else { 
    ESP_LOGE(TAG, "I2S Install Error %d", err);
  }
#endif
}

void I2S_Audio::deInit() {
	if (_input_buf) { free(_input_buf); _input_buf = nullptr; }
	if (_output_buf) { free(_output_buf); _output_buf = nullptr; }
#ifdef USE_V3  
	i2s_channel_disable(tx_handle);
	i2s_del_channel(tx_handle);
#else
	i2s_zero_dma_buffer(_i2s_port);
	i2s_driver_uninstall(_i2s_port);
#endif
}


void   I2S_Audio::readBuffer(BUF_TYPE* buf) {
	size_t bytes_read = 0;
	int32_t err = 0;
#ifdef USE_V3
	err = i2s_channel_read(rx_handle, buf, _buffer_size, &bytes_read, portMAX_DELAY);
	_read_remain_smp = DMA_BUFFER_LEN;
#else
	err = i2s_read(_i2s_port, (void*) buf, _buffer_size, &bytes_read, portMAX_DELAY);
#endif
	if (err != ESP_OK || bytes_read < _buffer_size) {
		ESP_LOGI(TAG, "I2S read, err %d bytes read: %d", err,  bytes_read);
	}
	_read_remain_smp = bytes_read / WHOLE_SAMPLE_BYTES;
}

void   I2S_Audio::writeBuffer(BUF_TYPE* buf){
	size_t bytes_written = 0;
	int32_t err = 0;
#ifdef USE_V3 
	err = i2s_channel_write(tx_handle, buf, _buffer_size, &bytes_written, portMAX_DELAY);
#else
	err = i2s_write(_i2s_port, (char *)buf, _buffer_size, &bytes_written, portMAX_DELAY);
#endif
	_write_remain_smp = bytes_written / WHOLE_SAMPLE_BYTES;
	if (err != ESP_OK) {
		ESP_LOGE(TAG, "Error writing to port");  
	}
}

void I2S_Audio::getSamples(float* sampleLeft, float* sampleRight){
  getSamples(sampleLeft, sampleRight, _input_buf );
}

void I2S_Audio::getSamples(float& sampleLeft, float& sampleRight){
  getSamples(sampleLeft, sampleRight, _input_buf );  
}

void I2S_Audio::getSamples(float* sampleLeft, float* sampleRight, BUF_TYPE* buf ){
  int n = DMA_BUFFER_LEN - _read_remain_smp;
#if AUDIO_CHANNEL_NUM == 2
  *sampleLeft = convertInSample(_input_buf[AUDIO_CHANNEL_NUM * n ]);
  *sampleRight = convertInSample(_input_buf[AUDIO_CHANNEL_NUM * n + 1]);
#else
  *sampleLeft = convertInSample(_input_buf[AUDIO_CHANNEL_NUM * n + chan]);
#endif
  _read_remain_smp--;
  if (_read_remain_smp <= 0) {
    readBuffer(buf);
  } 
}

void I2S_Audio::getSamples(float& sampleLeft, float& sampleRight, BUF_TYPE* buf ){  
  int n = DMA_BUFFER_LEN - _read_remain_smp;
#if AUDIO_CHANNEL_NUM == 2
  sampleLeft = convertInSample(_input_buf[AUDIO_CHANNEL_NUM * n ]);
  sampleRight = convertInSample(_input_buf[AUDIO_CHANNEL_NUM * n + 1]);
#else
  sampleLeft = convertInSample(_input_buf[AUDIO_CHANNEL_NUM * n + _chan]);
#endif
  _read_remain_smp--;
  if (_read_remain_smp <= 0) {
    readBuffer(buf);
  } 
}

void I2S_Audio::putSamples(float* sampleLeft, float* sampleRight){
  putSamples(sampleLeft, sampleRight, _output_buf );
}

void I2S_Audio::putSamples(float& sampleLeft, float& sampleRight){
  putSamples(sampleLeft, sampleRight, _output_buf );
}

void I2S_Audio::putSamples(float* sampleLeft, float* sampleRight, BUF_TYPE* buf ){
  int n = DMA_BUFFER_LEN - _write_remain_smp;
#if AUDIO_CHANNEL_NUM == 2
  buf[AUDIO_CHANNEL_NUM * n ] = convertOutSample(*sampleLeft);
  buf[AUDIO_CHANNEL_NUM * n + 1] = convertOutSample(*sampleRight);
#else
  buf[ n ] = convertOutSample(*sampleLeft);
#endif
  _write_remain_smp--;
  if (_write_remain_smp <= 0) {
    writeBuffer(buf);
  }  
}

void I2S_Audio::putSamples(float& sampleLeft, float& sampleRight, BUF_TYPE* buf ){
  int n = DMA_BUFFER_LEN - _write_remain_smp;
#if AUDIO_CHANNEL_NUM == 2
  buf[AUDIO_CHANNEL_NUM * n ] = convertOutSample(sampleLeft);
  buf[AUDIO_CHANNEL_NUM * n + 1] = convertOutSample(sampleRight);
#else
  buf[ n ] = convertOutSample(sampleLeft);
#endif
  _write_remain_smp--;
  if (_write_remain_smp <= 0) {
    writeBuffer(buf);
  }  
}

void I2S_Audio::writeBuffers(float* L, float* R) {
    if (!_output_buf) return;

    for (int i = 0; i < DMA_BUFFER_LEN; ++i) {
        int16_t l = convertOutSample(L[i]);
        int16_t r = convertOutSample(R[i]);

#if CHANNEL_SAMPLE_BYTES == 4
        _output_buf[i] = (uint16_t)l | ((uint32_t)(uint16_t)r << 16);
#else
        _output_buf[2 * i + 0] = l;
        _output_buf[2 * i + 1] = r;
#endif
    }

#ifdef USE_V3  
    size_t bytes_written = 0;
    i2s_channel_write(tx_handle, _output_buf, _buffer_size, &bytes_written, portMAX_DELAY);
#else
    size_t bytes_written = 0;
    i2s_write(_i2s_num, _output_buf, _buffer_size, &bytes_written, portMAX_DELAY);
#endif
}

