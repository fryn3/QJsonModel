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

QJsonTreeItem::QJsonTreeItem(QJsonTreeItem *parent)
    : mParent(parent)
{
}

QJsonTreeItem::QJsonTreeItem(const QJsonValue &value, QJsonTreeItem *parent, const QString &key)
    : mKey(key), mParent(parent)
{
    if (value.isObject())
    {
        mValue = QJsonObject();
        //Get all QJsonValue childs
        for (QString key : value.toObject().keys())
        {
            QJsonValue v = value.toObject().value(key);
            QJsonTreeItem *child = new QJsonTreeItem(v, this, key);
            appendChild(child);
        }
    }
    else if (value.isArray())
    {
        mValue = QJsonArray();
        //Get all QJsonValue childs
        for (QJsonValue v : value.toArray())
        {
            QJsonTreeItem *child = new QJsonTreeItem(v, this);
            appendChild(child);
        }
    }
    else
    {
        mValue = value;
    }
}

QJsonTreeItem::~QJsonTreeItem()
{
    qDeleteAll(mChilds);
}

void QJsonTreeItem::appendChild(QJsonTreeItem *item)
{
    mChilds.append(item);
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

bool QJsonTreeItem::setValue(const QJsonValue &value)
{
    if (isArrayOrObject() || value.isArray() || value.isObject())
    {
        return false;
    }
    mValue = value;
    return true;
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

bool QJsonTreeItem::isArrayOrObject() const
{
    return type() == QJsonValue::Array || type() == QJsonValue::Object;
}

const std::array<QString, 2> QJsonModel::mHEADERS_STR { "key", "value" };

QJsonModel::QJsonModel(QObject *parent)
    : QAbstractItemModel(parent)
    , mRootItem(new QJsonTreeItem())
{
}

QJsonModel::QJsonModel(const QString &fileName, QObject *parent)
    : QAbstractItemModel(parent)
    , mRootItem(new QJsonTreeItem())
{
    load(fileName);
}

QJsonModel::QJsonModel(QIODevice *device, QObject *parent)
    : QAbstractItemModel(parent)
    , mRootItem(new QJsonTreeItem())
{
    load(device);
}

QJsonModel::QJsonModel(const QByteArray &json, QObject *parent)
    : QAbstractItemModel(parent)
    , mRootItem(new QJsonTreeItem())
{
    loadJson(json);
}

QJsonModel::~QJsonModel()
{
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
    QJsonParseError parse_error;
    auto jdoc = QJsonDocument::fromJson(json, &parse_error);

    if (jdoc.isNull())
    {
        qDebug() << Q_FUNC_INFO << "Cannot load json." << parse_error.errorString();
        qDebug() << "The input json string is:" << json;
        return false;
    }

    beginResetModel();
    if (jdoc.isArray())
    {
        mRootItem = std::make_unique<QJsonTreeItem>(QJsonValue(jdoc.array()), nullptr, "root");
    }
    else
    {
        mRootItem = std::make_unique<QJsonTreeItem>(QJsonValue(jdoc.object()), nullptr, "root");
    }
    endResetModel();

    return true;
}

bool QJsonModel::appendToArray(const QByteArray &json)
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
    mRootItem.get()->appendChild(new QJsonTreeItem(value, mRootItem.get()));
    endResetModel();

    return true;
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
            if (item->parent()->value().type() == QJsonValue::Array)
            {
                return QString("[%1]").arg(item->row());
            }
            else
            {
                return item->key();
            }
        }
        else if (index.column() == 1)
        {
            return item->value().toVariant();
        }
    }
    else if (role == Qt::EditRole)
    {
        if (index.column() == 0)
        {
            return item->key();
        }
        else if (index.column() == 1)
        {
            return item->value().toVariant();
        }
    }

    return QVariant();
}

bool QJsonModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (role == Qt::EditRole)
    {
        QJsonTreeItem *item = static_cast<QJsonTreeItem*>(index.internalPointer());
        if (index.column() == 0)
        {
            item->setKey(value.toString());
        }
        else if (index.column() == 1)
        {
            if (!item->setValue(QJsonValue::fromVariant(value)))
            {
                return false;
            }
        }
        emit dataChanged(index, index, {Qt::EditRole});
        return true;
    }

    return false;
}

QVariant QJsonModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (role != Qt::DisplayRole)
    {
        return QVariant();
    }

    if (orientation == Qt::Horizontal && section >= 0 && section < columnCount())
    {
        return mHEADERS_STR.at(section);
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
        parentItem = mRootItem.get();
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

    QJsonTreeItem *item = static_cast<QJsonTreeItem*>(index.internalPointer());
    QJsonTreeItem *parentItem = item->parent();

    if (parentItem == mRootItem.get())
    {
        return QModelIndex();
    }

    return createIndex(parentItem->row(), 0, parentItem);
}

int QJsonModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid())
    {
        return mRootItem->childCount();
    }
    else
    {
        return static_cast<QJsonTreeItem*>(parent.internalPointer())->childCount();
    }
}

int QJsonModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)

    return mHEADERS_STR.size();
}

Qt::ItemFlags QJsonModel::flags(const QModelIndex &index) const
{
    auto item = static_cast<QJsonTreeItem*>(index.internalPointer());

    if (((index.column() == 1) && !(item->isArrayOrObject()))
            || (index.column() == 0 && item->parent()->type() != QJsonValue::Array))
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
    auto v = genJson(mRootItem.get());

    if (v.isArray())
    {
        return QJsonDocument(v.toArray());
    }
    else
    {
        return QJsonDocument(v.toObject());
    }
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
