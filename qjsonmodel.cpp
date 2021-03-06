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


QJsonTreeItem::QJsonTreeItem(QJsonTreeItem *parent)
    : mParent(parent)
    , mMaxChildCount(0)
{
}

QJsonTreeItem::~QJsonTreeItem()
{
    qDeleteAll(mChilds);
}

void QJsonTreeItem::appendChild(QJsonTreeItem *item)
{
    mChilds.append(item);

    if ((mMaxChildCount > 0) && (mChilds.size() > mMaxChildCount))
    {
        mChilds.erase(mChilds.begin(), mChilds.begin() + (mChilds.size() - mMaxChildCount));
    }
}

QJsonTreeItem *QJsonTreeItem::child(int row)
{
    return mChilds.value(row);
}

QJsonTreeItem *QJsonTreeItem::parent()
{
    return mParent;
}

int QJsonTreeItem::childCount() const
{
    return mChilds.count();
}

int QJsonTreeItem::row() const
{
    return mParent ? mParent->mChilds.indexOf(const_cast<QJsonTreeItem*>(this)) : 0;
}

void QJsonTreeItem::setKey(const QString &key)
{
    mKey = key;
}

void QJsonTreeItem::setValue(const QJsonValue &value)
{
    mValue = value;
}

const QString &QJsonTreeItem::key() const
{
    return mKey;
}

const QJsonValue &QJsonTreeItem::value() const
{
    return mValue;
}

QJsonValue::Type QJsonTreeItem::type() const
{
    return mValue.type();
}

int QJsonTreeItem::maxChildCount() const
{
    return mMaxChildCount;
}

void QJsonTreeItem::setMaxChildCount(int value)
{
    mMaxChildCount = value;
}

QJsonTreeItem *QJsonTreeItem::load(const QJsonValue &value, QJsonTreeItem *parent, const QString &key)
{
    QJsonTreeItem *rootItem = new QJsonTreeItem(parent);
    rootItem->setKey(key);

    if (value.isObject())
    {
        rootItem->setValue(QJsonObject());
        //Get all QJsonValue childs
        for (QString key : value.toObject().keys())
        {
            QJsonValue v = value.toObject().value(key);
            QJsonTreeItem *child = load(v, rootItem);
            child->setKey(key);
            rootItem->appendChild(child);
        }
    }
    else if (value.isArray())
    {
        rootItem->setValue(QJsonArray());
        //Get all QJsonValue childs
        int index = 0;
        for (QJsonValue v : value.toArray())
        {
            QJsonTreeItem *child = load(v, rootItem);
            child->setKey(QString::number(index));
            rootItem->appendChild(child);
            ++index;
        }
    }
    else
    {
        rootItem->setValue(value);
    }

    return rootItem;
}


QJsonModel::QJsonModel(QObject *parent)
    : QAbstractItemModel(parent)
    , mRootItem(new QJsonTreeItem())
{
    mHeaders.append("key");
    mHeaders.append("value");
}

QJsonModel::QJsonModel(const QString &fileName, QObject *parent)
    : QAbstractItemModel(parent)
    , mRootItem(new QJsonTreeItem())
{
    mHeaders.append("key");
    mHeaders.append("value");
    load(fileName);
}

QJsonModel::QJsonModel(QIODevice *device, QObject *parent)
    : QAbstractItemModel(parent)
    , mRootItem(new QJsonTreeItem())
{
    mHeaders.append("key");
    mHeaders.append("value");
    load(device);
}

QJsonModel::QJsonModel(const QByteArray &json, QObject *parent)
    : QAbstractItemModel(parent)
    , mRootItem(new QJsonTreeItem())
{
    mHeaders.append("key");
    mHeaders.append("value");
    loadJson(json);
}

QJsonModel::~QJsonModel()
{
    delete mRootItem;
}

bool QJsonModel::load(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly))
    {
        return false;
    }

    return load(&file);
}

bool QJsonModel::load(QIODevice *device)
{
    return loadJson(device->readAll());
}

bool QJsonModel::loadJson(const QByteArray &json)
{
    auto jdoc = QJsonDocument::fromJson(json);

    if (jdoc.isNull())
    {
        qDebug() << Q_FUNC_INFO << "cannot load json";
        return false;
    }

    beginResetModel();
    delete mRootItem;
    if (jdoc.isArray())
    {
        mRootItem = QJsonTreeItem::load(QJsonValue(jdoc.array()));
    }
    else
    {
        mRootItem = QJsonTreeItem::load(QJsonValue(jdoc.object()));
    }
    endResetModel();

    return true;
}

bool QJsonModel::appendToArray(const QByteArray &json, const QString &key)
{
    if (mRootItem->type() != QJsonValue::Array)
    {
        return false;
    }

    QJsonParseError parse_error;
    auto jdoc = QJsonDocument::fromJson(json, &parse_error);

    if (jdoc.isNull())
    {
        qDebug() << Q_FUNC_INFO << "Cannot load json." << parse_error.errorString();
        qDebug() << "The input json string is:" << json;
        return false;
    }

    beginResetModel();
    auto value = jdoc.isArray() ? QJsonValue(jdoc.array()) : QJsonValue(jdoc.object());
    mRootItem->appendChild(QJsonTreeItem::load(value, mRootItem, key.isEmpty() ? QString::number(mRootItem->childCount()) : key));
    endResetModel();

    return true;
}

int QJsonModel::maxArraySize() const
{
    return mRootItem->maxChildCount();
}

void QJsonModel::setMaxArraySize(int value)
{
    mRootItem->setMaxChildCount(value);
}

QVariant QJsonModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
    {
        return QVariant();
    }

    QJsonTreeItem *item = static_cast<QJsonTreeItem*>(index.internalPointer());

    if (role == Qt::DisplayRole)
    {
        if (index.column() == 0)
        {
            return QString("%1").arg(item->key());
        }
        else if (index.column() == 1)
        {
            return QString("%1").arg(item->value().toVariant().toString());
        }
    }
    else if (role == Qt::EditRole)
    {
        if (index.column() == 1)
        {
            return QString("%1").arg(item->value().toVariant().toString());
        }
    }

    return QVariant();
}

bool QJsonModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::EditRole)
    {
        if (index.column() == 1)
        {
            QJsonTreeItem *item = static_cast<QJsonTreeItem*>(index.internalPointer());
            item->setValue(QJsonValue::fromVariant(value));
            emit dataChanged(index, index, {Qt::EditRole});
            return true;
        }
    }

    return false;
}

QVariant QJsonModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
    {
        return QVariant();
    }

    if (orientation == Qt::Horizontal)
    {
        return mHeaders.value(section);
    }
    else
    {
        return QVariant();
    }
}

QModelIndex QJsonModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
    {
        return QModelIndex();
    }

    QJsonTreeItem *parentItem;

    if (!parent.isValid())
    {
        parentItem = mRootItem;
    }
    else
    {
        parentItem = static_cast<QJsonTreeItem*>(parent.internalPointer());
    }

    QJsonTreeItem *childItem = parentItem->child(row);
    if (childItem)
    {
        return createIndex(row, column, childItem);
    }
    else
    {
        return QModelIndex();
    }
}

QModelIndex QJsonModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
    {
        return QModelIndex();
    }

    QJsonTreeItem *childItem = static_cast<QJsonTreeItem*>(index.internalPointer());
    QJsonTreeItem *parentItem = childItem->parent();

    if (parentItem == mRootItem)
    {
        return QModelIndex();
    }

    return createIndex(parentItem->row(), 0, parentItem);
}

int QJsonModel::rowCount(const QModelIndex &parent) const
{
    QJsonTreeItem *parentItem;
    if (parent.column() > 0)
    {
        return 0;
    }

    if (!parent.isValid())
    {
        parentItem = mRootItem;
    }
    else
    {
        parentItem = static_cast<QJsonTreeItem*>(parent.internalPointer());
    }

    return parentItem->childCount();
}

int QJsonModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return 2;
}

Qt::ItemFlags QJsonModel::flags(const QModelIndex &index) const
{
    auto item = static_cast<QJsonTreeItem*>(index.internalPointer());

    auto isArray = QJsonValue::Array == item->type();
    auto isObject = QJsonValue::Object == item->type();

    if ((index.column() == 1) && !(isArray || isObject))
    {
        return Qt::ItemIsEditable | QAbstractItemModel::flags(index);
    }
    else
    {
        return QAbstractItemModel::flags(index);
    }
}

QJsonDocument QJsonModel::json() const
{
    auto v = genJson(mRootItem);
    QJsonDocument doc;

    if (v.isObject())
    {
        doc = QJsonDocument(v.toObject());
    }
    else
    {
        doc = QJsonDocument(v.toArray());
    }

    return doc;
}

QJsonValue QJsonModel::genJson(QJsonTreeItem *item) const
{
    auto type = item->type();
    int nchild = item->childCount();

    if (QJsonValue::Object == type)
    {
        QJsonObject jo;
        for (int i = 0; i < nchild; ++i)
        {
            auto ch = item->child(i);
            auto key = ch->key();
            jo.insert(key, genJson(ch));
        }
        return jo;
    }
    else if (QJsonValue::Array == type)
    {
        QJsonArray arr;
        for (int i = 0; i < nchild; ++i)
        {
            auto ch = item->child(i);
            arr.append(genJson(ch));
        }
        return arr;
    }
    else
    {
        return item->value();
    }
}
