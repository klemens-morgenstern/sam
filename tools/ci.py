#!/usr/bin/python3


from pathlib import Path
from typing import List, Union
import subprocess
import os
import stat
from shutil import rmtree, copytree, ignore_patterns
import argparse
import sys

_is_windows = os.name == 'nt'
_boost_root = Path(os.path.expanduser('~')).joinpath('boost-root')
_supports_dir_exist_ok = sys.version_info.minor >= 8

def _run(args: List[str]) -> None:
    print('+ ', args, flush=True)
    subprocess.run(args, check=True)


def _add_to_path(path: Path) -> None:
    sep = ';' if _is_windows  else ':'
    os.environ['PATH'] = '{}{}{}'.format(path, sep, os.environ["PATH"])


def _mkdir_and_cd(path: Path) -> None:
    os.makedirs(str(path), exist_ok=True)
    os.chdir(str(path))


def _cmake_bool(value: bool) -> str:
    return 'ON' if value else 'OFF'


def _common_settings(
    boost_root: Path
) -> None:
    _add_to_path(boost_root)


def _remove_readonly(func, path, _):
    os.chmod(path, stat.S_IWRITE)
    func(path)


def _install_boost(
    boost_root: Path,
    source_dir: Path,
    clean: bool = False,
    branch: str = 'develop'
) -> None:
    assert boost_root.is_absolute()
    assert source_dir.is_absolute()
    lib_dir = boost_root.joinpath('libs', 'sam')

    # See if target exists
    if boost_root.exists() and clean:
        rmtree(str(boost_root), onerror=_remove_readonly)
    
    # Clone Boost
    is_clean = not boost_root.exists()
    if is_clean:
        _run(['git', 'clone', '-b', branch, '--depth', '1', 'https://github.com/boostorg/boost.git', str(boost_root)])
    os.chdir(str(boost_root))

    # Put our library inside boost root
    if lib_dir.exists() and (clean or not _supports_dir_exist_ok):
        rmtree(str(lib_dir), onerror=_remove_readonly)
    copytree(
        str(source_dir),
        str(lib_dir),
        ignore=ignore_patterns('__build*__'),
        **({ 'dirs_exist_ok': True } if _supports_dir_exist_ok else {})
    )

    # Install Boost dependencies
    if is_clean:
        _run(["git", "config", "submodule.fetchJobs", "8"])
        _run(["git", "submodule", "update", "-q", "--init", 'tools/boostdep'])
        _run(["python", "tools/boostdep/depinst/depinst.py", "--include", "example", "sam"])

    # Bootstrap
    if is_clean:
        if _is_windows:
            _run(['cmd', '/q', '/c', 'bootstrap.bat'])
        else:
            _run(['bash', 'bootstrap.sh'])
        _run(['b2', 'headers'])


def _b2_build(
    source_dir: Path,
    toolset: str,
    cxxstd: str,
    variant: str,
    stdlib: str = 'native',
    address_model: str = '64',
    clean: bool = False,
    boost_branch: str = 'develop'
) -> None:
    # Get Boost. This leaves us inside boost root
    _install_boost(
        _boost_root,
        source_dir=source_dir,
        clean=clean,
        branch=boost_branch
    )

    # Invoke b2
    _run([
        'b2',
        '--abbreviate-paths',
        'toolset={}'.format(toolset),
        'cxxstd={}'.format(cxxstd),
        'address-model={}'.format(address_model),
        'variant={}'.format(variant),
        'stdlib={}'.format(stdlib),
        'warnings-as-errors=on',
        '-j4',
        'libs/sam/test',
        'libs/sam/example/'
    ])


def _build_prefix_path(*paths: Union[str, Path]) -> str:
    return ';'.join(str(p) for p in paths)


def _cmake_build(
    source_dir: Path,
    generator: str = 'Ninja',
    build_shared_libs: bool = True,
    valgrind: bool = False,
    coverage: bool = False,
    clean: bool = False,
    standalone_tests: bool = True,
    add_subdir_tests: bool = True,
    install_tests: bool = True,
    build_type: str = 'Debug',
    cxxstd: str = '20',
    boost_branch: str = 'develop'
) -> None:
    # Config
    home = Path(os.path.expanduser('~'))
    b2_distro = home.joinpath('b2-distro')
    cmake_distro = home.joinpath('cmake-distro')
    test_folder = _boost_root.joinpath('libs', 'sam', 'test', 'cmake_test')
    os.environ['CMAKE_BUILD_PARALLEL_LEVEL'] = '4'
    if not _is_windows:
        old_ld_libpath = os.environ.get("LD_LIBRARY_PATH", "")
        os.environ['LD_LIBRARY_PATH'] = '{}/lib:{}'.format(b2_distro, old_ld_libpath)

    # Get Boost
    _install_boost(
        _boost_root,
        source_dir,
        clean=clean,
        branch=boost_branch
    )

    # Generate "pre-built" b2 distro
    if standalone_tests:
        _run([
            'b2',
            '--prefix={}'.format(b2_distro),
            '--with-system',
            '--with-context',
            '--with-date_time',
            '--with-test',
            '-d0',
            'cxxstd={}'.format(cxxstd),
            'install'
        ])

        # Don't include our library, as this confuses coverage reports
        if coverage:
            rmtree(b2_distro.joinpath('include', 'boost', 'sam'))

    # Library tests, run from the superproject
    _mkdir_and_cd(_boost_root.joinpath('__build_cmake_test__'))
    _run([
        'cmake',
        '-G',
        generator,
        '-DCMAKE_BUILD_TYPE={}'.format(build_type),
        '-DCMAKE_CXX_STANDARD={}'.format(cxxstd),
        '-DBOOST_INCLUDE_LIBRARIES=sam',
        '-DBUILD_SHARED_LIBS={}'.format(_cmake_bool(build_shared_libs)),
        '-DBUILD_TESTING=ON',
        '-DBoost_VERBOSE=ON',
        '..'
    ])
    _run(['cmake', '--build', '.', '--target', 'tests', '--config', build_type])
    _run(['ctest', '--output-on-failure', '--build-config', build_type])

    # Library tests, using the b2 Boost distribution generated before
    if standalone_tests:
        _mkdir_and_cd(_boost_root.joinpath('libs', 'sam', '__build_standalone__'))
        _run([
            'cmake',
            '-DCMAKE_PREFIX_PATH={}'.format(_build_prefix_path(b2_distro)),
            '-DCMAKE_BUILD_TYPE={}'.format(build_type),
            '-DCMAKE_CXX_STANDARD={}'.format(cxxstd),
            '-G',
            generator,
            '..'
        ])
        _run(['cmake', '--build', '.'])
        _run(['ctest', '--output-on-failure', '--build-config', build_type])
    

    # Subdir tests, using add_subdirectory()
    if add_subdir_tests:
        _mkdir_and_cd(test_folder.joinpath('__build_cmake_subdir_test__'))
        _run([
            'cmake',
            '-G',
            generator,
            '-DBOOST_CI_INSTALL_TEST=OFF',
            '-DCMAKE_BUILD_TYPE={}'.format(build_type),
            '-DBUILD_SHARED_LIBS={}'.format(_cmake_bool(build_shared_libs)),
            '..'
        ])
        _run(['cmake', '--build', '.', '--config', build_type])
        _run(['ctest', '--output-on-failure', '--build-config', build_type])

    # Install the library
    if install_tests:
        _mkdir_and_cd(_boost_root.joinpath('__build_cmake_install_test__'))
        _run([
            'cmake',
            '-G',
            generator,
            '-DCMAKE_BUILD_TYPE={}'.format(build_type),
            '-DBOOST_INCLUDE_LIBRARIES=sam',
            '-DBUILD_SHARED_LIBS={}'.format(_cmake_bool(build_shared_libs)),
            '-DCMAKE_INSTALL_PREFIX={}'.format(cmake_distro),
            '-DBoost_VERBOSE=ON',
            '-DBoost_DEBUG=ON',
            '-DCMAKE_INSTALL_MESSAGE=NEVER',
            '..'
        ])
        _run(['cmake', '--build', '.', '--target', 'install', '--config', build_type])

    # Subdir tests, using find_package with the library installed in the previous step
    if install_tests:
        _mkdir_and_cd(test_folder.joinpath('__build_cmake_install_test__'))
        _run([
            'cmake',
            '-G',
            generator,
            '-DBOOST_CI_INSTALL_TEST=ON',
            '-DCMAKE_BUILD_TYPE={}'.format(build_type),
            '-DBUILD_SHARED_LIBS={}'.format(_cmake_bool(build_shared_libs)),
            '-DCMAKE_PREFIX_PATH={}'.format(_build_prefix_path(cmake_distro)),
            '..'
        ])
        _run(['cmake', '--build', '.', '--config', build_type])
        _run(['ctest', '--output-on-failure', '--build-config', build_type])

    # Subdir tests, using find_package with the b2 distribution.
    # These are incompatible with coverage builds (we rmtree include/boost/<lib>)
    if standalone_tests and not coverage:
        _mkdir_and_cd(_boost_root.joinpath('libs', 'sam', 'test', 'cmake_b2_test', '__build_cmake_b2_test__'))
        _run([
            'cmake',
            '-G',
            generator,
            '-DCMAKE_PREFIX_PATH={}'.format(_build_prefix_path(b2_distro)),
            '-DCMAKE_BUILD_TYPE={}'.format(build_type),
            '-DBUILD_TESTING=ON',
            '..'
        ])
        _run(['cmake', '--build', '.', '--config', build_type])
        _run(['ctest', '--output-on-failure', '--build-config', build_type])

    # Gather coverage data, if available
    if coverage:
        lib_dir = str(_boost_root.joinpath('libs', 'sam'))
        os.chdir(lib_dir)

        # Generate an adequate coverage.info file to upload. Codecov's
        # default is to compute coverage for tests and examples, too, which
        # is not correct
        _run(['lcov', '--capture', '--no-external', '--directory', '.', '-o', 'coverage.info'])
        _run(['lcov', '-o', 'coverage.info', '--extract', 'coverage.info', '**include/boost/sam/**'])

        # Make all file parts rooted at $BOOST_ROOT/libs/sam (required by codecov)
        with open('coverage.info', 'rt') as f:
            lines = [l.replace('SF:{}'.format(lib_dir), 'SF:') for l in f]
        with open('coverage.info', 'wt') as f:
            f.writelines(lines)
        
        # Upload the results
        _run(['codecov', '-Z', '-f', 'coverage.info'])


def _str2bool(v: Union[bool, str]) -> bool:
    if isinstance(v, bool):
        return v
    elif v == '1':
        return True
    elif v == '0':
        return False
    else:
        raise argparse.ArgumentTypeError('Boolean value expected.')


def _deduce_boost_branch() -> str:
    # Are we in GitHub Actions?
    if os.environ.get('GITHUB_ACTIONS') is not None:
        ci = 'GitHub Actions'
        ref = os.environ.get('GITHUB_BASE_REF', '') or os.environ.get('GITHUB_REF', '')
        res = 'master' if ref == 'master' or ref.endswith('/master') else 'develop'
    elif os.environ.get('DRONE') is not None:
        ref = os.environ.get('DRONE_BRANCH', '')
        ci = 'Drone'
        res = 'master' if ref == 'master' else 'develop'
    else:
        ci = 'Unknown'
        ref = ''
        res = 'develop'
    
    print('+  Found CI {}, ref={}, deduced branch {}'.format(ci, ref, res))

    return res


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--build-kind', choices=['b2', 'cmake'], required=True)
    parser.add_argument('--source-dir', type=Path, required=True)
    parser.add_argument('--boost-branch', default=None) # None means "let this script deduce it"
    parser.add_argument('--generator', default='Ninja')
    parser.add_argument('--build-shared-libs', type=_str2bool, default=True)
    parser.add_argument('--valgrind', type=_str2bool, default=False)
    parser.add_argument('--coverage', type=_str2bool, default=False)
    parser.add_argument('--clean', type=_str2bool, default=False)
    parser.add_argument('--cmake-standalone-tests', type=_str2bool, default=True)
    parser.add_argument('--cmake-add-subdir-tests', type=_str2bool, default=True)
    parser.add_argument('--cmake-install-tests', type=_str2bool, default=True)
    parser.add_argument('--cmake-build-type', choices=['Debug', 'Release', 'MinSizeRel'], default='Debug')
    parser.add_argument('--toolset', default='clang')
    parser.add_argument('--cxxstd', default='20')
    parser.add_argument('--variant', default='release')
    parser.add_argument('--stdlib', choices=['native', 'libc++'], default='native')
    parser.add_argument('--address-model', choices=['32', '64'], default='64')

    args = parser.parse_args()

    _common_settings(_boost_root)
    boost_branch = _deduce_boost_branch() if args.boost_branch is None else args.boost_branch

    if args.build_kind == 'b2':
        _b2_build(
            source_dir=args.source_dir,
            toolset=args.toolset,
            cxxstd=args.cxxstd,
            variant=args.variant,
            stdlib=args.stdlib,
            address_model=args.address_model,
            clean=args.clean,
            boost_branch=boost_branch,
        )
    elif args.build_kind == 'cmake':
        _cmake_build(
            source_dir=args.source_dir,
            generator=args.generator,
            build_shared_libs=args.build_shared_libs,
            valgrind=args.valgrind,
            coverage=args.coverage,
            clean=args.clean,
            standalone_tests=args.cmake_standalone_tests,
            add_subdir_tests=args.cmake_add_subdir_tests,
            install_tests=args.cmake_install_tests,
            build_type=args.cmake_build_type,
            cxxstd=args.cxxstd,
            boost_branch=boost_branch,
        )


if __name__ == '__main__':
    main()
