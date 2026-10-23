#!/usr/bin/env python3
"""doc2md - convert PDF and EPUB files to Markdown.

Usage:
    python doc2md.py book.epub paper.pdf
    python doc2md.py ~/Books -r -o ~/Books-md

Each input file produces one .md file. PDFs are rebuilt from the text layer:
font sizes become headings, lines are joined into paragraphs, hyphenation is
undone, running headers/footers and page numbers are dropped, and tables are
rendered as Markdown tables. EPUBs are read in spine (reading) order and their
XHTML is converted chapter by chapter.
"""

from __future__ import annotations

import argparse
import posixpath
import re
import statistics
import sys
import zipfile
from collections import Counter
from dataclasses import dataclass, field
from pathlib import Path
from urllib.parse import unquote
from xml.etree import ElementTree as ET

SUPPORTED = {".pdf", ".epub"}
BULLET_RE = re.compile(r"^[•▪●◦‣⁃∙·■□*]\s*")
ORDERED_RE = re.compile(r"^(\(?\d{1,3}[.)]|\(?[a-zA-Z][.)])\s+")
CID_RE = re.compile(r"\(cid:\d+\)")
PAGE_NUM_RE = re.compile(r"^(page\s+)?\d{1,4}(\s*(/|of)\s*\d{1,4})?$|^[ivxlcdm]{1,7}$", re.I)


def tidy(md: str) -> str:
    """Normalize whitespace: strip trailing spaces, collapse blank runs."""
    md = md.replace(" ", " ").replace("​", "")
    md = re.sub(r"[ \t]+\n", "\n", md)
    md = re.sub(r"\n{3,}", "\n\n", md)
    return md.strip() + "\n"


# --------------------------------------------------------------------------
# PDF
# --------------------------------------------------------------------------

@dataclass
class Line:
    text: str
    size: float
    bold: bool
    x0: float
    x1: float
    top: float
    bottom: float
    col: int = 0  # reading-order region: 0 above columns, 1 left, 2 right, 3 below

    @property
    def key(self) -> tuple[int, float]:
        return (self.col, self.top)


@dataclass
class PageData:
    width: float
    height: float
    lines: list[Line] = field(default_factory=list)
    # (sort key, markdown) blocks such as tables, merged with text by position
    blocks: list[tuple[tuple[int, float], str]] = field(default_factory=list)
    gutter: float | None = None  # x of the column gap on two-column pages


def _words_to_lines(words: list[dict]) -> list[Line]:
    """Group pdfplumber words into visual lines."""
    words = sorted(words, key=lambda w: (round(w["top"]), w["x0"]))
    rows: list[list[dict]] = []
    for w in words:
        if rows:
            row = rows[-1]
            ref = row[0]
            height = max(ref["bottom"] - ref["top"], 1)
            if abs(w["top"] - ref["top"]) <= height * 0.5:
                row.append(w)
                continue
        rows.append([w])

    lines = []
    for row in rows:
        row.sort(key=lambda w: w["x0"])
        # Unmapped glyphs come through as "(cid:N)"; at line start they are
        # almost always bullets, elsewhere they are unreadable noise.
        if row[0]["text"].startswith("(cid:"):
            row[0]["text"] = CID_RE.sub("\u2022", row[0]["text"], count=1)
        for w in row:
            w["text"] = CID_RE.sub("", w["text"])
        row = [w for w in row if w["text"]]
        if not row:
            continue
        parts = [row[0]["text"]]
        for prev, cur in zip(row, row[1:]):
            gap = cur["x0"] - prev["x1"]
            # Large horizontal gaps are column-like separations; keep a space.
            parts.append(("" if gap < 0.5 else " ") + cur["text"])
        sizes = [w.get("size", 0) for w in row]
        fonts = [w.get("fontname", "") for w in row]
        bold = sum("bold" in f.lower() or "black" in f.lower() or "heavy" in f.lower()
                   for f in fonts) > len(fonts) / 2
        lines.append(Line(
            text="".join(parts).strip(),
            size=round(statistics.median(sizes), 1) if sizes else 0,
            bold=bold,
            x0=min(w["x0"] for w in row),
            x1=max(w["x1"] for w in row),
            top=min(w["top"] for w in row),
            bottom=max(w["bottom"] for w in row),
        ))
    return [ln for ln in lines if ln.text]


def _find_gutter(words: list[dict], width: float) -> float | None:
    """Return the x of an empty vertical strip splitting two text columns."""
    if len(words) < 40:
        return None
    lo, hi = int(width * 0.3), int(width * 0.7)
    covered = [0] * (hi - lo)
    for w in words:
        for x in range(max(lo, int(w["x0"])), min(hi, int(w["x1"]) + 1)):
            covered[x - lo] += 1
    # A few full-width lines (titles, abstracts) may cross the gutter.
    limit = max(2, len(words) // 200)
    best, run_start = None, None
    for i, c in enumerate(covered + [limit + 1]):
        if c <= limit:
            run_start = i if run_start is None else run_start
        elif run_start is not None:
            if i - run_start >= 6 and (best is None or i - run_start > best[1] - best[0]):
                best = (run_start, i)
            run_start = None
    if not best:
        return None
    gutter = lo + (best[0] + best[1]) / 2
    left = sum(w["x1"] < gutter for w in words)
    right = sum(w["x0"] > gutter for w in words)
    return gutter if min(left, right) > len(words) * 0.2 else None


def _column_lines(words: list[dict], gutter: float | None) -> list[Line]:
    """Build lines, reading a two-column page left column first."""
    if gutter is None:
        return _words_to_lines(words)
    left = [w for w in words if w["x1"] <= gutter]
    right = [w for w in words if w["x0"] >= gutter]
    span = [w for w in words if w["x0"] < gutter < w["x1"]]
    lines = []
    col_top = min((w["top"] for w in left + right), default=0)
    for ln in _words_to_lines(span):
        ln.col = 0 if ln.top < col_top else 3
        lines.append(ln)
    for col, group in ((1, left), (2, right)):
        for ln in _words_to_lines(group):
            ln.col = col
            lines.append(ln)
    return sorted(lines, key=lambda ln: ln.key)


def _table_to_md(rows: list[list]) -> str | None:
    rows = [[re.sub(r"\s+", " ", (c or "")).strip().replace("|", "\\|") for c in r]
            for r in rows if r and any((c or "").strip() for c in r)]
    if len(rows) < 2 or max(len(r) for r in rows) < 2:
        return None
    width = max(len(r) for r in rows)
    rows = [r + [""] * (width - len(r)) for r in rows]
    out = ["| " + " | ".join(rows[0]) + " |", "|" + " --- |" * width]
    out += ["| " + " | ".join(r) + " |" for r in rows[1:]]
    return "\n".join(out)


def _in_bbox(word: dict, bbox: tuple) -> bool:
    x0, top, x1, bottom = bbox
    cx = (word["x0"] + word["x1"]) / 2
    cy = (word["top"] + word["bottom"]) / 2
    return x0 <= cx <= x1 and top <= cy <= bottom


def _read_pdf_pages(path: Path, tables: bool) -> tuple[list[PageData], dict]:
    import pdfplumber

    pages = []
    with pdfplumber.open(path) as pdf:
        meta = pdf.metadata or {}
        for page in pdf.pages:
            data = PageData(width=float(page.width), height=float(page.height))
            words = page.extract_words(extra_attrs=["size", "fontname"],
                                       keep_blank_chars=False, use_text_flow=False)
            data.gutter = _find_gutter(words, data.width)
            if tables:
                try:
                    found = page.find_tables()
                except Exception:
                    found = []
                for t in found:
                    md = _table_to_md(t.extract())
                    if md:
                        col = 0
                        if data.gutter is not None:
                            col = 1 if t.bbox[2] <= data.gutter else 2 if t.bbox[0] >= data.gutter else 0
                        data.blocks.append(((col, t.bbox[1]), md))
                        words = [w for w in words if not _in_bbox(w, t.bbox)]
            data.lines = _column_lines(words, data.gutter)
            pages.append(data)
            page.flush_cache()
    return pages, meta


def _norm_repeat(text: str) -> str:
    return re.sub(r"\d+", "#", text.lower()).strip()


def _strip_headers_footers(pages: list[PageData]) -> None:
    """Remove running heads/feet that repeat across pages, and page numbers."""
    margin = 0.09
    counts: Counter[str] = Counter()
    for p in pages:
        seen = set()
        for ln in p.lines:
            if ln.top < p.height * margin or ln.bottom > p.height * (1 - margin):
                seen.add(_norm_repeat(ln.text))
        counts.update(seen)
    threshold = max(3, len(pages) * 0.4)
    repeated = {t for t, c in counts.items() if c >= threshold}

    for p in pages:
        kept = []
        for ln in p.lines:
            in_margin = ln.top < p.height * margin or ln.bottom > p.height * (1 - margin)
            if in_margin and (_norm_repeat(ln.text) in repeated
                              or PAGE_NUM_RE.match(ln.text.strip())):
                continue
            kept.append(ln)
        p.lines = kept


def _body_size(pages: list[PageData]) -> float:
    weights: Counter[float] = Counter()
    for p in pages:
        for ln in p.lines:
            weights[ln.size] += len(ln.text)
    return weights.most_common(1)[0][0] if weights else 10.0


def _heading_levels(pages: list[PageData], body: float) -> dict[float, int]:
    sizes: Counter[float] = Counter()
    for p in pages:
        for ln in p.lines:
            if ln.size >= body * 1.15 and len(ln.text) < 150:
                sizes[ln.size] += 1
    # Ignore one-off giant sizes only if there are many heading sizes; keep
    # the largest few as H1..H3 and lump the rest into H4.
    ordered = sorted(sizes, reverse=True)
    return {s: min(i + 1, 4) for i, s in enumerate(ordered)}


def _join(prev: str, nxt: str) -> str:
    """Join two wrapped lines, undoing end-of-line hyphenation."""
    if re.search(r"[A-Za-z]-$", prev) and nxt[:1].isalpha():
        return prev[:-1] + nxt if nxt[:1].islower() else prev + nxt
    return prev + " " + nxt


def _list_marker(text: str) -> str | None:
    if BULLET_RE.match(text):
        return "- " + BULLET_RE.sub("", text)
    if re.match(r"^[-–—]\s+\S", text):
        return "- " + text[1:].lstrip()
    m = ORDERED_RE.match(text)
    if m:
        return text
    return None


def _page_to_md(page: PageData, body: float, levels: dict[float, int]) -> list[tuple[tuple, str]]:
    """Turn one page's lines into (sort key, markdown block) pairs."""
    blocks: list[tuple[tuple, str]] = []
    if not page.lines:
        return blocks

    gaps = [b.top - a.bottom for a, b in zip(page.lines, page.lines[1:])
            if a.col == b.col and a.size == body and b.size == body
            and 0 <= b.top - a.bottom < body * 3]
    normal_gap = statistics.median(gaps) if gaps else body * 0.4
    # Per-column left/right text edges, for indent and short-line checks.
    edges = {}
    for col in {ln.col for ln in page.lines}:
        col_lines = [ln for ln in page.lines if ln.col == col]
        body_x0 = [ln.x0 for ln in col_lines if ln.size == body] or [col_lines[0].x0]
        edges[col] = (statistics.median(body_x0), max(ln.x1 for ln in col_lines))

    para: list[str] = []
    para_key: tuple = (0, 0.0)
    kind = "p"  # "p" or "li"

    def flush():
        nonlocal para, kind
        if para:
            text = para[0]
            for nxt in para[1:]:
                text = _join(text, nxt)
            blocks.append((para_key, text))
        para = []
        kind = "p"

    prev: Line | None = None
    for ln in page.lines:
        text = ln.text
        left, right = edges[ln.col]
        # A column change reads like a line wrap; it is not a vertical gap.
        dy = ln.top - prev.bottom if prev is not None and prev.col == ln.col else 0
        level = levels.get(ln.size)
        is_heading = level is not None or (
            ln.bold and ln.size >= body and len(text) < 80
            and not text.endswith((".", ",", ";", ":")) and prev is not None
            and dy > normal_gap * 1.5)

        if is_heading:
            # Merge multi-line headings of the same size.
            if blocks and prev is not None and not para and prev.size == ln.size \
                    and blocks[-1][1].startswith("#") and 0 <= dy < body:
                top, md = blocks[-1]
                blocks[-1] = (top, md + " " + text)
            else:
                flush()
                blocks.append((ln.key, "#" * (level or 4) + " " + text))
            prev = ln
            continue

        item = _list_marker(text)
        new_para = (
            not para
            or item is not None
            or dy > normal_gap * 1.6 + 1
            or (prev is not None and prev.size != ln.size)
            or (kind == "p" and ln.x0 > left + body * 0.8)  # first-line indent
            or (prev is not None and prev.col == ln.col and prev.x1 < right - body * 4
                and prev.text.endswith((".", "!", "?", ":", "”", '"'))))
        if kind == "li" and item is None and prev is not None \
                and dy <= normal_gap * 1.6 + 1 and ln.x0 > left:
            new_para = False  # wrapped continuation of a list item
        if new_para:
            flush()
            para_key = ln.key
            kind = "li" if item is not None else "p"
        para.append(item if item is not None else text)
        prev = ln
    flush()
    return blocks


def _merge_across_pages(chunks: list[str]) -> list[str]:
    """Join a paragraph split by a page break."""
    out: list[str] = []
    for c in chunks:
        if (out and not out[-1].startswith(("#", "|", "- ", "<!--"))
                and not c.startswith(("#", "|", "- ", "<!--"))
                and not re.search(r"[.!?:”\")]$", out[-1])
                and c[:1].islower()):
            out[-1] = _join(out[-1], c)
        else:
            out.append(c)
    return out


def pdf_to_markdown(path: Path, tables: bool = True, page_markers: bool = False) -> str:
    pages, meta = _read_pdf_pages(path, tables)
    if not any(p.lines or p.blocks for p in pages):
        raise ValueError("no extractable text (scanned PDF? run OCR first, e.g. ocrmypdf)")
    _strip_headers_footers(pages)
    body = _body_size(pages)
    levels = _heading_levels(pages, body)

    chunks: list[str] = []
    for n, page in enumerate(pages, 1):
        if page_markers:
            chunks.append(f"<!-- page {n} -->")
        items = _page_to_md(page, body, levels) + page.blocks
        items.sort(key=lambda b: b[0])
        chunks.extend(md for _, md in items)
    chunks = _merge_across_pages(chunks)

    title = (meta.get("Title") or "").strip()
    header = ""
    if title and not any(c.startswith("# ") for c in chunks[:5]):
        header = f"# {title}\n\n"
        author = (meta.get("Author") or "").strip()
        if author:
            header += f"*{author}*\n\n"
    return tidy(header + "\n\n".join(chunks))


# --------------------------------------------------------------------------
# EPUB
# --------------------------------------------------------------------------

def _xml_find(root: ET.Element, local: str) -> list[ET.Element]:
    return [el for el in root.iter() if el.tag.rsplit("}", 1)[-1] == local]


def _epub_layout(zf: zipfile.ZipFile):
    container = ET.fromstring(zf.read("META-INF/container.xml"))
    opf_path = _xml_find(container, "rootfile")[0].get("full-path")
    opf = ET.fromstring(zf.read(opf_path))
    base = posixpath.dirname(opf_path)

    def meta(name):
        els = _xml_find(opf, name)
        return " & ".join(e.text.strip() for e in els if e.text and e.text.strip())

    manifest = {}
    for item in _xml_find(opf, "item"):
        href = posixpath.normpath(posixpath.join(base, unquote(item.get("href", ""))))
        manifest[item.get("id")] = (href, item.get("media-type", ""))
    spine = []
    for ref in _xml_find(opf, "itemref"):
        entry = manifest.get(ref.get("idref"))
        if entry and ("html" in entry[1] or entry[0].endswith((".htm", ".html", ".xhtml"))):
            spine.append(entry[0])
    return meta("title"), meta("creator"), spine


def _html_to_md(html: bytes, images: bool) -> str:
    from bs4 import BeautifulSoup
    from markdownify import markdownify

    soup = BeautifulSoup(html, "lxml") if _has_lxml() else BeautifulSoup(html, "html.parser")
    body = soup.body or soup
    for tag in body.find_all(["script", "style", "svg", "head"]):
        tag.decompose()
    for img in body.find_all("img"):
        alt = (img.get("alt") or "").strip()
        if images and alt:
            img.replace_with(f"[Image: {alt}]")
        else:
            img.decompose()
    for a in body.find_all("a"):
        href = a.get("href", "")
        if not re.match(r"^(https?|mailto):", href):
            # Internal cross-references are meaningless outside the book.
            if a.get_text(strip=True):
                a.unwrap()
            else:
                a.decompose()
    # Footnote markers like <sup>12</sup> read better inline as ^12.
    for sup in body.find_all("sup"):
        sup.replace_with(f"^{sup.get_text(strip=True)}")

    md = markdownify(str(body), heading_style="ATX", bullets="-",
                     strip=["span", "div", "section", "body", "html"])
    return md


_LXML = None


def _has_lxml() -> bool:
    global _LXML
    if _LXML is None:
        try:
            import lxml  # noqa: F401
            _LXML = True
        except ImportError:
            _LXML = False
    return _LXML


def epub_to_markdown(path: Path, images: bool = True) -> str:
    with zipfile.ZipFile(path) as zf:
        title, author, spine = _epub_layout(zf)
        names = set(zf.namelist())
        chapters = []
        for href in spine:
            if href not in names:
                continue
            md = tidy(_html_to_md(zf.read(href), images))
            if md.strip():
                chapters.append(md)
    if not chapters:
        raise ValueError("no readable chapters found")
    header = ""
    if title:
        header = f"# {title}\n\n" + (f"*{author}*\n\n" if author else "")
    return tidy(header + "\n\n---\n\n".join(chapters))


# --------------------------------------------------------------------------
# CLI
# --------------------------------------------------------------------------

def collect(inputs: list[str], recursive: bool) -> list[Path]:
    files = []
    for raw in inputs:
        p = Path(raw).expanduser()
        if p.is_dir():
            pattern = "**/*" if recursive else "*"
            files += sorted(f for f in p.glob(pattern)
                            if f.is_file() and f.suffix.lower() in SUPPORTED)
        elif p.is_file():
            if p.suffix.lower() in SUPPORTED:
                files.append(p)
            else:
                print(f"skip: {p} (not .pdf or .epub)", file=sys.stderr)
        else:
            print(f"skip: {p} (not found)", file=sys.stderr)
    return files


def convert(path: Path, args) -> str:
    if path.suffix.lower() == ".pdf":
        return pdf_to_markdown(path, tables=not args.no_tables, page_markers=args.page_markers)
    return epub_to_markdown(path, images=not args.no_images)


def main(argv: list[str] | None = None) -> int:
    ap = argparse.ArgumentParser(description="Convert PDF and EPUB files to Markdown.")
    ap.add_argument("inputs", nargs="+", help="files or directories to convert")
    ap.add_argument("-o", "--out-dir", help="write .md files here (default: next to each input)")
    ap.add_argument("-r", "--recursive", action="store_true", help="search directories recursively")
    ap.add_argument("-f", "--force", action="store_true", help="overwrite existing .md files")
    ap.add_argument("--stdout", action="store_true", help="print Markdown instead of writing files")
    ap.add_argument("--page-markers", action="store_true", help="PDF: insert <!-- page N --> comments")
    ap.add_argument("--no-tables", action="store_true", help="PDF: skip table detection (faster)")
    ap.add_argument("--no-images", action="store_true", help="EPUB: drop image alt-text placeholders")
    args = ap.parse_args(argv)

    files = collect(args.inputs, args.recursive)
    if not files:
        print("no .pdf or .epub files found", file=sys.stderr)
        return 1

    out_dir = Path(args.out_dir).expanduser() if args.out_dir else None
    failures = 0
    claimed: set[Path] = set()
    for f in files:
        target = (out_dir / f.with_suffix(".md").name) if out_dir else f.with_suffix(".md")
        if target.resolve() in claimed:  # e.g. book.pdf and book.epub side by side
            target = target.with_name(f.name + ".md")
        claimed.add(target.resolve())
        if not args.stdout and target.exists() and not args.force:
            print(f"skip: {target} exists (use --force)", file=sys.stderr)
            continue
        try:
            md = convert(f, args)
        except Exception as e:  # keep going through a batch
            failures += 1
            print(f"error: {f}: {e}", file=sys.stderr)
            continue
        if args.stdout:
            sys.stdout.write(md)
        else:
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_text(md, encoding="utf-8")
            print(f"{f} -> {target} ({len(md.split()):,} words)")
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
