#ifndef BK_MEDIA_PCM_MIX_H
#define BK_MEDIA_PCM_MIX_H
#include "media/pcm.h"
/* Piecewise-frequency playback keeps an exact source phase whose fraction
 * denominator is output_rate. Changing Hz must retain this phase. elapsed
 * counts OUTPUT frames from start. Backends use submitted frames for mixing
 * and consumed frames for audible position/envelope. These are independent.
 * Non-loop ended phase is {source frame count,0}; loop phases wrap. */
typedef struct {
  size_t frame;
  uint32_t fraction;
} BkPcmPhase;
int bk_pcm_phase_advance(const BkPcm *, uint32_t output_rate, uint32_t hz,
                         uint64_t elapsed, int loop, BkPcmPhase start,
                         BkPcmPhase *out);
/* Adds to initialized, finite stereo FLOAT samples in signed16 amplitude
 * units; caller clips/quantizes once after mixing all voices. Volume[-10000,0]
 * and pan[-10000,10000] follow DirectSound attenuation units. Mono duplicates
 * to both sides; stereo retains channels. Linear interpolation and integer
 * rational phase are portable policy, not bit-identical Windows driver DSP.
 * Loop interpolation wraps, non-loop holds its last frame until duration.
 * Zero frames is valid; invalid input leaves destination unchanged.
 * Source PCM is borrowed only during this call. */
int bk_pcm_mix_phase(const BkPcm *, uint32_t output_rate, uint32_t hz,
                     BkPcmPhase, int loop, int32_t volume, int32_t pan,
                     float *stereo, size_t frames, char error[256]);
/* Saturating nearest-integer (ties away from0) stereo output conversion. */
int bk_pcm_quantize(const float *stereo, int16_t *out, size_t frames);
#endif
