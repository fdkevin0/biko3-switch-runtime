/* Read-only CPU resources through the production session's role adapter. */
#include "../runtime/scene/ending_normal_session.c"

static int check_bindings(EndingNormalScene *s, unsigned kind, char e[256]) {
  static const unsigned normal[] = {0, 1, BK_MODEL_NONE, 2, 3, 4};
  static const unsigned single[] = {0, BK_MODEL_NONE, BK_MODEL_NONE, 1, 2, 3};
  BkActorForest *forest = scene_forest(s);
  if (!forest || !scene_primary(s) || !scene_actor(s, ENDING_ROLE_BACKGROUND))
    return fail(e, "probe requires a complete stage with background");
  for (unsigned role = 0; role < ENDING_ROLE_COUNT; ++role) {
    unsigned physical = kind == 2 ? role : kind == 0 ? normal[role] : single[role];
    BkActorPose *pose = physical == BK_MODEL_NONE ? NULL :
        kind == 0 ? bk_ending_normal_assets_pose(s->assets, physical) :
        kind == 1 ? bk_ending_secondary_assets_pose(s->secondary_assets, physical) :
        kind == 2 ? bk_ending_tertiary_assets_pose(s->tertiary_assets, physical) :
        kind == 3 ? bk_ending_selected_assets_pose(s->selected_assets, physical) :
                    bk_ending_auxiliary_assets_pose(s->auxiliary_assets, physical);
    uint32_t registry = !pose ? BK_MODEL_NONE : kind == 2
        ? bk_ending_tertiary_assets_registry(s->tertiary_assets, physical) : physical;
    uint32_t root = !pose ? 0 :
        kind == 0 ? bk_ending_normal_assets_root(s->assets, physical) :
        kind == 1 ? bk_ending_secondary_assets_root(s->secondary_assets, physical) :
        kind == 2 ? bk_ending_tertiary_assets_root(s->tertiary_assets, physical) :
        kind == 3 ? bk_ending_selected_assets_root(s->selected_assets, physical) :
                    bk_ending_auxiliary_assets_root(s->auxiliary_assets, physical);
    if (scene_actor(s, role) != pose || scene_registry(s, role) != registry ||
        scene_root(s, role) != root)
      return fail(e, "cached role differs from resource owner");
    if (pose && root != BK_FRAME_NONE) {
      uint32_t actor, frame;
      if (!bk_actor_forest_binding(forest, root, &actor, &frame) ||
          actor != registry || !bk_actor_pose_local(pose, frame))
        return fail(e, "role root does not belong to bound pose/registry");
    }
  }
  if (scene_actor(s, ENDING_ROLE_COUNT) ||
      scene_registry(s, ENDING_ROLE_COUNT) != BK_MODEL_NONE ||
      scene_root(s, ENDING_ROLE_COUNT) || scene_primary(NULL))
    return fail(e, "invalid role unexpectedly resolves");
  return 1;
}

static int profile(BkResourceStore *store, unsigned kind, unsigned group,
                   unsigned variant, unsigned selection, char e[256]) {
  EndingNormalScene scene = {0};
  uint32_t clocks[4] = {100, 101, 102, 103}, random = UINT32_C(0x4cc582);
  BkMenuCamera camera = {0};
  BkEndingCameraPresets presets;
  int ok = 0;
  if (!bk_menu_camera_dialogue(&camera))
    goto done;
  switch (kind) {
  case 0:
    scene.assets = bk_ending_normal_assets_create(store, group, variant,
        clocks, &random, &camera, &presets, e);
    if (!scene.assets || !bk_ending_normal_assets_load_background(scene.assets, store, e))
      goto done;
    break;
  case 1:
    scene.secondary_assets = bk_ending_secondary_assets_create(store, group, variant,
        clocks, &random, &camera, &presets, e);
    if (!scene.secondary_assets ||
        !bk_ending_secondary_assets_load_background(scene.secondary_assets, store, e))
      goto done;
    break;
  case 2:
    scene.tertiary_assets = bk_ending_tertiary_assets_create(store, group, variant,
        clocks, &random, &camera, &presets, e);
    if (!scene.tertiary_assets ||
        !bk_ending_tertiary_assets_load_background(scene.tertiary_assets, store, e))
      goto done;
    break;
  case 3: {
    BkEndingSelectedConfig config;
    if (!bk_ending_selected_config(&config, group, variant, selection))
      goto done;
    BkEndingSelectedLoad load = {group, variant, selection, 0, config.primary, NULL};
    scene.selected_assets = bk_ending_selected_assets_create(store, &load,
        clocks, &random, &camera, &presets, e);
    if (!scene.selected_assets ||
        !bk_ending_selected_assets_load_background(scene.selected_assets, store, e))
      goto done;
    break;
  }
  case 4:
    scene.auxiliary_assets = bk_ending_auxiliary_assets_create(store, group, variant,
        clocks, &random, &camera, &presets, e);
    if (!scene.auxiliary_assets ||
        !bk_ending_auxiliary_assets_load_background(scene.auxiliary_assets, store, e))
      goto done;
    break;
  }
  bind_scene_actors(&scene);
  if (!check_bindings(&scene, kind, e))
    goto done;
  release_resources(&scene);
  for (unsigned role = 0; role < ENDING_ROLE_COUNT; ++role)
    if (scene_actor(&scene, role) || scene_registry(&scene, role) != BK_MODEL_NONE ||
        scene_root(&scene, role)) {
      fail(e, "released stage retained a role binding");
      goto done;
    }
  ok = 1;
done:
  release_resources(&scene);
  if (!ok)
    fprintf(stderr, "role kind%u group%u variant%u selection%u: %s\n",
            kind, group, variant, selection, e);
  return ok;
}

int main(int argc, char **argv) {
  if (argc != 2)
    return 2;
  char e[256] = {0}, path[1024];
  BkResourceStore *store = bk_resources_create(e);
  int rc = 1;
  unsigned profiles = 0;
  const char *packs[] = {"bk3_00", "bk3_02", "bk3_03", "bk3_04", "bk3_06",
                         "bk3_08", "bk3_09", "bk3_10", "bk3_11", "bk3_12",
                         "bk3_13", "bk3_18", "fambom"};
  if (!store)
    goto done;
  for (unsigned i = 0; i < sizeof(packs) / sizeof(*packs); ++i)
    if (snprintf(path, sizeof(path), "%s/%s.pp", argv[1], packs[i]) >= (int)sizeof(path) ||
        !bk_resources_mount(store, packs[i], path, e))
      goto done;
  for (unsigned kind = 0; kind < 5; ++kind)
    for (unsigned group = 0; group < 5; ++group)
      for (unsigned variant = 0; variant < 2; ++variant)
        for (unsigned selection = 0; selection < (kind == 3 ? 3u : 1u); ++selection) {
          if (!profile(store, kind, group, variant, selection, e))
            goto done;
          ++profiles;
        }
  printf("PASS ending role bindings: profiles%u, five loaders, six roles, release clear\n",
         profiles);
  rc = 0;
done:
  if (rc)
    fprintf(stderr, "%s\n", e);
  bk_resources_destroy(store);
  return rc;
}
