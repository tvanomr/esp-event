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

        class default_binding_base
        {
        protected:
            esp_event_base_t event_base;
            int32_t event_id;
            esp_event_handler_instance_t id;
            ~default_binding_base();
            default_binding_base() = default;

        public:
            default_binding_base(default_binding_base &&);
            default_binding_base(const default_binding_base &) = delete;
        };

        template <typename F, typename Arg>
        concept ptr_invocable = std::invocable<F, Arg> && std::default_initializable<F> && std::is_pointer<Arg>::value;

        template <typename Arg, ptr_invocable<Arg> F>
        class default_binding : public default_binding_base
        {
            friend class default_queue;
            F func;
            void run(Arg value)
            {
                func(value);
            }
            default_binding(F &&processor) : func(processor) {}

        public:
            default_binding(default_binding &&processor) = default;
            ~default_binding() = default;
        };

        template <typename Arg>
        concept pointer = std::is_pointer<Arg>::value;

        template <typename T, pointer Arg>
        class default_method_binding : public default_binding_base
        {
            friend class default_queue;
            void (T::*func)(Arg);
            T *object;
            default_method_binding(T *base, void (T::*method)(Arg)) : object(base), func(method) {}
            void run(Arg value)
            {
                (object->*func)(value);
            }

        public:
            default_method_binding(default_method_binding &&processor) = default;
            ~default_method_binding() = default;
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
                    auto binding = static_cast<default_binding<F, Arg>*>(event_handler_arg);
                    binding.run(static_cast<Arg>(event_data));
                };
                auto err = esp_event_handler_instance_register(
                    event_base, event_id, handler,
                    static_cast<void *>(binding), &binding.id);
                ESP_ERROR_CHECK(err);
                return binding;
            }
            template <typename Arg, typename F>
                requires ptr_invocable<F, Arg>
            void unsafe_bind(esp_event_base_t event_base, int32_t event_id, F &&func)
            {
                F *callback = new F();
                *func = func;
                static esp_event_handler_t handler = [](void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
                {
                    auto binding = static_cast<F *>(event_handler_arg);
                    binding(static_cast<Arg>(event_data));
                };
                esp_event_handler_instance_t instance;
                auto err = esp_event_handler_instance_register(
                    event_base, event_id, handler,
                    static_cast<void *>(callback), &instance);
                ESP_ERROR_CHECK(err);
            }

            template <typename T, pointer Arg>
            std::unique_ptr<default_method_binding<T, Arg>> bind_method(esp_event_base_t event_base, int32_t event_id, T *object, void (T::*method)(Arg))
            {
                auto binding = new default_method_binding<T, Arg>(object, method);
                binding.event_base = event_base;
                binding.event_id = event_id;
                static esp_event_handler_t handler = [](void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
                {
                    auto binding = static_cast<default_method_binding<T, Arg>*>(event_handler_arg);
                    binding.run(static_cast<Arg>(event_data));
                };
                auto err = esp_event_handler_instance_register(event_base, event_id, handler, static_cast<void *>(binding), &binding.id);
                ESP_ERROR_CHECK(err);
                return binding;
            }

            template<typename T,pointer Arg>
            void unsafe_bind_method(esp_event_base_t event_base, int32_t event_id, T *object, void (T::*method)(Arg))
            {
                struct data
                {
                    T* object;
                    void (T::*method)(Arg);
                };
                auto binding = new data;
                binding.object=object;
                binding.method=method;
                static esp_event_handler_t handler = [](void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
                {
                    auto binding = static_cast<data*>(event_handler_arg);
                    (binding.object->*binding.method)(static_cast<Arg>(event_data));
                };
                esp_event_handler_instance_t instance;
                auto err = esp_event_handler_instance_register(event_base, event_id, handler, static_cast<void *>(binding), &instance);
                ESP_ERROR_CHECK(err);
            }
        };

    }
}

#endif