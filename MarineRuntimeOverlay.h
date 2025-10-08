#pragma once
#include "esp_partition.h"
#include "esp_heap_caps.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "debug_port.h"

class MarineRuntimeOverlay {
public:
    static void printPartitionInfo() {
        const esp_partition_t* running = esp_ota_get_running_partition();
        DEBUG_PORT.println("=== Partition Info ===");
        DEBUG_PORT.print("Running partition label: ");
        DEBUG_PORT.println(running ? running->label : "Unknown");
        DEBUG_PORT.print("Address: 0x");
        DEBUG_PORT.println((uintptr_t)(running ? running->address : 0), HEX);
        DEBUG_PORT.print("Size: ");
        DEBUG_PORT.println(running ? running->size : 0);
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