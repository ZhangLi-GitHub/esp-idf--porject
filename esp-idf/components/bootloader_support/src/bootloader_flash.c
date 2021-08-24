/*
 * SPDX-FileCopyrightText: 2015-2021 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stddef.h>

#include <bootloader_flash_priv.h>
#include <esp_log.h>
#include <esp_flash_encrypt.h>
#include "sdkconfig.h"
#include "soc/soc_caps.h"

#if CONFIG_IDF_TARGET_ESP32
#   include "soc/spi_struct.h"
#   include "soc/spi_reg.h"
    /* SPI flash controller */
#   define SPIFLASH SPI1
#else
#   include "soc/spi_mem_struct.h"
#   include "soc/spi_mem_reg.h"
    /* SPI flash controller */
#   define SPIFLASH SPIMEM1
#endif

#if CONFIG_IDF_TARGET_ESP32
#include "esp32/rom/spi_flash.h"
#elif CONFIG_IDF_TARGET_ESP32S2
#include "esp32s2/rom/spi_flash.h"
#elif CONFIG_IDF_TARGET_ESP32S3
#include "esp32s3/rom/spi_flash.h"
#elif CONFIG_IDF_TARGET_ESP32C3
#include "esp32c3/rom/spi_flash.h"
#elif CONFIG_IDF_TARGET_ESP32H2
#include "esp32h2/rom/spi_flash.h"
#endif

#ifdef CONFIG_EFUSE_VIRTUAL_KEEP_IN_FLASH
#define ENCRYPTION_IS_VIRTUAL 1
#else
#define ENCRYPTION_IS_VIRTUAL 0
#endif

#define BYTESHIFT(VAR, IDX)    (((VAR) >> ((IDX) * 8)) & 0xFF)
#define ISSI_ID                0x9D
#define GD_Q_ID_HIGH           0xC8
#define GD_Q_ID_MID            0x40
#define GD_Q_ID_LOW            0x16

#define ESP_BOOTLOADER_SPIFLASH_BP_MASK_ISSI    (BIT7 | BIT5 | BIT4 | BIT3 | BIT2)
#define ESP_BOOTLOADER_SPIFLASH_QE_16B           BIT9   // QE position when you write 16 bits at one time.
#define ESP_BOOTLOADER_SPIFLASH_QE_8B            BIT1   // QE position when you write 8 bits(for SR2) at one time.
#define ESP_BOOTLOADER_SPIFLASH_WRITE_8B         (8)
#define ESP_BOOTLOADER_SPIFLASH_WRITE_16B        (16)

#ifndef BOOTLOADER_BUILD
/* Normal app version maps to esp_spi_flash.h operations...
 */
static const char *TAG = "bootloader_mmap";

static spi_flash_mmap_handle_t map;

uint32_t bootloader_mmap_get_free_pages(void)
{
    return spi_flash_mmap_get_free_pages(SPI_FLASH_MMAP_DATA);
}

const void *bootloader_mmap(uint32_t src_addr, uint32_t size)
{
    if (map) {
        ESP_LOGE(TAG, "tried to bootloader_mmap twice");
        return NULL; /* existing mapping in use... */
    }
    const void *result = NULL;
    uint32_t src_page = src_addr & ~(SPI_FLASH_MMU_PAGE_SIZE - 1);
    size += (src_addr - src_page);
    esp_err_t err = spi_flash_mmap(src_page, size, SPI_FLASH_MMAP_DATA, &result, &map);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "spi_flash_mmap failed: 0x%x", err);
        return NULL;
    }
    return (void *)((intptr_t)result + (src_addr - src_page));
}

void bootloader_munmap(const void *mapping)
{
    if (mapping && map) {
        spi_flash_munmap(map);
    }
    map = 0;
}

esp_err_t bootloader_flash_read(size_t src, void *dest, size_t size, bool allow_decrypt)
{
    if (allow_decrypt && esp_flash_encryption_enabled()) {
        return spi_flash_read_encrypted(src, dest, size);
    } else {
        return spi_flash_read(src, dest, size);
    }
}

esp_err_t bootloader_flash_write(size_t dest_addr, void *src, size_t size, bool write_encrypted)
{
    if (write_encrypted && !ENCRYPTION_IS_VIRTUAL) {
#if CONFIG_IDF_TARGET_ESP32
        return spi_flash_write_encrypted(dest_addr, src, size);
#else
        return esp_rom_spiflash_write_encrypted(dest_addr, src, size);
#endif
    } else {
        return spi_flash_write(dest_addr, src, size);
    }
}

esp_err_t bootloader_flash_erase_sector(size_t sector)
{
    return spi_flash_erase_sector(sector);
}

esp_err_t bootloader_flash_erase_range(uint32_t start_addr, uint32_t size)
{
    return spi_flash_erase_range(start_addr, size);
}

#else
/* Bootloader version, uses ROM functions only */
#if CONFIG_IDF_TARGET_ESP32
#include "esp32/rom/spi_flash.h"
#include "esp32/rom/cache.h"
#elif CONFIG_IDF_TARGET_ESP32S2
#include "esp32s2/rom/spi_flash.h"
#include "esp32s2/rom/cache.h"
#include "soc/cache_memory.h"
#elif CONFIG_IDF_TARGET_ESP32S3
#include "esp32s3/rom/spi_flash.h"
#include "esp32s3/rom/cache.h"
#include "soc/cache_memory.h"
#elif CONFIG_IDF_TARGET_ESP32C3
#include "esp32c3/rom/spi_flash.h"
#include "esp32c3/rom/cache.h"
#include "soc/cache_memory.h"
#elif CONFIG_IDF_TARGET_ESP32H2
#include "esp32h2/rom/spi_flash.h"
#include "esp32h2/rom/cache.h"
#include "soc/cache_memory.h"
#endif
static const char *TAG = "bootloader_flash";

#if CONFIG_IDF_TARGET_ESP32
/* Use first 50 blocks in MMU for bootloader_mmap,
   50th block for bootloader_flash_read
*/
#define MMU_BLOCK0_VADDR  SOC_DROM_LOW
#define MMU_SIZE          (0x320000)
#define MMU_BLOCK50_VADDR (MMU_BLOCK0_VADDR + MMU_SIZE)
#define FLASH_READ_VADDR MMU_BLOCK50_VADDR

#else // !CONFIG_IDF_TARGET_ESP32

/* Use first 63 blocks in MMU for bootloader_mmap,
   63th block for bootloader_flash_read
*/
#define MMU_BLOCK0_VADDR  SOC_DROM_LOW
#define MMU_SIZE          (0x3f0000)
#define MMU_BLOCK63_VADDR (MMU_BLOCK0_VADDR + MMU_SIZE)
#define FLASH_READ_VADDR MMU_BLOCK63_VADDR
#endif

#define MMU_FREE_PAGES    (MMU_SIZE / FLASH_BLOCK_SIZE)

static bool mapped;

// Current bootloader mapping (ab)used for bootloader_read()
static uint32_t current_read_mapping = UINT32_MAX;

uint32_t bootloader_mmap_get_free_pages(void)
{
    /**
     * Allow mapping up to 50 of the 51 available MMU blocks (last one used for reads)
     * Since, bootloader_mmap function below assumes it to be 0x320000 (50 pages), we can safely do this.
     */
    return MMU_FREE_PAGES;
}

const void *bootloader_mmap(uint32_t src_addr, uint32_t size)
{
    if (mapped) {
        ESP_LOGE(TAG, "tried to bootloader_mmap twice");
        return NULL; /* can't map twice */
    }
    if (size > MMU_SIZE) {
        ESP_LOGE(TAG, "bootloader_mmap excess size %x", size);
        return NULL;
    }

    uint32_t src_addr_aligned = src_addr & MMU_FLASH_MASK;
    uint32_t count = bootloader_cache_pages_to_map(size, src_addr);
#if CONFIG_IDF_TARGET_ESP32
    Cache_Read_Disable(0);
    Cache_Flush(0);
#elif CONFIG_IDF_TARGET_ESP32S2
    uint32_t autoload = Cache_Suspend_ICache();
    Cache_Invalidate_ICache_All();
#elif CONFIG_IDF_TARGET_ESP32S3
    uint32_t autoload = Cache_Suspend_DCache();
    Cache_Invalidate_DCache_All();
#elif CONFIG_IDF_TARGET_ESP32C3
    uint32_t autoload = Cache_Suspend_ICache();
    Cache_Invalidate_ICache_All();
#elif CONFIG_IDF_TARGET_ESP32H2
    uint32_t autoload = Cache_Suspend_ICache();
    Cache_Invalidate_ICache_All();
#endif
    ESP_LOGD(TAG, "mmu set paddr=%08x count=%d size=%x src_addr=%x src_addr_aligned=%x",
             src_addr & MMU_FLASH_MASK, count, size, src_addr, src_addr_aligned );
#if CONFIG_IDF_TARGET_ESP32
    int e = cache_flash_mmu_set(0, 0, MMU_BLOCK0_VADDR, src_addr_aligned, 64, count);
#elif CONFIG_IDF_TARGET_ESP32S2
    int e = Cache_Ibus_MMU_Set(MMU_ACCESS_FLASH, MMU_BLOCK0_VADDR, src_addr_aligned, 64, count, 0);
#else // S3, C3, H2
    int e = Cache_Dbus_MMU_Set(MMU_ACCESS_FLASH, MMU_BLOCK0_VADDR, src_addr_aligned, 64, count, 0);
#endif
    if (e != 0) {
        ESP_LOGE(TAG, "cache_flash_mmu_set failed: %d\n", e);
#if CONFIG_IDF_TARGET_ESP32
        Cache_Read_Enable(0);
#elif CONFIG_IDF_TARGET_ESP32S2
        Cache_Resume_ICache(autoload);
#elif CONFIG_IDF_TARGET_ESP32S3
        Cache_Resume_DCache(autoload);
#elif CONFIG_IDF_TARGET_ESP32C3
        Cache_Resume_ICache(autoload);
#elif CONFIG_IDF_TARGET_ESP32H2
        Cache_Resume_ICache(autoload);
#endif
        return NULL;
    }
#if CONFIG_IDF_TARGET_ESP32
    Cache_Read_Enable(0);
#elif CONFIG_IDF_TARGET_ESP32S2
    Cache_Resume_ICache(autoload);
#elif CONFIG_IDF_TARGET_ESP32S3
    Cache_Resume_DCache(autoload);
#elif CONFIG_IDF_TARGET_ESP32C3
    Cache_Resume_ICache(autoload);
#elif CONFIG_IDF_TARGET_ESP32H2
    Cache_Resume_ICache(autoload);
#endif

    mapped = true;

    return (void *)(MMU_BLOCK0_VADDR + (src_addr - src_addr_aligned));
}

void bootloader_munmap(const void *mapping)
{
    if (mapped)  {
#if CONFIG_IDF_TARGET_ESP32
        /* Full MMU reset */
        Cache_Read_Disable(0);
        Cache_Flush(0);
        mmu_init(0);
#elif CONFIG_IDF_TARGET_ESP32S2
        //TODO, save the autoload value.
        Cache_Suspend_ICache();
        Cache_Invalidate_ICache_All();
        Cache_MMU_Init();
#elif CONFIG_IDF_TARGET_ESP32S3
        Cache_Suspend_DCache();
        Cache_Invalidate_DCache_All();
        Cache_MMU_Init();
#elif CONFIG_IDF_TARGET_ESP32C3
        //TODO, save the autoload value.
        Cache_Suspend_ICache();
        Cache_Invalidate_ICache_All();
        Cache_MMU_Init();
#elif CONFIG_IDF_TARGET_ESP32H2
        Cache_Suspend_ICache();
        Cache_Invalidate_ICache_All();
        Cache_MMU_Init();
#endif
        mapped = false;
        current_read_mapping = UINT32_MAX;
    }
}

static esp_err_t spi_to_esp_err(esp_rom_spiflash_result_t r)
{
    switch (r) {
    case ESP_ROM_SPIFLASH_RESULT_OK:
        return ESP_OK;
    case ESP_ROM_SPIFLASH_RESULT_ERR:
        return ESP_ERR_FLASH_OP_FAIL;
    case ESP_ROM_SPIFLASH_RESULT_TIMEOUT:
        return ESP_ERR_FLASH_OP_TIMEOUT;
    default:
        return ESP_FAIL;
    }
}

static esp_err_t bootloader_flash_read_no_decrypt(size_t src_addr, void *dest, size_t size)
{
#if CONFIG_IDF_TARGET_ESP32
    Cache_Read_Disable(0);
    Cache_Flush(0);
#elif CONFIG_IDF_TARGET_ESP32S2
    uint32_t autoload = Cache_Suspend_ICache();
#elif CONFIG_IDF_TARGET_ESP32S3
    uint32_t autoload = Cache_Suspend_DCache();
#elif CONFIG_IDF_TARGET_ESP32C3
    uint32_t autoload = Cache_Suspend_ICache();
#elif CONFIG_IDF_TARGET_ESP32H2
    uint32_t autoload = Cache_Suspend_ICache();
#endif
    esp_rom_spiflash_result_t r = esp_rom_spiflash_read(src_addr, dest, size);
#if CONFIG_IDF_TARGET_ESP32
    Cache_Read_Enable(0);
#elif CONFIG_IDF_TARGET_ESP32S2
    Cache_Resume_ICache(autoload);
#elif CONFIG_IDF_TARGET_ESP32S3
    Cache_Resume_DCache(autoload);
#elif CONFIG_IDF_TARGET_ESP32C3
    Cache_Resume_ICache(autoload);
#elif CONFIG_IDF_TARGET_ESP32H2
    Cache_Resume_ICache(autoload);
#endif

    return spi_to_esp_err(r);
}

static esp_err_t bootloader_flash_read_allow_decrypt(size_t src_addr, void *dest, size_t size)
{
    uint32_t *dest_words = (uint32_t *)dest;

    for (size_t word = 0; word < size / 4; word++) {
        uint32_t word_src = src_addr + word * 4;  /* Read this offset from flash */
        uint32_t map_at = word_src & MMU_FLASH_MASK; /* Map this 64KB block from flash */
        uint32_t *map_ptr;
        if (map_at != current_read_mapping) {
            /* Move the 64KB mmu mapping window to fit map_at */
#if CONFIG_IDF_TARGET_ESP32
            Cache_Read_Disable(0);
            Cache_Flush(0);
#elif CONFIG_IDF_TARGET_ESP32S2
            uint32_t autoload = Cache_Suspend_ICache();
            Cache_Invalidate_ICache_All();
#elif CONFIG_IDF_TARGET_ESP32S3
            uint32_t autoload = Cache_Suspend_DCache();
            Cache_Invalidate_DCache_All();
#elif CONFIG_IDF_TARGET_ESP32C3
            uint32_t autoload = Cache_Suspend_ICache();
            Cache_Invalidate_ICache_All();
#elif CONFIG_IDF_TARGET_ESP32H2
            uint32_t autoload = Cache_Suspend_ICache();
            Cache_Invalidate_ICache_All();
#endif
            ESP_LOGD(TAG, "mmu set block paddr=0x%08x (was 0x%08x)", map_at, current_read_mapping);
#if CONFIG_IDF_TARGET_ESP32
            int e = cache_flash_mmu_set(0, 0, FLASH_READ_VADDR, map_at, 64, 1);
#elif CONFIG_IDF_TARGET_ESP32S2
            int e = Cache_Ibus_MMU_Set(MMU_ACCESS_FLASH, MMU_BLOCK63_VADDR, map_at, 64, 1, 0);
#elif CONFIG_IDF_TARGET_ESP32S3
            int e = Cache_Dbus_MMU_Set(MMU_ACCESS_FLASH, MMU_BLOCK63_VADDR, map_at, 64, 1, 0);
#elif CONFIG_IDF_TARGET_ESP32C3
            int e = Cache_Dbus_MMU_Set(MMU_ACCESS_FLASH, MMU_BLOCK63_VADDR, map_at, 64, 1, 0);
#elif CONFIG_IDF_TARGET_ESP32H2
            int e = Cache_Dbus_MMU_Set(MMU_ACCESS_FLASH, MMU_BLOCK63_VADDR, map_at, 64, 1, 0);
#endif
            if (e != 0) {
                ESP_LOGE(TAG, "cache_flash_mmu_set failed: %d\n", e);
#if CONFIG_IDF_TARGET_ESP32
                Cache_Read_Enable(0);
#elif CONFIG_IDF_TARGET_ESP32S2
                Cache_Resume_ICache(autoload);
#elif CONFIG_IDF_TARGET_ESP32S3
                Cache_Resume_DCache(autoload);
#elif CONFIG_IDF_TARGET_ESP32C3
                Cache_Resume_ICache(autoload);
#elif CONFIG_IDF_TARGET_ESP32H2
                Cache_Resume_ICache(autoload);
#endif
                return ESP_FAIL;
            }
            current_read_mapping = map_at;
#if CONFIG_IDF_TARGET_ESP32
            Cache_Read_Enable(0);
#elif CONFIG_IDF_TARGET_ESP32S2
            Cache_Resume_ICache(autoload);
#elif CONFIG_IDF_TARGET_ESP32S3
            Cache_Resume_DCache(autoload);
#elif CONFIG_IDF_TARGET_ESP32C3
            Cache_Resume_ICache(autoload);
#elif CONFIG_IDF_TARGET_ESP32H2
            Cache_Resume_ICache(autoload);
#endif
        }
        map_ptr = (uint32_t *)(FLASH_READ_VADDR + (word_src - map_at));
        dest_words[word] = *map_ptr;
    }
    return ESP_OK;
}

esp_err_t bootloader_flash_read(size_t src_addr, void *dest, size_t size, bool allow_decrypt)
{
    if (src_addr & 3) {
        ESP_LOGE(TAG, "bootloader_flash_read src_addr 0x%x not 4-byte aligned", src_addr);
        return ESP_FAIL;
    }
    if (size & 3) {
        ESP_LOGE(TAG, "bootloader_flash_read size 0x%x not 4-byte aligned", size);
        return ESP_FAIL;
    }
    if ((intptr_t)dest & 3) {
        ESP_LOGE(TAG, "bootloader_flash_read dest 0x%x not 4-byte aligned", (intptr_t)dest);
        return ESP_FAIL;
    }

    if (allow_decrypt) {
        return bootloader_flash_read_allow_decrypt(src_addr, dest, size);
    } else {
        return bootloader_flash_read_no_decrypt(src_addr, dest, size);
    }
}

esp_err_t bootloader_flash_write(size_t dest_addr, void *src, size_t size, bool write_encrypted)
{
    esp_err_t err;
    size_t alignment = write_encrypted ? 32 : 4;
    if ((dest_addr % alignment) != 0) {
        ESP_LOGE(TAG, "bootloader_flash_write dest_addr 0x%x not %d-byte aligned", dest_addr, alignment);
        return ESP_FAIL;
    }
    if ((size % alignment) != 0) {
        ESP_LOGE(TAG, "bootloader_flash_write size 0x%x not %d-byte aligned", size, alignment);
        return ESP_FAIL;
    }
    if (((intptr_t)src % 4) != 0) {
        ESP_LOGE(TAG, "bootloader_flash_write src 0x%x not 4 byte aligned", (intptr_t)src);
        return ESP_FAIL;
    }

    err = bootloader_flash_unlock();
    if (err != ESP_OK) {
        return err;
    }

    if (write_encrypted && !ENCRYPTION_IS_VIRTUAL) {
        return spi_to_esp_err(esp_rom_spiflash_write_encrypted(dest_addr, src, size));
    } else {
        return spi_to_esp_err(esp_rom_spiflash_write(dest_addr, src, size));
    }
}

esp_err_t bootloader_flash_erase_sector(size_t sector)
{
    return spi_to_esp_err(esp_rom_spiflash_erase_sector(sector));
}

esp_err_t bootloader_flash_erase_range(uint32_t start_addr, uint32_t size)
{
    if (start_addr % FLASH_SECTOR_SIZE != 0) {
        return ESP_ERR_INVALID_ARG;
    }
    if (size % FLASH_SECTOR_SIZE != 0) {
        return ESP_ERR_INVALID_SIZE;
    }
    size_t start = start_addr / FLASH_SECTOR_SIZE;
    size_t end = start + size / FLASH_SECTOR_SIZE;
    const size_t sectors_per_block = FLASH_BLOCK_SIZE / FLASH_SECTOR_SIZE;

    esp_rom_spiflash_result_t rc = ESP_ROM_SPIFLASH_RESULT_OK;
    for (size_t sector = start; sector != end && rc == ESP_ROM_SPIFLASH_RESULT_OK; ) {
        if (sector % sectors_per_block == 0 && end - sector >= sectors_per_block) {
            rc = esp_rom_spiflash_erase_block(sector / sectors_per_block);
            sector += sectors_per_block;
        } else {
            rc = esp_rom_spiflash_erase_sector(sector);
            ++sector;
        }
    }
    return spi_to_esp_err(rc);
}

#endif

FORCE_INLINE_ATTR bool is_issi_chip(const esp_rom_spiflash_chip_t* chip)
{
    return BYTESHIFT(chip->device_id, 2) == ISSI_ID;
}

// For GD25Q32, GD25Q64, GD25Q127C, GD25Q128, which use single command to read/write different SR.
FORCE_INLINE_ATTR bool is_gd_q_chip(const esp_rom_spiflash_chip_t* chip)
{
    return BYTESHIFT(chip->device_id, 2) == GD_Q_ID_HIGH && BYTESHIFT(chip->device_id, 1) == GD_Q_ID_MID && BYTESHIFT(chip->device_id, 0) >= GD_Q_ID_LOW;
}

esp_err_t IRAM_ATTR __attribute__((weak)) bootloader_flash_unlock(void)
{
    uint16_t status = 0;    // status for SR1 or SR1+SR2 if writing SR with 01H + 2Bytes.
    uint16_t new_status = 0;
    uint8_t status_sr2 = 0;    // status_sr2 for SR2.
    uint8_t new_status_sr2 = 0;
    uint8_t write_sr_bit = 0;
    esp_err_t err = ESP_OK;

    esp_rom_spiflash_wait_idle(&g_rom_flashchip);
    if (is_issi_chip(&g_rom_flashchip)) {
        write_sr_bit = ESP_BOOTLOADER_SPIFLASH_WRITE_8B;
        // ISSI chips have different QE position

        status = bootloader_execute_flash_command(CMD_RDSR, 0, 0, 8);

        /* Clear all bits in the mask.
        (This is different from ROM esp_rom_spiflash_unlock, which keeps all bits as-is.)
        */
        new_status = status & (~ESP_BOOTLOADER_SPIFLASH_BP_MASK_ISSI);
        // Skip if nothing needs to be cleared. Otherwise will waste time waiting for the flash to clear nothing.
    } else if (is_gd_q_chip(&g_rom_flashchip)) {
        /* The GD chips behaviour is to clear all bits in SR1 and clear bits in SR2 except QE bit.
           Use 01H to write SR1 and 31H to write SR2.
        */
        write_sr_bit = ESP_BOOTLOADER_SPIFLASH_WRITE_8B;

        status = bootloader_execute_flash_command(CMD_RDSR, 0, 0, 8);
        new_status = 0;

        status_sr2 = bootloader_execute_flash_command(CMD_RDSR2, 0, 0, 8);
        new_status_sr2 = status_sr2 & ESP_BOOTLOADER_SPIFLASH_QE_8B;
    } else {
        /* For common behaviour, like XMC chips, Use 01H+2Bytes to write both SR1 and SR2*/
        write_sr_bit = ESP_BOOTLOADER_SPIFLASH_WRITE_16B;
        status = bootloader_execute_flash_command(CMD_RDSR, 0, 0, 8) | (bootloader_execute_flash_command(CMD_RDSR2, 0, 0, 8) << 8);

        /* Clear all bits except QE, if it is set.
        (This is different from ROM esp_rom_spiflash_unlock, which keeps all bits as-is.)
        */
        new_status = status & ESP_BOOTLOADER_SPIFLASH_QE_16B;
    }

    if (status != new_status) {
        /* if the status in SR not equal to the ideal status, the status need to be updated */
        esp_rom_spiflash_wait_idle(&g_rom_flashchip);
        bootloader_execute_flash_command(CMD_WREN, 0, 0, 0);
        esp_rom_spiflash_wait_idle(&g_rom_flashchip);
        bootloader_execute_flash_command(CMD_WRSR, new_status, write_sr_bit, 0);
        esp_rom_spiflash_wait_idle(&g_rom_flashchip);
    }

    if (status_sr2 != new_status_sr2) {
        /* If the status in SR2 not equal to the ideal status, the status need to be updated.
           It doesn't need to be updated if status in SR2 is 0.
           Note: if we need to update both SR1 and SR2, the `CMD_WREN` needs to be sent again.
        */
        esp_rom_spiflash_wait_idle(&g_rom_flashchip);
        bootloader_execute_flash_command(CMD_WREN, 0, 0, 0);
        esp_rom_spiflash_wait_idle(&g_rom_flashchip);
        bootloader_execute_flash_command(CMD_WRSR2, new_status_sr2, write_sr_bit, 0);
        esp_rom_spiflash_wait_idle(&g_rom_flashchip);
    }

    bootloader_execute_flash_command(CMD_WRDI, 0, 0, 0);
    esp_rom_spiflash_wait_idle(&g_rom_flashchip);
    return err;
}

#ifndef g_rom_spiflash_dummy_len_plus // ESP32-C3 uses a macro to access ROM data here
extern uint8_t g_rom_spiflash_dummy_len_plus[];
#endif
uint32_t IRAM_ATTR bootloader_execute_flash_command(uint8_t command, uint32_t mosi_data, uint8_t mosi_len, uint8_t miso_len)
{
    uint32_t old_ctrl_reg = SPIFLASH.ctrl.val;
#if CONFIG_IDF_TARGET_ESP32
    SPIFLASH.ctrl.val = SPI_WP_REG_M; // keep WP high while idle, otherwise leave DIO mode
#else
    SPIFLASH.ctrl.val = SPI_MEM_WP_REG_M; // keep WP high while idle, otherwise leave DIO mode
#endif
    SPIFLASH.user.usr_dummy = 0;
    SPIFLASH.user.usr_addr = 0;
    SPIFLASH.user.usr_command = 1;
    SPIFLASH.user2.usr_command_bitlen = 7;

    SPIFLASH.user2.usr_command_value = command;
    SPIFLASH.user.usr_miso = miso_len > 0;
#if CONFIG_IDF_TARGET_ESP32
    SPIFLASH.miso_dlen.usr_miso_dbitlen = miso_len ? (miso_len - 1) : 0;
#else
    SPIFLASH.miso_dlen.usr_miso_bit_len = miso_len ? (miso_len - 1) : 0;
#endif
    SPIFLASH.user.usr_mosi = mosi_len > 0;
#if CONFIG_IDF_TARGET_ESP32
    SPIFLASH.mosi_dlen.usr_mosi_dbitlen = mosi_len ? (mosi_len - 1) : 0;
#else
    SPIFLASH.mosi_dlen.usr_mosi_bit_len = mosi_len ? (mosi_len - 1) : 0;
#endif
    SPIFLASH.data_buf[0] = mosi_data;

    if (g_rom_spiflash_dummy_len_plus[1]) {
        /* When flash pins are mapped via GPIO matrix, need a dummy cycle before reading via MISO */
        if (miso_len > 0) {
            SPIFLASH.user.usr_dummy = 1;
            SPIFLASH.user1.usr_dummy_cyclelen = g_rom_spiflash_dummy_len_plus[1] - 1;
        } else {
            SPIFLASH.user.usr_dummy = 0;
            SPIFLASH.user1.usr_dummy_cyclelen = 0;
        }
    }

    SPIFLASH.cmd.usr = 1;
    while (SPIFLASH.cmd.usr != 0) {
    }

    SPIFLASH.ctrl.val = old_ctrl_reg;
    return SPIFLASH.data_buf[0];
}

void bootloader_enable_wp(void)
{
    bootloader_execute_flash_command(CMD_WRDI, 0, 0, 0);   /* Exit OTP mode */
}

#if SOC_CACHE_SUPPORT_WRAP
esp_err_t bootloader_flash_wrap_set(spi_flash_wrap_mode_t mode)
{
    uint32_t reg_bkp_ctrl = SPIFLASH.ctrl.val;
    uint32_t reg_bkp_usr  = SPIFLASH.user.val;
    SPIFLASH.user.fwrite_dio = 0;
    SPIFLASH.user.fwrite_dual = 0;
    SPIFLASH.user.fwrite_qio = 1;
    SPIFLASH.user.fwrite_quad = 0;
    SPIFLASH.ctrl.fcmd_dual = 0;
    SPIFLASH.ctrl.fcmd_quad = 0;
    SPIFLASH.user.usr_dummy = 0;
    SPIFLASH.user.usr_addr = 1;
    SPIFLASH.user.usr_command = 1;
    SPIFLASH.user2.usr_command_bitlen = 7;
    SPIFLASH.user2.usr_command_value = CMD_WRAP;
    SPIFLASH.user1.usr_addr_bitlen = 23;
    SPIFLASH.addr = 0;
    SPIFLASH.user.usr_miso = 0;
    SPIFLASH.user.usr_mosi = 1;
    SPIFLASH.mosi_dlen.usr_mosi_bit_len = 7;
    SPIFLASH.data_buf[0] = (uint32_t) mode << 4;;
    SPIFLASH.cmd.usr = 1;
    while(SPIFLASH.cmd.usr != 0)
    { }

    SPIFLASH.ctrl.val = reg_bkp_ctrl;
    SPIFLASH.user.val = reg_bkp_usr;
    return ESP_OK;
}
#endif //SOC_CACHE_SUPPORT_WRAP