#pragma packed(16)
#pragma GCC optimize ("O3")
#pragma GCC optimize ("fast-math")
#pragma GCC optimize ("unsafe-math-optimizations")
#pragma GCC optimize ("no-math-errno")


#define FORMAT_LITTLEFS_IF_FAILED

#include <Arduino.h>
#include "esp_log.h"
#include "config.h"

#include <FS.h>
#include "SD_MMC.h"
#include <LittleFS.h>

// SET_LOOP_TASK_STACK_SIZE(20000);

constexpr int NUM_VOICES = MAX_VOICES;

#include "i2s_in_out.h" 

#include "FmDrumSynth.h"
#include "DrumkitStorage.h"

constexpr char* TAG = "Main";

float DRAM_ATTR outL[DMA_BUFFER_LEN];
float DRAM_ATTR outR[DMA_BUFFER_LEN];


float DRAM_ATTR sendL[DMA_BUFFER_LEN];
float DRAM_ATTR sendR[DMA_BUFFER_LEN];

FmDrumSynth synth;
I2S_Audio audio; 

#include <MIDI.h>
#if MIDI_IN_DEV == USE_USB_MIDI_DEVICE
    #include "src/usbmidi/src/USB-MIDI.h"
#endif

TaskHandle_t audioTaskHandle;
TaskHandle_t midiTaskHandle;
TaskHandle_t guiTaskHandle;

#if MIDI_IN_DEV == USE_MIDI_STANDARD
    MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, MIDI);
#elif MIDI_IN_DEV == USE_USB_MIDI_DEVICE
    USBMIDI_CREATE_INSTANCE(0, MIDI); 
#endif

// ========================== GUI ==============================================================================================
#ifdef ENABLE_GUI
    #include "TextGUI.h"
    TextGUI gui;
#endif

// -- MIDI mapping --

void handleNoteOn(uint8_t ch, uint8_t note, uint8_t velocity) {
#ifdef ENABLE_GUI
    gui.pause(100);
#endif
    synth.handleNoteOn(note, velocity);
}

void handleNoteOff(uint8_t ch, uint8_t note, uint8_t vel) {
#ifdef ENABLE_GUI
    gui.pause(100);
#endif
    synth.handleNoteOff(note);
}

// ========================== Audio Task =======================================================================================
static void IRAM_ATTR audioTask(void*) {
    while (true) {
        synth.renderAudioBlock(outL, outR);
        audio.writeBuffers(outL, outR);
    }
}

// -- MIDI Task --
static void IRAM_ATTR midiTask(void*) {
    while (true) {
        MIDI.read();
        vTaskDelay(1);
        taskYIELD();

#ifdef ENABLE_GUI
        gui.process();
#endif

    }
}

#ifdef ENABLE_GUI
// ========================== Core 1 GUI Task ===============================================================================================
static void IRAM_ATTR gui_task(void *userData) { 
    vTaskDelay(50);
    ESP_LOGI(TAG, "Starting Task3");
    
    while (true) {
        gui.draw();
        vTaskDelay(1);
    }

}
#endif



// ============================ Setup ========================================================================================================
void setup() {
    btStop();

#if defined(USE_SIN_LUT)
    init_sin_tbl();
#endif

    SD_MMC.setPins(SDMMC_CLK, SDMMC_CMD, SDMMC_D0, SDMMC_D1, SDMMC_D2, SDMMC_D3);
    if (!SD_MMC.begin()) {
        ESP_LOGE(TAG, "SD init failed");
    } else {
        ESP_LOGE(TAG, "SD initialized");
    }

    if (!LittleFS.begin()) {
        ESP_LOGE(TAG, "LittleFS init failed");
    } else {
        ESP_LOGE(TAG, "LittleFS initialized");
    }

    if (!FS_USED.exists(DRUMKIT_DIR)) {
        FS_USED.mkdir(DRUMKIT_DIR);
    }

    if (!FS_USED.exists(PRESET_DIR)) {
        FS_USED.mkdir(PRESET_DIR);
    }

#if MIDI_IN_DEV == USE_USB_MIDI_DEVICE
  // Change USB Device Descriptor Parameter
    USB.VID(0x1209);
    USB.PID(0x1305);
    USB.productName("S3 FM Drums");
    USB.manufacturerName("copych");
    USB.usbVersion(0x0200);
    USB.usbClass(TUSB_CLASS_AUDIO);
    USB.usbSubClass(0x00);
    USB.usbProtocol(0x00);
    USB.usbAttributes(0x80);
#endif
    MIDI.begin(MIDI_CHANNEL_OMNI);
    MIDI.setHandleNoteOn(handleNoteOn);
    MIDI.setHandleNoteOff(handleNoteOff);
    delay(800);
    
    synth.init();

#ifdef ENABLE_GUI
    gui.begin();
    gui.message( "Synth Loading...");
    bool ok = DrumkitStorage::loadDrumkit(FS_USED, "/drumkits/Drumkit_default.json", synth.getPatchMap());
    gui.message(ok ? "Loaded OK" : "Load Failed");
    delay(100);
    ESP_LOGI(TAG, "GUI splash");
#endif


#ifdef ENABLE_GUI
    gui.startMenu();
    ESP_LOGI(TAG, "GUI started");
#endif


    audio.setSampleRate(SAMPLE_RATE);
    audio.setMode(I2S_Audio::MODE_OUT);
    audio.init();

    // Core 0: audio
    xTaskCreatePinnedToCore(audioTask, "audio", 8000, nullptr, 8, &audioTaskHandle, 0);
    // Core 1: MIDI + UI
    xTaskCreatePinnedToCore(midiTask,  "midi",  8000, nullptr, 5,  &midiTaskHandle, 1);

#ifdef ENABLE_GUI
    xTaskCreatePinnedToCore( gui_task, "GUITask", 8000, NULL, 4, &guiTaskHandle, 1 );
#endif

}

void loop() {
    vTaskDelete(nullptr);
}
