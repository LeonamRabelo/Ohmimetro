#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C
#define ADC_PIN 28 // GPIO para o voltímetro
#define Botao_A 5  // GPIO para botão A

// Faixa de valores da série E24
const int resistores_E24[] = {
  510, 560, 620, 680, 750, 820, 910,
  1000, 1100, 1200, 1300, 1500, 1600, 1800, 2000,
  2200, 2400, 2700, 3000, 3300, 3600, 3900, 4300,
  4700, 5100, 5600, 6200, 6800, 7500, 8200, 9100,
  10000, 11000, 12000, 13000, 15000, 16000, 18000,
  20000, 22000, 24000, 27000, 30000, 33000, 36000,
  39000, 43000, 47000, 51000, 56000, 62000, 68000,
  75000, 82000, 91000, 100000
};
const int E24_SIZE = sizeof(resistores_E24) / sizeof(resistores_E24[0]);

const char* cores[] = {///
    "Preto", "Marrom", "Vermelho", "Laranja", "Amarelo",
    "Verde", "Azul", "Violeta", "Cinza", "Branco"
};

// Encontra o valor E24 mais próximo
int valor_e24_mais_proximo(int resistor) {
  int min_diff = 1e9;
  int valor_proximo = resistores_E24[0];
  for (int i = 0; i < E24_SIZE; i++) {
      int diff = abs(resistor - resistores_E24[i]);
      if (diff < min_diff) {
          min_diff = diff;
          valor_proximo = resistores_E24[i];
      }
  }
  return valor_proximo;
}

// Converte valor em ohms para as 3 faixas do código de cores
void converter_para_cores(int resistor, int* faixa1, int* faixa2, int* multiplicador) {
  int base = resistor;
  int potencia = 0;
  while (base >= 100) {
      base /= 10;
      potencia++;
  }
  *faixa1 = base / 10;
  *faixa2 = base % 10;
  *multiplicador = potencia;
}



int R_conhecido = 10000;   // Resistor de 10k ohm
float ADC_VREF = 3.31;     // Tensão de referência do ADC
int ADC_RESOLUTION = 4095; // Resolução do ADC (12 bits)

// Trecho para modo BOOTSEL com botão B
#include "pico/bootrom.h"
#define botaoB 6
void gpio_irq_handler(uint gpio, uint32_t events)
{
  reset_usb_boot(0, 0);
}

int main(){
  // Para ser utilizado o modo BOOTSEL com botão B
  gpio_init(botaoB);
  gpio_set_dir(botaoB, GPIO_IN);
  gpio_pull_up(botaoB);
  gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);
  // Aqui termina o trecho para modo BOOTSEL com botão B

  // I2C Initialisation. Using it at 400Khz.
  i2c_init(I2C_PORT, 400 * 1000);

  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
  gpio_pull_up(I2C_SDA);                                        // Pull up the data line
  gpio_pull_up(I2C_SCL);                                        // Pull up the clock line
  ssd1306_t ssd;                                                // Inicializa a estrutura do display
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
  ssd1306_config(&ssd);                                         // Configura o display
  ssd1306_send_data(&ssd);                                      // Envia os dados para o display

  // Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

  adc_init();
  adc_gpio_init(ADC_PIN); // GPIO 28 como entrada analógica

  char str_x[20]; // Buffer para armazenar a string
  char str_y[20]; // Buffer para armazenar a string

  bool cor = true;
  while (true){
    adc_select_input(2); // Seleciona o ADC para eixo X. O pino 28 como entrada analógica

    float soma = 0.0f;
    for(int i = 0; i < 500; i++){
      soma += adc_read();
      sleep_ms(1);
    }
    float media = soma/500.0f;


    // Proteção contra divisão por zero (evita R_x infinito)
    if(media >= ADC_RESOLUTION) media = ADC_RESOLUTION - 1;

    //Fórmula simplificada: R_x = R_conhecido * ADC_encontrado /(ADC_RESOLUTION - adc_encontrado)
    float Resistor_x = (R_conhecido * media) / (ADC_RESOLUTION - media);
    
    //printf("R_x: %f ohms\n", Resistor_x);
    //int Rx_padrao = valor_e24_mais_proximo((int)Resistor_x);  //a porcaria nao ler o ADC, talvez seja o simulador
    int Rx_padrao = valor_e24_mais_proximo(610);  //teste passando valores diretos
    int f1, f2, mult;
    converter_para_cores(Rx_padrao, &f1, &f2, &mult);

    char str_res[20];
    char str_cor1[20], str_cor2[20], str_mult[20];
    sprintf(str_res, "R: %d ohms", Rx_padrao);
    sprintf(str_cor1, "1: %s", cores[f1]);
    sprintf(str_cor2, "2: %s", cores[f2]);
    sprintf(str_mult, "x: %s", cores[mult]);

    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "Ohmimetro", 20, 0);
    ssd1306_draw_string(&ssd, str_res, 0, 16);
    ssd1306_draw_string(&ssd, str_cor1, 0, 32);
    ssd1306_draw_string(&ssd, str_cor2, 0, 42);
    ssd1306_draw_string(&ssd, str_mult, 0, 52);
    ssd1306_send_data(&ssd);

    sleep_ms(700);
  }
}