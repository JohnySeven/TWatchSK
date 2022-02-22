#include "sound_player.h"
#include "../config.h"
#include "FreeRTOS.h"
#include "driver/i2s.h"

const char *PLAYER_TAG = "I2S";

struct SoundTask_t
{
  unsigned char *sound;
  int len;
  char name[32];
  int repeat;
};

#ifdef LILYGO_WATCH_2020_V1 || LILYGO_WATCH_2020_V3

SoundPlayer::SoundPlayer()
{
  player_queue_handle_ = xQueueCreate(32, sizeof(SoundTask_t));
  xTaskCreate(player_task_func, "sound_player", CONFIG_MAIN_TASK_STACK_SIZE, player_queue_handle_, 5, &player_task_);
}

void init_i2s()
{
  auto ttgo = TTGOClass::getWatch();
  ttgo->power->setLDO3Mode(AXP202_LDO3_MODE_DCIN);
  ttgo->power->setLDO3Voltage(3000);
  ttgo->power->setPowerOutPut(AXP202_LDO3, AXP202_ON);

  i2s_config_t i2s_config = {
      .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
      .sample_rate = 44100,
      .bits_per_sample = (i2s_bits_per_sample_t)16,
      .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
      .communication_format = (i2s_comm_format_t)(I2S_COMM_FORMAT_I2S | I2S_COMM_FORMAT_I2S_MSB),
      .dma_buf_count = 32,
      .dma_buf_len = 64,
      .use_apll = 0};

  i2s_config.intr_alloc_flags = ESP_INTR_FLAG_LEVEL1;

  i2s_pin_config_t pin_config =
      {
          .bck_io_num = TWATCH_DAC_IIS_BCK,
          .ws_io_num = TWATCH_DAC_IIS_WS,
          .data_out_num = TWATCH_DAC_IIS_DOUT,
          .data_in_num = I2S_PIN_NO_CHANGE};

  if(i2s_driver_install(I2S_NUM_0, &i2s_config, 0, NULL) == ESP_OK)
  {
     if(i2s_set_pin(I2S_NUM_0, &pin_config) == ESP_OK)
     {
       ESP_LOGI(PLAYER_TAG, "I2S initialized OK!");
     }
  }
}

void deinit_i2s()
{
  auto ttgo = TTGOClass::getWatch();
  ttgo->power->setLDO3Mode(AXP202_LDO3_MODE_DCIN);
  ttgo->power->setPowerOutPut(AXP202_LDO3, AXP202_OFF);
  i2s_stop(I2S_NUM_0);
  i2s_driver_uninstall(I2S_NUM_0);
}

void SoundPlayer::play_raw_from_const(const char *name, const unsigned char *raw, int size, int repeat)
{
  SoundTask_t task;
  strcpy(task.name, name);
  task.sound = (unsigned char *)raw;
  task.len = size;
  task.repeat = repeat;

  xQueueSend(this->player_queue_handle_, &task, portMAX_DELAY);
}

void SoundPlayer::player_task_func(void *pvParameter)
{
  SoundTask_t currentTask;
  QueueHandle_t queue = (QueueHandle_t)pvParameter;

  ESP_LOGI(PLAYER_TAG, "Async task dispatcher started!");

  while (true)
  {
    if (xQueueReceive(queue, &currentTask, portMAX_DELAY))
    {
      ESP_LOGI(PLAYER_TAG, "Playing sound %s repeat=%d", currentTask.name, currentTask.repeat);
      init_i2s();
      ssize_t nread;
      size_t nwritten;
      int res;
      i2s_zero_dma_buffer(I2S_NUM_0);
      i2s_start(I2S_NUM_0);

      for (int r = 0; r < currentTask.repeat; r++)
      {
        int bytes = currentTask.len;
        unsigned char *ptr = currentTask.sound;

        while (bytes > 0)
        {
          int batchSize = (bytes - 128) ? 128 : bytes;
          auto ret = i2s_write(I2S_NUM_0, ptr, batchSize, &nwritten, portMAX_DELAY);
          bytes -= nwritten;
          ptr += nwritten;

          if (ret != ESP_OK)
          {
            ESP_LOGI(PLAYER_TAG, "Unable to send I2S data, failed with result=%d", ret);
          }
        }
      }
      deinit_i2s();
      ESP_LOGI(PLAYER_TAG, "Sound playing %s finished", currentTask.name);
    }
  }
}
#else

SoundPlayer::SoundPlayer()
{

}

void SoundPlayer::play_raw_from_const(const char *name, const unsigned char *raw, int size, int repeat)
{
  log_w("TWatch 2020 V2 doesn't support playing sounds.");
}

#endif

SoundPlayer::~SoundPlayer()
{
}