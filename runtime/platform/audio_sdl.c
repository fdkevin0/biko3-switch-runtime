#include "platform/audio_output.h"
#include <SDL3/SDL.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* SDL3 push-stream audio backend.
 *
 * The runtime mixer expects a switch-like four-block device queue and an
 * absolute count of *consumed* stereo frames. SDL's stream queue gives us
 * exactly that: submitted minus still-queued data. The pump thread mirrors
 * the Switch backend (poll, fill, short sleep) while SDL's audio thread pulls
 * blocks at the device rate and inserts silence on underrun. */

enum { RATE = 48000, BLOCK_FRAMES = 480, BUFFER_COUNT = 4 };
enum { BLOCK_BYTES = BLOCK_FRAMES * 2 * (int)sizeof(int16_t) };

struct BkAudioOutput {
  SDL_AudioStream *stream;
  BkAudio *audio;
  pthread_t thread;
  pthread_mutex_t mutex;
  int thread_started, started, failed;
  atomic_int stop_requested, pump_failed;
  char pump_error[256];
  uint64_t submitted;
};

static void lock_mixer(void *context) {
  pthread_mutex_lock(&((BkAudioOutput *)context)->mutex);
}
static void unlock_mixer(void *context) {
  pthread_mutex_unlock(&((BkAudioOutput *)context)->mutex);
}

static int fail(BkAudioOutput *output, const char *what, char error[256]) {
  output->failed = 1;
  snprintf(error, 256, "audio output: %s", what);
  return 0;
}

static int submit(void *context, const int16_t *samples, size_t frames,
                  char error[256]) {
  BkAudioOutput *output = context;
  if (output->failed)
    return fail(output, "failed session", error);
  if (!samples || frames != BLOCK_FRAMES)
    return fail(output, "invalid block", error);
  int queued = SDL_GetAudioStreamQueued(output->stream);
  if (queued < 0)
    return fail(output, SDL_GetError(), error);
  if ((uint64_t)queued / 4 + frames > BUFFER_COUNT * BLOCK_FRAMES)
    return fail(output, "queue overflow", error);
  if (!SDL_PutAudioStreamData(output->stream, samples, BLOCK_BYTES))
    return fail(output, SDL_GetError(), error);
  output->submitted += frames;
  return 1;
}

static int poll_consumed(void *context, uint64_t *consumed, char error[256]) {
  BkAudioOutput *output = context;
  if (output->failed)
    return fail(output, "failed session", error);
  int queued = SDL_GetAudioStreamQueued(output->stream);
  if (queued < 0)
    return fail(output, SDL_GetError(), error);
  uint64_t frames = (uint64_t)queued / 4;
  *consumed = frames < output->submitted ? output->submitted - frames : 0;
  return 1;
}

BkAudioOutput *bk_audio_output_open(BkAudioSink *sink, FILE *log,
                                    char error[256]) {
  if (!sink) {
    snprintf(error, 256, "audio output: missing sink");
    return NULL;
  }
  if (!SDL_InitSubSystem(SDL_INIT_AUDIO)) {
    snprintf(error, 256, "audio output: SDL_InitSubSystem: %s", SDL_GetError());
    return NULL;
  }
  BkAudioOutput *output = calloc(1, sizeof(*output));
  if (!output) {
    snprintf(error, 256, "audio output: allocation failed");
    return NULL;
  }
  pthread_mutex_init(&output->mutex, NULL);
  atomic_init(&output->stop_requested, 0);
  atomic_init(&output->pump_failed, 0);
  SDL_AudioSpec spec = {SDL_AUDIO_S16, 2, RATE};
  output->stream =
      SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &spec, NULL,
                                NULL);
  if (!output->stream) {
    snprintf(error, 256, "audio output: SDL_OpenAudioDeviceStream: %s",
             SDL_GetError());
    free(output);
    return NULL;
  }
  if (!SDL_ResumeAudioStreamDevice(output->stream)) {
    snprintf(error, 256, "audio output: SDL_ResumeAudioStreamDevice: %s",
             SDL_GetError());
    SDL_DestroyAudioStream(output->stream);
    free(output);
    return NULL;
  }
  output->started = 1;
  *sink = (BkAudioSink){output, RATE, BLOCK_FRAMES,
                        BUFFER_COUNT * BLOCK_FRAMES, submit, poll_consumed};
  if (log) {
    fprintf(log,
            "Audio: SDL3 stream, 48000Hz PCM16 stereo; %dx10ms queue; "
            "consumed follows SDL stream queue.\n",
            BUFFER_COUNT);
    fflush(log);
  }
  return output;
}

static void *pump(void *context) {
  BkAudioOutput *output = context;
  while (!atomic_load_explicit(&output->stop_requested, memory_order_acquire)) {
    if (!bk_audio_poll(output->audio, output->pump_error) ||
        !bk_audio_fill(output->audio, output->pump_error)) {
      atomic_store_explicit(&output->pump_failed, 1, memory_order_release);
      break;
    }
    struct timespec pause = {0, 2000000};
    nanosleep(&pause, NULL);
  }
  return NULL;
}

int bk_audio_output_start(BkAudioOutput *output, BkAudio *audio, char e[256]) {
  if (!output || !audio || output->audio || output->failed) {
    snprintf(e, 256, "audio output: invalid pump start");
    return 0;
  }
  BkAudioSync sync = {output, lock_mixer, unlock_mixer};
  if (!bk_audio_set_sync(audio, &sync, e))
    return 0;
  output->audio = audio;
  atomic_store(&output->stop_requested, 0);
  atomic_store(&output->pump_failed, 0);
  if (!bk_audio_poll(audio, e) || !bk_audio_fill(audio, e))
    goto bad;
  if (pthread_create(&output->thread, NULL, pump, output)) {
    snprintf(e, 256, "audio output: cannot create pump thread");
    goto bad;
  }
  output->thread_started = 1;
  return 1;
bad:
  bk_audio_output_stop(output);
  return 0;
}

int bk_audio_output_check(BkAudioOutput *output, char e[256]) {
  if (!output || !output->thread_started) {
    snprintf(e, 256, "audio output: pump not running");
    return 0;
  }
  if (atomic_load_explicit(&output->pump_failed, memory_order_acquire)) {
    memcpy(e, output->pump_error, 256);
    return 0;
  }
  return 1;
}

void bk_audio_output_stop(BkAudioOutput *output) {
  if (!output)
    return;
  atomic_store_explicit(&output->stop_requested, 1, memory_order_release);
  if (output->thread_started) {
    pthread_join(output->thread, NULL);
    output->thread_started = 0;
  }
  if (output->audio) {
    char ignored[256];
    bk_audio_set_sync(output->audio, NULL, ignored);
    output->audio = NULL;
  }
}

void bk_audio_output_close(BkAudioOutput *output) {
  if (!output)
    return;
  bk_audio_output_stop(output);
  if (output->stream)
    SDL_DestroyAudioStream(output->stream);
  pthread_mutex_destroy(&output->mutex);
  free(output);
}
