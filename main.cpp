//Giovanni Enrique Tabima Galeano 20221673029
//Daniel Urrego 20211673121

#include "mbed.h"
#include <cstdio>

// Definiciones
#define Analog_Pin    A0
#define VoltajeADC    3.3
#define Exp_Voltaje   1000
#define Tiempo_Volt   2s
#define escritura     0x40
#define poner_brillo  0x88
#define dir_display   0xC0
#define Tiempo_rebote 50ms

// Prototipos
void leer_temperatura(void);
void imprimir_temperatura(void);
void send_byte(char data);
void send_data(int number);
void condicion_start(void);
void condicion_stop(void);
void check_alarm(void);

// Hilos
Thread temperatura_thread;
Thread imprimir_thread;
Thread alarm_thread;

// Variables globales
int temperature_C = 0;
int temperatura_mV = 0;

bool isCelsius = 0;  // Variable global para la unidad
bool Q0 = 0;
bool Q1 = 0;
bool Q2 = 0;
DigitalIn Boton(D5);  // Botón para cambiar unidad
DigitalOut buzzer(D4); // Buzzer para la alarma
DigitalOut led(LED1);//Led para saber que esta funcionando el codigo

// Pines
DigitalOut sclk(D2);  // Clock pin
DigitalInOut dio(D3); // Data pin

const char digitToSegment[10] = { 0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F };

int main()
{
    // Inicializar hilos
    temperatura_thread.start(leer_temperatura);
    imprimir_thread.start(imprimir_temperatura);
    alarm_thread.start(check_alarm);
    Q0 = 0;
    Q1 = 0;
    Q2 = 0;

    while (true) {
        led = !led;
        Q2 = Q1;
        Q1 = Q0;
        Q0 = Boton;
        //Antirebote
        if(!Q0 & Q1 & Q2)
        {

            isCelsius = !isCelsius;
            printf("Celcius vale:  %d\n\r", isCelsius);


        }
        ThisThread::sleep_for(Tiempo_rebote);
        
        ThisThread::sleep_for(1s); // Dormir para evitar el uso excesivo de CPU
    }
}

void leer_temperatura() {
    AnalogIn lm35_sensor(Analog_Pin);
    while (true) {
        int raw_value = lm35_sensor.read_u16(); // Leer valor de 8 bits
        int voltage_mV = (raw_value * 3300) / 65536; // Convertir a milivoltios
        temperatura_mV = voltage_mV;
        temperature_C = temperatura_mV / (Exp_Voltaje / 1000); // Convertir a grados Celsius
        ThisThread::sleep_for(Tiempo_Volt); // Esperar el intervalo de actualización
    }
}

void imprimir_temperatura() {
    while (true) {
        int display_temp = isCelsius == 0 ? temperature_C : (temperature_C * 9 / 5) + 32;
        send_data(display_temp); // Enviar la temperatura al display
        ThisThread::sleep_for(Tiempo_Volt); // Esperar el intervalo de actualización
    }
}



void check_alarm(void) {
    while (true) {
        int threshold = isCelsius == 0 ? 900 : 1940; // 90°C o 194°F
        
        if (temperature_C >= threshold) {
            buzzer = 1; // Activar buzzer
        } else {
            buzzer = 0; // Desactivar buzzer
        }
        
        ThisThread::sleep_for(500ms); // Revisar cada 500 ms
    }
}

void condicion_start(void)
{
    sclk = 1;
    dio.output();
    dio = 1;
    ThisThread::sleep_for(1ms);
    dio = 0;
    sclk = 0;
}

void condicion_stop(void)
{
    sclk = 0;
    dio.output();
    dio = 0;
    ThisThread::sleep_for(1ms);
    sclk = 1;
    dio = 1;
}

void send_byte(char data)
{
    dio.output();
    for (int i = 0; i < 8; i++)
    {
        sclk = 0;
        dio = (data & 0x01) ? 1 : 0;
        data >>= 1;
        sclk = 1;
    }
    // Esperar el ACK
    dio.input();
    sclk = 0;
    ThisThread::sleep_for(1ms);
    // Esperar señal de ACK si es necesario
    if (dio == 0) 
    {
        sclk = 1;
        sclk = 0;
    }
}

void send_data(int number) {
    condicion_start();
    send_byte(escritura);
    condicion_stop();
    condicion_start();
    send_byte(dir_display);

    // Descomponer el número en dígitos y enviar al display
    int digit[4] = {0};
    for (int i = 0; i < 4; i++)
    {
        digit[i] = number % 10;
        number /= 10;
    }

    // Enviar los datos al display de derecha a izquierda
    for(int i = 3; i >= 0; i--) {
        send_byte(digitToSegment[digit[i]]);
    }
}
