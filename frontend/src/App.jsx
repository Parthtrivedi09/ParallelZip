import { useRef, useState } from "react";
import "./App.css";


const API_URL = "http://127.0.0.1:8000";


function formatBytes(bytes) {
    if (bytes === 0) return "0 B";

    const units = ["B", "KB", "MB", "GB"];
    const index = Math.floor(
        Math.log(bytes) / Math.log(1024)
    );

    return `${(bytes / Math.pow(1024, index)).toFixed(2)} ${units[index]}`;
}


function App() {

    const [file, setFile] = useState(null);
    const [mode, setMode] = useState("compress");
    const [threads, setThreads] = useState(4);

    const [loading, setLoading] = useState(false);
    const [dragging, setDragging] = useState(false);

    const [error, setError] = useState("");
    const [result, setResult] = useState(null);

    const fileInputRef = useRef(null);


    // -----------------------------------------------------
    // FILE SELECTION
    // -----------------------------------------------------

    function selectFile(selectedFile) {

        if (!selectedFile) return;

        setFile(selectedFile);

        // Previous results no longer belong to the
        // newly selected file.
        setResult(null);
        setError("");
    }


    function handleDrop(event) {

        event.preventDefault();

        setDragging(false);

        const droppedFile =
            event.dataTransfer.files[0];

        selectFile(droppedFile);
    }


    function removeFile(event) {

        event.stopPropagation();

        setFile(null);
        setResult(null);
        setError("");

        if (fileInputRef.current) {
            fileInputRef.current.value = "";
        }
    }


    // -----------------------------------------------------
    // MODE
    // -----------------------------------------------------

    function changeMode(newMode) {

        setMode(newMode);

        setResult(null);
        setError("");
    }


    // -----------------------------------------------------
    // PROCESS FILE
    // -----------------------------------------------------

    async function processFile() {

        if (!file) {
            setError("Select a file before continuing.");
            return;
        }

        setLoading(true);
        setError("");
        setResult(null);

        const startTime = performance.now();

        try {

            const formData =
                new FormData();

            formData.append(
                "file",
                file
            );

            formData.append(
                "threads",
                threads.toString()
            );


            const response =
                await fetch(
                    `${API_URL}/${mode}`,
                    {
                        method: "POST",
                        body: formData
                    }
                );


            if (!response.ok) {

                let message =
                    "The operation failed.";

                try {

                    const data =
                        await response.json();

                    message =
                        data.detail || message;

                }
                catch {
                    // Response was not JSON.
                }

                throw new Error(message);
            }


            const blob =
                await response.blob();


            const endTime =
                performance.now();


            const elapsedSeconds =
                (endTime - startTime) / 1000;


            // Throughput measured from the complete
            // browser request duration.
            const throughput =
                elapsedSeconds > 0
                    ? (
                        file.size /
                        (1024 * 1024)
                    ) / elapsedSeconds
                    : 0;


            let outputName;

            if (mode === "compress") {

                outputName =
                    `${file.name}.pzip`;

            }
            else {

                outputName =
                    file.name.endsWith(".pzip")
                        ? file.name.slice(0, -5)
                        : `${file.name}.decoded`;
            }


            // Create browser URL for the returned file.
            const downloadUrl =
                URL.createObjectURL(blob);


            const compressedRatio =
                file.size > 0
                    ? blob.size / file.size
                    : 0;


            const spaceSaved =
                mode === "compress" &&
                file.size > 0
                    ? (
                        1 -
                        blob.size / file.size
                    ) * 100
                    : null;


            setResult({
                outputName,
                downloadUrl,

                originalSize: file.size,
                outputSize: blob.size,

                elapsedSeconds,
                throughput,

                compressionRatio:
                    mode === "compress"
                        ? compressedRatio
                        : null,

                spaceSaved,

                threads
            });

        }
        catch (err) {

            setError(
                err.message ||
                "Something went wrong."
            );
        }
        finally {

            setLoading(false);
        }
    }


    // -----------------------------------------------------
    // DOWNLOAD
    // -----------------------------------------------------

    function downloadResult() {

        if (!result) return;

        const link =
            document.createElement("a");

        link.href =
            result.downloadUrl;

        link.download =
            result.outputName;

        document.body.appendChild(link);

        link.click();

        link.remove();
    }


    return (

        <div className="app">

            {/* =================================================
                NAVBAR
            ================================================= */}

            <nav className="navbar">

                <div className="nav-content">

                    <div className="brand">

                        <div className="brand-icon">
                            PZ
                        </div>

                        <div>
                            <div className="brand-name">
                                ParallelZip
                            </div>

                            <div className="brand-subtitle">
                                Compression Engine
                            </div>
                        </div>

                    </div>


                    <div className="nav-badge">

                        <span className="status-dot"></span>

                        Engine Ready

                    </div>

                </div>

            </nav>


            {/* =================================================
                MAIN
            ================================================= */}

            <main className="main">

                {/* HERO */}

                <section className="hero">

                    <div className="eyebrow">
                        MULTITHREADED • LOSSLESS • C++17
                    </div>

                    <h1>
                        Compress files.
                        <br />

                        <span>
                            Use every core.
                        </span>
                    </h1>

                    <p>
                        A multithreaded Huffman compression
                        engine with parallel chunk processing,
                        CRC32 integrity verification and a
                        custom binary archive format.
                    </p>

                </section>


                {/* =================================================
                    MAIN CARD
                ================================================= */}

                <section className="workspace">

                    {/* MODE TABS */}

                    <div className="mode-tabs">

                        <button
                            className={
                                mode === "compress"
                                    ? "mode active"
                                    : "mode"
                            }
                            onClick={() =>
                                changeMode("compress")
                            }
                        >
                            Compress
                        </button>


                        <button
                            className={
                                mode === "decompress"
                                    ? "mode active"
                                    : "mode"
                            }
                            onClick={() =>
                                changeMode("decompress")
                            }
                        >
                            Decompress
                        </button>

                    </div>


                    <div className="workspace-body">

                        {/* -----------------------------------------
                            FILE DROP ZONE
                        ------------------------------------------ */}

                        <div className="section-heading">

                            <div>
                                <h2>
                                    {
                                        mode === "compress"
                                            ? "Choose a file"
                                            : "Choose a PZIP archive"
                                    }
                                </h2>

                                <p>
                                    {
                                        mode === "compress"
                                            ? "Files are processed as raw binary data."
                                            : "Select an archive created by ParallelZip."
                                    }
                                </p>
                            </div>

                        </div>


                        <div
                            className={
                                dragging
                                    ? "drop-zone dragging"
                                    : file
                                        ? "drop-zone selected"
                                        : "drop-zone"
                            }

                            onClick={() =>
                                fileInputRef.current?.click()
                            }

                            onDragOver={(event) => {
                                event.preventDefault();
                                setDragging(true);
                            }}

                            onDragLeave={() =>
                                setDragging(false)
                            }

                            onDrop={handleDrop}
                        >

                            <input
                                ref={fileInputRef}

                                type="file"

                                hidden

                                accept={
                                    mode === "decompress"
                                        ? ".pzip"
                                        : undefined
                                }

                                onChange={(event) =>
                                    selectFile(
                                        event.target.files[0]
                                    )
                                }
                            />


                            {!file ? (

                                <>

                                    <div className="upload-icon">
                                        ↑
                                    </div>

                                    <h3>
                                        Drop your file here
                                    </h3>

                                    <p>
                                        or click to browse
                                        from your computer
                                    </p>

                                </>

                            ) : (

                                <div className="selected-file">

                                    <div className="file-icon">
                                        FILE
                                    </div>


                                    <div className="file-details">

                                        <strong>
                                            {file.name}
                                        </strong>

                                        <span>
                                            {formatBytes(file.size)}
                                        </span>

                                    </div>


                                    <button
                                        className="remove-file"
                                        onClick={removeFile}
                                        title="Remove file"
                                    >
                                        ×
                                    </button>

                                </div>

                            )}

                        </div>


                        {/* -----------------------------------------
                            THREAD SELECTOR
                        ------------------------------------------ */}

                        <div className="thread-section">

                            <div className="section-heading">

                                <div>
                                    <h2>
                                        Worker threads
                                    </h2>

                                    <p>
                                        Choose how many C++ workers
                                        process chunks concurrently.
                                    </p>
                                </div>


                                <div className="thread-value">
                                    {threads} threads
                                </div>

                            </div>


                            <div className="thread-grid">

                                {[1, 2, 4, 8].map(
                                    (number) => (

                                        <button
                                            key={number}

                                            className={
                                                threads === number
                                                    ? "thread-option active"
                                                    : "thread-option"
                                            }

                                            onClick={() =>
                                                setThreads(number)
                                            }
                                        >

                                            <strong>
                                                {number}
                                            </strong>

                                            <span>
                                                {
                                                    number === 1
                                                        ? "Single"
                                                        : number <= 4
                                                            ? "Balanced"
                                                            : "Maximum"
                                                }
                                            </span>

                                        </button>

                                    )
                                )}

                            </div>

                        </div>


                        {/* ERROR */}

                        {error && (

                            <div className="error-message">

                                <strong>
                                    Operation failed
                                </strong>

                                <span>
                                    {error}
                                </span>

                            </div>

                        )}


                        {/* PROCESS BUTTON */}

                        <button
                            className="primary-button"

                            disabled={
                                !file ||
                                loading
                            }

                            onClick={
                                processFile
                            }
                        >

                            {loading ? (

                                <>
                                    <span className="spinner"></span>

                                    Processing...
                                </>

                            ) : (

                                mode === "compress"
                                    ? "Compress File"
                                    : "Decompress Archive"

                            )}

                        </button>

                    </div>

                </section>


                {/* =================================================
                    RESULTS
                ================================================= */}

                {result && (

                    <section className="results">

                        <div className="result-header">

                            <div>

                                <div className="success-label">
                                    ✓ OPERATION COMPLETE
                                </div>

                                <h2>
                                    {
                                        mode === "compress"
                                            ? "Compression complete"
                                            : "Decompression complete"
                                    }
                                </h2>

                            </div>


                            <button
                                className="download-button"
                                onClick={downloadResult}
                            >
                                Download
                            </button>

                        </div>


                        <div className="metrics">

                            <Metric
                                label="Input Size"
                                value={
                                    formatBytes(
                                        result.originalSize
                                    )
                                }
                            />


                            <Metric
                                label="Output Size"
                                value={
                                    formatBytes(
                                        result.outputSize
                                    )
                                }
                            />


                            <Metric
                                label="Time"
                                value={
                                    `${result.elapsedSeconds.toFixed(2)} s`
                                }
                            />


                            <Metric
                                label="Throughput"
                                value={
                                    `${result.throughput.toFixed(1)} MB/s`
                                }
                            />


                            <Metric
                                label="Threads"
                                value={
                                    result.threads
                                }
                            />


                            {mode === "compress" && (

                                <Metric
                                    label="Space Saved"
                                    value={
                                        `${result.spaceSaved.toFixed(1)}%`
                                    }

                                    warning={
                                        result.spaceSaved < 0
                                    }
                                />

                            )}

                        </div>


                        {mode === "compress" &&
                            result.spaceSaved < 0 && (

                            <div className="compression-warning">

                                This file became slightly larger.
                                The input may already contain
                                compressed data such as JPEG,
                                PNG, MP4, ZIP or PDF content.

                            </div>

                        )}


                        <div className="output-file">

                            <div>

                                <span>
                                    Output
                                </span>

                                <strong>
                                    {result.outputName}
                                </strong>

                            </div>


                            <span>
                                {formatBytes(result.outputSize)}
                            </span>

                        </div>

                    </section>

                )}


                {/* =================================================
                    ENGINE INFO
                ================================================= */}

                <section className="engine-section">

                    <div className="section-title">

                        <span>
                            UNDER THE HOOD
                        </span>

                        <h2>
                            Built from scratch.
                        </h2>

                        <p>
                            ParallelZip combines compression,
                            systems programming and concurrency
                            in one pipeline.
                        </p>

                    </div>


                    <div className="feature-grid">

                        <Feature
                            number="01"
                            title="Huffman Coding"
                            description="Variable-length prefix codes reduce the representation of frequently occurring bytes."
                        />

                        <Feature
                            number="02"
                            title="Parallel Chunking"
                            description="Files are divided into independent chunks that can be processed concurrently."
                        />

                        <Feature
                            number="03"
                            title="Thread Pool"
                            description="Persistent C++ worker threads consume compression tasks from a synchronized queue."
                        />

                        <Feature
                            number="04"
                            title="CRC32 Integrity"
                            description="Every decompressed file is checked against its original checksum for corruption."
                        />

                    </div>

                </section>


                {/* PIPELINE */}

                <section className="pipeline">

                    <span className="pipeline-title">
                        COMPRESSION PIPELINE
                    </span>


                    <div className="pipeline-flow">

                        <PipelineStep
                            number="01"
                            text="Binary Input"
                        />

                        <Arrow />

                        <PipelineStep
                            number="02"
                            text="Chunk"
                        />

                        <Arrow />

                        <PipelineStep
                            number="03"
                            text="Thread Pool"
                        />

                        <Arrow />

                        <PipelineStep
                            number="04"
                            text="Huffman"
                        />

                        <Arrow />

                        <PipelineStep
                            number="05"
                            text=".pzip"
                        />

                    </div>

                </section>

            </main>


            {/* =================================================
                FOOTER
            ================================================= */}

            <footer className="footer">

                <div>

                    <strong>
                        ParallelZip
                    </strong>

                    <span>
                        C++17 / Huffman / Multithreading /
                        CRC32 / FastAPI / React
                    </span>

                </div>

            </footer>

        </div>
    );
}


// =============================================================
// SMALL COMPONENTS
// =============================================================

function Metric({
    label,
    value,
    warning = false
}) {

    return (

        <div className="metric">

            <span>
                {label}
            </span>

            <strong className={
                warning
                    ? "warning-value"
                    : ""
            }>
                {value}
            </strong>

        </div>
    );
}


function Feature({
    number,
    title,
    description
}) {

    return (

        <div className="feature">

            <span className="feature-number">
                {number}
            </span>

            <h3>
                {title}
            </h3>

            <p>
                {description}
            </p>

        </div>
    );
}


function PipelineStep({
    number,
    text
}) {

    return (

        <div className="pipeline-step">

            <span>
                {number}
            </span>

            <strong>
                {text}
            </strong>

        </div>
    );
}


function Arrow() {

    return (
        <div className="arrow">
            →
        </div>
    );
}


export default App;