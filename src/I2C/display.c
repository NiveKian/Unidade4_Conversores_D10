#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "ssd1306.h"
#include "font.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define endereco 0x3C

ssd1306_t ssd; // Inicializa a estrutura do display
bool ret_fill = false;

void display_init()
{
  // I2C Initialisation. Using it at 400Khz.
  i2c_init(I2C_PORT, 400 * 1000);

  gpio_set_function(I2C_SDA, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
  gpio_set_function(I2C_SCL, GPIO_FUNC_I2C); // Set the GPIO pin function to I2C
  gpio_pull_up(I2C_SDA);                     // Pull up the data line
  gpio_pull_up(I2C_SCL);                     // Pull up the clock line

  ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT); // Inicializa o display
  ssd1306_config(&ssd);                                         // Configura o display
  ssd1306_send_data(&ssd);                                      // Envia os dados para o display

  // Limpa o display. O display inicia com todos os pixels apagados.
  ssd1306_fill(&ssd, false);
  ssd1306_send_data(&ssd);

  // Atualiza o conteúdo do display
  ssd1306_fill(&ssd, false);                      // Limpa o display
  ssd1306_rect(&ssd, 3, 3, 122, 58, true, false); // Desenha um retângulo

  // ssd1306_draw_string(&ssd, "EMBTEC CON U4D10", 8, 10); // Desenha uma string

  ssd1306_send_data(&ssd); // Atualiza o display
}

void resetDisplay() {
  ssd1306_fill(&ssd, !ret_fill);                      // Limpa o display
  ssd1306_rect(&ssd, 3, 3, 122, 58, ret_fill, !ret_fill); // Desenha um retângulo
  ssd1306_send_data(&ssd);              // Atualiza o display
}

void change_ret() {
  ret_fill = !ret_fill;
  resetDisplay();
}

void setDisplay(char *str, uint8_t x, uint8_t y)
{
  ssd1306_draw_string(&ssd, str, x, y); // Desenha uma string
  ssd1306_send_data(&ssd);              // Atualiza o display
}