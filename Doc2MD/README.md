# Doc2MD

A command-line tool that converts **PDF** and **EPUB** files to clean
Markdown, so books, papers, and manuals are easy to read, search, diff, or
feed to an LLM.

## Install

Requires Python 3.9+.

```sh
pip install -r requirements.txt
```

## Usage

```sh
python doc2md.py book.epub paper.pdf          # writes book.md and paper.md next to the inputs
python doc2md.py ~/Books -r -o ~/Books-md      # convert a whole folder tree into one output folder
python doc2md.py paper.pdf --stdout | less     # print instead of writing a file
```

| Option | Effect |
| --- | --- |
| `-o, --out-dir DIR` | Write `.md` files into `DIR` instead of next to each input |
| `-r, --recursive` | Search directories recursively |
| `-f, --force` | Overwrite existing `.md` files (they are skipped by default) |
| `--stdout` | Print the Markdown instead of writing files |
| `--page-markers` | PDF: insert `<!-- page N -->` comments at page breaks |
| `--no-tables` | PDF: skip table detection (faster on long documents) |
| `--no-images` | EPUB: drop `[Image: alt text]` placeholders |

If `book.pdf` and `book.epub` are converted in the same run, the second one
is written as `book.epub.md` (or `book.pdf.md`) so neither overwrites the other.

## What it does

**EPUB**: reads chapters in spine (reading) order and converts the XHTML to
Markdown. Headings, emphasis, lists, tables, block quotes and external links
are kept. Internal cross-reference links are unwrapped to plain text,
footnote markers become `^1`, and images become `[Image: alt text]`. The book
title and author go at the top, and chapters are separated by `---`.

**PDF**: rebuilds structure from the text layer:

- larger font sizes become `#`/`##`/`###` headings, and short bold lines become headings too
- wrapped lines are joined back into paragraphs, including paragraphs split across pages
- end-of-line hyphenation is undone (`extra-` + `ordinary` becomes `extraordinary`)
- running headers, footers and page numbers are removed
- bullet and numbered lists become Markdown lists
- ruled tables become Markdown tables
- two-column layouts are read left column first, then right

## Limitations

- **Scanned PDFs** have no text layer. Run OCR first (for example
  `ocrmypdf in.pdf out.pdf`) and then convert the result.
- **PDF structure is inferred** from fonts and positions, so unusual layouts
  can produce odd headings or paragraph breaks. Math, footnotes and
  sidebars come through as plain text.
- **DRM-protected EPUBs** cannot be read.

## Tests

```sh
pip install reportlab   # used to generate test PDFs
python -m unittest discover -s tests
```
