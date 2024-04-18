#!/usr/bin/env python
#
# vim:tw=80:ai:tabstop=4:softtabstop=4:shiftwidth=4:expandtab
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
#

from setuptools import setup
import os

common_name='libconcord'
common_version='1.5'

if os.environ.get("WIN32WHEEL", None) == "1": 
    #Win32 Wheel Option Set
    setup(
        name=common_name,
        version=common_version,
        packages=['libconcord'],
        zip_safe=False,
        package_data={
                '': ['*.dll', '*.exe']
            },
        options={
                "bdist_wheel": {
                    "plat_name": "win32",
                },
            },
    )
else:
    #Default Option Set
    setup(
        name=common_name,
        version=common_version,
        py_modules=['libconcord'],
    )

