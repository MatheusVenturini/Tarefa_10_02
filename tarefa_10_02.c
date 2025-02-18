#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/i2c.h"
#include "ssd1306.h"

#define I2C_PORT i2c1
#define I2C_SDA 14
#define I2C_SCL 15
#define OLED_ADDR 0x3C
#define JOYSTICK_X_PIN 26
#define JOYSTICK_Y_PIN 27
#define JOYSTICK_BTN 22
#define BUTTON_A 5
#define LED_R 13
#define LED_G 11
#define LED_B 12

ssd1306_t ssd;
bool led_state = false;
bool display_border = false;
bool leds_active = true;

// Valores centrais do joystick
int x_center = 3100;
int y_center = 2000;

// Função para controlar o LED verde
void toggle_led_green(uint gpio, uint32_t events) {
    static bool led_green_state = false;  // Estado do LED Verde
    led_green_state = !led_green_state;   // Alterna entre ligado e desligado
    gpio_put(LED_G, led_green_state);     // Atualiza o LED
}


// Função para controlar os LEDs
void toggle_leds(uint gpio, uint32_t events) {
    leds_active = !leds_active;
    if (!leds_active) {
        pwm_set_gpio_level(LED_R, 0);
        pwm_set_gpio_level(LED_B, 0);
    }
}

// Configuração do PWM
uint pwm_init_gpio(uint gpio) {
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    pwm_set_wrap(slice_num, 4095);
    pwm_set_enabled(slice_num, true);
    return slice_num;
}

int main() {
    stdio_init_all();
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);
    ssd1306_init(&ssd, 128, 64, false, OLED_ADDR, I2C_PORT);
    ssd1306_fill(&ssd, false);
    ssd1306_send_data(&ssd);

    adc_init();
    adc_gpio_init(JOYSTICK_X_PIN);
    adc_gpio_init(JOYSTICK_Y_PIN);

    gpio_init(JOYSTICK_BTN);
    gpio_set_dir(JOYSTICK_BTN, GPIO_IN);
    gpio_pull_up(JOYSTICK_BTN);
    gpio_set_irq_enabled_with_callback(JOYSTICK_BTN, GPIO_IRQ_EDGE_FALL| GPIO_IRQ_EDGE_RISE, true, &toggle_led_green);

    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &toggle_leds);

    gpio_init(LED_G);
    gpio_set_dir(LED_G, GPIO_OUT);
    gpio_put(LED_G, 0);  // Inicialmente desligado

    uint pwm_r = pwm_init_gpio(LED_R);
    uint pwm_b = pwm_init_gpio(LED_B);

    while (true) {

        adc_select_input(0);
        uint16_t y = adc_read();
        adc_select_input(1);
        uint16_t x = adc_read();

        // Ajuste dos LEDs: LED vermelho responde ao eixo X, LED azul ao eixo Y
        if (leds_active) {
            pwm_set_gpio_level(LED_R, abs(2048 - x) * 2);  // Vermelho responde ao X
            pwm_set_gpio_level(LED_B, abs(2048 - y) * 2);  // Azul responde ao Y
        }

        // Cálculo das coordenadas ajustadas do quadrado
        int square_y = ((y_center - y) / 32) + 32;  // Ajuste para centralizar e corrigir limites
        int square_x = 32- ((x_center - x) * 64 / 2048) + 64;  // Ajuste correto do eixo Y (invertido)

        // Limitação para manter dentro dos limites da tela
        if (square_x < 0) square_x = 4;
        if (square_x > 118) square_x = 116; // Expandimos o limite direito
        if (square_y < 2) square_y = 4;
        if (square_y > 56) square_y = 54;

        // Atualiza a tela OLED
        ssd1306_fill(&ssd, false);
        ssd1306_rect(&ssd, display_border ? 0 : 2, display_border ? 0 : 2, 126, 62, true, false);
        ssd1306_rect(&ssd, square_y, square_x, 8, 8, true, false);
        ssd1306_send_data(&ssd);

        ssd1306_fill(&ssd, false); // Limpa o display

        if (gpio_get(JOYSTICK_BTN) == 0) {  // Se o botão for pressionado
            gpio_put(LED_G, 1);  // Liga o LED verde
            ssd1306_rect(&ssd, 1, 1, 126, 62, true, false); // Segunda camada
            ssd1306_rect(&ssd, 2, 2, 124, 60, true, false); // Terceira camada
            ssd1306_send_data(&ssd);
            
        } else {
            gpio_put(LED_G, 0);  // Desliga o LED verde
        }

        sleep_ms(100);
    }
}
