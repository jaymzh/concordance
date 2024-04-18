#!/usr/bin/python3

import os
import shutil
import subprocess
import tempfile
import glob

cp = subprocess.run([
    'mingw-ldd',
    os.path.dirname(os.path.abspath(__file__)) + '/../.libs/concordance.exe',
    '--dll-lookup-dirs',
    os.environ['MINGW_SYSROOT_BIN'],
    os.path.dirname(os.path.abspath(__file__)) + '/../../libconcord/.libs',
], capture_output=True, check=True, text=True)
with tempfile.TemporaryDirectory() as tempdir:
    subdir = os.path.join(tempdir,'libconcord')
    os.mkdir(subdir)
    for line in cp.stdout.splitlines():
        dll = line.split('=>')[1].strip()
        if dll != 'not found':
            shutil.copy2(dll, subdir)
    shutil.copy2(os.path.dirname(os.path.abspath(__file__)) + '/../.libs/concordance.exe', subdir)
    shutil.copy2(os.path.dirname(os.path.abspath(__file__)) + '/../../libconcord/bindings/python/libconcord.py', subdir + '/__init__.py')
    shutil.copy2(os.path.dirname(os.path.abspath(__file__)) + '/../../libconcord/bindings/python/setup.py', tempdir)
    shutil.copy2(os.path.dirname(os.path.abspath(__file__)) + '/../../libconcord/bindings/python/pyproject.toml', tempdir)

    curdir = os.getcwd()
    os.chdir(tempdir)

    os.environ["WIN32WHEEL"]="1"
    subprocess.run([ 'python3', '-m', 'build', '-w' ])
    for file in glob.glob('dist/*.whl'):
        print("Found wheel: " + file)
        shutil.copy2(file, os.path.dirname(os.path.abspath(__file__)))
    os.chdir(curdir)
