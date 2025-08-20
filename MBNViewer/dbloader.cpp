#include "dbloader.h"
#include <QDir>
#include <QFileInfo>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QVariant>
#include <QDebug>

DBTableData loadAllDbFiles(const QString &dirPath)
{
    DBTableData dataList;

    // 只做一件事：在 dirPath 目录下按 *.db 过滤，非递归
    QDir dir(dirPath);
    const QStringList filters{ "*.db" };
    const QFileInfoList fileList = dir.entryInfoList(
        filters, QDir::Files | QDir::NoDotAndDotDot | QDir::Readable);

    for (const QFileInfo &fi : fileList) {
        const QString dbFile = fi.absoluteFilePath();
        const QString connName = dbFile; // 连接名用绝对路径，确保唯一

        {
            QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connName);
            db.setDatabaseName(dbFile);
            if (!db.open()) {
                qWarning() << "Failed to open:" << dbFile;
                QSqlDatabase::removeDatabase(connName);
                continue;
            }

            TableData tableData;

            {
                QSqlQuery query(db);
                if (!query.exec("SELECT * FROM data")) {
                    qWarning() << "Query failed in:" << dbFile;
                    db.close();
                    QSqlDatabase::removeDatabase(connName);
                    continue;
                }

                while (query.next()) {
                    RowData row;
                    const QSqlRecord rec = query.record();
                    for (int i = 0; i < rec.count(); ++i)
                        row << query.value(i);
                    tableData << row;
                }
            } // QSqlQuery 析构

            dataList << tableData;
            db.close();
        } // QSqlDatabase 析构

        QSqlDatabase::removeDatabase(connName);
    }

    return dataList;
}
