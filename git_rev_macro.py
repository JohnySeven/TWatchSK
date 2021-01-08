Import("env")
import subprocess

revision = (
    subprocess.check_output(["git", "rev-parse", "HEAD"])
    .strip()
    .decode("utf-8")
)
# General options that are passed to the C++ compiler
env.Append(CXXFLAGS=["-DGIT_REV='\"%s\"'" % revision])