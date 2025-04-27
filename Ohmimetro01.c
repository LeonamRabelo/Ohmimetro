#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/i2c.h"
#include "lib/ssd1306.h"
#include "lib/font.h"
#include "hardware/pio.h"
#include "ws2812.pio.h"
#include "pico/bootrom.h"

#define botaoB 6
#define WS2812_PIN 7    //Pino do WS2812
#define NUM_PIXELS 25    //Quantidade de LEDs
#define IS_RGBW false   //Maquina PIO para RGBW
#define I2C_PORT i2c1 //I2C port
#define I2C_SDA 14    //I2C SDA -> dados
#define I2C_SCL 15    //I2C SCL -> clock
#define endereco 0x3C //Endereço do display
#define ADC_PIN 28    //GPIO para leitura ADC
#define Botao_A 5     //GPIO para botão A
ssd1306_t ssd;         //Estrutura do display

//Funcao para modularizar a inicialização dos componentes
void inicializar_componentes(){
  //Inicializa o pio
  PIO pio = pio0;
  int sm = 0;
  uint offset = pio_add_program(pio, &ws2812_program);
  ws2812_program_init(pio, sm, offset, WS2812_PIN, 800000, IS_RGBW);

  //Para ser utilizado o modo BOOTSEL com botão B
  gpio_init(botaoB);
  gpio_set_dir(botaoB, GPIO_IN);
  gpio_pull_up(botaoB);

  // I2C Initialisation. Using it at 400Khz.
  i2c_init(I2C_PORT, 400 * 1000);

  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);                    // Set the GPIO pin function to I2C
  gpio_pull_up(I2C_SDA);                                        // Pull up the data line
  gpio_pull_up(I2C_SCL);                                        // Pull up the clock line                                              // Inicializa a estrutura do display
  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
  ssd1306_config(&ssd);                                         // Configura o display
  ssd1306_send_data(&ssd);                                      // Envia os dados para o display
  // Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);
  
  adc_init();
  adc_gpio_init(ADC_PIN); //GPIO 28 como entrada analógica
}

//Variável global para armazenar a cor (Entre 0 e 255 para intensidade)
uint8_t led_r = 20; //Intensidade do vermelho
uint8_t led_g = 20; //Intensidade do verde
uint8_t led_b = 20; //Intensidade do azul

//Função para ligar um LED
static inline void put_pixel(uint32_t pixel_grb){
    pio_sm_put_blocking(pio0, 0, pixel_grb << 8u);
}

//Função para converter cores RGB para um valor de 32 bits
static inline uint32_t urgb_u32(uint8_t r, uint8_t g, uint8_t b){
    return ((uint32_t)(r) << 8) | ((uint32_t)(g) << 16) | (uint32_t)(b);
}

//Struct para armazenar as cores em formato RGB
typedef struct{
  uint8_t r, g, b;
}CorRGB;

CorRGB cores_rgb[] = {  //Tabela de cores RGB para os resistores
  {0, 0, 0},        //Preto
  {139, 69, 19},    //Marrom
  {255, 0, 0},      //Vermelho
  {255, 195, 0},    //Laranja
  {255, 255, 0},    //Amarelo
  {0, 255, 0},      //Verde
  {0, 0, 255},      //Azul
  {148, 0, 211},    //Violeta
  {140, 140, 160},  //Cinza
  {255, 255, 255}   //Branco
};

//Funcao para pintar as faixas RGB na Matriz de LEDS WS2812
void cor_faixas_RGB_WS2812(int faixa1, int faixa2, int multiplicador){
  //Usa a funcao urgb_u32 para converter as cores RGB para um valor de 32 bits com base na tabela
  uint32_t cor1 = urgb_u32(cores_rgb[faixa1].r, cores_rgb[faixa1].g, cores_rgb[faixa1].b);
  uint32_t cor2 = urgb_u32(cores_rgb[faixa2].r, cores_rgb[faixa2].g, cores_rgb[faixa2].b);
  uint32_t cor3 = urgb_u32(cores_rgb[multiplicador].r, cores_rgb[multiplicador].g, cores_rgb[multiplicador].b);
  uint32_t fundo = urgb_u32(2, 2, 2);
  //Pinta as faixas RGB na Matriz
  for(int i = 0; i < NUM_PIXELS; i++){
      int linha = i / 5;      //Linha da matriz
      int coluna;

      //Zig-zague: linhas pares invertem a leitura
      if(linha % 2 == 0){
          coluna = 4 - (i % 5);   //Linha par: direita para esquerda
      }else{
          coluna = i % 5;         //Linha ímpar: esquerda para direita
      }

      if(coluna == 1){
          put_pixel(cor1);  //Coluna 1 = Faixa 1
      }else if(coluna == 2){
          put_pixel(cor2);  //Coluna 2 = Faixa 2
      }else if(coluna == 3){
          put_pixel(cor3);  //Coluna 3 = Multiplicador
      }else{
          put_pixel(fundo);     //Apaga os outros LEDs
      }
  }
}

//Faixa de valores da série E24 de 510 ate 100k ohms
const int resistores_E24[] ={
  510, 560, 620, 680, 750, 820, 910,
  1000, 1100, 1200, 1300, 1500, 1600, 1800, 2000,
  2200, 2400, 2700, 3000, 3300, 3600, 3900, 4300,
  4700, 5100, 5600, 6200, 6800, 7500, 8200, 9100,
  10000, 11000, 12000, 13000, 15000, 16000, 18000,
  20000, 22000, 24000, 27000, 30000, 33000, 36000,
  39000, 43000, 47000, 51000, 56000, 62000, 68000,
  75000, 82000, 91000, 100000
};
const int E24_SIZE = sizeof(resistores_E24) / sizeof(resistores_E24[0]);  //Tamanho da tabela

//Faixa de cores da serie E24, no caso as 3 faixas iniciais, ignorando a ultima faixa de tolerancia, seguindo a ordem de cores da tabela
const char* cores[] = {
    "Preto", "Marrom", "Vermelho", "Laranja", "Amarelo",
    "Verde", "Azul", "Violeta", "Cinza", "Branco"
};

//Encontra o valor E24 mais próximo
int valor_e24_mais_proximo(int resistor){
  int min_diff = 1e9;       //Define um valor muito grande (1bilhao->9 digitos) para inicializar a variavel e realizar as comparações
  int valor_proximo = resistores_E24[0];  //Define o primeiro valor da faixa E24 como o mais proximo
  for (int i = 0; i < E24_SIZE; i++){ //Verifica qual o valor mais proximo
      int diff = abs(resistor - resistores_E24[i]); //Calcula a diferença absoluta entre o valor medido e cada valor da tabela
      if (diff < min_diff){ //Verifica se a diferença atual é menor que a menor diferença encontrada
          min_diff = diff;  //Atualiza a menor diferença
          valor_proximo = resistores_E24[i];  //Atualiza o valor mais proximo com o valor da tabela na posição i
      }
  }
  return valor_proximo; //Retorna o valor mais proximo
}

//Converte valor em ohms para as 3 faixas do código de cores
void converter_para_cores(int resistor, int* faixa1, int* faixa2, int* multiplicador){
  int base = resistor;      //Guarda o valor a ser convertido
  int potencia = 0;         //Guarda a potência do valor
  while(base >= 100){       //Enquanto o valor for maior ou igual a 100, ele divide o valor por 10 e incrementa a potência
      base /= 10;           //Divide o valor por 10, guardando o resultado da novo valor
      potencia++;           //Incrementa a potencia para o multiplicador, ja que cresce aumentando um digito
  }
  *faixa1 = base / 10;        //Guarda o valor do primeiro digito da base final
  *faixa2 = base % 10;        //Guarda o valor do segundo digito da base final
  *multiplicador = potencia;  //Guarda a potência
}

void gpio_irq_handler(uint gpio, uint32_t events)
{
  reset_usb_boot(0, 0);
}

//Parâmetros do Ohmimetro
int R_conhecido = 10000;   //Resistor de 10k ohm
float ADC_VREF = 3.31;     //Tensão de referência do ADC
int ADC_RESOLUTION = 4095; //Resolução do ADC (12 bits)
int main(){
  inicializar_componentes();  //Funcao para inicializar os componentes utilizados no projeto

  char str_x[20]; // Buffer para armazenar a string
  char str_y[20]; // Buffer para armazenar a string
  bool cor = true;

  gpio_set_irq_enabled_with_callback(botaoB, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);  //Interrupção do botão B

  while (true){
    adc_select_input(2); // Seleciona o ADC para eixo X. O pino 28 como entrada analógica

    //Leitura do ADC, com media de 500 leituras, para melhorar a leitura por meio da media
    float soma = 0.0f;
    for(int i = 0; i < 500; i++){
      soma += adc_read();
      sleep_ms(1);
    }
    float media = soma/500.0f;

    //Fórmula simplificada: Resistor_x = R_conhecido * ADC_encontrado /(ADC_RESOLUTION - adc_encontrado)
    float Resistor_x = (R_conhecido * media) / (ADC_RESOLUTION - media);

    int Rx_padrao = valor_e24_mais_proximo((int)Resistor_x);  //wokwi parece nao ler o ADC 28, DESMARCAR ESSA LINHA PARA TESTE FISICO
    //int Rx_padrao = valor_e24_mais_proximo(1820);  //Teste passando valores diretos, COMENTAR ESSA LINHA EM TESTE FISICO
    int faixa1, faixa2, multiplicador;
    converter_para_cores(Rx_padrao, &faixa1, &faixa2, &multiplicador);  //Os valores serao guardados nas variaveis faixa1, faixa2 e multiplicador de forma direta, uso de ponteiros
    cor_faixas_RGB_WS2812(faixa1, faixa2, multiplicador);

    //Buffer para escrever os dados e desenhar no display
    char str_resistor[20];
    char str_cor1[20], str_cor2[20], str_mult[20];
    sprintf(str_resistor, "%d ohms", Rx_padrao);        //Converte o valor do resistor para string
    sprintf(str_cor1, "1:%s", cores[faixa1]);           //Converte o valor da faixa 1 para string
    sprintf(str_cor2, "2:%s", cores[faixa2]);           //Converte o valor da faixa 2 para string
    sprintf(str_mult, "X:%s", cores[multiplicador]);    //Converte o valor do multiplicador para string

    ssd1306_fill(&ssd, false);                          //Limpa o display para poder escrever novamente
    ssd1306_rect(&ssd, 3, 2, 120, 61, cor, !cor);      //Desenha um retângulo ao redor do display
    ssd1306_draw_string(&ssd, str_resistor, 26, 0);   //Escreve o valor do resistor no display

    //Desenho do resistor estilizado no display
    ssd1306_line(&ssd, 23, 10, 98, 10, true);           //Desenha base de cima
    ssd1306_line(&ssd, 23, 27, 98, 27, true);           //Desenha base de baixo
    ssd1306_line(&ssd, 28, 10, 28, 27, true);           //Desenha cor faixa 1
    ssd1306_line(&ssd, 29, 10, 29, 27, true);           //Desenha cor faixa 1
    ssd1306_line(&ssd, 30, 10, 30, 27, true);           //Desenha cor faixa 1
    ssd1306_draw_string(&ssd, "1", 32, 15);
    ssd1306_line(&ssd, 46, 10, 46, 27, true);           //Desenha cor faixa 2
    ssd1306_line(&ssd, 47, 10, 47, 27, true);           //Desenha cor faixa 2
    ssd1306_line(&ssd, 48, 10, 48, 27, true);           //Desenha cor faixa 2
    ssd1306_draw_string(&ssd, "2", 50, 15);
    ssd1306_line(&ssd, 64, 10, 64, 27, true);             //Desenha cor faixa do multiplicador
    ssd1306_line(&ssd, 65, 10, 65, 27, true);             //Desenha cor faixa do multiplicador
    ssd1306_line(&ssd, 66, 10, 66, 27, true);             //Desenha cor faixa do multiplicador
    ssd1306_draw_string(&ssd, "X", 68, 15);
    ssd1306_line(&ssd, 88, 10, 88, 27, true);             //Desenha simulando faixa da tolerancia
    ssd1306_line(&ssd, 89, 10, 89, 27, true);             //Desenha simulando faixa da tolerancia
    ssd1306_line(&ssd, 23, 10, 8, 17, true);              //Desenha lado esquerdo-cima
    ssd1306_line(&ssd, 23, 27, 8, 17, true);              //Desenha lado esquerdo-baixo
    ssd1306_line(&ssd, 98, 10, 111, 17, true);            //Desenha lado direito-cima
    ssd1306_line(&ssd, 98, 27, 111, 16, true);            //Desenha lado direito-baixo
    ssd1306_line(&ssd, 2, 17, 8, 17, true);               //Desenha linha esquerda de saida
    ssd1306_line(&ssd, 111, 17, 120, 17, true);           //Desenha linha direita de saida
    //Imprime a legenda das cores
    ssd1306_rect(&ssd, 32, 18, 85, 32, true, false);      //Desenha retângulo de borda da legenda
    ssd1306_draw_string(&ssd, str_cor1, 20, 34);          //Desenha legenda da cor 1
    ssd1306_draw_string(&ssd, str_cor2, 20, 44);          //Desenha legenda da cor 2
    ssd1306_draw_string(&ssd, str_mult, 20, 54);          //Desenha legenda do multiplicador

    ssd1306_send_data(&ssd);  //Envia os dados para o display

    sleep_ms(700);
  }
}