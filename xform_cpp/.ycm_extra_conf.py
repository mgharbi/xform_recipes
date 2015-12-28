import os
import ycm_core
from clang_helpers import PrepareClangFlags

# Set this to the absolute path to the folder (NOT the file!) containing the
# compile_commands.json file to use that instead of 'flags'. See here for
# more details: http://clang.llvm.org/docs/JSONCompilationDatabase.html
# Most projects will NOT need to set this to anything; you can just change the
# 'flags' list of compilation flags. Notice that YCM itself uses that approach.
compilation_database_folder = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'build')

print compilation_database_folder

gtest_dir = os.getenv("GTEST_DIR")
halide_dir = os.getenv("HALIDE")
eigen_dir = os.getenv("EIGEN3_INCLUDE_DIR")
android_sdk_dir = os.getenv("ANDROID_HOME")
android_ndk_dir = os.getenv("ANDROID_NDK")

# These are the compilation flags that will be used in case there's no
# compilation database set.
flags = [
'-Wall', '-Wextra', '-Werror',
'-Wno-long-long', '-Wno-variadic-macros',
'-fexceptions',
'-DNDEBUG',

# Language
'-std=c++11',
'-x', 'c++',

# System Includes
# '-isystem',
# '../BoostParts',
'-isystem', '/usr/include',
'-isystem', '/usr/local/include',
'-isystem', '/usr/local/cuda/include',
'-isystem', gtest_dir+'/include/',
'-isystem', halide_dir+'/include/',
'-isystem', eigen_dir,
'-isystem', '/System/Library/Frameworks/JavaVM.framework/Headers',
'-isystem', android_ndk_dir+'/platforms/android-21/arch-arm/usr/include',

# Library Includes

# Project Includes
'-I', 'src',
'-I', 'include',


]

if compilation_database_folder:
    database = ycm_core.CompilationDatabase(compilation_database_folder)
else:
    database = None


def DirectoryOfThisScript():
    return os.path.dirname(os.path.abspath(__file__))


def MakeRelativePathsInFlagsAbsolute(flags, working_directory):
    if not working_directory:
        return flags
    new_flags = []
    make_next_absolute = False
    path_flags = ['-isystem', '-I', '-iquote', '--sysroot=']
    for flag in flags:
        new_flag = flag

        if make_next_absolute:
            make_next_absolute = False
            if not flag.startswith('/'):
                new_flag = os.path.join(working_directory, flag)

        for path_flag in path_flags:
            if flag == path_flag:
                make_next_absolute = True
                break

            if flag.startswith(path_flag):
                path = flag[len(path_flag):]
                new_flag = path_flag + os.path.join(working_directory, path)
                break

        if new_flag:
            new_flags.append(new_flag)
    return new_flags


def FlagsForFile(filename):
    if database:
        # Bear in mind that compilation_info.compiler_flags_ does NOT return a
        # python list, but a "list-like" StringVec object
        compilation_info = database.GetCompilationInfoForFile(filename)
        final_flags = PrepareClangFlags(
            MakeRelativePathsInFlagsAbsolute(
                compilation_info.compiler_flags_,
                compilation_info.compiler_working_dir_),
            filename)
        relative_to = DirectoryOfThisScript()
        final_flags.extend(MakeRelativePathsInFlagsAbsolute(flags, relative_to))
    else:
        relative_to = DirectoryOfThisScript()
        final_flags = MakeRelativePathsInFlagsAbsolute(flags, relative_to)

    return {
        'flags': final_flags,
        'do_cache': True}
