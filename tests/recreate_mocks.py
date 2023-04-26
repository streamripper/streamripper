#!/usr/bin/python3
import subprocess
from glob import glob
import os

headers = (
        '../lib/debug.h',
        '../lib/mchar.h',
        '../lib/socklib.h',
        )

def create_mockup(header_fpath):
    subprocess.check_call(['ruby', '../vendor/cmock/lib/cmock.rb', '-ocmock_cfg.yml', header])


# remove old mockups
files_to_remove = glob('mocks/*.*')
for file_to_remove in files_to_remove:
    os.unlink(file_to_remove)

# create new mockups
for header in headers:
    create_mockup(header)


