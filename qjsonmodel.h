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
    explicit QJsonTreeItem(const QVariant &value, QJsonTreeItem *parent = nullptr, const QString &key = QString());
    ~QJsonTreeItem() = default;
    void appendChild(std::shared_ptr<QJsonTreeItem> item);
    QJsonTreeItem *parent();
    std::shared_ptr<QJsonTreeItem> child(int row);
    int childCount() const;

    /*!
     * \brief Returns the index from the parent.
     * \return the index from the parent list mChilds or -1.
     */
    int row() const;
    bool setKey(const QString &key);
    bool setValue(const QJsonValue &value);
    bool setValue(const QVariant &value);
    const QString &key() const;
    const QJsonValue &value() const;
    QJsonValue::Type type() const;
    bool isArrayOrObject() const;
    QJsonValue jsonValue() const;

private:
    QJsonTreeItem *mParent;
    QString mKey;
    QJsonValue mValue;
    QList<std::shared_ptr<QJsonTreeItem>> mChilds;
};

/*!
 * \class QJsonModel
 * \brief The QJsonModel class outputs a JSON file in tree format.
 */
class QJsonModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    static const QString ITEM_NAME;     // QJsonModel
    static const bool IS_QML_REG;

    enum Columns {
        ColBegin = Qt::UserRole,
        ColKey = ColBegin,
        ColValue,
        ColType,

        ColEnd
    };
    Q_ENUM(Columns)
    static const std::array<QString, ColEnd - ColBegin> HEADERS_STR;

    explicit QJsonModel(QObject *parent = nullptr);
    explicit QJsonModel(const QString &fileName, QObject *parent = nullptr);
    explicit QJsonModel(QIODevice *device, QObject *parent = nullptr);
    explicit QJsonModel(const QByteArray &json, QObject *parent = nullptr);
    virtual ~QJsonModel() = default;
    Q_INVOKABLE bool load(const QString &fileName);
    bool load(QIODevice *device);
    Q_INVOKABLE bool load(const QByteArray &json);
    bool load(const QJsonDocument &json);
    Q_INVOKABLE void clear();
public slots:
    bool addChildren(const QModelIndex &index, const QVariant &value = QVariant(), const QString &key = QString());
    bool addSibling(const QModelIndex &index, const QVariant &value = QVariant(), const QString &key = QString());

public:
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
    QHash<int, QByteArray> roleNames() const Q_DECL_OVERRIDE;
    QJsonDocument toJsonDoc() const;
    Q_INVOKABLE QJsonValue toJson() const;
    Q_INVOKABLE QCborValue toCbor() const;

    /*!
     * \brief Переводит в QByteArray.
     *
     * Модет переводить в строковоковое представление (при isJson = true),
     * либо в бинарном(CBOR) виде (при isJson = false).
     * \param isJson - тип возвращаемого массива.
     * \return массив байт.
     */
    Q_INVOKABLE QByteArray toByteArray(bool isJson) const;
private:
    std::shared_ptr<QJsonTreeItem> mRootItem;
};


#endif
