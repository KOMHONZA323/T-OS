#ifndef MOUSE_EVENT_H
#define MOUSE_EVENT_H

#include <stdint.h>
#include <stdbool.h>

// Phase 2: Data Parsing & Protocol Decoding
// The raw bytes are parsed into this usable structure.
typedef struct {
    int32_t dx;          // Relative X movement
    int32_t dy;          // Relative Y movement
    bool left_button;    // Left button pressed
    bool right_button;   // Right button pressed
    bool middle_button;  // Middle button pressed
} MousePacket;

// Phase 3: The Event Queue (Kernel to User Space)
// A ring buffer to store MousePackets for non-blocking consumption.
#define MOUSE_QUEUE_SIZE 256

typedef struct {
    MousePacket buffer[MOUSE_QUEUE_SIZE];
    volatile uint32_t head;
    volatile uint32_t tail;
} MouseEventQueue;

// Global Event Queue instance
extern MouseEventQueue mouse_event_queue;

void mouse_queue_init(void);
bool mouse_queue_push(MousePacket packet);
bool mouse_queue_pop(MousePacket *packet);

#endif // MOUSE_EVENT_H
