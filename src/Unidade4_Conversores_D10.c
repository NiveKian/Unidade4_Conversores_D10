#include <stdio.h>
#include <stdint.h>
#include <ctype.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"
#include "hardware/adc.h"
#include "I2C/display.h"

#define LED_RED 13
#define LED_BLUE 12
#define LED_GREEN 11

#define BUTTON_A 5
#define BUTTON_B 6
#define BUTTON_JOY 22

#define JOYSTICK_X_PIN 26 // GPIO para eixo X
#define JOYSTICK_Y_PIN 27 // GPIO para eixo Y
#define ADC_CHANNEL_O 0
#define ADC_CHANNEL_1 1

#define DEBOUNCE_DELAY 500

static volatile uint32_t last_interrupt_time = 0; // Variavel que salva ultima interrupção

// Variaveis de estado do botão
bool isButtonA = false;
bool isButtonB = false;
bool isButtonJoy = false;

// Variaveis de controle PWM
const float PWM_DIVISER = 29.0;
const uint16_t WRAP_PERIOD = 2200;
bool leds_active = true;
uint led_red_slice;
uint led_blue_slice;

// Variaveis de controle Joystick
uint16_t joystick_value_x;
uint16_t joystick_value_y;

/**
 * Função para inicializar PWM nos LEDS
 */
uint init_gpio_PWM(uint newPwm, float diviser, uint16_t period, uint16_t level)
{
  gpio_set_function(newPwm, GPIO_FUNC_PWM);   // habilitar o pino GPIO como PWM
  uint slice = pwm_gpio_to_slice_num(newPwm); // obter o canal PWM da GPIO

  // Clock w wrap inportante para definir a frequencia de 50hz (20000 microsegundos)
  pwm_set_clkdiv(slice, diviser); // define o divisor de clock do PWM
  pwm_set_wrap(slice, period);    // definir o valor de wrap

  // Define ciclo de trabalho inicial e ativa o programa
  pwm_set_gpio_level(newPwm, level); // definir o cico de trabalho (duty cycle) do pwm
  pwm_set_enabled(slice, true);      // habilita o pwm no slice correspondente

  return slice;
}

/**
 * Função para os valores do eixos do joystick
 */
void joystick_read_axis(uint16_t *js_val_x, uint16_t *js_val_y)
{
  // Leitura do eixo X do joystick
  adc_select_input(1);
  sleep_us(2);
  *js_val_x = adc_read();

  // Leitura do eixo Y do joystick
  adc_select_input(0);
  sleep_us(2);
  *js_val_y = adc_read();

  // Estabiliza o valor do meio
  if (*js_val_y <= 1890 && *js_val_y >= 1880)
  {
    *js_val_y = 1886;
  }
  else if (*js_val_x <= 2190 && *js_val_x >= 2180)
  {
    *js_val_x = 2186;
  }
}

/**
 * Move o botão no display;
 */
void moveButton()
{
  uint8_t x = (joystick_value_x * 127) / 4096;
  uint8_t y = 63 - ((joystick_value_y * 63) / 4096);

  // Confere se esta no meio
  if (joystick_value_x == 2186)
  {
    x = 63;
  }
  if (joystick_value_y == 1886)
  {
    y = 31;
  }

  // confere se supera limite inferior
  x = (x > 3) ? x : 3;
  y = (y > 3) ? y : 3;

  // confere se supera limite superior
  x = (x < 122) ? x : 122;
  y = (y < 58) ? y : 58;

  resetDisplay();
  setDisplay("0", x, y);
}

/**
 * Muda o valor PWM dos leds caso joystick for movido
 */
void doAxisLed()
{
  // Cria as variaeveis de Level
  uint16_t level_red = 0;
  uint16_t level_blue = 0;

  // EIXO X MOVE MUDA LED VERMELHO
  if (joystick_value_x < 2186)
  {
    level_red = 2186 - joystick_value_x;
  }
  else if (joystick_value_x > 2186)
  {
    level_red = joystick_value_x - 2186;
  }
  pwm_set_gpio_level(LED_RED, level_red);

  // EIXO Y MOVE MUDA LED AZUL
  if (joystick_value_y < 1886)
  {
    level_blue = 1886 - joystick_value_y;
  }
  else if (joystick_value_y > 1886)
  {
    level_blue = joystick_value_y - 1886;
  }
  pwm_set_gpio_level(LED_BLUE, level_blue);
}

/**
 * Função que realiza função do BOTÃO A
 */
void dobuttonA()
{
  if (leds_active)
  {
    pwm_set_enabled(led_red_slice, false);
    pwm_set_enabled(led_blue_slice, false);
    leds_active = false;
  }
  else
  {
    pwm_set_enabled(led_red_slice, true);
    pwm_set_enabled(led_blue_slice, true);
    leds_active = true;
  }

  isButtonA = false;
}

/**
 * Função que realiza função do BOTÃO B
 */
void dobuttonB()
{
  //  code
  isButtonB = false;
}

/**
 * Função que realiza função do BOTÃO do Joystick
 */
void dobuttonJOY()
{
  gpio_put(LED_GREEN, !gpio_get(LED_GREEN)); // Alternar o estado do LED Verde a cada acionamento.
  change_ret();                              // Modifica a borda do display para indicar quando foi pressionado
  isButtonJoy = false;
}

/**
 * Função que organiza os callbacks do sistema
 */
void gpio_callback(uint gpio, uint32_t events)
{
  // static uint32_t last_interrupt_time = 0;
  uint32_t current_time = to_ms_since_boot(get_absolute_time()); // CONTROLE DE DEBOUNCING NA MAIN ???

  // Verifica se o tempo de debouncing passou
  if (current_time - last_interrupt_time > DEBOUNCE_DELAY)
  {
    if (gpio == BUTTON_A)
    {
      isButtonA = true;
    }
    if (gpio == BUTTON_B)
    {
      isButtonB = true;
    }
    if (gpio == BUTTON_JOY)
    {
      isButtonJoy = true;
    }
    // Atualiza o tempo da última interrupção
    last_interrupt_time = current_time;
  }
}

/**
 * Função que inicializa o sistema
 */
void setup()
{
  // Inicia os LEDS com pwm
  led_red_slice = init_gpio_PWM(LED_RED, PWM_DIVISER, WRAP_PERIOD, 0);
  led_blue_slice = init_gpio_PWM(LED_BLUE, PWM_DIVISER, WRAP_PERIOD, 0);

  // Inicia os LEDS normalmente
  gpio_init(LED_GREEN);
  gpio_set_dir(LED_GREEN, GPIO_OUT);
  gpio_put(LED_GREEN, false);

  // Inicia os Botões
  gpio_init(BUTTON_A);
  gpio_set_dir(BUTTON_A, GPIO_IN);
  gpio_pull_up(BUTTON_A);
  gpio_init(BUTTON_B);
  gpio_set_dir(BUTTON_B, GPIO_IN);
  gpio_pull_up(BUTTON_B);
  gpio_init(BUTTON_JOY);
  gpio_set_dir(BUTTON_JOY, GPIO_IN);
  gpio_pull_up(BUTTON_JOY);

  // Configuração da interrupção com callback
  gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
  gpio_set_irq_enabled(BUTTON_B, GPIO_IRQ_EDGE_FALL, true);
  gpio_set_irq_enabled(BUTTON_JOY, GPIO_IRQ_EDGE_FALL, true);

  // Inicia JOYSTICK
  adc_init();
  adc_gpio_init(JOYSTICK_X_PIN);
  adc_gpio_init(JOYSTICK_Y_PIN);

  // Incializa o display I2C
  display_init();
}

/**
 * Função principal do sistema
 */
int main()
{
  // Inicializa sistema
  stdio_init_all();
  setup();

  // Programa Inciado!
  printf("Programa Inciado: Joystick PWM\n");

  // Loop Principal do programa
  while (true)
  {
    if (isButtonA) // Detecta mudança de botão A e processa
    {
      dobuttonA();
    }
    else if (isButtonJoy) // Detecta mudança JOYSTICK BUTTON
    {
      dobuttonJOY();
    }
    else
    {
      joystick_read_axis(&joystick_value_x, &joystick_value_y); // LER VALORES JOYSTICK
      doAxisLed();                                              // Muda força LED de acordo com Joystick
      moveButton();                                             // MUDAR POSIÇÃO NO DISPLAY DE ACORDO COM VALOR JOYSTICK (bordas)
    }

    // Reduz processamento
    sleep_ms(50);
  }
}
