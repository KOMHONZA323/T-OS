#include "mouse_event.h"
#include <stdint.h>
#include <stdbool.h>

MouseEventQueue mouse_event_queue;

void mouse_queue_init(void) {
    mouse_event_queue.head = 0;
    mouse_event_queue.tail = 0;
}

// Phase 3: Push into the Ring Buffer
bool mouse_queue_push(MousePacket packet) {
    uint32_t next = (mouse_event_queue.head + 1) % MOUSE_QUEUE_SIZE;
    if (next == mouse_event_queue.tail) {
        return false; // Queue full
    }
    mouse_event_queue.buffer[mouse_event_queue.head] = packet;
    mouse_event_queue.head = next;
    return true;
}

// Phase 3: Pop from the Ring Buffer
bool mouse_queue_pop(MousePacket *packet) {
    if (mouse_event_queue.head == mouse_event_queue.tail) {
        return false; // Queue empty
    }
    *packet = mouse_event_queue.buffer[mouse_event_queue.tail];
    mouse_event_queue.tail = (mouse_event_queue.tail + 1) % MOUSE_QUEUE_SIZE;
    return true;
}
