/*
BStone: Unofficial source port of Blake Stone: Aliens of Gold and Blake Stone: Planet Strike
Copyright (c) 2013-2022 Boris I. Bendovsky (bibendovsky@hotmail.com) and Contributors
SPDX-License-Identifier: MIT
*/

#include <cassert>
#include "SDL_audio.h"
#include "bstone_memory_pool_1x.h"
#include "bstone_sys_sdl_exception.h"
#include "bstone_sys_sdl_push_audio_device.h"
#include "bstone_exception.h"

namespace bstone {
namespace sys {

namespace {

class SdlPushAudioDevice final : public PushAudioDevice
{
public:
	SdlPushAudioDevice(Logger& logger, const PushAudioDeviceOpenParam& param);
	SdlPushAudioDevice(const SdlPushAudioDevice&) = delete;
	SdlPushAudioDevice& operator=(const SdlPushAudioDevice&) = delete;
	~SdlPushAudioDevice() override;

	static void* operator new(std::size_t count);
	static void operator delete(void* ptr) noexcept;

private:
	Logger& logger_;
	int rate_{};
	int channel_count_{};
	int frame_count_{};
	PushAudioDeviceCallback* callback_{};
	SDL_AudioDeviceID sdl_audio_device_id_{};

private:
	int do_get_rate() const noexcept override;
	int do_get_channel_count() const noexcept override;
	int do_get_frame_count() const noexcept override;

	void do_pause(bool is_pause) override;

private:
	static void SDLCALL sdl_callback(void* userdata, Uint8* stream, int len);
	void callback(float* samples, int sample_count);
};

// ==========================================================================

using SdlPushAudioDevicePool = MemoryPool1XT<SdlPushAudioDevice>;

SdlPushAudioDevicePool sdl_push_audio_device_pool{};

// ==========================================================================

SdlPushAudioDevice::SdlPushAudioDevice(Logger& logger, const PushAudioDeviceOpenParam& param)
try
	:
	logger_{logger}
{
	logger_.log_information("<<< Start up SDL callback audio device.");

	if (param.channel_count <= 0 || param.channel_count > 255)
	{
		BSTONE_STATIC_THROW("Channel count out of range.");
	}

	if (param.desired_frame_count <= 0 || param.desired_frame_count > 65535)
	{
		BSTONE_STATIC_THROW("Frame count out of range.");
	}

	if (param.callback == nullptr)
	{
		BSTONE_STATIC_THROW("Null callback.");
	}

	callback_ = param.callback;

	auto desired_spec = SDL_AudioSpec{};
	desired_spec.freq = param.desired_rate;
	desired_spec.format = AUDIO_F32SYS;
	desired_spec.channels = static_cast<decltype(SDL_AudioSpec::channels)>(param.channel_count);
	desired_spec.samples = static_cast<decltype(SDL_AudioSpec::samples)>(param.desired_frame_count);
	desired_spec.callback = sdl_callback;
	desired_spec.userdata = this;

	auto effective_spec = SDL_AudioSpec{};

	sdl_audio_device_id_ = SDL_OpenAudioDevice(
		nullptr,
		SDL_FALSE,
		&desired_spec,
		&effective_spec,
		SDL_AUDIO_ALLOW_FREQUENCY_CHANGE | SDL_AUDIO_ALLOW_SAMPLES_CHANGE);

	if (sdl_audio_device_id_ <= 1)
	{
		sdl_fail();
	}

	rate_ = effective_spec.freq;
	channel_count_ = effective_spec.channels;
	frame_count_ = effective_spec.samples;

	logger_.log_information(">>> SDL callback audio device started up.");
}
BSTONE_STATIC_THROW_NESTED_FUNC

SdlPushAudioDevice::~SdlPushAudioDevice()
{
	logger_.log_information("Shut down SDL callback audio device.");

	SDL_CloseAudioDevice(sdl_audio_device_id_);
}

void* SdlPushAudioDevice::operator new(std::size_t count)
try
{
	return sdl_push_audio_device_pool.allocate(count);
}
BSTONE_STATIC_THROW_NESTED_FUNC

void SdlPushAudioDevice::operator delete(void* ptr) noexcept
{
	sdl_push_audio_device_pool.deallocate(ptr);
}

int SdlPushAudioDevice::do_get_rate() const noexcept
{
	return rate_;
}

int SdlPushAudioDevice::do_get_channel_count() const noexcept
{
	return channel_count_;
}

int SdlPushAudioDevice::do_get_frame_count() const noexcept
{
	return frame_count_;
}

void SdlPushAudioDevice::do_pause(bool is_pause)
{
	SDL_PauseAudioDevice(sdl_audio_device_id_, is_pause ? SDL_TRUE : SDL_FALSE);
}

void SDLCALL SdlPushAudioDevice::sdl_callback(void* userdata, Uint8* stream, int len)
{
	static_cast<SdlPushAudioDevice*>(userdata)->callback(
		reinterpret_cast<float*>(stream),
		len / static_cast<int>(sizeof(float)));
}

void SdlPushAudioDevice::callback(float* samples, int sample_count)
try
{
	callback_->invoke(samples, sample_count);
}
BSTONE_STATIC_THROW_NESTED_FUNC

} // namespace

// ==========================================================================

PushAudioDeviceUPtr make_sdl_push_audio_device(Logger& logger, const PushAudioDeviceOpenParam& param)
try
{
	return std::make_unique<SdlPushAudioDevice>(logger, param);
}
BSTONE_STATIC_THROW_NESTED_FUNC

} // namespace sys
} // namespace bstone
