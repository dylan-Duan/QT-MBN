#pragma once
#include <QList>
#include <QVariant>

using RowData = QList<QVariant>;
using TableData = QList<RowData>;
using DBTableData = QList<TableData>;

DBTableData loadAllDbFiles(const QString &dirname);
