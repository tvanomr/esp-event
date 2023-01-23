#include <stdio.h>
#include "esp-event.h"
#include "esp_event.h"

namespace esp
{
    namespace event
    {
        default_queue::default_queue()
        {
            auto err = esp_event_loop_create_default();
            if (err == ESP_ERR_INVALID_STATE)
                was_initialized = true;
            else
            {
                was_initialized = false;
                ESP_ERROR_CHECK(err);
            }
        }
        default_queue::~default_queue()
        {
            if (was_initialized)
                esp_event_loop_delete_default();
        }

        default_queue *default_queue::get()
        {
            static default_queue queue;
            return &queue;
        }
    }
}