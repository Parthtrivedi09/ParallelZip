from pathlib import Path
import shutil
import tempfile

from fastapi import FastAPI, File, Form, HTTPException, UploadFile
from fastapi.middleware.cors import CORSMiddleware
from fastapi.responses import FileResponse

from backend.engine import compress_file, decompress_file

app = FastAPI(
    title="ParallelZip API",
    description="API for the ParallelZip compression engine",
    version="1.0"
)


# ---------------------------------------------------------
# CORS
# ---------------------------------------------------------
#
# Our React frontend will normally run on port 5173 while
# FastAPI runs on port 8000.
#
# Browsers treat those as different origins, so FastAPI must
# explicitly allow requests from the frontend.

app.add_middleware(
    CORSMiddleware,
    allow_origins=[
        "http://localhost:5173"
    ],
    allow_credentials=True,
    allow_methods=["*"],
    allow_headers=["*"],
)


# ---------------------------------------------------------
# HEALTH CHECK
# ---------------------------------------------------------

@app.get("/")
def root():

    return {
        "message": "ParallelZip API is running"
    }


# ---------------------------------------------------------
# COMPRESS
# ---------------------------------------------------------

@app.post("/compress")
async def compress(
    file: UploadFile = File(...),
    threads: int = Form(4)
):

    if threads < 1:
        raise HTTPException(
            status_code=400,
            detail="Thread count must be greater than zero."
        )

    # Create a temporary directory for this request.
    #
    # This prevents uploaded files from cluttering the
    # project directory.
    temp_directory = Path(
        tempfile.mkdtemp(prefix="parallelzip_")
    )

    try:

        filename = Path(
            file.filename or "uploaded_file"
        ).name

        input_path = (
            temp_directory /
            filename
        )

        # Save uploaded file to disk.
        with input_path.open("wb") as output_file:

            shutil.copyfileobj(
                file.file,
                output_file
            )

        # Call our C++ engine.
        archive_path, console_output = compress_file(
            input_path,
            threads
        )

        # FileResponse sends the generated archive
        # back to the browser.
        return FileResponse(
            path=archive_path,
            filename=filename + ".pzip",
            media_type="application/octet-stream",
            headers={
                "X-ParallelZip-Output":
                    console_output.replace("\n", " | ")
            }
        )

    except Exception as error:

        raise HTTPException(
            status_code=500,
            detail=str(error)
        )


# ---------------------------------------------------------
# DECOMPRESS
# ---------------------------------------------------------

@app.post("/decompress")
async def decompress(
    file: UploadFile = File(...),
    threads: int = Form(4)
):

    if threads < 1:
        raise HTTPException(
            status_code=400,
            detail="Thread count must be greater than zero."
        )

    temp_directory = Path(
        tempfile.mkdtemp(prefix="parallelzip_")
    )

    try:

        filename = Path(
            file.filename or "archive.pzip"
        ).name

        archive_path = (
            temp_directory /
            filename
        )

        with archive_path.open("wb") as output_file:

            shutil.copyfileobj(
                file.file,
                output_file
            )

        output_path, console_output = decompress_file(
            archive_path,
            threads
        )

        # Remove .pzip from the downloaded filename
        # when possible.
        output_filename = filename

        if output_filename.endswith(".pzip"):
            output_filename = output_filename[:-5]

        return FileResponse(
            path=output_path,
            filename=output_filename,
            media_type="application/octet-stream",
            headers={
                "X-ParallelZip-Output":
                    console_output.replace("\n", " | ")
            }
        )

    except Exception as error:

        raise HTTPException(
            status_code=500,
            detail=str(error)
        )




# Terminal 1 — backend:

# uvicorn backend.main:app --reload

# Terminal 2:

# cd frontend
# npm run dev