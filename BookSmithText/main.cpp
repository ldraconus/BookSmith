#include "mainwindow.h"

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

namespace TEXT {
    struct Scene {
        QList<QString> _tags;
        QString _html;
        QList<struct Scene> _children;

        Scene(): _html("") { }
        Scene(const Scene& s): _tags(s._tags), _html(s._html), _children(s._children) { }

        Scene& operator=(const Scene& s) { if (this != &s) { _tags = s._tags; _html = s._html; _children = s._children; } return *this; }
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
            _dir = filename.left(sep + 1);
            filename = filename.mid(sep + 1);
        }
        sep = filename.lastIndexOf("\\");
        if (sep != -1) {
            _dir = filename.left(sep + 1);
            filename = filename.mid(sep + 1);
        }

        QFile file(_dir + "/" + filename + ".novel");
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

    static void sceneToDocument(QTextEdit& edit, const Scene& scene)
    {
        QTextEdit temp;
        if (scene._tags.indexOf("chapter") != -1 ||
            scene._tags.indexOf("Chapter") != -1 ||
            scene._tags.indexOf("CHAPTER") != -1) {
            temp.setHtml(scene._html);
            QTextCursor cursor = temp.textCursor();
            cursor.setPosition(0);
            temp.setTextCursor(cursor);
            cursor = temp.textCursor();
            QTextBlockFormat blk = cursor.blockFormat();
            blk.setPageBreakPolicy(QTextFormat::PageBreak_AlwaysBefore);
            cursor.mergeBlockFormat(blk);
            temp.setTextCursor(cursor);
        }
        else if (scene._tags.indexOf("scene") != -1 ||
                 scene._tags.indexOf("Scene") != -1 ||
                 scene._tags.indexOf("SCENE") != -1) temp.setHtml(scene._html);
        temp.selectAll();
        if (temp.textCursor().selectedText().length() != 0) {
            temp.cut();
            edit.paste();
            edit.insertPlainText("\r\n");
        }
        for (const auto& scn: scene._children) sceneToDocument(edit, scn);
    }

    static void novelToDocument(QTextEdit& edit, Tree& tree)
    {
        sceneToDocument(edit, tree._top);
    }
}

#ifdef Q_OS_MACOS
namespace TEXT {
int text(int argc, char *argv[])
{
#else
int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
#endif

    if (argc != 3) return -1;

    if (!TEXT::open(argv[1])) return -1;

    QTextEdit edit;
    TEXT::novelToDocument(edit, TEXT::tree);
    QTextDocument* document = edit.document();
    QTextDocumentWriter writer(argv[2]);
    writer.setFormat("plaintext");
    writer.write(document);

    return 0;
}
#ifdef Q_OS_MACOS
}
#endif
