import os
import re
import sys
import platform
import subprocess

from setuptools import setup, find_packages, Extension
from setuptools.command.build_ext import build_ext
from distutils.version import LooseVersion

def maybe_install_c_libs():
    if sys.platform == 'linux':
        cmd = 'sudo apt-get update'
        subprocess.check_call(cmd.split())
        cmd = 'sudo apt-get install libzmq3-dev libzmq5'
        subprocess.check_call(cmd.split())
        '''
        Build latest version 3.1.1 from git using cmake
        apt-get installed old 0.5.7 version, which can't build code.
        see: https://packages.ubuntu.com/source/xenial/msgpack
        '''
        # cmd = 'sudo apt-get install libmsgpack-dev'
        cmd = './requirements.sh'
        subprocess.check_call(cmd.split())

class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=''):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)

#Reference: https://github.com/pybind/cmake_example/blob/master/setup.py
class CMakeBuild(build_ext):
    def run(self):
        try:
            out = subprocess.check_output(['cmake', '--version'])
        except OSError:
            raise RuntimeError(
                "CMake must be installed to build the following extensions: " +
                ", ".join(e.name for e in self.extensions))

        for ext in self.extensions:
            self.build_extension(ext)

    def build_extension(self, ext):
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))
        cmake_args = ['-DCMAKE_LIBRARY_OUTPUT_DIRECTORY=' + extdir,
                    '-DPYTHON_EXECUTABLE=' + sys.executable]
        build_args = []

        if platform.system() != "Windows":
            build_args += ['--', '-j2']

        env = os.environ.copy()
        env['CXXFLAGS'] = '{} -DVERSION_INFO=\\"{}\\"'.format(
            env.get('CXXFLAGS', ''),
            self.distribution.get_version())
        if not os.path.exists(self.build_temp):
            os.makedirs(self.build_temp)
        subprocess.check_call(['cmake', ext.sourcedir] + cmake_args,
                              cwd=self.build_temp, env=env)
        subprocess.check_call(['cmake', '--build', '.'] + build_args,
                              cwd=self.build_temp)
        print()  # Add an empty line for cleaner output.

project_name = 'nvzmq_ops'
__version__ = '0.0.1'

REQUIRED_PACKAGES = [
    'tensorflow >= 1.12.0',
]

maybe_install_c_libs()
setup(
    name=project_name,
    version=__version__,
    description=('The TensorFlow custom zmq op'),
    author='Nvidia',
    install_requires=REQUIRED_PACKAGES,
    packages=['nvzmq_ops'],
    # Set extension name with top_level dir, otherwise it will be copied to pip dist-packages dir.
    # Alternative solution: pre build a nvzmq_ops.so and include it into MANIFEST.in.
    ext_modules=[CMakeExtension('nvzmq_ops/nvzmq_ops')],
    cmdclass=dict(build_ext=CMakeBuild),
    include_package_data=True,
    zip_safe=False,
    license='Apache-2.0',
)