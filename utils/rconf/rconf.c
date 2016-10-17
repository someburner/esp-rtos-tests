#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

/* Comment out if you don't have config checksums enabled in your project */
#define BOOT_CONFIG_CHKSUM

/* Total # of roms for your project */
#define ROM_COUNT 4
/* Index of the rom you want to boot after flashing this config */
#define ROM_TO_BOOT 0

/* Define the start addresses of each rom.                        *
 * NOTE: Add/remove as necessary. Be sure to update ROM_LOCATIONS *
 * appropriately or else the compiler will error                  */
#define ROM0_LOC 0x002000
#define ROM1_LOC 0x082000
#define ROM2_LOC 0x102000
#define ROM3_LOC 0x202000

#define ROM_LOCATIONS \
   ROM0_LOC, \
   ROM1_LOC, \
   ROM2_LOC, \
   ROM3_LOC

/* Name of the config binary to output */
#define CONF_BINNAME "rconf.bin"

/* ------------------- Most won't need to edit below here ------------------- */


#define N_ELEMENTS(X)           (sizeof(X)/sizeof(*(X)))

#if !(defined(ROM_TO_BOOT))
   #error "must define ROM_TO_BOOT!"
#endif
#if !(defined(ROM_COUNT))
   #error "must define ROM_COUNT!"
#endif

#if ROM_TO_BOOT >= 0 && ROM_TO_BOOT < ROM_COUNT
      #define ROM_NUM ROM_TO_BOOT
#else
      #error "Invalid rom num!"
#endif

uint32_t checkCount[] = { ROM_LOCATIONS };

/* rboot_config struct                                                        *
 * taken from: https://github.com/raburton/rboot/blob/master/rboot.h          *
 * NOTE: Relevant #defines from rboot.h in your ESP project must match those  *
 * here. (BOOT_CONFIG_CHKSUM, ROM_COUNT, etc.)                                */
typedef struct {
   uint8_t magic;           // our magic
   uint8_t version;         // config struct version
   uint8_t mode;            // boot loader mode
   uint8_t current_rom;     // currently selected rom
   uint8_t gpio_rom;        // rom to use for gpio boot
   uint8_t count;           // number of roms in use
   uint8_t last_main_rom;
   uint8_t last_secondary_rom;
   uint32_t roms[ROM_COUNT]; // flash addresses of the roms
#ifdef BOOT_CONFIG_CHKSUM
   uint8_t chksum;          // boot config chksum
#endif
} rboot_config;

rboot_config myConfig = {
   0xE1,       // magic - constant
   0x01,       // version - constant
   0x00,       // mode - standard (not gpio)
   ROM_NUM,    // current_rom (rom to boot)
   0,          // gpio_rom (unused)
   ROM_COUNT,  // count
   ROM_NUM,    // last_main_rom
   0,          // last_secondary_rom
   { ROM_LOCATIONS } // roms[0-N]
   #ifdef BOOT_CONFIG_CHKSUM
   ,0x00       // Checksum of this config, updated later
   #endif
};

#ifdef BOOT_CONFIG_CHKSUM
#define CHKSUM_INIT 0xef
static uint8_t calc_chksum(uint8_t *start, uint8_t *end) {
	uint8_t chksum = CHKSUM_INIT;
	while(start < end) {
		chksum ^= *start;
		start++;
	}
	return chksum;
}
#endif

int main() {
   int i;

   if (N_ELEMENTS(checkCount) != ROM_COUNT )
   {
      printf("error: Location list count != ROM_COUNT\n");
      return -1;
   }

   FILE *fp = fopen("rconf.bin", "w+");
   if (!fp)
   {
      printf("couldn't open file\n");
      return -1;
   }
#ifdef BOOT_CONFIG_CHKSUM
   uint8_t csum = calc_chksum((uint8_t*)&myConfig, (uint8_t*)&myConfig.chksum);
   printf("config checksum = %d\n", csum);
   myConfig.chksum = csum;
#endif

   size_t ret = fwrite(&myConfig, sizeof(uint8_t), sizeof(myConfig), fp);

   if (ret != sizeof(myConfig))
   {
      printf("fwrite error: returned %d\n", (int)ret);
   } else {
      printf("Successfull wrote %d bytes.\n\n",  (int)ret);
      printf("Rom Locations:\n");
      for (i=0; i<ROM_COUNT; i++)
      {
         printf("rom[%d] -> %08x\n", i, myConfig.roms[i]);
      }
      printf("\nDefault rom is %d\n", (int)myConfig.current_rom);
      printf("\nFlash %s to sector 0x001000\n", CONF_BINNAME);
   }

   fclose(fp);

   return 0;
}
