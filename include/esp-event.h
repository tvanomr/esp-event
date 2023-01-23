#ifndef __ESP_EVENT_H__
#define __ESP_EVENT_H__

#include <concepts>
#include <set>
#include <memory>
#include "esp_event.h"

namespace esp
{
    namespace event
    {
        class default_queue;

        template <typename F, typename Arg>
        concept ptr_invocable = std::invocable<F, Arg> && std::is_pointer<Arg>::value;

        template <typename Arg, ptr_invocable<Arg> F>
        class default_binding
        {
            friend class default_queue;
            F func;
            esp_event_base_t event_base;
            int32_t event_id;
            esp_event_handler_instance_t id;
            void run(Arg value)
            {
                func(value);
            }
            ~default_binding();
            default_binding(F &&processor) : func(processor) {}

        public:
            default_binding(default_binding &&processor) = delete;
        };

        class default_queue
        {
            bool was_initialized;

            default_queue();
            ~default_queue();

            template <typename Arg, ptr_invocable<Arg> F>
            friend class default_binding;

            template <typename F, typename Arg>
            void unbind(default_binding<F, Arg> &binding)
            {
                if (binding.id)
                {
                    esp_event_handler_instance_unregister(binding.event_base, binding.event_id, binding.id);
                    binding.id = 0;
                }
            }

        public:
            static default_queue *get();
            template <typename Arg, typename F>
                requires ptr_invocable<F, Arg>
            std::unique_ptr<default_binding<F, Arg>> bind(esp_event_base_t event_base, int32_t event_id, F &&func)
            {
                auto binding = new default_binding<F, Arg>(func);
                binding.event_base = event_base;
                binding.event_id = event_id;
                static esp_event_handler_t handler = [](void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
                {
                    auto binding = static_cast<default_binding<F, Arg>>(event_handler_arg);
                    binding.run();
                };
                auto err = esp_event_handler_instance_register(
                    event_base, event_id, handler,
                    static_cast<void *>(binding), &binding.id);
                ESP_ERROR_CHECK(err);
                return binding;
            }
        };

        template <typename Arg, typename F>
        default_binding<Arg, F>::~default_binding()
        {
            default_queue::get()->unbind(*this);
        }

    }
}

#endif