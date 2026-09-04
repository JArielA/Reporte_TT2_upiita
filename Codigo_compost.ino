//Librerias
#include <OneWire.h>  //Sensor de temperatura
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>  //LCD con I2C
#include <DHT.h>                //Sensor de humedad

//Declaracion de los hilos
TaskHandle_t taskAiro, taskTimeCont, taskTempControl, taskMoni;

//Defeinicion de los pines
#define DHTTYPE 11  //Se define el tipo de sensor DHT
#define DHTPIN 17   //Pin del sensor DHT11

//Lista de pines a utilizar
const int Push1 = 12;
const int Push2 = 13;
const int Push3 = 14;
const int reset = 15;
const int inicio = 27;
const int res_electrica = 5;
const int bomba_agua = 18;
const int ventilador = 33;
const int motor_eje = 32;
const int Hume = 2;  //Pin de lectura del sensor de humedad
//Estado inicial de las entradas
int button_state1 = 0;
int button_state2 = 0;
int button_state3 = 0;
int inicio_state = 0;
//Banderas de activacion
int bandera_agua = 0, bandera_calor = 0, bandera_ventilacion = 0, bandera_inicio = 0;
int bandera_fin = 0, bandera_prueba = 0, bandera_prueba_airo = 0, bandera_error = 0, bandera_prueba_temp = 0;
//Contadores para tiempo de proceso
int segundo = 0, minuto = 0, hora = 0, time_proceso=0;

int dataSensor = 0;

//Configuracion de la LCD
int lcdColumns = 20;  //Columnas
int lcdRows = 4;      //Filas
LiquidCrystal_I2C lcd(0x27, lcdColumns, lcdRows);

OneWire ourWire(4);                   //Se establece el pin de lectura del DS18B20
DallasTemperature DS18B20(&ourWire);  //Declaracion de la variable objeto

DHT dht(DHTPIN, DHTTYPE);  // Declaracion del sensor DHT11

float hum, tem;          //Variables de lectura de las variables fisicas
float hum_lcd, tem_lcd;  //Variables para mostrar en el LCD

// Definicion de tiempos de espera
#define DELAY_1_SECOND 1000
#define DELAY_5_SECONDS 5000
#define DELAY_10_SECONDS 10000
#define DELAY_30_SECONDS 30000
#define DELAY_100_MS 100
#define DELAY_500_MS 500
#define DELAY_2_SECONDS 2000
#define DELAY_10_MINUTES 600000
#define DELAY_15_MINUTES 900000
#define DELAY_5_MINUTES 300000
#define DELAY_3_MINUTES 180000
#define DELAY_4_MINUTES 240000
#define DELAY_7_MINUTES 240000
#define DELAY_1_MINUTE 60000
#define DELAY_150_MINUTES 9000000
// Tiempos de las fases bacterianas 72 hrs
#define INICIO 0 
#define FIN_MES_1 25
#define INICIO_TER 25
#define FIN_TER 60
#define INICIO_MES_2 60
#define FINAL 72

//Temperatura fase mesofila 1
#define TEMP_MES1_MIN 25
#define TEMP_MES1_INT1 30  //25
#define TEMP_MES1_INT2 35  //28
#define TEMP_MES1_MAX 45  //38
//Temperaturas fase termoifila 
#define TEMP_TER_MIN 45
#define TEMP_TER_INT1 50
#define TEMP_TER_INT2 55
#define TEMP_TER_MAX 60
//Temperaturas fase mesofila 2
#define TEMP_MES2_MIN 30
#define TEMP_MES2_INT1 37
#define TEMP_MES2_INT2 39
#define TEMP_MES2_MAX 45

// Subrutinas
void Mensajes_LCD();
void Idem_procesos();
void Interfaz();
void Idem_procesos_Serial();
void ModoPrueba();
void PruebaAiro();
void PruebaTemp();
void DesacTermo();
void ActivaTermo();

void setup() {
  xTaskCreatePinnedToCore(
    AiroFunction,    //Nombre del loop creado
    "AiroFunction",  //Nombre
    10000,           //Tamano de la pila
    NULL,            //Parametro casi siempre Nulo
    1,               //Prioridad de la tarea
    &taskAiro,       //Nombre de la tarea
    0);              //Nucleo donde se ejecuta

  xTaskCreatePinnedToCore(
    TimeContFunction,    //Nombre del loop creado
    "TimeContFunction",  //Nombre
    10000,               //Tamano de la pila
    NULL,                //Parametro casi siempre Nulo
    1,                   //Prioridad de la tarea
    &taskTimeCont,       //Nombre de la tarea
    0);                  //Nucleo donde se ejecuta
  xTaskCreatePinnedToCore(
    Monitoreo,    //Nombre del loop creado
    "Monitoreo",  //Nombre
    10000,        //Tamano de la pila
    NULL,         //Parametro casi siempre Nulo
    1,            //Prioridad de la tarea
    &taskMoni,    //Nombre de la tarea
    0);           //Nucleo donde se ejecuta
  xTaskCreatePinnedToCore(
    TempControl,       //Nombre del loop creado
    "TempControl",     //Nombre
    10000,             //Tamano de la pila
    NULL,              //Parametro casi siempre Nulo
    1,                 //Prioridad de la tarea
    &taskTempControl,  //Nombre de la tarea
    0);                //Nucleo donde se ejecuta


  //Inicializar sensor DS18B20
  DS18B20.begin();
  //Inicializamos el sensor DHT11
  dht.begin();

  pinMode(Push1, INPUT);
  pinMode(Push2, INPUT);
  pinMode(Push3, INPUT);
  pinMode(inicio, INPUT);
  pinMode(res_electrica, OUTPUT);
  pinMode(bomba_agua, OUTPUT);
  pinMode(ventilador, OUTPUT);
  pinMode(motor_eje, OUTPUT);
  pinMode(reset, OUTPUT);

  digitalWrite(reset,HIGH);

  // inicializar LCD
  lcd.init();
  lcd.backlight();

  Mensajes_LCD(1);

  Serial.begin(9600);
}

void loop() {
  //Codigo para iniciar y finalizar los procesos de compostaje
  if (bandera_inicio == 0 && bandera_prueba == 0) {
    Mensajes_LCD(8);
    inicio_state = digitalRead(inicio);
    if (inicio_state == 1) {
      bandera_inicio = 1;
      segundo = 0;
      minuto = 0;
      hora = 0;
      time_proceso = 0;
      for (int i = 0; i < 10; i++) {
        Mensajes_LCD(10);
      }
    }
    button_state1 = digitalRead(Push1);
    if (button_state1 == 1) {
      bandera_prueba = 1;
      ModoPrueba();
    }
  } else if (bandera_inicio == 1 && bandera_prueba == 0) {
    Interfaz();
    if (bandera_fin == 1) {
      bandera_fin = 0;
      Mensajes_LCD(9);
    }
  }
}

void TimeContFunction(void *parameter) {
  while (true) {
    vTaskDelay(DELAY_1_SECOND);
    segundo = segundo + 1;
    if (segundo == 60) {
      segundo = 0;
      minuto = minuto + 1;
    }
    if (minuto == 60) {
      hora = hora + 1;
      minuto = 0;
      time_proceso = time_proceso + 1;
      }
    if (bandera_inicio == 1 && time_proceso == FINAL) {
      minuto = 0;
      segundo = 0;
      hora = 0;
      time_proceso = 0;
      bandera_fin = 1;
      bandera_inicio = 0;
      digitalWrite(ventilador, LOW);
      digitalWrite(motor_eje, LOW);
      digitalWrite(res_electrica, LOW);
      //FinaldeProceso();
      delay(DELAY_150_MINUTES);
    }
  }
  vTaskDelay(10);  //Evitar advertencia de watchdog
}

void Monitoreo(void *parameter) {
  while (true) {
    DS18B20.requestTemperatures();
    tem_lcd = DS18B20.getTempCByIndex(0);
    vTaskDelay(DELAY_1_SECOND);
    hum_lcd = dht.readHumidity();
    vTaskDelay(DELAY_1_SECOND);
  }
  vTaskDelay(10);  //Evitar advertencia de watchdog
}

void ActivaTermo(){
  digitalWrite(res_electrica, HIGH);
  digitalWrite(ventilador, LOW);
  digitalWrite(motor_eje, HIGH);
  vTaskDelay(DELAY_5_MINUTES);
}
void DesacTermo(){
  digitalWrite(res_electrica, LOW);
  digitalWrite(ventilador, LOW);
  digitalWrite(motor_eje, LOW);
}
void TempControl(void *parameter) {
  while (true) {
    vTaskDelay(DELAY_100_MS);
    digitalWrite(res_electrica, LOW);

    // CONTROL DE TEMPERATURA
    if (bandera_inicio == 1 && bandera_ventilacion == 0 && bandera_error ==0) {
      //Fase mesofila inicial
      if (time_proceso < FIN_MES_1) {
        if (tem_lcd < TEMP_MES1_MIN) {
          bandera_calor = 1;
          Serial.println("punto 1 MES1");
          ActivaTermo();
          //vTaskDelay(DELAY_5_MINUTES);
        } else if(tem_lcd >= TEMP_MES1_MIN && tem_lcd < TEMP_MES1_INT1){
          bandera_calor = 0;
          Serial.println("punto 2 MES1");
          DesacTermo();
        }else if (tem_lcd >= TEMP_MES1_INT1 && tem_lcd < TEMP_MES1_INT2) {
          bandera_calor = 1;
          Serial.println("punto 3 MES1");
          ActivaTermo();
        } else if (tem_lcd >= TEMP_MES1_INT2 && tem_lcd < TEMP_MES1_MAX) {
          bandera_calor = 0;
          Serial.println("punto 4 MES1");
          DesacTermo();
        } else if (tem_lcd > TEMP_MES1_MAX) {
          bandera_calor = 0;
          Serial.println("punto 5 MES1");
          digitalWrite(res_electrica, LOW);
          digitalWrite(ventilador, HIGH);
        } else{
          bandera_calor = 0;
         
        }
      }

      //Fase termofila
      if (time_proceso >= INICIO_TER && time_proceso < FIN_TER) {
        if (tem_lcd < TEMP_TER_MIN) {
          bandera_calor = 1;
          Serial.println("punto 1 TER");
          ActivaTermo();
          vTaskDelay(DELAY_5_MINUTES);
        } else if (tem_lcd >= TEMP_TER_MIN && tem_lcd < TEMP_TER_INT1) {
          bandera_calor = 0;
          Serial.println("punto 2 TER");
          DesacTermo();
        } else if (tem_lcd >= TEMP_TER_INT1 && tem_lcd < TEMP_TER_INT2) {
          bandera_calor = 1;
          Serial.println("punto 3 TER");
          ActivaTermo();
        } else if (tem_lcd >= TEMP_TER_INT2 && tem_lcd < TEMP_TER_MAX) {
          bandera_calor = 0;
          Serial.println("punto 4 TER");
          DesacTermo();
        } else if (tem_lcd > TEMP_TER_MAX) {
          bandera_calor = 0;
          Serial.println("punto 5 TER");
          digitalWrite(ventilador, HIGH);
          digitalWrite(res_electrica, LOW);
        } else{
          bandera_calor = 0;
        }
      }

      //Fase mesofila final
      if (time_proceso >= INICIO_MES_2 && time_proceso < FINAL) {
        if (tem_lcd < TEMP_MES2_MIN) {
          bandera_calor = 1;
          Serial.println("punto 1 MES2");
          ActivaTermo();
        } else if (tem_lcd >= TEMP_MES2_MIN && tem_lcd < TEMP_MES2_MAX) {
          bandera_calor = 0;
          Serial.println("punto 2 MES2");
          DesacTermo();
        } else if (tem_lcd > TEMP_MES2_MAX) {
          bandera_calor = 0;
          Serial.println("punto 3 MES2");
          digitalWrite(ventilador, HIGH);
          digitalWrite(res_electrica, LOW);
          digitalWrite(motor_eje, LOW);
        } else{
          bandera_calor = 0;
        }
      }
    }

    //Funcion de prueba
    if (bandera_prueba_temp == 1) {
      digitalWrite(res_electrica, HIGH);
      delay(DELAY_30_SECONDS);
      bandera_prueba_temp = 0;
    }
  }
  vTaskDelay(10);  //Evitar advertencia de watchdog
}

void AiroFunction(void *parameter) {
  while (true) {
    delay(DELAY_500_MS);
  
    while (bandera_inicio == 1 && bandera_error == 0) {
      vTaskDelay(DELAY_5_SECONDS);

      if (bandera_calor == 0) {
        bandera_ventilacion = 0;
        digitalWrite(motor_eje, LOW);
        digitalWrite(ventilador, LOW);
        vTaskDelay(DELAY_150_MINUTES);
        if(bandera_calor==0){
          bandera_ventilacion = 1;
          digitalWrite(motor_eje, HIGH);
          digitalWrite(ventilador, HIGH);
          vTaskDelay(DELAY_10_MINUTES);
        }
        
      }
    }
    if (bandera_inicio == 0 && bandera_prueba_airo == 0) {
      digitalWrite(motor_eje, LOW);
      digitalWrite(ventilador, LOW);
    }
    if (bandera_prueba_airo == 1) {
      digitalWrite(motor_eje, HIGH);
      digitalWrite(ventilador, HIGH);
      delay(DELAY_30_SECONDS);
      bandera_prueba_airo = 0;
    }
  }

  vTaskDelay(10);  //Evitar advertencia de watchdog
}

void Mensajes_LCD(int sel_Mensaje) {
  switch (sel_Mensaje) {
    case 1:  //Menu de la interfaz
      lcd.setCursor(0, 0);
      lcd.print("Menu");
      lcd.setCursor(0, 1);
      lcd.print("1 Monitoreo");
      lcd.setCursor(0, 2);
      lcd.print("2 proceso");
      lcd.setCursor(0, 3);
      lcd.print("3 Tiempo");
      break;
    case 2:  //Lectura de las variables
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Temperatura °C");
      lcd.setCursor(5, 1);
      lcd.print(tem_lcd);
      lcd.setCursor(0, 2);
      lcd.print("% de humedad");
      lcd.setCursor(5, 3);
      lcd.print(hum_lcd);
      delay(5000);
      lcd.clear();
      break;
    case 3:  //Proceso de aeriacion
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Proceso en ejecucion");
      lcd.setCursor(0, 2);
      lcd.print("Aireacion");
      lcd.setCursor(0, 3);
      lcd.print(" ");
      delay(5000);
      lcd.clear();
      break;
    case 4:  // Elevar la temperatura
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Proceso en ejecucion");
      lcd.setCursor(0, 2);
      lcd.print("Elevando la");
      lcd.setCursor(0, 3);
      lcd.print("Temperatura");
      delay(5000);
      lcd.clear();
      break;
    case 5:  // ELevar la humedad
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Proceso en ejecucion");
      lcd.setCursor(0, 2);
      lcd.print("Elevendo la");
      lcd.setCursor(0, 3);
      lcd.print("Humedad");
      delay(5000);
      lcd.clear();
      break;
    case 6:  // Fermentacion
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Proceso en ejecucion");
      lcd.setCursor(0, 2);
      lcd.print("Fermentacion");
      lcd.setCursor(0, 3);
      lcd.print(" ");
      delay(5000);
      lcd.clear();
      break;
    case 7:  //Tiempo de proceso
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Tiempo total de");
      lcd.setCursor(0, 1);
      lcd.print("proceso");
      lcd.setCursor(0, 3);
      lcd.print(hora);
      lcd.setCursor(2, 3);
      lcd.print(":");
      lcd.setCursor(3, 3);
      lcd.print(minuto);
      lcd.setCursor(5, 3);
      lcd.print(":");
      lcd.setCursor(6, 3);
      lcd.print(segundo);
      delay(500);
      lcd.clear();
      break;
    case 8:  //Pantalla de sistema apagado
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Sistema apagado");
      lcd.setCursor(0, 1);
      lcd.print("B1 Iniciar proceso");
      lcd.setCursor(0, 2);
      lcd.print("B2 Modo prueba");
      lcd.setCursor(0, 3);
      lcd.print(" ");
      delay(500);
      break;
    case 9:  //Pantalla para final del proceso
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Proceso terminado");
      lcd.setCursor(0, 1);
      lcd.print("Descargue la");
      lcd.setCursor(0, 2);
      lcd.print("Materia");
      lcd.setCursor(0, 3);
      lcd.print(" ");
      delay(5000);
      lcd.clear();
      break;
    case 10:  //Pantalla de inicio
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Iniciando ...");
      delay(500);
      break;
    case 11:  //Bienvenida modo prueba
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Modo prueba");
      lcd.setCursor(0, 1);
      lcd.print("Cada ciclo de");
      lcd.setCursor(0, 2);
      lcd.print("Trabajo, se activa");
      lcd.setCursor(0, 3);
      lcd.print("de forma individual");
      delay(DELAY_5_SECONDS);
      lcd.clear();
      break;
    case 12:  //Menu modo prueba
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("B1: Aireacion");
      lcd.setCursor(0, 1);
      lcd.print("B2: temperatura");
      lcd.setCursor(0, 3);
      lcd.print("B4: Salir");
      delay(DELAY_500_MS);
      lcd.clear();
      break;
    case 13:  // Mensaje de ejecucion de prueba de temperatura
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Ejecutando la prueba");
      lcd.setCursor(0, 1);
      lcd.print("Temperatura inicial");
      lcd.setCursor(0, 2);
      lcd.print(tem_lcd);
      delay(DELAY_5_SECONDS);
      break;
    case 14:  //Finalizacion de prueba de temperatura
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Prueba Finalizada");
      lcd.setCursor(0, 2);
      lcd.print("Temperatura final");
      lcd.setCursor(0, 3);
      lcd.print(tem_lcd);
      delay(DELAY_5_SECONDS);
      lcd.clear();
      break;
    case 15:  //Inicio prueba aireacion
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Prueba de Aireacion");
      lcd.setCursor(0, 1);
      lcd.print("Se enciende");
      lcd.setCursor(0, 2);
      lcd.print("Ventilador");
      lcd.setCursor(0, 3);
      lcd.print("Eje de aireacion");
      delay(DELAY_5_SECONDS);
      break;
    case 16:  //Final de prueba de aireacion
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Prueba finalizada");
      lcd.setCursor(0, 1);
      lcd.print("Apagando los");
      lcd.setCursor(0, 2);
      lcd.print("mecanismos de");
      lcd.setCursor(0, 3);
      lcd.print("aireacion");
      delay(DELAY_5_SECONDS);
      lcd.clear();
      break;
    case 17:  //Salida del modo prueba
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Saliendo de modo");
      lcd.setCursor(0, 1);
      lcd.print("prueba");
      lcd.setCursor(0, 2);
      lcd.print("Eres dirigido a");
      lcd.setCursor(0, 3);
      lcd.print("Menu inicio");
      delay(DELAY_5_SECONDS);
      lcd.clear();
      break;
    default:
      // Mensaje de prueba
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("Mensaje de prueba");
      lcd.setCursor(0, 2);
      lcd.print("Para ver si funca ");
      lcd.setCursor(0, 3);
      lcd.print("El boton");
      delay(500);
      lcd.clear();
      break;
  }
}

void Idem_procesos() {
  if (bandera_calor == 1 && bandera_ventilacion == 0) {
    //Serial.println("Elevar temperatura");
    Mensajes_LCD(4);
  } else if (bandera_agua == 1 && bandera_ventilacion == 0) {
    Mensajes_LCD(5);
    //Serial.println("Elevar humedad");
  } else if (bandera_ventilacion == 1 && bandera_calor == 0 && bandera_agua == 0) {
    Mensajes_LCD(3);
    //Serial.println("Aireacion");
  } else {
    Mensajes_LCD(6);
    //Serial.println("Fermentacion");
  }
  delay(10);
}

void Idem_procesos_Serial() {
  if (bandera_calor == 1 && bandera_ventilacion == 0) {
    Serial.println("Elevar temperatura");
  } else if (bandera_ventilacion == 1 && bandera_calor == 0) {
    Serial.println("Aireacion");
  } else if (bandera_calor==0 && bandera_ventilacion ==0){
    Serial.println("Fermentacion");
  } else{
    Serial.println("Error");
    bandera_error = 1;
    bandera_ventilacion = 0;
    bandera_calor =0;
    digitalWrite(res_electrica, LOW);
    digitalWrite(ventilador, LOW);
    digitalWrite(motor_eje, LOW);
    delay(DELAY_500_MS);
    bandera_error = 0;
  }
  delay(1000);
}

void Interfaz() {
  // Codigo de la interfaz
  button_state1 = digitalRead(Push1);
  if (button_state1 == HIGH) {
    //Lectura de las variables internas
    Mensajes_LCD(2);
  }

  button_state2 = digitalRead(Push2);
  if (button_state2 == HIGH) {
    //Proceso en ejecucion
    Idem_procesos();
  }

  button_state3 = digitalRead(Push3);
  if (button_state3 == HIGH) {
    //Tiempo de procesamiento
    for (int i = 0; i < 10; i++) {
      Mensajes_LCD(7);
    }
    //Mensajes_LCD(7);
  } else {
    Mensajes_LCD(1);
  }
  delay(200);
  Idem_procesos_Serial();
}
void ModoPrueba() {
  Mensajes_LCD(11);
  while (button_state3 == 0) {
    Mensajes_LCD(12);
    button_state1 = digitalRead(Push1);
    if (button_state1 == 1) {
      Serial.println(bandera_prueba_airo);
      PruebaAiro();
      Serial.println(bandera_prueba_airo);
    }
    button_state2 = digitalRead(Push2);
    if (button_state2 == 1) {
      PruebaTemp();
    }
    button_state3 = digitalRead(Push3);
  }
  Mensajes_LCD(17);
  digitalWrite(reset,LOW);
  bandera_prueba = 0;
}

void PruebaTemp() {
  Mensajes_LCD(13);
  bandera_prueba_temp = 1;
  Serial.println(bandera_prueba_temp);
  while (bandera_prueba_temp == 1) {
    Serial.println(bandera_prueba_temp);
  }
  Serial.println(bandera_prueba_temp);
  Mensajes_LCD(14);
  //band = 1;
  //Mensajes_LCD(14);
}

void PruebaAiro() {
  Mensajes_LCD(15);
  bandera_prueba_airo = 1;
  Serial.println(bandera_prueba_airo);
  while (bandera_prueba_airo == 1) {
    Serial.println(bandera_prueba_airo);
  }
  Serial.println(bandera_prueba_airo);
  Mensajes_LCD(16);
  //Mensajes_LCD(16);
}