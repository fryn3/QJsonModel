/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2011 SCHUTZ Sacha
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "qjsonmodel.h"
#include <QFile>
#include <QDebug>
#include <QFont>
#include <memory>
#include <QQmlEngine>

static bool qmlRegisterType(QString itemName) {
    qmlRegisterType<QJsonModel>(QString("cpp.%1").arg(itemName).toUtf8(),
                       12, 34, itemName.toUtf8());
    return true;
}

QJsonTreeItem::QJsonTreeItem(QJsonTreeItem *parent)
    : mParent(parent) { }

QJsonTreeItem::QJsonTreeItem(const QJsonValue &value, QJsonTreeItem *parent, const QString &key)
    : mParent(parent), mKey(key) {
    setValue(value);
}

QJsonTreeItem::QJsonTreeItem(const QVariant &value, QJsonTreeItem *parent, const QString &key)
    : mParent(parent), mKey(key) {
    setValue(value);
}

void QJsonTreeItem::appendChild(std::shared_ptr<QJsonTreeItem> item) {
    mChilds.append(item);
}

std::shared_ptr<QJsonTreeItem> QJsonTreeItem::child(int row) {
    return mChilds.value(row);
}

QJsonTreeItem *QJsonTreeItem::parent() {
    return mParent;
}

int QJsonTreeItem::childCount() const {
    return mChilds.count();
}

int QJsonTreeItem::row() const {
    if (!mParent) {
        return -1;
    }
    for (int i = 0; i < mParent->mChilds.size(); ++i) {
        if (mParent->mChilds.at(i).get() == this) {
            return i;
        }
    }
    return -1;
}

bool QJsonTreeItem::setKey(const QString &key) {
    if (mKey == key) {
        return false;
    }
    if (!mParent) {
        mKey = key;
        return true;
    }
    for (const auto &itCh: mParent->mChilds) {
        if (itCh->key() == key) {
            return false;
        }
    }
    mKey = key;
    return true;
}

bool QJsonTreeItem::setValue(const QJsonValue &value) {
    if (jsonValue() == value) {
        return false;
    }
    mChilds.clear();
    if (value.isObject()) {
        mValue = QJsonObject();
        for (const QString &k : value.toObject().keys()) {
            QJsonValue v = value.toObject().value(k);
            appendChild(std::make_shared<QJsonTreeItem>(v, this, k));
        }
    } else if (value.isArray()) {
        mValue = QJsonArray();
        for (const QJsonValue &v : value.toArray()) {
            appendChild(std::make_shared<QJsonTreeItem>(v, this));
        }
    } else {
        mValue = value;
    }
    return true;
}

bool QJsonTreeItem::setValue(const QVariant &value) {
    return setValue(QJsonValue::fromVariant(value));
}

const QString &QJsonTreeItem::key() const {
    return mKey;
}

const QJsonValue &QJsonTreeItem::value() const {
    return mValue;
}

QJsonValue::Type QJsonTreeItem::type() const {
    return mValue.type();
}

bool QJsonTreeItem::isArrayOrObject() const {
    return type() == QJsonValue::Array || type() == QJsonValue::Object;
}

QJsonValue QJsonTreeItem::jsonValue() const
{
    if (type() == QJsonValue::Object) {
        QJsonObject jo;
        for (int i = 0; i < mChilds.size(); ++i) {
            auto ch = mChilds.at(i);
            auto key = ch->key();
            jo.insert(key, ch->jsonValue());
        }
        return jo;
    } else if (type() == QJsonValue::Array) {
        QJsonArray arr;
        for (int i = 0; i < mChilds.size(); ++i) {
            auto ch = mChilds.at(i);
            arr.append(ch->jsonValue());
        }
        return arr;
    } else {
        return value();
    }
}

// QJsonModel

const QString QJsonModel::ITEM_NAME = "QJsonModel";

const bool QJsonModel::IS_QML_REG = qmlRegisterType(QJsonModel::ITEM_NAME);

const std::array<QString, QJsonModel::ColEnd - QJsonModel::ColBegin>
                        QJsonModel::HEADERS_STR { "key", "value", "type" };

QJsonModel::QJsonModel(QObject *parent)
    : QAbstractItemModel(parent),
      mRootItem(std::make_unique<QJsonTreeItem>(QJsonValue(
        QJsonObject({{"", QJsonValue()}})), nullptr, "root")) { }

QJsonModel::QJsonModel(const QString &fileName, QObject *parent)
    : QAbstractItemModel(parent), mRootItem(nullptr) {
    load(fileName);
}

QJsonModel::QJsonModel(QIODevice *device, QObject *parent)
    : QAbstractItemModel(parent), mRootItem(nullptr) {
    load(device);
}

QJsonModel::QJsonModel(const QByteArray &json, QObject *parent)
    : QAbstractItemModel(parent), mRootItem(nullptr) {
    load(json);
}

bool QJsonModel::load(const QString &fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    return load(&file);
}

bool QJsonModel::load(QIODevice *device) {
    return load(device->readAll());
}

bool QJsonModel::load(const QByteArray &json) {
    QJsonParseError parse_error;
    auto jdoc = QJsonDocument::fromJson(json, &parse_error);

    if (jdoc.isNull()) {
        qDebug() << Q_FUNC_INFO << "Cannot load json." << parse_error.errorString();
        qDebug() << "The input json string is:" << json;
        return false;
    }

    return load(jdoc);
}

bool QJsonModel::load(const QJsonDocument &json) {
    beginResetModel();
    if (json.isArray()) {
        mRootItem = std::make_unique<QJsonTreeItem>(QJsonValue(json.array()), nullptr, "root");
    } else {
        mRootItem = std::make_unique<QJsonTreeItem>(QJsonValue(json.object()), nullptr, "root");
    }
    endResetModel();
    return true;
}

void QJsonModel::clear() {
    beginResetModel();
    mRootItem = std::make_unique<QJsonTreeItem>(QJsonValue(QJsonObject({{"", QJsonValue()}})), nullptr, "root");
    endResetModel();
}

bool QJsonModel::addChildren(const QModelIndex &index, const QVariant &value, const QString &key) {
    if (!index.isValid()) {
        return false;
    }

    auto item = static_cast<QJsonTreeItem*>(index.internalPointer());
    if (!item->isArrayOrObject() && item->type() != QJsonValue::Null) {
        return false;
    }
    if (item->type() == QJsonValue::Null) {
        item->setValue(QJsonValue(QJsonObject()));
    }
    auto v = QJsonValue::fromVariant(value);
    beginInsertRows(index, item->childCount(), item->childCount());
    item->appendChild(std::make_shared<QJsonTreeItem>(v, item, key));
    emit dataChanged(QJsonModel::index(index.row(),     0,             index),
                     QJsonModel::index(index.row() + 1, columnCount(), index),
                     {Qt::EditRole});
    endInsertRows();
    return true;
}

bool QJsonModel::addSibling(const QModelIndex &index, const QVariant &value, const QString &key) {
    if (!index.isValid()) {
        return false;
    }

    auto indexParent = index.parent();
    auto itemParent = static_cast<QJsonTreeItem*>(indexParent.internalPointer());
    if (!itemParent) {
        /// \todo this is done poorly
        itemParent = mRootItem.get();
    }
    auto v = QJsonValue::fromVariant(value);
    beginInsertRows(indexParent, itemParent->childCount(), itemParent->childCount());
    itemParent->appendChild(std::make_shared<QJsonTreeItem>(v, itemParent, key));
    emit dataChanged(QJsonModel::index(rowCount(indexParent), 0, indexParent),
                     QJsonModel::index(rowCount(indexParent), columnCount(), indexParent));
    endInsertRows();
    return true;
}

bool QJsonModel::appendToArray(const QByteArray &json) {
    if (mRootItem->type() != QJsonValue::Array) {
        return false;
    }

    QJsonParseError parse_error;
    auto jdoc = QJsonDocument::fromJson(json, &parse_error);

    if (jdoc.isNull()) {
        qDebug() << Q_FUNC_INFO << "Cannot load json." << parse_error.errorString();
        qDebug() << "The input json string is:" << json;
        return false;
    }

    beginResetModel();
    auto value = jdoc.isArray() ? QJsonValue(jdoc.array()) : QJsonValue(jdoc.object());
    mRootItem->appendChild(std::make_shared<QJsonTreeItem>(value, mRootItem.get()));
    endResetModel();

    return true;
}

QVariant QJsonModel::data(const QModelIndex &index, int role) const {
    if (!index.isValid()) {
        return QVariant();
    }
    auto item = static_cast<QJsonTreeItem*>(index.internalPointer());

    if (!item) {
        qDebug() << __PRETTY_FUNCTION__ << index;
        return QVariant();
    }
    if (role >= Qt::UserRole) {
        return data(QJsonModel::index(index.row(), role - Qt::UserRole, index.parent()),
                    Qt::DisplayRole);
    }
    switch (role) {
    case Qt::DisplayRole:
        switch (index.column() + Qt::UserRole) {
        case ColKey:
            if (item->parent()->value().type() == QJsonValue::Array) {
                return QString("[%1]").arg(item->row());
            } else {
                return item->key();
            }
        case ColValue:
            switch (item->type()) {
            case QJsonValue::Object:
            case QJsonValue::Array:
                return QVariant();
            default:
                return item->value().toVariant();
            }
        case ColType:
            switch (item->type()) {
            case QJsonValue::Null:
                return "null";
            case QJsonValue::Bool:
                return "bool";
            case QJsonValue::Double:
                return "double";
            case QJsonValue::String:
                return "string";
            case QJsonValue::Array:
                return QString("Array [%1]").arg(item->childCount());
            case QJsonValue::Object:
                return QString("Object [%1]").arg(item->childCount());
            default:
                Q_ASSERT(false && "bad item type");
            }
        default:
            Q_ASSERT(false && "bad column");
        }
    case Qt::EditRole:
        switch (index.column() + Qt::UserRole) {
        case ColKey:
            return item->key();
        default:
            return item->value().toVariant();
        }
    }

    return QVariant();
}

bool QJsonModel::setData(const QModelIndex &index, const QVariant &value, int role) {
    if (role != Qt::EditRole) {
        return false;
    }
    bool success = true;
    auto item = static_cast<QJsonTreeItem*>(index.internalPointer());
    bool isRemoveRows = item->isArrayOrObject() && index.column() != ColKey;
    if (index.column() == ColKey) {
        success = item->setKey(value.toString());
    } else if (index.column() == ColValue) {
        if (isRemoveRows) {
            beginRemoveRows(index, 0, item->childCount());
            success = item->setValue(value);
            if (success) {
                emit dataChanged(index, index, {Qt::EditRole});
            }
            endRemoveRows();
        } else {
            success = item->setValue(value);
        }
    }
    if (success && !isRemoveRows) {
        emit dataChanged(index, index, {Qt::EditRole});
    }
    return success;
}

QVariant QJsonModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (role != Qt::DisplayRole) {
        return QVariant();
    }

    if (orientation == Qt::Horizontal && section >= 0 && section < columnCount()) {
        return HEADERS_STR.at(section);
    } else {
        return QVariant();
    }
}

QModelIndex QJsonModel::index(int row, int column, const QModelIndex &parent) const {
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    QJsonTreeItem *parentItem;

    if (parent.isValid()) {
        parentItem = static_cast<QJsonTreeItem*>(parent.internalPointer());
    } else {
        parentItem = mRootItem.get();
    }

    auto childItem = parentItem->child(row);
    if (childItem) {
        return createIndex(row, column, childItem.get());
    } else {
        return QModelIndex();
    }
}

QModelIndex QJsonModel::parent(const QModelIndex &index) const {
    if (!index.isValid()) {
        return QModelIndex();
    }

    auto item = static_cast<QJsonTreeItem*>(index.internalPointer());
    QJsonTreeItem *parentItem = item->parent();

    if (parentItem == mRootItem.get()) {
        return QModelIndex();
    }

    return createIndex(parentItem->row(), 0, parentItem);
}

int QJsonModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return mRootItem->childCount();
    } else {
        return static_cast<QJsonTreeItem*>(parent.internalPointer())->childCount();
    }
}

int QJsonModel::columnCount(const QModelIndex &parent) const {
    Q_UNUSED(parent)

    return HEADERS_STR.size();
}

Qt::ItemFlags QJsonModel::flags(const QModelIndex &index) const {
    auto item = static_cast<QJsonTreeItem*>(index.internalPointer());

    if (((index.column() ==ColValue) && !(item->isArrayOrObject()))
            || (index.column() == ColKey
                && item->parent()->type() != QJsonValue::Array)) {
        return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
    } else {
        return QAbstractItemModel::flags(index);
    }
}

QHash<int, QByteArray> QJsonModel::roleNames() const {
    auto roles = QAbstractItemModel::roleNames();
    for (uint i = 0; i < HEADERS_STR.size(); ++i) {
        roles.insert(ColBegin + i, HEADERS_STR.at(i).toUtf8());
    }
    return roles;
}

QJsonDocument QJsonModel::toJsonDoc() const {
    auto v = toJson();
    if (v.isArray()) {
        return QJsonDocument(v.toArray());
    } else {
        return QJsonDocument(v.toObject());
    }
}

QJsonValue QJsonModel::toJson() const {
    return mRootItem->jsonValue();
}

QCborValue QJsonModel::toCbor() const {
    return QCborValue::fromJsonValue(toJson());
}

QByteArray QJsonModel::toByteArray(bool isJson) const
{
    if (isJson) {
        return QJsonDocument(toJson().toObject()).toJson();
    }
    return toCbor().toCbor();
}
