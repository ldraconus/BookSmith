#include "mainwindow.h"
#include "Zippy/Zippy.hpp"

#include <map>

#include <QApplication>
#include <QTextDocumentWriter>
#include <QTextDocument>
#include <QTextBlock>
#include <QTextEdit>
#include <QTextFormat>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QUuid>

struct Scene {
    QList<QString> _tags;
    QString _html;
    QString _name;
    QList<struct Scene> _children;

    Scene(): _html(""), _name("") { }
    Scene(const Scene& s): _tags(s._tags), _html(s._html), _name(s._name), _children(s._children) { }

    Scene& operator=(const Scene& s) { if (this != &s) { _tags = s._tags; _html = s._html; _name = s._name; _children = s._children; } return *this; }
};

struct Tree {
    Scene _top;
};

static QString _dir;
static Tree tree;

static Scene objectToScene(const QJsonObject& obj)
{
    Scene scene;
    scene._html = obj["doc"].toString("");
    scene._name = obj["name"].toString("");
    const QJsonArray& tags = obj["tags"].toArray();
    for (auto tag: tags) scene._tags.append(tag.toString(""));
    return scene;
}

static Scene objectToItem(const QJsonObject& obj)
{
    Scene scene = objectToScene(obj);
    const QJsonArray& children = obj["children"].toArray();
    for (auto child: children) scene._children.append(objectToItem(child.toObject()));
    return scene;
}

static bool open(QString filename)
{
    int ext = filename.lastIndexOf(".novel");
    if (ext != -1) filename = filename.left(ext);
    int sep = filename.lastIndexOf("/");
    if (sep != -1) {
        _dir = filename.left(sep);
        filename = filename.right(filename.length() - sep - 1);
    }
    sep = filename.lastIndexOf("\\");
    if (sep != -1) {
        _dir = filename.left(sep);
        filename = filename.right(filename.length() - sep - 1);
    }

    filename = _dir + "/" + filename + ".novel";

    QFile file;
    file.setFileName(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) return false;
    QByteArray data(file.readAll());
    QString jsonStr(data);
    file.close();
    QJsonDocument json = QJsonDocument::fromJson(jsonStr.toUtf8());

    const QJsonObject& top = json.object();
    if (!top.contains("document")) return false;

    const QJsonObject& doc = top["document"].toObject();
    const QJsonObject& root = doc["root"].toObject();
    tree._top = objectToItem(root);

    return true;
}

struct Chapter {
    QString _name;
    QString _html;
    Chapter(): _name(""), _html("") { }
    Chapter(const QString& n): _name(n), _html("") { }
    Chapter(const Chapter& c): _name(c._name), _html(c._html) { }

    Chapter& operator=(const Chapter& c) { if (this != &c) { _name = c._name; _html = c._html; } return *this; }
};

static QString Cover = "";
static QString Author = "Anonymous";
static QString Name = "";
static QString Rights = "Public Domain";
static QString Language = "en-US";
static QString Publisher = "Independent";
static QString Id = "";

static bool recognizeTag(const QList<QString>& tags, const QString& tag)
{
    return (tags.indexOf(tag) != -1 ||
            tags.indexOf(tag.left(1).toUpper() + tag.right(tag.length() - 1)) != -1 ||
            tags.indexOf(tag.toUpper()) != -1);
}

static bool SaveStringByTag(const QList<QString>& tags, QString& where, QString tag, const QString& html)
{
    if (recognizeTag(tags, tag)) {
        QTextEdit edit;
        edit.setHtml(html);
        where = edit.toPlainText();
        return true;
    }
    return false;
}

static void sceneToDocument(QList<Chapter>& book, QTextEdit& edit, const Scene& scene)
{
    if (!SaveStringByTag(scene._tags, Cover, "cover", scene._html) &&
        !SaveStringByTag(scene._tags, Author, "author", scene._html) &&
        !SaveStringByTag(scene._tags, Rights, "rights", scene._html) &&
        !SaveStringByTag(scene._tags, Language, "language", scene._html) &&
        !SaveStringByTag(scene._tags, Publisher, "publisher", scene._html) &&
        !SaveStringByTag(scene._tags, Id, "id", scene._html)) {
        QTextEdit temp;
        if (recognizeTag(scene._tags, "chapter")) {
            int last = book.length() - 1;
            if (last >= 0) book[last]._html = edit.toHtml();
            Chapter n(scene._name);
            book.append(n);
            edit.clear();
            temp.setHtml(scene._html);
        }
        else if (recognizeTag(scene._tags, "scene")) temp.setHtml(scene._html);
        temp.selectAll();
        if (temp.textCursor().selectedText().length() != 0) {
            temp.cut();
            edit.paste();
            edit.insertPlainText("\r\n");
        }
    }
    for (const auto& scn: scene._children) sceneToDocument(book, edit, scn);
}

static void novelToDocument(QList<Chapter>& book, Tree& tree)
{
    QTextEdit edit;
    Name = tree._top._name;
    sceneToDocument(book, edit, tree._top);
    int last = book.length() - 1;
    if (last >= 0) book[last]._html = edit.toHtml();
}

static int chapterNumWidth(const QList<Chapter>& book)
{
    char buffer[15];
    sprintf(buffer, "%d", book.length());
    return (int) strlen(buffer);
}

static QString chapterManifest(const QList<Chapter>& book)
{
    QString manifest = "";
    int len = chapterNumWidth(book);
    for (int i = 1; i < book.length() + 1; ++i) {
        char line[1024];
        sprintf(line, "        <item id=\"chapter%0*d\" href=\"chap%0*d.xhtml\" media-type=\"application/xhtml+xml\" />\n", len, i, len, i);
        manifest += line;
    }
    return manifest;
}

static QString spineTOC(const QList<Chapter>& book)
{
    QString spine;
    int len = chapterNumWidth(book);
    for (int i = 1; i < book.length() + 1; ++i) {
        char line[1024];
        sprintf(line, "        <itemref idref=\"chapter%0*d\" />\n", len, i);
        spine += line;
    }
    return spine;
}

static QString navPoints(const QList<Chapter>& book)
{
    QString nav;
    int len = chapterNumWidth(book);
    for (int i = 1; i < book.length() + 1; ++i) {
        char line[1024];
        sprintf(line,     "      <navPoint id=\"chapter%0*d\" playOrder=\"%d\">\n"
                          "          <navLabel>\n"
                          "              <text>%s</text>\n"
                          "          </navLabel>\n"
                          "          <content src=\"chap%0*d.xhtml\"/>\n"
                          "      </navPoint>\n", len, i, i, book[i - 1]._name.toStdString().c_str(), len, i);
        nav += line;
    }
    return nav;

}

static QString convertHTML(QString html, QString title)
{
    // remove all style="..."
    html = "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n" + html;
    int pos = html.indexOf("<!DOCTYPE");
    QString left = html.left(pos);
    html = html.right(html.length() - pos);
    pos = html.indexOf(">");
    html = left + "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">" + html.right(html.length() - pos - 1);
    pos = html.indexOf("<html>") + 5;
    html = html.left(pos) + " xmlns=\"http://www.w3.org/1999/xhtml\"" + html.right(html.length() - pos);
    pos = html.indexOf("<style ");
    int end = html.indexOf("</style>") + 8;
    html = html.left(pos) + html.right(html.length() - end);
    while ((pos = html.indexOf(" align=\"center\"")) != -1) html = html.left(pos) + html.right(html.length() - pos - 15);
    while ((pos = html.indexOf(" align=\"right\"")) != -1) html = html.left(pos) + html.right(html.length() - pos - 14);
    while ((pos = html.indexOf(" align=\"left\"")) != -1) html = html.left(pos) + html.right(html.length() - pos - 13);
    while ((pos = html.indexOf(" align=\"full\"")) != -1) html = html.left(pos) + html.right(html.length() - pos - 13);
    pos = html.indexOf("<head>") + 6;
    html = html.left(pos) + "<title>" + title + "</title>" + html.right(html.length() - pos);
    return html;
}

#ifdef NOTDEF
class ZipEntry {
public:
    typedef QList<unsigned char> ZipEntryData;

private:
    ZipEntryData _binary;
    QString _data;
    QString _name;
    bool _compress;

public:
    ZipEntry(bool c = true): _compress(c) { }
    ZipEntry(const QString& n, const QString& d, bool c = true): _data(d), _name(n), _compress(c) { }
    ZipEntry(const QString& n, const ZipEntryData& b, bool c = true): _binary(b), _name(n), _compress(c) { }
    ZipEntry(const ZipEntry& z): _binary(z._binary), _data(z._data), _name(z._name), _compress(z._compress) { }

    ZipEntry& operator=(const ZipEntry& z) { if (this != &z) { _binary = z._binary; _data = z._data; _name = z._name; _compress = z._compress; } return *this; }

    QString Data() const { return _data; }
    QString Name() const { return _name; }
    const ZipEntryData& Binary() const { return _binary; }
    bool Compress() const { return _compress; }

    zip_source_t* asSource() const;
};

zip_source_t* ZipEntry::asSource() const
{
    zip_error_t err;
    if (Data().isEmpty()) {
        char* data = (char*) malloc(Binary().size());
        for (int i = 0; i < Binary().size(); ++i) data[i] = Binary()[i];
        return zip_source_buffer_create((const void*) data, (size_t) Binary().size(), 1, &err);
    }
    else return zip_source_buffer_create((const void*) Data().toStdString().c_str(), (size_t) Data().length(), 1, &err);
}

class Zip {
private:
    zip_t* _zip;
    QList<ZipEntry> _files;

public:
    Zip() {}

    void Close();
    void Create(const QString& filename);
    void AddEntry(const QString& n, const QString& d, bool c = true);
    void AddEntry(const QString& n, const ZipEntry::ZipEntryData& b, bool c = true);
    void Save();

    static const bool DO_NOT_COMPRESS = false;
};

void Zip::Create(const QString& filename)
{
    int err;
    _zip = zip_open(filename.toStdString().c_str(), ZIP_CREATE | ZIP_TRUNCATE, &err);
    _files.clear();
}

void Zip::AddEntry(const QString& n, const QString& d, bool c)
{
    _files.append(ZipEntry(n, d, c));
}

void Zip::AddEntry(const QString& n, const ZipEntry::ZipEntryData& b, bool c)
{
    _files.append(ZipEntry(n, b, c));
}

void Zip::Close()
{
    zip_close(_zip);
}

static QList<QString> split(QString name, const QString& sep)
{
    QList<QString> parts;
    int pos;
    while ((pos = name.indexOf(sep)) != -1) {
        parts.append(name.left(pos));
        name = name.right(name.length() - pos - 1);
    }
    parts.append(name);
    return parts;
}

void Zip::Save()
{
    std::map<QString, bool> dirs;
    for (const auto& x: _files) {
        zip_source_t* src = x.asSource();
        QList<QString> parts = split(x.Name(), "/");
        QString dir = "";
        for (int i = 0; i < parts.size() - 1; ++i) {
            if (i != 0) dir += "/";
            dir += parts[i];
            if (!dirs[dir]) zip_add_dir(_zip, dir.toStdString().c_str());
            dirs[dir] = true;
        }
        zip_int64_t idx = zip_file_add(_zip, x.Name().toStdString().c_str(), src, ZIP_FL_OVERWRITE);
        if (!x.Compress()) zip_set_file_compression(_zip, idx, ZIP_CM_STORE, 0);
        else zip_set_file_compression(_zip, idx, ZIP_CM_DEFAULT, 0);
    }
}

static void loadCover(const QString& name, ZipEntry::ZipEntryData& jpg)
{
    QFile file(name);
    if (!file.open(QIODevice::ReadOnly)) return;
    QByteArray data(file.readAll());
    for (const auto& x: data) jpg.append(x);
}
#else
static void loadCover(const QString& name, Zippy::ZipEntryData& jpg)
{
    QFile file(name);
    if (!file.open(QIODevice::ReadOnly)) return;
    QByteArray data(file.readAll());
    for (const auto& x: data) jpg.emplace_back(x);
}
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    if (argc != 3) return -1;

    if (!open(argv[1])) return -1;

    QList<Chapter> book;
    novelToDocument(book, tree);

    // create the zip file from the book
    Zippy::ZipArchive arch;
    arch.Create(argv[2]);
    arch.AddEntry("mimetype", "application/epub+zip");
    arch.AddEntry("META-INF/container.xml",
                  "<?xml version=\"1.0\"?>"
                  "<container xmlns=\"urn:oasis:names:tc:opendocument:xmlns:container\" version=\"1.0\">"
                  "  <rootfiles>"
                  "    <rootfile media-type=\"application/oebps-package+xml\" full-path=\"OEBPS/content.opf\"/>"
                  "  </rootfiles>"
                  "</container>");
    if (!Cover.isEmpty()) {
        Zippy::ZipEntryData jpg;
        loadCover(Cover, jpg);
        arch.AddEntry("OEBPS/images/Cover.jpg", jpg);
        arch.AddEntry("OEBPS/Cover.xhtml",
                      "<?xml version=\"1.0\" encoding=\"utf-8\"?>"
                      "<!DOCTYPE html PUBLIC \"-//W3C//DTD XHTML 1.1//EN\" \"http://www.w3.org/TR/xhtml11/DTD/xhtml11.dtd\">"
                      "<html xmlns=\"http://www.w3.org/1999/xhtml\"><head><title>Chapter 1</title><meta name=\"qrichtext\" content=\"1\" /></head>"
                      "   <body>\n"
                      "      <img src=\"images/Cover.jpg\"/>\n"
                      "   </body>\n"
                      "</html>");
    }
    if (!Cover.isEmpty()) {
        Zippy::ZipEntryData jpg;
        loadCover(Cover, jpg);
        arch.AddEntry("OEBPS/images/Cover.jpg", jpg);
        arch.AddEntry("OEBPS/Cover.xhtml",
                      "<!DOCTYPE HTML PUBLIC \"-//W3C//DTD HTML 4.0//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">\n"
                      "<html>\n"
                      "   <body>\n"
                      "      <img src=\"images/Cover.jpg\"/>\n"
                      "   </body>\n"
                      "</html>");
    }
    arch.AddEntry("OEBPS/page-template.xpgt",
                  "<ade:template xmlns=\"http://www.w3.org/1999/xhtml\" xmlns:ade=\"http://ns.adobe.com/2006/ade\""
                  "         xmlns:fo=\"http://www.w3.org/1999/XSL/Format\">"
                  "  <fo:layout-master-set>"
                  "    <fo:simple-page-master master-name=\"single_column\">"
                  "        <fo:region-body margin-bottom=\"3pt\" margin-top=\"0.5em\" margin-left=\"3pt\" margin-right=\"3pt\"/>"
                  "    </fo:simple-page-master>"
                  "    <fo:simple-page-master master-name=\"single_column_head\">"
                  "        <fo:region-before extent=\"8.3em\"/>"
                  "        <fo:region-body margin-bottom=\"3pt\" margin-top=\"6em\" margin-left=\"3pt\" margin-right=\"3pt\"/>"
                  "    </fo:simple-page-master>"
                  "    <fo:simple-page-master master-name=\"two_column\" margin-bottom=\"0.5em\" margin-top=\"0.5em\" margin-left=\"0.5em\" margin-right=\"0.5em\">"
                  "        <fo:region-body column-count=\"2\" column-gap=\"10pt\"/>"
                  "    </fo:simple-page-master>"
                  "    <fo:simple-page-master master-name=\"two_column_head\" margin-bottom=\"0.5em\" margin-left=\"0.5em\" margin-right=\"0.5em\">"
                  "        <fo:region-before extent=\"8.3em\"/>"
                  "        <fo:region-body column-count=\"2\" margin-top=\"6em\" column-gap=\"10pt\"/>"
                  "    </fo:simple-page-master>"
                  "    <fo:simple-page-master master-name=\"three_column\" margin-bottom=\"0.5em\" margin-top=\"0.5em\" margin-left=\"0.5em\" margin-right=\"0.5em\">"
                  "        <fo:region-body column-count=\"3\" column-gap=\"10pt\"/>"
                  "    </fo:simple-page-master>"
                  "    <fo:simple-page-master master-name=\"three_column_head\" margin-bottom=\"0.5em\" margin-top=\"0.5em\" margin-left=\"0.5em\" margin-right=\"0.5em\">"
                  "        <fo:region-before extent=\"8.3em\"/>"
                  "        <fo:region-body column-count=\"3\" margin-top=\"6em\" column-gap=\"10pt\"/>"
                  "    </fo:simple-page-master>"
                  "    <fo:page-sequence-master>"
                  "        <fo:repeatable-page-master-alternatives>"
                  "            <fo:conditional-page-master-reference master-reference=\"three_column_head\" page-position=\"first\" ade:min-page-width=\"80em\"/>"
                  "            <fo:conditional-page-master-reference master-reference=\"three_column\" ade:min-page-width=\"80em\"/>"
                  "            <fo:conditional-page-master-reference master-reference=\"two_column_head\" page-position=\"first\" ade:min-page-width=\"50em\"/>"
                  "            <fo:conditional-page-master-reference master-reference=\"two_column\" ade:min-page-width=\"50em\"/>"
                  "            <fo:conditional-page-master-reference master-reference=\"single_column_head\" page-position=\"first\" />"
                  "            <fo:conditional-page-master-reference master-reference=\"single_column\"/>"
                  "        </fo:repeatable-page-master-alternatives>"
                  "    </fo:page-sequence-master>"
                  "  </fo:layout-master-set>"
                  "  <ade:style>"
                  "    <ade:styling-rule selector=\".title_box\" display=\"adobe-other-region\" adobe-region=\"xsl-region-before\"/>"
                  "  </ade:style>"
                  "</ade:template>");
    arch.AddEntry("OEBPS/stylesheet.css",
                  "/* Style Sheet */\n"
                  "/* This defines styles and classes used in the book */\n"
                  "body { margin-left: 5%; margin-right: 5%; margin-top: 5%; margin-bottom: 5%; text-align: justify; }\n"
                  "pre { font-size: x-small; }\n"
                  "h1 { text-align: center; }\n"
                  "h2 { text-align: center; }\n"
                  "h3 { text-align: center; }\n"
                  "h4 { text-align: center; }\n"
                  "h5 { text-align: center; }\n"
                  "h6 { text-align: center; }\n"
                  ".CI {\n"
                  "    text-align:center;\n"
                  "    margin-top:0px;\n"
                  "    margin-bottom:0px;\n"
                  "    padding:0px;\n"
                  "    }\n"
                  ".center   {text-align: center;}\n"
                  ".smcap    {font-variant: small-caps;}\n"
                  ".u        {text-decoration: underline;}\n"
                  ".bold     {font-weight: bold;}\n");
    if (Id.isEmpty()) Id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    arch.AddEntry("OEBPS/content.opf",
                  ("<?xml version=\"1.0\" encoding=\"UTF-8\" ?>\n"
                   "<package xmlns=\"http://www.idpf.org/2007/opf\" unique-identifier=\"BookID\" version=\"2.0\" >\n"
                   "    <metadata xmlns:dc=\"http://purl.org/dc/elements/1.1/\" xmlns:opf=\"http://www.idpf.org/2007/opf\">\n"
                   "        <dc:title>" + Name + "</dc:title>\n"
                   "        <dc:creator opf:role=\"aut\">" + Author + "</dc:creator>\n"
                   "        <dc:language>" + Language + "</dc:language>\n"
                   "        <dc:rights>" + Rights + "</dc:rights>\n"
                   "        <dc:publisher>" + Publisher + "</dc:publisher>\n"
                   "        <dc:identifier id=\"BookID\" opf:scheme=\"UUID\">" + Id +"</dc:identifier>\n"
                   "        <meta name=\"cover\" content=\"images/Cover.jpg\" />"
                   "    </metadata>\n"
                   "    <manifest>\n"
                   "        <item id=\"cover_jpg\" href=\"images/Cover.jpg\" media-type=\"image/jpeg\" />"
                   "        <item id=\"ncx\" href=\"toc.ncx\" media-type=\"application/x-dtbncx+xml\" />\n"
                   "        <item id=\"style\" href=\"stylesheet.css\" media-type=\"text/css\" />\n"
                   "        <item id=\"pagetemplate\" href=\"page-template.xpgt\" media-type=\"application/vnd.adobe-page-template+xml\" />\n"
                   "        <item id=\"cover_html\" href=\"Cover.xhtml\" media-type=\"application/xhtml+xml\" />\n" +
                   chapterManifest(book) +
                   "    </manifest>\n"
                   "    <spine toc=\"ncx\">\n" +
                   spineTOC(book) +
                   "    </spine>\n"
                   "</package>").toStdString().c_str());
    arch.AddEntry("OEBPS/toc.ncx",
                  ("<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
                   "<ncx xmlns=\"http://www.daisy.org/z3986/2005/ncx/\" version=\"2005-1\">\n"
                   "   <head>\n"
                   "       <meta name=\"dtb:uid\" content=\"" + Id + "\"/>\n"
                   "       <meta name=\"dtb:depth\" content=\"1\"/>\n"
                   "       <meta name=\"dtb:totalPageCount\" content=\"0\"/>\n"
                   "       <meta name=\"dtb:maxPageNumber\" content=\"0\"/>\n"
                   "   </head>\n"
                   "   <docTitle>\n"
                   "       <text>" + Name + "</text>\n"
                   "   </docTitle>\n"
                   "   <navMap>" +
                   navPoints(book) +
                   "   </navMap>\n"
                   "</ncx>").toStdString().c_str());
    int len = chapterNumWidth(book);
    for (int i = 1; i <= book.length(); ++i) {
        char name[1024];
        sprintf(name, "OEBPS/chap%0*d.xhtml", len, i);
        arch.AddEntry(name, convertHTML(book[i - 1]._html, book[i - 1]._name).toStdString().c_str());
    }
    arch.Save();
    arch.Close();
    return 0;
}
