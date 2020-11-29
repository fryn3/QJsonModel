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

#ifndef QJSONMODEL_H
#define QJSONMODEL_H

#include <QAbstractItemModel>
#include <QJsonDocument>
#include <QJsonValue>
#include <QJsonArray>
#include <QJsonObject>
#include <QIcon>

/*!
 * \class QJsonTreeItem
 * \brief QJsonTreeItem Tree Node (~data storage for one item).
 *
 * Each node has a key and a value. QJsonTreeItem also saves information about
 * its parent and child nodes. QJsonTreeItem is used in the QJsonModel class.
 */
class QJsonTreeItem
{
public:
    explicit QJsonTreeItem(QJsonTreeItem *parent = nullptr);
    explicit QJsonTreeItem(const QJsonValue &value, QJsonTreeItem *parent = nullptr, const QString &key = QString());
    ~QJsonTreeItem();
    void appendChild(QJsonTreeItem *item);
    QJsonTreeItem *child(int row);
    QJsonTreeItem *parent();
    int childCount() const;

    /*!
     * \brief Returns the index from the parent.
     * \return the index from the parent list mChilds.
     */
    int row() const;
    void setKey(const QString &key);
    bool setValue(const QJsonValue &value);
    const QString &key() const;
    const QJsonValue &value() const;
    QJsonValue::Type type() const;
    bool isArrayOrObject() const;

private:
    QString mKey;
    QJsonValue mValue;
    QList<QJsonTreeItem*> mChilds;
    QJsonTreeItem *mParent;
};

/*!
 * \class QJsonModel
 * \brief The QJsonModel class outputs a JSON file in tree format.
 */
class QJsonModel : public QAbstractItemModel
{
    Q_OBJECT

    static const std::array<QString, 2> mHEADERS_STR;
public:
    explicit QJsonModel(QObject *parent = nullptr);
    explicit QJsonModel(const QString &fileName, QObject *parent = nullptr);
    explicit QJsonModel(QIODevice *device, QObject *parent = nullptr);
    explicit QJsonModel(const QByteArray &json, QObject *parent = nullptr);
    virtual ~QJsonModel();
    bool load(const QString &fileName);
    bool load(QIODevice *device);
    bool loadJson(const QByteArray &json);

    /*!
     * \brief Appending a JSON node at the end of the array
     *
     * Works only if mRootItem->type() == QJsonValue::Array.
     * \param json - array in JSON format.
     * \return true if success.
     */
    bool appendToArray(const QByteArray &json);
    QVariant data(const QModelIndex &index, int role) const Q_DECL_OVERRIDE;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) Q_DECL_OVERRIDE;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const Q_DECL_OVERRIDE;
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    QModelIndex parent(const QModelIndex &index) const Q_DECL_OVERRIDE;
    int rowCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    int columnCount(const QModelIndex &parent = QModelIndex()) const Q_DECL_OVERRIDE;
    Qt::ItemFlags flags(const QModelIndex &index) const Q_DECL_OVERRIDE;
    QJsonDocument json() const;

private:
    QJsonValue genJson(QJsonTreeItem *item) const;

private:
    std::unique_ptr<QJsonTreeItem> mRootItem;
};


#endif
