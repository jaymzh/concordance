#!/usr/bin/python3

import os
import shutil
import subprocess
import tempfile

cp = subprocess.run([
    'mingw-ldd',
    os.path.dirname(os.path.abspath(__file__)) + '/../.libs/concordance.exe',
    '--dll-lookup-dirs',
    os.environ['MINGW_SYSROOT_BIN'],
    os.path.dirname(os.path.abspath(__file__)) + '/../../libconcord/.libs',
], capture_output=True, check=True, text=True)
with tempfile.TemporaryDirectory() as tempdir:
    for line in cp.stdout.splitlines():
        dll = line.split('=>')[1].strip()
        if dll != 'not found':
            shutil.copy2(dll, tempdir)

    os.environ['BUNDLE_DLL_PATH'] = tempdir
    subprocess.run([
        'makensis',
        os.path.dirname(os.path.abspath(__file__)) + '/concordance.nsi',
    ])
