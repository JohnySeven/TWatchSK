#include "sound_player.h"
#include "driver/i2s.h"

#define CCCC(c1, c2, c3, c4)    ((c4 << 24) | (c3 << 16) | (c2 << 8) | c1)

/* these are data structures to process wav file */
typedef enum headerState_e {
    HEADER_RIFF, HEADER_FMT, HEADER_DATA, DATA
} headerState_t;

typedef struct wavRiff_s {
    uint32_t chunkID;
    uint32_t chunkSize;
    uint32_t format;
} wavRiff_t;

typedef struct wavProperties_s {
    uint32_t chunkID;
    uint32_t chunkSize;
    uint16_t audioFormat;
    uint16_t numChannels;
    uint32_t sampleRate;
    uint32_t byteRate;
    uint16_t blockAlign;
    uint16_t bitsPerSample;
} wavProperties_t;
/* variables hold file, state of process wav file and wav file properties */    
headerState_t state = HEADER_RIFF;
wavProperties_t wavProps;

//i2s configuration
int i2s_num = 0; // i2s port number
i2s_config_t i2s_config = {
     .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_DAC_BUILT_IN),
     .sample_rate = 44100,
     .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT, /* the DAC module will only take the 8bits from MSB */
     .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
     .communication_format = (i2s_comm_format_t)I2S_COMM_FORMAT_I2S_MSB,
     .intr_alloc_flags = 0, // default interrupt priority
     .dma_buf_count = 8,
     .dma_buf_len = 64,
     .use_apll = 0
    };
    
i2s_pin_config_t pin_config = {
    .bck_io_num = 26, //this is BCK pin
    .ws_io_num = 25, // this is LRCK pin
    .data_out_num = 22, // this is DATA output pin
    .data_in_num = -1   //Not used
};
/*
int i2s_write_sample_nb(uint8_t sample){
  return i2s_write((i2s_port_t)i2s_num, (const char *)&sample, sizeof(uint8_t), 100);
}
///read 4 bytes of data from wav file
int read4bytes(File file, uint32_t *chunkId){
  int n = file.read((uint8_t *)chunkId, sizeof(uint32_t));
  return n;
}

int readbyte(File file, uint8_t *chunkId){
  int n = file.read((uint8_t *)chunkId, sizeof(uint8_t));
  return n;
}

//these are function to process wav file
int readRiff(File file, wavRiff_t *wavRiff){
  int n = file.read((uint8_t *)wavRiff, sizeof(wavRiff_t));
  return n;
}
int readProps(File file, wavProperties_t *wavProps){
  int n = file.read((uint8_t *)wavProps, sizeof(wavProperties_t));
  return n;
}*/

SoundPlayer::SoundPlayer()
{

}

SoundPlayer::~SoundPlayer()
{

}