#ifndef ACCESSDATABASE_H
#define ACCESSDATABASE_H

#define gDatabase AppUtils::getDatabase()
#define gDbSignal emit AppUtils::getDatabaseSignaller()

#include <QDebug>
#include <QMimeData>
#include <QSqlDatabase>
#include <QSqlField>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlResult>
#include <QSqlTableModel>
#include <QVariant>
#include <iostream>

using namespace std;

class AccessDatabase;

class DatabaseObject;

class SqlSelection;

namespace DbStructure {
    const QString TABLE_TRANSFER = "transfer";
    const QString DIVIS_TRANSFER = "divisTansfer";
    const QString FIELD_TRANSFER_ID = "id";
    const QString FIELD_TRANSFER_FILE = "file";
    const QString FIELD_TRANSFER_NAME = "name";
    const QString FIELD_TRANSFER_SIZE = "size";
    const QString FIELD_TRANSFER_MIME = "mime";
    const QString FIELD_TRANSFER_TYPE = "type";
    const QString FIELD_TRANSFER_DIRECTORY = "directory";
    const QString FIELD_TRANSFER_SKIPPEDBYTES = "skippedBytes";
    const QString FIELD_TRANSFER_DEVICEID = "deviceId";
    const QString FIELD_TRANSFER_GROUPID = "groupId";
    const QString FIELD_TRANSFER_FLAG = "flag";
    const QString FIELD_TRANSFER_ACCESSPORT = "accessPort";

    const QString TABLE_TRANSFERGROUP = "transferGroup";
    const QString FIELD_TRANSFERGROUP_ID = "id";
    const QString FIELD_TRANSFERGROUP_SAVEPATH = "savePath";
    const QString FIELD_TRANSFERGROUP_DATECREATED = "dateCreated";

    const QString TABLE_DEVICES = "devices";
    const QString FIELD_DEVICES_ID = "deviceId";
    const QString FIELD_DEVICES_USER = "user";
    const QString FIELD_DEVICES_BRAND = "brand";
    const QString FIELD_DEVICES_MODEL = "model";
    const QString FIELD_DEVICES_BUILDNAME = "buildName";
    const QString FIELD_DEVICES_BUILDNUMBER = "buildNumber";
    const QString FIELD_DEVICES_LASTUSAGETIME = "lastUsedTime";
    const QString FIELD_DEVICES_ISRESTRICTED = "isRestricted";
    const QString FIELD_DEVICES_ISTRUSTED = "isTrusted";
    const QString FIELD_DEVICES_ISLOCALADDRESS = "isLocalAddress";
    const QString FIELD_DEVICES_TMPSECUREKEY = "tmpSecureKey";

    const QString TABLE_DEVICECONNECTION = "deviceConnection";
    const QString FIELD_DEVICECONNECTION_IPADDRESS = "ipAddress";
    const QString FIELD_DEVICECONNECTION_DEVICEID = "deviceId";
    const QString FIELD_DEVICECONNECTION_ADAPTERNAME = "adapterName";
    const QString FIELD_DEVICECONNECTION_LASTCHECKEDDATE = "lastCheckedDate";

    const QString TABLE_CLIPBOARD = "clipboard";
    const QString FIELD_CLIPBOARD_ID = "id";
    const QString FIELD_CLIPBOARD_TEXT = "text";
    const QString FIELD_CLIPBOARD_TIME = "time";

    const QString TABLE_TRANSFERASSIGNEE = "transferAssignee";
    const QString FIELD_TRANSFERASSIGNEE_GROUPID = "groupId";
    const QString FIELD_TRANSFERASSIGNEE_DEVICEID = "deviceId";
    const QString FIELD_TRANSFERASSIGNEE_CONNECTIONADAPTER = "connectionAdapter";
    const QString FIELD_TRANSFERASSIGNEE_ISCLONE = "isClone";

    extern QSqlField generateField(const QString &key, const QVariant::Type &type, bool nullable = true);

    extern QSqlField generateField(const QString &key, const QVariant &type);

    extern QString generateTableCreationSql(const QString &tableName, const QSqlRecord &record, bool mayExist = false);

    extern const char *transformType(const QVariant::Type &type);

    extern QSqlTableModel *gatherTableModel(AccessDatabase *db, DatabaseObject *dbObject);

    extern QSqlTableModel *gatherTableModel(AccessDatabase *db, const QString &tableName);
}

class SqlSelection : public QObject {
public:
    QString tag;
    QString tableName;
    QStringList columns;
    QString where;
    QList<QVariant> whereArgs;
    QString groupBy;
    QString having;
    QString orderBy;
    int limit = -1;

    void bindWhereClause(QSqlQuery &query);

    QString generateSpecifierClause(bool fromStatement = true);

    SqlSelection *setHaving(QString having);

    SqlSelection *setGroupBy(QString field, bool ascending);

    SqlSelection *setGroupBy(QString orderBy);

    SqlSelection *setLimit(int limit);

    SqlSelection *setOrderBy(QString field, bool ascending);

    SqlSelection *setOrderBy(QString limit);

    SqlSelection *setTableName(QString tableName);

    SqlSelection *setWhere(const QString &whereString);

    QSqlQuery *toDeletionQuery();

    QSqlQuery *toInsertionQuery();

    QSqlQuery *toSelectionQuery();

    QString toSelectionColumns();

    QSqlQuery *toUpdateQuery(QSqlRecord query);
};

class DatabaseObject : public QObject {
Q_OBJECT

public:
    explicit DatabaseObject(QObject *parent = nullptr);

    virtual SqlSelection *getWhere() = 0;

    virtual QSqlRecord getValues(AccessDatabase *db) = 0;

    virtual void onGeneratingValues(const QSqlRecord &record) = 0;

    virtual void onUpdatingObject(AccessDatabase *db)
    {}

    virtual void onInsertingObject(AccessDatabase *db)
    {}

    virtual void onRemovingObject(AccessDatabase *db)
    {}
};

class AccessDatabase : public QObject {
Q_OBJECT

    QSqlDatabase *db;

public:
    explicit AccessDatabase(QSqlDatabase *db, QObject *parent = nullptr);

    static QMap<QString, QSqlRecord> *getPassiveTables();

    QSqlDatabase *getDatabase();

    // Template declarations fails to compile on CPP files.
    template<typename T>
    QList<T *> *castQuery(SqlSelection &sqlSelection, T *classInstance)
    {
        auto *resultList = new QList<T *>();
        QSqlQuery *query = sqlSelection.toSelectionQuery();

        query->exec();

        if (query->first())
            do {
                T *dbObject = new T;

                dbObject->onGeneratingValues(query->record());

                resultList->append(dbObject);
            } while (query->next());

        return resultList;
    }

    void initialize();

public slots:

    bool contains(DatabaseObject *dbObject);

    void doSynchronized(const std::function<void(AccessDatabase *)> &listener)
    {
        listener(this);
    }

    bool insert(DatabaseObject *dbObject);

    bool publish(DatabaseObject *dbObject);

    bool reconstructRemote(DatabaseObject *dbObject);

    void reconstruct(DatabaseObject *dbObject);

    bool remove(SqlSelection *selection);

    bool remove(DatabaseObject *dbObject);

    bool update(DatabaseObject *dbObject);

    bool update(SqlSelection *selection, const QSqlRecord &record);

signals:

    bool signalPublish(DatabaseObject *);
};

class AccessDatabaseSignaller : public QObject {
Q_OBJECT

public:
    explicit AccessDatabaseSignaller(AccessDatabase *db, QObject *parent = nullptr)
            : QObject(parent)
    {
        connect(this, &AccessDatabaseSignaller::contains, db, &AccessDatabase::contains, Qt::BlockingQueuedConnection);
        connect(this, &AccessDatabaseSignaller::doNonDirect, db, &AccessDatabase::doSynchronized);
        connect(this, &AccessDatabaseSignaller::doSynchronized, db, &AccessDatabase::doSynchronized,
                Qt::BlockingQueuedConnection);
        connect(this, &AccessDatabaseSignaller::insert, db, &AccessDatabase::insert, Qt::BlockingQueuedConnection);
        connect(this, &AccessDatabaseSignaller::publish, db, &AccessDatabase::publish, Qt::BlockingQueuedConnection);
        connect(this, &AccessDatabaseSignaller::reconstruct,
                db, &AccessDatabase::reconstructRemote, Qt::BlockingQueuedConnection);
        connect(this, SIGNAL(remove(SqlSelection * )), db, SLOT(remove(SqlSelection * )), Qt::BlockingQueuedConnection);
        connect(this, SIGNAL(remove(DatabaseObject * )),
                db, SLOT(remove(DatabaseObject * )), Qt::BlockingQueuedConnection);
        connect(this,
                SIGNAL(update(SqlSelection * ,
                               const QSqlRecord & )),
                db, SLOT(update(SqlSelection * ,
                                 const QSqlRecord & )), Qt::BlockingQueuedConnection);
        connect(this, SIGNAL(update(DatabaseObject * )),
                db, SLOT(update(DatabaseObject * )), Qt::BlockingQueuedConnection);
    }

    void operator<<(const std::function<void(AccessDatabase *)> &listener)
    {
        emit doSynchronized(listener);
    }

signals:

    bool contains(DatabaseObject *dbObject);

    void doNonDirect(const std::function<void(AccessDatabase *)> &listener);

    void doSynchronized(const std::function<void(AccessDatabase *)> &listener);

    bool insert(DatabaseObject *dbObject);

    bool publish(DatabaseObject *dbObject);

    bool reconstruct(DatabaseObject *dbObject);

    bool remove(SqlSelection *selection);

    bool remove(DatabaseObject *dbObject);

    bool update(DatabaseObject *dbObject);

    bool update(SqlSelection *selection, const QSqlRecord &values);
};

#endif // ACCESSDATABASE_H