"""
Because we are using pthreads it generates a worker script that is loaded from a file. 
This script inlines the worker script into the runtime script to comply with `.wex`.
"""

# Adapted from https://github.com/emscripten-core/emscripten/issues/9796#issuecomment-832117267
# @jeffRTC

import sys


def escapeIt(string):
    return string.replace("`", "\\`")


def fixDollarSign(string):
    return string.replace("$", "\$")


def main():
    if len(sys.argv) < 4:
        print(
            "Usage: python inline_worker.py <module-name> <runtime-script> <worker-script>")
        return

    moduleName = sys.argv[1]
    runtimeScriptFilePath = sys.argv[2]
    workerScriptFilePath = sys.argv[3]

    print(f"Inlining {workerScriptFilePath} for module '{moduleName}'")

    runtimeScript = ""
    workerScript = ""

    with open(runtimeScriptFilePath, "r") as f:
        runtimeScript = f.read()

    with open(workerScriptFilePath, "r") as f:
        workerScript = f.read()

    # Escape backticks and dollar signs
    workerScript = escapeIt(workerScript)
    workerScript = fixDollarSign(workerScript)

    # Replace the worker script loading with the inlined worker script
    target = f'locateFile(\'{moduleName}.worker.js\')'
    replace = f'URL.createObjectURL(new Blob([`{workerScript}`], {{ type: "application/javascript" }}))'

    result = runtimeScript.replace(target, replace)

    with open(runtimeScriptFilePath, "w") as f:
        f.write(result)

    print(f"Successfully inlined worker script for module '{moduleName}'")


if __name__ == "__main__":
    main()
