import subprocess
from pathlib import Path


# ---------------------------------------------------------
# PATH CONFIGURATION
# ---------------------------------------------------------

# ParallelZIP/
PROJECT_ROOT = Path(__file__).resolve().parent.parent


# Change this if CMake placed the executable somewhere else.
#
# Common possibilities:
#
# build/parallelzip.exe
# build/Debug/parallelzip.exe

ENGINE_PATH = PROJECT_ROOT / "build" / "parallelzip.exe"


def compress_file(
    input_path: Path,
    thread_count: int
) -> tuple[Path, str]:
    """
    Run the C++ ParallelZip compression engine.

    Returns:
        output_path:
            Location of the generated .pzip archive.

        console_output:
            Text printed by the C++ program. Later we can
            parse this to obtain time, throughput, ratio, etc.
    """

    if not ENGINE_PATH.exists():
        raise FileNotFoundError(
            f"ParallelZip engine not found at: {ENGINE_PATH}"
        )

    # Equivalent terminal command:
    #
    # parallelzip.exe compress file.txt 4

    result = subprocess.run(
        [
            str(ENGINE_PATH),
            "compress",
            str(input_path),
            str(thread_count)
        ],
        capture_output=True,
        text=True
    )

    # Non-zero return code means the C++ program failed.
    if result.returncode != 0:
        raise RuntimeError(
            result.stderr or result.stdout
        )

    # Our engine currently creates:
    #
    # file.txt -> file.txt.pzip

    output_path = Path(
        str(input_path) + ".pzip"
    )

    if not output_path.exists():
        raise RuntimeError(
            "Compression completed but .pzip file was not created."
        )

    return output_path, result.stdout


def decompress_file(
    archive_path: Path,
    thread_count: int
) -> tuple[Path, str]:
    """
    Run the C++ ParallelZip decompression engine.
    """

    if not ENGINE_PATH.exists():
        raise FileNotFoundError(
            f"ParallelZip engine not found at: {ENGINE_PATH}"
        )

    # Equivalent terminal command:
    #
    # parallelzip.exe decompress file.pzip 4

    result = subprocess.run(
        [
            str(ENGINE_PATH),
            "decompress",
            str(archive_path),
            str(thread_count)
        ],
        capture_output=True,
        text=True
    )

    if result.returncode != 0:
        raise RuntimeError(
            result.stderr or result.stdout
        )

    # Current engine naming:
    #
    # file.pzip -> file.pzip.decoded

    output_path = Path(
        str(archive_path) + ".decoded"
    )

    if not output_path.exists():
        raise RuntimeError(
            "Decompression completed but output file was not created."
        )

    return output_path, result.stdout