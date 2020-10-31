#include "mainwindow.h"

#include <QApplication>
#include <QMessageBox>
#include <QtPrintSupport/QPrintEngine>
#include <QtPrintSupport/QPrinter>
#include <QTextDocument>
#include <QTextDocumentFragment>
#include <QTextBlock>
#include <QTextEdit>
#include <QTextFormat>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace PDF {
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

    static QString Cover;

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

    static void sceneToDocument(QTextEdit& edit, const Scene& scene)
    {
        QTextEdit temp;
        if (!SaveStringByTag(scene._tags, Cover, "cover", scene._html)) {
            if (recognizeTag(scene._tags, "chapter")) {
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
            else if (recognizeTag(scene._tags, "scene")) temp.setHtml(scene._html);
            temp.selectAll();
            if (temp.textCursor().selectedText().length() != 0) {
                temp.cut();
                edit.paste();
                edit.insertPlainText("\r\n");
            }
        }
        for (const auto& scn: scene._children) sceneToDocument(edit, scn);
    }

    static void insertCover(QTextEdit& edit) {
        if (!Cover.isEmpty()) {
            QTextDocumentFragment fragment;
            fragment = QTextDocumentFragment::fromHtml("<img height=\"803\" width=\"621\" src='file:///" + Cover + "'>");
            QTextCursor cursor  = edit.textCursor();
            cursor.setPosition(0);
            edit.setTextCursor(cursor);
            QTextBlockFormat blk = cursor.blockFormat();
            blk.setPageBreakPolicy(QTextFormat::PageBreak_AlwaysAfter);
            edit.setTextCursor(cursor);
            edit.textCursor().insertFragment(fragment);
            cursor.mergeBlockFormat(blk);
        }
    }

    static void novelToDocument(QTextEdit& edit, Tree& tree)
    {
        sceneToDocument(edit, tree._top);
        insertCover(edit);
    }
}

namespace PDF {
int pdf(int argc, char *argv[])
{
    if (argc != 3) return -1;
    if (!PDF::open(argv[1])) return -1;

    QPrinter printer(QPrinter::PrinterResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(argv[2]);

    QTextEdit edit;
    PDF::novelToDocument(edit, PDF::tree);
    QTextDocument* document = edit.document();
    document->print(&printer);

    return 0;
}
}
