// Copyright (C) 2023 Toitware ApS.
//
// Permission to use, copy, modify, and/or distribute this software for any
// purpose with or without fee is hereby granted.
//
// THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH
// REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
// FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT,
// INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
// LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
// OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
// PERFORMANCE OF THIS SOFTWARE.

#include <stdio.h>
#include <string.h>
#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include <esp_ota_ops.h>
#include <esp_spi_flash.h>
#include <esp_system.h>

#include "growPartition.h"
#include "new_partition.h"

esp_err_t GrowPartition::replace_partition_table() {
    if (PARTITION_TABLE_SIZE != TOTAL_PARTITION_LEN) {
        printf("Unexpected partition table size.\n");
        esp_ota_mark_app_invalid_rollback_and_reboot();
        return ESP_FAIL;
    }

    // Copy the partition table to DRAM. This is a requirement of spi_flash_write.
    uint8_t* new_partition_table = (uint8_t*)malloc(TOTAL_PARTITION_LEN);
    if (new_partition_table == NULL) {
        printf("Failed to allocate memory for partition table.\n");
        return ESP_FAIL;
    }

    memset(new_partition_table, 0xff, TOTAL_PARTITION_LEN);
    memcpy(new_partition_table, NEW_PARTITION, NEW_PARTITION_LEN);

    if (new_partition_table[0] != 0xAA || new_partition_table[1] != 0x50) {
        // No need to free the memory here, since we are rebooting.
        printf("Incorrect magic number: 0x%x%x\n", new_partition_table[0], new_partition_table[1]);
        esp_ota_mark_app_invalid_rollback_and_reboot();
        return ESP_FAIL;
    }

    esp_err_t err = ESP_FAIL;
    for (int attempts = 1; attempts <= RETRY_LIMIT; attempts++) {
        printf("Attempt %d of %d:\n", attempts, RETRY_LIMIT);
        printf("Erasing partition table...\n");
        err = spi_flash_erase_range(PARTITION_TABLE_ADDRESS, PARTITION_TABLE_ALIGNED_SIZE);
        if (err != ESP_OK) {
            printf("Failed to erase partition table: 0x%x\n", err);
            vTaskDelay(pdMS_TO_TICKS(RETRY_WAIT));
            continue;
        }

        err = spi_flash_write(PARTITION_TABLE_ADDRESS, new_partition_table, PARTITION_TABLE_ALIGNED_SIZE);
        if (err != ESP_OK) {
            printf("Failed to write partition table: 0x%x\n", err);
            vTaskDelay(pdMS_TO_TICKS(RETRY_WAIT));
            continue;
        }
        break;
    }
    free(new_partition_table);
    printf("Wrote new partition table.\n");
    return err;
}

bool GrowPartition::running_from_ota1() {
    const esp_partition_t* running_partition = esp_ota_get_running_partition();
    return  ;
}

bool GrowPartition::invalid_or_already_written() {
    // Check if the partition table has already been written.
    uint8_t buffer[256];
    for (int offset = 0; offset < NEW_PARTITION_LEN; offset += 256) {
        esp_err_t err = spi_flash_read(PARTITION_TABLE_ADDRESS + offset, buffer, 256);
        if (err != ESP_OK) {
            printf("Failed to read partition table: 0x%x\n", err);
            return false;
        }
        if (offset == 0 && (buffer[0] != 0xAA || buffer[1] != 0x50)) {
            printf("Invalid existing partition table. Maybe bad offset.\n");
            return false;
        }
        for (int i = 0; i < 256; i++) {
            if (buffer[i] != NEW_PARTITION[offset + i]) {
                printf("Partition table differs at %d\n", offset + i);
                return false;
            }
        }
    }
    return true;
}

bool GrowPartition::grow() {
    printf("Running from partition %s\n", esp_ota_get_running_partition()->label);
    if (invalid_or_already_written()) {
        printf("Partition is already grown.\n");
        return false;
    }
    if (!running_from_ota1()) {
        printf("Can't update: running partition needs to be ota_1.\n");
        return false;
    }

    // Update the partition table and restart to roll back to OTA_0.
    if (replace_partition_table() != ESP_OK) {
        printf("Failed to replace partition table.\n");
        return false;
    }
    printf("Rebooting to roll-back.\n");
    //esp_ota_mark_app_invalid_rollback_and_reboot ();
    return true; // Must reboot after this
    //esp_restart();
}
