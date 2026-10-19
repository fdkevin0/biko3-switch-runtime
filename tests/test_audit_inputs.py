"""Empty resource audits must fail without overwriting earlier evidence."""
import contextlib
import io
import hashlib
import json
import os
from pathlib import Path
import struct
import subprocess
import sys
import tempfile
import unittest
import wave
from unittest.mock import patch

import audit_game
import audit_models
import audit_pcm
from test_assets import make_pp


class AuditInputs(unittest.TestCase):
    def test_missing_empty_and_non_directory_inputs_do_not_load_libraries(self):
        for module, loader in [(audit_game, 'ctypes.CDLL'), (audit_models, 'library'),
                               (audit_pcm, 'library')]:
            with self.subTest(audit=module.__name__), tempfile.TemporaryDirectory() as folder:
                root = Path(folder)
                (root/'empty').mkdir()
                (root/'file').write_bytes(b'not a directory')
                for data in [root/'missing', root/'empty', root/'file']:
                    with self.subTest(data=data.name), patch.object(module, 'ROOT', root), \
                            patch.object(sys, 'argv', [module.__name__, str(data)]), \
                            patch(module.__name__+'.'+loader) as load, \
                            contextlib.redirect_stderr(io.StringIO()):
                        with self.assertRaises(SystemExit) as error:
                            module.main()
                        self.assertEqual(error.exception.code, 2)
                        load.assert_not_called()
                        self.assertFalse((root/'local').exists())

    def test_no_target_payload_preserves_existing_report(self):
        for module, name in [(audit_game, 'audit.json'),
                             (audit_models, 'cpu-model-audit.json'),
                             (audit_pcm, 'pcm-audit.json')]:
            with self.subTest(audit=module.__name__), tempfile.TemporaryDirectory() as folder:
                root = Path(folder)
                data = root/'Data'; data.mkdir()
                (data/'empty.pp').write_bytes(struct.pack('<II', 0, 0))
                output = root/'local'/name
                output.parent.mkdir()
                output.write_text('previous audit evidence')
                with patch.object(module, 'ROOT', root), \
                        patch.object(sys, 'argv', [module.__name__, str(data)]), \
                        patch(module.__name__+('.ctypes.CDLL' if module is audit_game else '.library')), \
                        patch('subprocess.check_output', return_value=b''), \
                        patch('subprocess.run'), \
                        contextlib.redirect_stderr(io.StringIO()), \
                        contextlib.redirect_stdout(io.StringIO()):
                    with self.assertRaises(SystemExit) as error:
                        module.main()
                    self.assertEqual(error.exception.code, 2)
                self.assertEqual(output.read_text(), 'previous audit evidence')

    def test_game_audit_keeps_non_image_archive_coverage(self):
        build = Path(os.environ.get('BK3_BUILD_DIR', audit_game.ROOT/'build'))
        with tempfile.TemporaryDirectory() as folder:
            root = Path(folder)
            data = root/'Data'; data.mkdir()
            raw = make_pp([('message.txt', b'hello')])
            (data/'fixture.pp').write_bytes(raw)
            with patch.object(audit_game, 'ROOT', root), \
                    patch.dict(os.environ, {'BK3_BUILD_DIR': str(build)}), \
                    patch.object(sys, 'argv', ['audit_game', str(data)]), \
                    contextlib.redirect_stdout(io.StringIO()):
                audit_game.main()
            report = json.loads((root/'local/audit.json').read_text())
            self.assertEqual(report['entries'], 1)
            self.assertEqual(report['images_pixel_exact'], 0)
            self.assertEqual(report['archives'][0]['sha256'], hashlib.sha256(raw).hexdigest())

    def test_pcm_probe_requires_payload_and_accepts_uppercase_wav(self):
        build = Path(os.environ.get('BK3_BUILD_DIR', audit_pcm.ROOT/'build'))
        with tempfile.TemporaryDirectory() as folder:
            archive = Path(folder)/'test.pp'
            archive.write_bytes(make_pp([]))
            result = subprocess.run([str(build/'pcm-probe'), str(archive)], capture_output=True)
            self.assertNotEqual(result.returncode, 0)
            self.assertNotIn(b'PASS', result.stdout)
            self.assertIn(b'no PCM assets', result.stderr)
            output = io.BytesIO()
            with wave.open(output, 'wb') as wav:
                wav.setparams((1, 2, 22050, 0, 'NONE', 'not compressed'))
                wav.writeframes(struct.pack('<3h', -32768, 0, 32767))
            archive.write_bytes(make_pp([('TONE.WAV', output.getvalue())]))
            result = subprocess.run([str(build/'pcm-probe'), str(archive)],
                                    capture_output=True, check=True)
            self.assertIn(b'PASS PCM 1 files, 3 sample frames,', result.stdout)


if __name__ == '__main__':
    unittest.main()
