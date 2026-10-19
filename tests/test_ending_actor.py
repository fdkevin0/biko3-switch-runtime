"""Shared ending assets retain authored roots and reject unsupported effects."""
import ctypes as C
from pathlib import Path
import struct
import tempfile
import unittest

from model_binding import library
from test_assets import make_pp
from test_model import chunk, fixture


class Actor(C.Structure):
    _fields_ = [(name, C.c_void_p) for name in
                ('model', 'clips', 'pose', 'materials', 'animation', 'morph')]
    _fields_ += [('root', C.c_uint32)]


class EndingActor(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.lib = library()
        bindings = {
            'bk_resources_create': ([C.c_void_p], C.c_void_p),
            'bk_resources_mount': ([C.c_void_p, C.c_char_p, C.c_char_p, C.c_void_p], C.c_int),
            'bk_resources_destroy': ([C.c_void_p], None),
            'bk_ending_actor_load': ([C.POINTER(Actor), C.c_void_p, C.c_char_p,
                                      C.c_char_p, C.c_int, C.c_void_p], C.c_int),
            'bk_ending_actor_destroy': ([C.POINTER(Actor)], None),
            'bk_ending_actor_advance': ([C.POINTER(Actor), C.c_float, C.c_void_p], C.c_int),
            'bk_ending_actor_advance_plain': ([C.POINTER(Actor), C.c_float, C.c_int,
                                              C.c_void_p], C.c_int),
            'bk_actor_pose_local': ([C.c_void_p, C.c_uint32], C.POINTER(C.c_float)),
        }
        for name, (args, result) in bindings.items():
            function = getattr(cls.lib, name)
            function.argtypes = args
            function.restype = result

    def load(self, model, effects, expected=True, message=None, include_model=True):
        xan = b'fixture.x'.ljust(256, b'\0') * 2 + bytes(0x5190 - 512)
        entries = [('actor.xan', xan)]
        if include_model:
            entries.append(('fixture.x', model))
        with tempfile.TemporaryDirectory() as folder:
            path = Path(folder) / 'fixture.pp'
            path.write_bytes(make_pp(entries))
            error = C.create_string_buffer(256)
            store = self.lib.bk_resources_create(error)
            self.assertTrue(store, error.value)
            actor = Actor()
            try:
                self.assertTrue(self.lib.bk_resources_mount(
                    store, b'bk3_03', str(path).encode(), error), error.value)
                result = self.lib.bk_ending_actor_load(
                    C.byref(actor), store, b'bk3_03', b'actor.xan', effects, error)
                self.assertEqual(bool(result), expected, error.value)
                if message:
                    self.assertIn(message, error.value)
                if expected:
                    self.assertTrue(actor.model and actor.clips and actor.pose)
                    self.assertEqual(bool(actor.materials), bool(effects))
                    self.assertFalse(actor.animation or actor.morph)
                    self.assertEqual(actor.root, 0)
                    matrix = self.lib.bk_actor_pose_local(actor.pose, actor.root)
                    self.assertEqual(list(matrix[:16]),
                                     [1, 0, 0, 0, 0, 1, 0, 0,
                                      0, 0, 1, 0, 2, 0, 0, 1])
                    self.assertTrue(self.lib.bk_ending_actor_advance(
                        C.byref(actor), .016, error), error.value)
                    for mode in range(3):
                        self.assertTrue(self.lib.bk_ending_actor_advance_plain(
                            C.byref(actor), .016, mode, error), error.value)
            finally:
                self.lib.bk_ending_actor_destroy(C.byref(actor))
                self.lib.bk_resources_destroy(store)

    def test_authored_root_with_and_without_material_effects(self):
        for effects in (0, 1):
            with self.subTest(effects=effects):
                self.load(fixture(), effects)

    def test_failed_loads_leave_a_releasable_prefix(self):
        self.load(fixture(), 1, False, b'resource missing', include_model=False)
        self.load(b'broken', 1, False)
        for tag in (b'LIGA', b'TEXA'):
            for effects in (0, 1):
                with self.subTest(tag=tag, effects=effects):
                    self.load(fixture() + chunk(tag, b''), effects, False,
                              b'animated resource effect')
        model = bytearray(fixture())
        struct.pack_into('<I', model, model.index(b'FRAM') + 8 + 396 + 172, 0)
        self.load(bytes(model), 1, False, b'multiple actor roots')

    def test_plain_actors_do_not_construct_unused_material_or_morph_effects(self):
        for tag in (b'MATA', b'MORP'):
            with self.subTest(tag=tag):
                model = fixture() + chunk(tag, b'broken')
                self.load(model, 0)
                self.load(model, 1, False)


if __name__ == '__main__':
    unittest.main()
