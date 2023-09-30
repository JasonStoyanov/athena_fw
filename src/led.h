// Creator: Yasen Stoyanov
#ifndef LED_H_
#define LED_H_

int led_init(void);
int led_set(uint32_t val);
int led_toggle(void);
void led_toggle_thread(int unused1, int unused2, int unused3);


#endif /* LED_H_ */