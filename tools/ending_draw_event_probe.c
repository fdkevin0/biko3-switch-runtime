/* Production draw adapter with explicitly controlled phase/mode/audio
 * fixtures. The real scene owns resources, geometry, viewports and mixer;
 * this is not a natural controller walkthrough or a production cheat API. */
#include "../runtime/scene/ending_normal_session.c"

#define CHECK(x) do { if (!(x)) { \
  fprintf(stderr, "ending draw group%u variant%u line%d (%s): %s\n", \
          group, variant, __LINE__, #x, e); goto done; } } while (0)

static unsigned profiles, frames, dual_views, empty_views, gain_checks, redraws,
                resize_redraws;
typedef struct { uint64_t submitted; } Sink;
static int submit(void *p, const int16_t *pcm, size_t n, char e[256]) {
  (void)pcm; (void)e;
  ((Sink *)p)->submitted += n;
  return 1;
}
static int poll(void *p, uint64_t *n, char e[256]) {
  (void)e;
  *n = ((Sink *)p)->submitted;
  return 1;
}
static int event_draw_snapshot(BkRenderer *renderer, EndingNormalScene *s,
                                char e[256]) {
  return bk_renderer_begin(renderer, e) &&
         bk_renderer_viewport(renderer, &s->viewport, e) &&
         bk_ending_normal_render_draw(s->render, e) &&
         bk_renderer_end(renderer, e);
}
static int sample(BkRenderer *renderer, EndingNormalScene *s,
                    unsigned phase, int32_t mode, int playing, char e[256]) {
  s->state->frame.phase = (int32_t)phase;
  s->state->control.mode_721ec4 = mode;
  s->active_seconds = .125f;
  s->now_ms += 125;
  uint32_t movie = bk_ending_normal_render_movie_frame(s->render);
  int32_t volume, pan;
  if (!bk_audio_get_gain(s->services.audio, 0, &volume, &pan))
    return fail(e, "missing commanded voice gain");
  int32_t latch = *s->duck_transition;
  if (!prepare_scene_draw(s, e))
    return 0;
  unsigned regular = phase == 7 ? 0 : phase == 1 ? 3 : 2;
  int dual = phase != 7 && (mode == 1 || (mode == 0 && playing));
  unsigned expected = regular + (dual ? regular - 1 : 0);
  if (bk_ending_normal_render_pass_count(s->render) != expected)
    return fail(e, "wrong live phase/mode/speech1 draw dispatch");
  for (unsigned i = 0; i < expected; i++)
    if (bk_ending_normal_render_pass_view(s->render, i) != (i >= regular))
      return fail(e, "draw pass used the wrong viewport snapshot");
  if (phase == 7 || mode < 0 || mode > 2) {
    int32_t after_volume, after_pan;
    if (!bk_audio_get_gain(s->services.audio, 0, &after_volume, &after_pan) ||
        after_volume != volume || after_pan != pan ||
        *s->duck_transition != latch ||
        (phase == 7 && movie != bk_ending_normal_render_movie_frame(s->render)))
      return fail(e, "suppressed event changed retained audio/video");
  }
  if (!event_draw_snapshot(renderer, s, e))
    return 0;
  uint8_t first[64 * 48 * 4], again[sizeof(first)];
  if (!bk_renderer_readback(renderer, first, sizeof(first), e))
    return 0;
  if (!expected)
    empty_views++;
  /* Pure redraws consume neither the event stage nor the native camera
   * setters. Pixel equality also catches replaying mutable BOM twice. */
  BkMenuCamera camera = s->camera;
  uint32_t rng = *s->random;
  movie = bk_ending_normal_render_movie_frame(s->render);
  latch = *s->duck_transition;
  if (!bk_audio_get_gain(s->services.audio, 0, &volume, &pan) ||
      !event_draw_snapshot(renderer, s, e) ||
      !bk_renderer_readback(renderer, again, sizeof(again), e))
    return 0;
  int32_t after_volume, after_pan;
  if (!bk_audio_get_gain(s->services.audio, 0, &after_volume, &after_pan) ||
      memcmp(first, again, sizeof(first)) ||
      memcmp(&camera, &s->camera, sizeof(camera)) || *s->random != rng ||
      movie != bk_ending_normal_render_movie_frame(s->render) ||
      *s->duck_transition != latch || after_volume != volume || after_pan != pan)
    return fail(e, "pure redraw changed picture, camera, movie or audio");
  frames++;
  redraws++;
  dual_views += dual;
  return 1;
}
static int restart(EndingNormalScene *s, unsigned slot, int32_t volume,
                     char e[256]) {
  BkEndingAudioCall command = {.operation = BK_ENDING_AUDIO_RESTART,
                                .slot = slot, .flags = 1, .volume = volume};
  int playing = 0;
  return bk_ending_audio_call(s->audio, s->group, s->variant, 0,
                               &command, &playing, e);
}
typedef struct {
  BkAudioStats stats;
  BkAudioCursor voices[2];
  int gain[2], playing[2];
  int32_t volume[2], pan[2];
} ResizeAudio;
static int resize_audio(BkAudio *audio, ResizeAudio *out) {
  memset(out, 0, sizeof(*out));
  out->stats = bk_audio_stats(audio);
  for (unsigned i = 0; i < 2; ++i) {
    if (!bk_audio_cursor(audio, i, out->voices + i) ||
        !bk_audio_playing(audio, i, out->playing + i))
      return 0;
    out->gain[i] = bk_audio_get_gain(audio, i, out->volume + i, out->pan + i);
  }
  return 1;
}
static int resize_snapshot(BkScene *scene, BkRenderer *renderer,
                           EndingNormalScene *s, char e[256]) {
  const unsigned sizes[][2] = {{32,24}, {97,61}, {63,79}, {64,48}};
  BkEndingState state = *s->state;
  BkMenuCamera camera = *s->camera_owner;
  BkVirtualPointer pointer = s->pointer;
  uint32_t random = *s->random;
  int32_t duck = *s->duck_transition;
  BkEndingNormalRender *snapshot = s->snapshot_render;
  uint32_t movie = bk_ending_normal_render_movie_frame(snapshot);
  ResizeAudio audio;
  if (!resize_audio(s->services.audio, &audio))
    return fail(e, "cannot inspect resize audio fixture");
  for (unsigned i = 0; i < sizeof(sizes) / sizeof(*sizes); ++i) {
    unsigned w = sizes[i][0], h = sizes[i][1];
    uint8_t first[97 * 79 * 4], again[sizeof(first)];
    size_t bytes = (size_t)w * h * 4;
    if (!bk_renderer_resize(renderer, w, h, e) ||
        !bk_scene_resize(scene, w, h, e) ||
        !bk_renderer_begin(renderer, e) ||
        !bk_scene_draw(scene, &(BkSceneFrame){0}, e) ||
        !bk_renderer_end(renderer, e) ||
        !bk_renderer_readback(renderer, first, bytes, e) ||
        !bk_renderer_begin(renderer, e) ||
        !bk_scene_draw(scene, &(BkSceneFrame){0}, e) ||
        !bk_renderer_end(renderer, e) ||
        !bk_renderer_readback(renderer, again, bytes, e))
      return 0;
    BkViewport invalid = s->viewport;
    invalid.width = w + 1;
    if (bk_ending_normal_render_set_viewports(snapshot, &invalid,
                                             &s->special_viewport, e) ||
        bk_ending_normal_render_set_viewports(snapshot, &s->viewport,
                                             &(BkViewport){0}, e) ||
        bk_ending_normal_render_set_viewports(snapshot, &s->viewport,
            &(BkViewport){UINT32_MAX, 0, 1, 1}, e))
      return fail(e, "resized snapshot accepted an invalid physical viewport");
    /* Bypass session relocation here: rejected rectangles must not corrupt
     * either cached view, and the already prepared UI remains reusable. */
    if (!bk_renderer_begin(renderer, e) ||
        !bk_renderer_viewport(renderer, &s->viewport, e) ||
        !bk_ending_normal_render_draw(snapshot, e) ||
        !bk_ending_ui_batch_draw(s->ui_batch, e) ||
        !bk_renderer_end(renderer, e) ||
        !bk_renderer_readback(renderer, again, bytes, e))
      return 0;
    ResizeAudio after;
    if (!resize_audio(s->services.audio, &after) ||
        memcmp(&audio, &after, sizeof(audio)) || memcmp(first, again, bytes) ||
        memcmp(&state, s->state, sizeof(state)) ||
        memcmp(&camera, s->camera_owner, sizeof(camera)) ||
        memcmp(&pointer, &s->pointer, sizeof(pointer)) ||
        *s->random != random || *s->duck_transition != duck ||
        s->snapshot_render != snapshot ||
        bk_ending_normal_render_movie_frame(snapshot) != movie)
      return fail(e, "resized redraw changed CPU state, camera, RNG, audio or movie");
    resize_redraws += 3;
  }
  return 1;
}
static int profile(BkRenderer *renderer, BkResourceStore *store,
                    unsigned group, unsigned variant, char e[256]) {
  int ok = 0;
  Sink sink = {0};
  BkAudioSink output = {&sink, 48000, 240, 960, submit, poll};
  BkAudio *audio = NULL;
  BkScene *scene = NULL;
  BkRenderStats baseline = bk_renderer_stats(renderer);
  CHECK(audio = bk_audio_create(&output, e));
  BkSceneServices services = {.resources = store,
                              .renderer = renderer,
                              .audio = audio};
  CHECK(scene = bk_ending_normal_scene_create(&services, group, variant, e));
  EndingNormalScene *s = bk_scene_custom_context(scene);
  CHECK(bk_renderer_begin(renderer, e) &&
          bk_scene_draw(scene, &(BkSceneFrame){0}, e) &&
          bk_renderer_end(renderer, e) &&
          bk_ending_normal_scene_after_present(scene, e));
  int present;
  CHECK(bk_ending_audio_present(s->audio, 1, &present) && !present);
  /* Only gain/status routing matters in these explicit fixtures. Use the
   * same real PCM effect in both slots, so no absent speech table entry can
   * masquerade as a stopped/present buffer. Parent voice choice is tested
   * separately by the real-input framing probe. */
  CHECK(bk_ending_audio_bind(s->audio, 0, "bk3_02", "se002.wav", e));
  CHECK(restart(s, 0, -1000, e));
  CHECK(bk_audio_gain(audio, 0, -1000, -321, e));
  for (int32_t mode = -1; mode <= 3; mode++) {
    CHECK(sample(renderer, s, 1, mode, 0, e));
    CHECK(sample(renderer, s, 9, mode, 0, e));
    CHECK(sample(renderer, s, 7, mode, 0, e));
  }
  CHECK(bk_ending_audio_bind(s->audio, 1, "bk3_02", "se002.wav", e));
  CHECK(bk_ending_audio_present(s->audio, 1, &present) && present);
  int playing;
  CHECK(bk_audio_playing(audio, 1, &playing) && !playing);
  CHECK(sample(renderer, s, 1, 0, 0, e));
  CHECK(restart(s, 1, -1700, e));
  CHECK(bk_audio_gain(audio, 1, -1700, 234, e));
  for (int32_t mode = -1; mode <= 3; mode++) {
    CHECK(sample(renderer, s, 1, mode, 1, e));
    CHECK(sample(renderer, s, 9, mode, 1, e));
    CHECK(sample(renderer, s, 7, mode, 1, e));
  }
  int32_t voice0, pan0, voice1, pan1;
  CHECK(bk_audio_get_gain(audio, 0, &voice0, &pan0));
  CHECK(voice0 < -1000 && voice0 >= -3000 && pan0 == -321);
  CHECK(bk_audio_get_gain(audio, 1, &voice1, &pan1));
  CHECK(voice1 == -1700 && pan1 == 234);
  CHECK(bk_audio_playing(audio, 0, &playing) && playing);
  gain_checks++;
  for (unsigned step = 0; step < 10; step++)
    CHECK(sample(renderer, s, 1, 2, 1, e));
  CHECK(bk_audio_get_gain(audio, 0, &voice0, &pan0) &&
          voice0 == -3000 && pan0 == -321 && !*s->duck_transition);
  gain_checks++;
  BkEndingAudioCall stop = {.operation = BK_ENDING_AUDIO_PAUSE, .slot = 1};
  CHECK(bk_ending_audio_call(s->audio, group, variant, 0, &stop, &playing, e));
  CHECK(bk_audio_playing(audio, 1, &playing) && !playing);
  for (unsigned step = 0; step < 10; step++)
    CHECK(sample(renderer, s, 1, 0, 0, e));
  CHECK(bk_audio_get_gain(audio, 0, &voice0, &pan0) &&
          voice0 == -1000 && pan0 == -321 && !*s->duck_transition);
  CHECK(bk_audio_get_gain(audio, 1, &voice1, &pan1) &&
          voice1 == -1700 && pan1 == 234);
  gain_checks++;
  /* The unused special rectangle must not break a regular/empty branch;
   * selecting it must fail instead of silently drawing the ordinary view. */
  unsigned saved_width = s->special_viewport.width;
  s->special_viewport.width = 0;
  CHECK(sample(renderer, s, 7, 1, 0, e));
  CHECK(sample(renderer, s, 1, 2, 0, e));
  s->state->control.mode_721ec4 = 1;
  CHECK(!prepare_scene_draw(s, e));
  CHECK(!bk_ending_normal_render_pass_count(s->render));
  s->special_viewport.width = saved_width;
  CHECK(sample(renderer, s, 1, 1, 0, e));
  CHECK(resize_snapshot(scene, renderer, s, e));
  /* The UI tail may retire a stage while its last dual-view snapshot remains
   * drawable. Resizing must relocate that snapshot without its live actors. */
  CHECK(ui_reload_release(s, BK_ENDING_LOAD_4CF318, e));
  CHECK(!s->render && s->retired.render == s->snapshot_render);
  CHECK(resize_snapshot(scene, renderer, s, e));
  bk_scene_destroy(scene);
  scene = NULL;
  BkRenderStats final = bk_renderer_stats(renderer);
  CHECK(final.live_allocations == baseline.live_allocations &&
          final.live_bytes == baseline.live_bytes);
  profiles++;
  ok = 1;
done:
  bk_scene_destroy(scene);
  bk_audio_destroy(audio);
  return ok;
}
int main(int argc, char **argv) {
  if (argc != 2)
    return 2;
  char e[256] = {0}, path[1024];
  BkRenderer *renderer = bk_renderer_create(64, 48, stdout, e);
  BkResourceStore *store = bk_resources_create(e);
  int result = 1;
  if (!renderer || !store)
    goto done;
  const char *packs[] = {"bk3_00", "bk3_02", "bk3_03", "bk3_04",
                          "bk3_06", "bk3_08", "bk3_18", "fambom"};
  for (unsigned i = 0; i < sizeof(packs) / sizeof(*packs); i++) {
    if (snprintf(path, sizeof(path), "%s/%s.pp", argv[1], packs[i]) >=
        (int)sizeof(path) || !bk_resources_mount(store, packs[i], path, e))
      goto done;
  }
  for (unsigned group = 0; group < 5; group++)
    for (unsigned variant = 0; variant < 2; variant++)
      if (!profile(renderer, store, group, variant, e))
        goto done;
  printf("PASS ending draw events: profiles%u frames%u dual%u empty%u "
         "gain_checks%u exact_redraws%u; real PCM status, retained duck, "
         "mode/phase dispatch, resource retirement, resized_redraws%u\n",
         profiles, frames, dual_views, empty_views, gain_checks, redraws, resize_redraws);
  result = 0;
done:
  if (result)
    fprintf(stderr, "%s\n", e);
  bk_resources_destroy(store);
  bk_renderer_destroy(renderer);
  return result;
}
