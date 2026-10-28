# CoreModel Peripheral Model Builder Agent

## 1. Prerequisites

- Node.js, version in `.nvmrc`
- git
- A C toolchain: `cc`/`gcc`/`clang`, `make`, `cmake` (3.16+), `ctest`
- `pdftotext` (Poppler) — macOS: `brew install poppler`
- An API key for your model provider (OpenAI, Anthropic, Google, etc.)

## 2. Setup

```bash
git clone <this-repository-url> coremodel-sensor-agent
cd coremodel-sensor-agent
npm install
git clone <your-coremodel-library-url> /path/to/coremodel
cp .env.example .env
```

Edit `.env`:

```bash
MASTRA_MODEL=openai/gpt-5.5
OPENAI_API_KEY=...

COREMODEL_LIBRARY_ROOT=/path/to/coremodel
COREMODEL_OUTPUT_ROOT=/path/to/coremodel-sensor-agent/sensor-models

COREMODEL_BUILD_COMMAND=cmake -S . -B build -DCOREMODEL_DIR=$COREMODEL_LIBRARY_ROOT && cmake --build build && ctest --test-dir build --output-on-failure
```

## 3. Verify

```bash
npm run typecheck
npm run verify:layout
```

## 4. Replicate: generate and build the BME280

Get the datasheet:

```bash
curl -o bst-bme280-ds002.pdf https://www.bosch-sensortec.com/media/boschsensortec/downloads/datasheets/bst-bme280-ds002.pdf
```

Run:

```bash
npm run build -- bst-bme280-ds002.pdf "BME280"
```

Result: `sensor-models/bme280/` — generated source, tests, build files, `README.md`, `GENERATION_REPORT.md`, and `sensor-model.json`, built and tested.

## Other usage

```bash
npm run build -- <path-to-datasheet.pdf> "<sensor or actuator name>"
```
