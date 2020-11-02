// Copyright 2020 Christopher A. Taylor

#include "kvm_turbojpeg.hpp"
#include "kvm_logger.hpp"

namespace kvm {

static logger::Channel Logger("Capture");


//------------------------------------------------------------------------------
// Decoder

TurboJpegDecoder::~TurboJpegDecoder()
{
    if (Handle) {
        tjDestroy(Handle);
        Handle = nullptr;
    }
}

std::shared_ptr<Frame> TurboJpegDecoder::Decompress(const uint8_t* data, int bytes)
{
    if (!Initialize()) {
        return nullptr;
    }

    int w = 0, h = 0, subsamp = 0;
    int r = tjDecompressHeader2(
        Handle,
        (uint8_t*)data,
        bytes,
        &w,
        &h,
        &subsamp);
    if (r != 0) {
        Logger.Error("tjDecompressHeader2 failed: r=", r, " err=", tjGetErrorStr());
        return nullptr;
    }

#if 0 // Faster but more complex:

    PixelFormat format;
    if (subsamp == TJSAMP_420) {
        format = PixelFormat::YUV420P;
    }
    else if (subsamp == TJSAMP_422) {
        format = PixelFormat::YUV422P;
    }
    else {
        Logger.Error("Unsupported subsamp: ", subsamp);
        return nullptr;
    }

    auto frame = Pool.Allocate(w, h, format);
    if (!frame) {
        Logger.Error("Allocate failed");
        return nullptr;
    }

    int strides[3] = {
        w,
        w/2,
        w/2
    };

    r = tjDecompressToYUVPlanes(
        Handle,
        (uint8_t*)data,
        bytes,
        frame->Planes,
        w,
        strides,
        h,
        TJFLAG_ACCURATEDCT);
    if (r != 0) {
        Logger.Error("tjDecompressToYUVPlanes failed: r=", r, " err=", tjGetErrorStr());
        return nullptr;
    }

#else // RGB:

    auto frame = Pool.Allocate(w, h, PixelFormat::RGB24);
    if (!frame) {
        Logger.Error("Allocate failed");
        return nullptr;
    }

    r = tjDecompress2(
        Handle,
        (uint8_t*)data,
        bytes,
        frame->Planes[0],
        w,
        w*3,
        h,
        TJPF_RGB,
        TJFLAG_ACCURATEDCT);
    if (r != 0) {
        Logger.Error("tjDecompressToYUVPlanes failed");
        return nullptr;
    }

#endif

    return frame;
}

bool TurboJpegDecoder::Initialize()
{
    if (Handle) {
        return true;
    }

    Handle = tjInitDecompress();
    if (!Handle) {
        Logger.Error("tjInitDecompress failed");
        return false;
    }

    return true;
}


} // namespace kvm
