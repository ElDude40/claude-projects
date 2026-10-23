"""Run with: python -m unittest discover -s tests  (PDF tests need reportlab)."""

import sys
import tempfile
import unittest
import zipfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
import doc2md  # noqa: E402

try:
    from reportlab.lib.pagesizes import letter
    from reportlab.lib.styles import getSampleStyleSheet
    from reportlab.platypus import (BaseDocTemplate, Frame, ListFlowable, ListItem,
                                    PageBreak, PageTemplate, Paragraph, SimpleDocTemplate,
                                    Table, TableStyle)
    HAVE_REPORTLAB = True
except ImportError:
    HAVE_REPORTLAB = False

BODY = ("This paragraph is long enough to wrap over several lines so the converter "
        "has to join them back into one paragraph again. ") * 3


def make_epub(path: Path) -> None:
    with zipfile.ZipFile(path, "w") as z:
        z.writestr("mimetype", "application/epub+zip", compress_type=zipfile.ZIP_STORED)
        z.writestr("META-INF/container.xml",
                   '<?xml version="1.0"?><container version="1.0" '
                   'xmlns="urn:oasis:names:tc:opendocument:xmlns:container"><rootfiles>'
                   '<rootfile full-path="OEBPS/content.opf" '
                   'media-type="application/oebps-package+xml"/></rootfiles></container>')
        z.writestr("OEBPS/content.opf",
                   '<?xml version="1.0"?><package xmlns="http://www.idpf.org/2007/opf" version="3.0">'
                   '<metadata xmlns:dc="http://purl.org/dc/elements/1.1/"><dc:title>Test Book</dc:title>'
                   '<dc:creator>Jane Doe</dc:creator></metadata><manifest>'
                   '<item id="c2" href="Text/ch%202.xhtml" media-type="application/xhtml+xml"/>'
                   '<item id="c1" href="Text/ch1.xhtml" media-type="application/xhtml+xml"/>'
                   '</manifest><spine><itemref idref="c1"/><itemref idref="c2"/></spine></package>')
        z.writestr("OEBPS/Text/ch1.xhtml",
                   '<html xmlns="http://www.w3.org/1999/xhtml"><head><style>p{}</style></head><body>'
                   '<h1>Chapter One</h1><p>Hello <em>world</em> and <a href="ch%202.xhtml">a link</a>'
                   '<sup>1</sup>.</p><ul><li>one</li><li>two</li></ul><img src="a.png" alt="A cat"/>'
                   '</body></html>')
        z.writestr("OEBPS/Text/ch 2.xhtml",
                   '<html xmlns="http://www.w3.org/1999/xhtml"><body><h2>Chapter Two</h2>'
                   '<p>See <a href="https://example.com">site</a>.</p></body></html>')


class EpubTests(unittest.TestCase):
    def test_spine_order_and_formatting(self):
        with tempfile.TemporaryDirectory() as d:
            path = Path(d) / "book.epub"
            make_epub(path)
            md = doc2md.epub_to_markdown(path)
        self.assertTrue(md.startswith("# Test Book\n\n*Jane Doe*"))
        self.assertLess(md.index("# Chapter One"), md.index("## Chapter Two"))
        self.assertIn("Hello *world* and a link^1.", md)  # internal link unwrapped
        self.assertIn("- one\n- two", md)
        self.assertIn("[Image: A cat]", md)
        self.assertIn("[site](https://example.com)", md)
        self.assertNotIn("p{}", md)


@unittest.skipUnless(HAVE_REPORTLAB, "reportlab not installed")
class PdfTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.dir = Path(self.tmp.name)
        self.ss = getSampleStyleSheet()

    def tearDown(self):
        self.tmp.cleanup()

    def test_structure(self):
        path = self.dir / "doc.pdf"
        ss = self.ss

        def header_footer(c, doc):
            c.setFont("Helvetica", 8)
            c.drawString(72, 750, "Running Header Text")
            c.drawString(300, 30, str(doc.page))

        story = []
        for n in range(1, 5):
            story += [Paragraph(f"Chapter {n}", ss["Heading1"]), Paragraph(BODY, ss["BodyText"]),
                      Paragraph("A Section", ss["Heading2"]), Paragraph(BODY, ss["BodyText"]),
                      ListFlowable([ListItem(Paragraph("First item", ss["BodyText"])),
                                    ListItem(Paragraph("Second item", ss["BodyText"]))],
                                   bulletType="bullet")]
            if n == 2:
                t = Table([["Name", "Age"], ["Ann", "34"], ["Bob", "29"]])
                t.setStyle(TableStyle([("GRID", (0, 0), (-1, -1), 0.5, "black")]))
                story.append(t)
            story.append(PageBreak())
        SimpleDocTemplate(str(path), pagesize=letter).build(story, onFirstPage=header_footer,
                                           onLaterPages=header_footer)
        md = doc2md.pdf_to_markdown(path)

        self.assertEqual(md.count("\n# Chapter") + md.startswith("# Chapter"), 4)
        self.assertEqual(md.count("## A Section"), 4)
        self.assertIn(BODY.strip(), md)  # wrapped lines rejoined into one paragraph
        self.assertIn("- First item\n\n- Second item", md)
        self.assertIn("| Name | Age |\n| --- | --- |\n| Ann | 34 |", md)
        self.assertNotIn("Running Header", md)
        self.assertNotRegex(md, r"(?m)^\d+$")

    def test_two_columns(self):
        path = self.dir / "two.pdf"
        width, height = letter
        col = (width - 144 - 18) / 2
        doc = BaseDocTemplate(str(path), pagesize=letter)
        doc.addPageTemplates([PageTemplate(frames=[
            Frame(72, 72, col, height - 144), Frame(72 + col + 18, 72, col, height - 144)])])
        story = []
        for n in range(1, 7):
            story += [Paragraph(f"Section {n}", self.ss["Heading2"]),
                      Paragraph(f"Text {n}. " + "Words flow down the left column first. " * 12,
                                self.ss["BodyText"])]
        doc.build(story)
        md = doc2md.pdf_to_markdown(path)
        positions = [md.index(f"# Section {n}\n") for n in range(1, 7)]
        self.assertEqual(positions, sorted(positions))
        self.assertIn("Text 1. Words flow down the left column first. Words", md)


class HelperTests(unittest.TestCase):
    def test_dehyphenate(self):
        self.assertEqual(doc2md._join("extra-", "ordinary"), "extraordinary")
        self.assertEqual(doc2md._join("non-", "English"), "non-English")
        self.assertEqual(doc2md._join("one", "two"), "one two")

    def test_paragraph_across_page_break(self):
        self.assertEqual(doc2md._merge_across_pages(["ends mid", "sentence.", "# H"]),
                         ["ends mid sentence.", "# H"])

    @unittest.skipUnless(HAVE_REPORTLAB, "reportlab not installed")
    def test_cli_name_collision(self):
        """book.epub and book.pdf in one run must not overwrite each other."""
        with tempfile.TemporaryDirectory() as d:
            make_epub(Path(d) / "book.epub")
            SimpleDocTemplate(str(Path(d) / "book.pdf")).build(
                [Paragraph(BODY, getSampleStyleSheet()["BodyText"])])
            out = Path(d) / "out"
            self.assertEqual(doc2md.main([d, "-o", str(out)]), 0)
            self.assertIn("Test Book", (out / "book.md").read_text())
            self.assertIn("This paragraph", (out / "book.pdf.md").read_text())


if __name__ == "__main__":
    unittest.main()
