#ifndef BUTTONS_H_
#define BUTTONS_H_

enum button_evt {
    BUTTON_EVT_PRESSED,
    BUTTON_EVT_RELEASED
};

typedef void (*button_event_handler_t)(enum button_evt evt);

int buttons_init(button_event_handler_t handler); 


#endif