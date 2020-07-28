// Copyright 2020 Christopher A. Taylor

#include "kvm_turbojpeg.hpp"
#include "kvm_capture.hpp"
#include "kvm_encode.hpp"
#include "kvm_logger.hpp"
using namespace kvm;

static logger::Channel Logger("TurboJpegTest");

#include <exception>

#include <csignal>
#include <atomic>
std::atomic<bool> Terminated = ATOMIC_VAR_INIT(false);
void SignalHandler(int)
{
    Terminated = true;
}

int main(int argc, char* argv[])
{
    SetCurrentThreadName("Main");

    CORE_UNUSED(argc);
    CORE_UNUSED(argv);

    Logger.Info("kvm_turbojpeg_test");

    V4L2Capture capture;
    TurboJpegDecoder decoder;
    MmalEncoder encoder;

    if (!capture.Initialize([&](const std::shared_ptr<CameraFrame>& buffer) {
        Logger.Info("Got frame #", buffer->FrameNumber, " bytes = ", buffer->ImageBytes);

        uint64_t t0 = GetTimeUsec();

        auto frame = decoder.Decompress(buffer->Image, buffer->ImageBytes);
        if (!frame) {
            Logger.Error("Failed to decode JPEG");
            return;
        }
        ScopedFunction frame_scope([&]() {
            decoder.Release(frame);
        });

        uint64_t t1 = GetTimeUsec();
        int64_t dt = t1 - t0;
        Logger.Info("Decoding JPEG took ", dt / 1000.f, " msec");

        t0 = GetTimeUsec();

        int bytes = 0;
        uint8_t* data = encoder.Encode(frame, bytes);

        t1 = GetTimeUsec();
        dt = t1 - t0;
        Logger.Info("Encoding H264 took ", dt / 1000.f, " msec");

        if (!data) {
            Logger.Error("Encode failed");
            return;
        }
    })) {
        Logger.Error("Failed to start capture");
        return kAppFail;
    }

    std::signal(SIGINT, SignalHandler);

    while (!Terminated && !capture.IsError()) {
        ThreadSleepForMsec(100);
    }

    Logger.Info("Shutting down...");

    std::terminate();

    capture.Shutdown();

    return kAppSuccess;
}
