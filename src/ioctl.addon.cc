#include <unistd.h>
#include <sys/ioctl.h>

#include <nan.h>

namespace NodeAsyncIoctl {
    using v8::Context;
    using v8::FunctionCallbackInfo;
    using v8::Isolate;
    using v8::Local;
    using v8::Object;
    using v8::Value;

    inline intptr_t GetParam(Local<Value> param) {
        if (param->IsUndefined()) {
            return 0;
        } else if (node::Buffer::HasInstance(param)) {
            return reinterpret_cast<intptr_t>(node::Buffer::Data(param));
        } else {
            return param->
                ToBigInt(Isolate::GetCurrent()->GetCurrentContext()).
                ToLocalChecked()->
                Uint64Value();
        }
    }

    void NonBlocking(const FunctionCallbackInfo<Value> &args) {
        struct ioctl_data {
            int fd;
            unsigned long request;
            intptr_t value;
            v8::Persistent<Value> value_handle;

            int error;
            int result;
        };
        struct thread_data {
            ioctl_data *params;
            size_t params_length;

            uv_work_t work;
            Nan::Callback callback;
            Nan::AsyncResource async_resource;

            thread_data(): async_resource("ioctl") {}
        };
        
        Isolate *isolate = args.GetIsolate();
        Local<Context> context = isolate->GetCurrentContext();
        v8::HandleScope scope(isolate);

        auto array = args[0].As<v8::Array>();
        auto length = array->Length();
        auto data = new thread_data();
        auto params = new ioctl_data[length];

        for (size_t i = 0; i < length; i++) {
            auto arg = array->Get(context, i).ToLocalChecked().As<v8::Array>();
            auto param = &params[i];
            auto fd = arg->Get(context, 0).ToLocalChecked();
            auto request = arg->Get(context, 1).ToLocalChecked();
            auto value = arg->Get(context, 2).ToLocalChecked();

            param->error = 0;
            param->result = -1;
            param->fd = fd->ToBigInt(context).ToLocalChecked()->Uint64Value();
            param->request = request->ToBigInt(context).ToLocalChecked()->Uint64Value();
            param->value = GetParam(value);
            param->value_handle.Reset(isolate, value);
        }

        data->params = params;
        data->params_length = length;
        data->work.data = data;
        data->callback.Reset(args[1].As<v8::Function>());

        uv_queue_work(
            uv_default_loop(),
            &data->work,
            [](uv_work_t *req) {
                auto data = static_cast<thread_data *>(req->data);
                auto params = data->params;
                auto length = data->params_length;

                for (size_t i = 0; i < length; i++) {
                    auto param = &params[i];
                    int result = ioctl(param->fd, param->request, param->value);

                    param->result = result;

                    if (result == -1) {
                        param->error = errno;

                        break;
                    }
                }
            },
            [](uv_work_t *req, int) {
                auto data = static_cast<thread_data *>(req->data);
                auto params = data->params;
                auto length = data->params_length;
                auto isolate = Isolate::GetCurrent();
                v8::HandleScope scope(isolate);
                auto context = isolate->GetCurrentContext();
                auto results = v8::Array::New(isolate, length);
                Local<Value> argv[] = { results };

                for (size_t i = 0; i < length; i++) {
                    auto param = &params[i];
                    auto result = v8::Array::New(isolate, 2);

                    result->Set(context, 0, v8::Number::New(isolate, param->error)).Check();
                    result->Set(context, 1, v8::Number::New(isolate, param->result)).Check();
                    results->Set(context, i, result).Check();
                }

                data->callback.Call(1, argv, &data->async_resource);

                delete[] params;
                delete data;
            }
        );
    }

    void Blocking(const FunctionCallbackInfo<Value> &args) {
        struct thread_data {
            int fd;
            unsigned long request;
            intptr_t param;
            v8::Persistent<Value> param_handle;

            int error;
            int result;
            uv_async_t async;
            Nan::Callback callback;
            Nan::AsyncResource async_resource;

            thread_data(): async_resource("ioctl") {}
        };
        
        auto data = new thread_data();
        Isolate *isolate = args.GetIsolate();
        Local<Context> context = isolate->GetCurrentContext();


        data->error = 0;
        data->result = -1;
        data->async.data = data;
        data->fd = args[0]->ToBigInt(context).ToLocalChecked()->Uint64Value();
        data->request = args[1]->ToBigInt(context).ToLocalChecked()->Uint64Value();
        data->param = GetParam(args[2]);
        data->param_handle.Reset(isolate, args[2]);
        data->callback.Reset(args[3].As<v8::Function>());

        uv_async_init(uv_default_loop(), &data->async, [](uv_async_t *async) {
            auto data = static_cast<thread_data *>(async->data);
            auto isolate = Isolate::GetCurrent();
            v8::HandleScope scope(isolate);
            Local<Value> argv[] = {
                v8::Number::New(isolate, data->error),
                v8::Number::New(isolate, data->result),
            };

            data->callback.Call(2, argv, &data->async_resource);

            uv_close((uv_handle_t *)async, [](uv_handle_t *handle) {
                auto data = static_cast<thread_data *>(handle->data);

                delete data;
            });
        });

        pthread_t thread_id;

        int error = pthread_create(
            &thread_id,
            nullptr,
            [](void *data_ptr) -> void * {
                auto data = static_cast<thread_data *>(data_ptr);
                int result = ioctl(data->fd, data->request, data->param);

                data->result = result;

                if (result == -1) {
                    data->error = errno;
                }

                uv_async_send(&data->async);

                return nullptr;
            },
            data
        );

        if (error != 0) {
            uv_close((uv_handle_t *)&data->async, [](uv_handle_t *handle) {
                auto data = static_cast<thread_data *>(handle->data);

                delete data;
            });

            Nan::ThrowError("Could start ioctl thread");
        }
    }

    void Initialize(Local<Object> exports) {
        NODE_SET_METHOD(exports, "blocking", Blocking);
        NODE_SET_METHOD(exports, "nonBlocking", NonBlocking);
    }

    NODE_MODULE(NODE_GYP_MODULE_NAME, Initialize)
}
