#pragma once
#include "esp_partition.h"
#include "esp_heap_caps.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "debug_port.h"

class MarineRuntimeOverlay {
public:
    static void printPartitionInfo() {
    DEBUG_PORT.println("=== Partition Info ===");

    const esp_partition_t* running = esp_ota_get_running_partition();
    if (running) {
        DEBUG_PORT.printf("Running partition address: 0x%X\n", running->address);
        DEBUG_PORT.printf("Running partition size: %u bytes\n", running->size);
    }

    const esp_partition_t* spiffs_part = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, "spiffs");

    if (spiffs_part) {
        DEBUG_PORT.printf("SPIFFS partition @ 0x%X, size: %u bytes\n",
                          spiffs_part->address, spiffs_part->size);
    } else {
        DEBUG_PORT.println("SPIFFS partition not found");
    }

    DEBUG_PORT.println("=== PSRAM Status ===");
    DEBUG_PORT.printf("Free PSRAM: %u\n", ESP.getFreePsram());
    DEBUG_PORT.printf("Largest allocatable block: %u\n", heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));

    DEBUG_PORT.println("=== Internal Heap Status ===");
    DEBUG_PORT.printf("Free heap: %u\n", ESP.getFreeHeap());
    DEBUG_PORT.printf("Minimum free heap ever: %u\n", ESP.getMinFreeHeap());
}

    static void printPSRAMStatus() {
        DEBUG_PORT.println("=== PSRAM Status ===");
        DEBUG_PORT.print("Free PSRAM: ");
        DEBUG_PORT.println(heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
        DEBUG_PORT.print("Largest allocatable block: ");
        DEBUG_PORT.println(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
    }

    static void printHeapStatus() {
        DEBUG_PORT.println("=== Internal Heap Status ===");
        DEBUG_PORT.print("Free heap: ");
        DEBUG_PORT.println(esp_get_free_heap_size());
        DEBUG_PORT.print("Minimum free heap ever: ");
        DEBUG_PORT.println(esp_get_minimum_free_heap_size());
    }

    static void checkBufferLocation(void* ptr, const char* label) {
        DEBUG_PORT.print(label);
        DEBUG_PORT.print(" @ 0x");
        DEBUG_PORT.print((uintptr_t)ptr, HEX);
        DEBUG_PORT.print(" | In PSRAM: ");
        DEBUG_PORT.println(esp_ptr_external_ram(ptr) ? "Yes" : "No");
    }

    static void runOverlay(void* buffer0, void* buffer1) {
        DEBUG_PORT.println("\n=== Marine Runtime Overlay ===");
        printPartitionInfo();
        printPSRAMStatus();
        printHeapStatus();
        checkBufferLocation(buffer0, "Screen Buffer [0]");
        checkBufferLocation(buffer1, "Screen Buffer [1]");
        DEBUG_PORT.println("==============================\n");
    }
};