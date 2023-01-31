#include <stdio.h>
#include "esp_event_cpp.h"
#include "esp_event.h"

namespace esp
{
    namespace event
    {
        default_binding_base::~default_binding_base()
        {
            if (id)
            {
                esp_event_handler_instance_unregister(event_base, event_id, id);
                id = 0;
            }
        }

        default_binding_base::default_binding_base(default_binding_base &&other)
        {
            event_base = other.event_base;
            event_id = other.event_id;
            id = other.id;
            other.event_base = 0;
            other.event_id = 0;
            other.id = 0;
        }

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