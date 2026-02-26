#ifndef _MTK_LED_H_
#define _MTK_LED_H_

#define LED_CTRL_REG       0x00
#define LED_STATUS_REG     0x04
#define LED_TOGGLE_REG     0x08
#define LED_INT_STATUS     0x0C
#define LED_INT_ENABLE     0x10

#define LED_ENABLE_BIT     BIT(0)
#define LED_INT_BIT        BIT(0)

#endif