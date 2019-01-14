#ifndef _FLASH_EXECUTE_H
#define _FLASH_EXECUTE_H

#include "usb_host.h"
#include "stm32f4xx_hal_flash.h"
//add by felix
#include "stm32f4xx_it.h"
#include "usbh_cdc.h"
#include "main.h"

#define ADDR_FLASH_SECTOR_0     ((uint32_t)0x08000000) /* Base @ of Sector 0, 16 Kbytes */
#define ADDR_FLASH_SECTOR_1     ((uint32_t)0x08004000) /* Base @ of Sector 1, 16 Kbytes */
#define ADDR_FLASH_SECTOR_2     ((uint32_t)0x08008000) /* Base @ of Sector 2, 16 Kbytes */
#define ADDR_FLASH_SECTOR_3     ((uint32_t)0x0800C000) /* Base @ of Sector 3, 16 Kbytes */
#define ADDR_FLASH_SECTOR_4     ((uint32_t)0x08010000) /* Base @ of Sector 4, 64 Kbytes */
#define ADDR_FLASH_SECTOR_5     ((uint32_t)0x08020000) /* Base @ of Sector 5, 128 Kbytes */
#define FLASH_USER_START_ADDR   ADDR_FLASH_SECTOR_5   /* Start @ of user Flash area */
#define FLASH_USER_END_ADDR     ADDR_FLASH_SECTOR_5  +  GetSectorSize(ADDR_FLASH_SECTOR_5) -1 /* End @ of user Flash area : sector start address + sector size -1 */




static uint32_t GetSector(uint32_t Address);
static uint32_t GetSectorSize(uint32_t Sector);

extern uint32_t FirstSector , NbOfSectors , Address ;
extern uint32_t SectorError;
extern __IO uint32_t data32;

void Erase_Flash(void);

#endif
