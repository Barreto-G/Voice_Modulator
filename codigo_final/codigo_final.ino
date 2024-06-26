#include <AudioTools.h>
#include "AudioLibs/AudioSTK.h"

#define SAMP_FREQ 22050
#define TEMPO_DEBOUNCE 20 //ms
TaskHandle_t Task1;

const byte BOT1 = 15;
const byte BOT2 = 4;
const byte BOT3 = 5;
const byte BOT4 = 18;
const byte BOT5 = 21;
const byte MUTE_PIN = 23;

// Efeitos Darth Vader
PitchShift tomGrave(0.8, 20000);
PitchShift tomAgudo(1.5, 1000);
STKNReverb reverb(0.9); 
Compressor comp(SAMP_FREQ, 5, 100, 10, 20, 0.3);
Distortion dist(4490);
Tremolo trem(200, 50, SAMP_FREQ);
STKEcho eco(100);

AudioInfo info(SAMP_FREQ, 1, 16);
AnalogAudioStream in;
AudioEffectStream effects(in);
I2SStream out;                        
StreamCopy copier(out, effects); // copy in to out

void desligaTudo(){ // Desliga todos os efeitos
    tomAgudo.setActive(false);
    eco.setActive(false);
    trem.setActive(false);
    tomGrave.setActive(false);
    reverb.setActive(false);
    comp.setActive(false);
    dist.setActive(false);
}

void ligaVader(){
  desligaTudo();
  tomGrave.setActive(true);
  reverb.setActive(true);
  comp.setActive(true);
  dist.setActive(true);
}

void ligaDroid(){
  desligaTudo();
  tomAgudo.setActive(true);
  eco.setActive(true);
  trem.setActive(true);
  dist.setActive(true);
}

void ligaMando(){
  desligaTudo();
  reverb.setActive(true);
  comp.setActive(true);
  dist.setActive(true);
}

void setup(void) {
  Serial.begin(115200);
  while(!Serial);
  AudioLogger::instance().begin(Serial, AudioLogger::Warning);
  
  // Adiciona os Efeitos para o Darth Vader e Mandaloriano
  effects.addEffect(tomGrave);
  effects.addEffect(reverb);
  effects.addEffect(comp);
  effects.addEffect(dist);
  // Adiciona os Efeitos do Droide
  effects.addEffect(tomAgudo);
  effects.addEffect(trem);
  effects.addEffect(eco);

  //Inicia com todos os efeitos desligados
  desligaTudo();

  // RX automatically uses port 0 with pin GPIO34
  auto cfgRx = in.defaultConfig(RX_MODE);
  cfgRx.copyFrom(info);
  in.begin(cfgRx);
 
  effects.begin(info);

  // TX on I2S_NUM_1 
  auto cfgTx = out.defaultConfig(TX_MODE);
  cfgTx.port_no = 1;
  cfgTx.copyFrom(info);
  cfgTx.pin_ws = 27;
  cfgTx.pin_bck = 25;
  cfgTx.pin_data = 26;
  out.begin(cfgTx);

  //create a task executed in Task1code() function, with priority 1 and executed on core 0
  xTaskCreatePinnedToCore(
                  Task1code, /* Task function. */
                    "Task1", /* name of task (shown below). */
                    10000,  /* Stack size of task */
                    NULL,   /* parameter of the task */
                    1,      /* priority of the task */
                    &Task1, /* handle to keep track of task */
                    0       /* pin task to core0*/
  );
}

void Task1code( void * pvParameters ){    // Task1 vai operar as mudancas de efeitos conforme de acordo com o botao pressionado
  Serial.println("Task1 running on core " + String(xPortGetCoreID())); 
  
  pinMode(BOT1, INPUT_PULLUP);  // Ativa a voz do Darth Vader
  pinMode(BOT2, INPUT_PULLUP);  // Ativa a voz dos droids
  pinMode(BOT3, INPUT_PULLUP);  // Ativa a voz do Mandaloriano
  pinMode(BOT4, INPUT_PULLUP);  // Desativa todos os efeitos
  pinMode(BOT5, INPUT_PULLUP);  // Muta a saida de audio
  pinMode(MUTE_PIN, OUTPUT);    // Saida para controlar o mute
  
  unsigned long timestamp_ultimo_acionamento = 0; //Realiza o debounce dos botoes
  
  while(1){  //loop que controlara a mudanca dos efeitos
    if(digitalRead(BOT1) == LOW){
      if ( (millis() - timestamp_ultimo_acionamento) >= TEMPO_DEBOUNCE ){
        timestamp_ultimo_acionamento = millis();
        Serial.println("BOT1 pressionado");
        ligaVader();
        vTaskDelay(pdMS_TO_TICKS(2000));
      }
    }

    if(digitalRead(BOT2) == LOW){
      if ( (millis() - timestamp_ultimo_acionamento) >= TEMPO_DEBOUNCE ){
        Serial.println("BOT2 pressionado");
        timestamp_ultimo_acionamento = millis();
        ligaDroid();
        vTaskDelay(pdMS_TO_TICKS(2000));
      }
    }

    if(digitalRead(BOT3) == LOW){
      if ( (millis() - timestamp_ultimo_acionamento) >= TEMPO_DEBOUNCE ){
        timestamp_ultimo_acionamento = millis();
        Serial.println("BOT3 pressionado");
        ligaMando();
        vTaskDelay(pdMS_TO_TICKS(2000));
      }
    }

    if(digitalRead(BOT4) == LOW){
      if ( (millis() - timestamp_ultimo_acionamento) >= TEMPO_DEBOUNCE ){
        timestamp_ultimo_acionamento = millis();
        Serial.println("BOT4 pressionado");
        desligaTudo();
        vTaskDelay(pdMS_TO_TICKS(2000));
      }
    }

    if(digitalRead(BOT5) == LOW){
      if ( (millis() - timestamp_ultimo_acionamento) >= TEMPO_DEBOUNCE ){
        timestamp_ultimo_acionamento = millis();
        Serial.println("BOT5 pressionado");
        digitalWrite(MUTE_PIN, !(digitalRead(MUTE_PIN)));
        vTaskDelay(pdMS_TO_TICKS(2000));
      }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

// loop vai apenas copiar o audio modificado (em effects) para a saida I2S
void loop() {
  copier.copy();
}
