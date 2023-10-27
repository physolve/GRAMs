#pragma once

#include <QObject>
#include <QVariantList>

class MyData : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList  data    READ data     WRITE setData     NOTIFY JSONdataChanged)
    Q_PROPERTY(bool          result  READ result   WRITE setResult   NOTIFY resultChanged)
    Q_PROPERTY(int           length  READ length   WRITE setLength   NOTIFY lengthChanged)

    Q_PROPERTY(QVariantList cfg     READ cfg    WRITE setStartCfg   NOTIFY startCfgChanged)
    Q_PROPERTY(int  lengthCfg       READ lengthCfg  WRITE setCfgLength  NOTIFY lengthCfgChanged)

    public:
    MyData();

    /*!
        * \brief data function returns a list of items.
        * \return type as QVarianList. 
        */
    QVariantList data () const;

    /*!
        * \brief result function returns final result by status value!
        * \return type as boolian.
        */
    bool result       () const;

    /*!
        * \brief : length function returns total item count!
        * \return type as int
        */
    int  length       () const;


    QVariantList cfg    () const;
    int lengthCfg       () const;


    /*!
        * \brief : fileExists function checks file path!
        * \param : path is string of current file path.
        * \return type as boolian.
        */
    bool fileExists(QString path);

    /*!
        * \brief : parse function gets json file from user to convert.
        * \param : path is string of current file path.
        */
    Q_INVOKABLE void parse(QString path);

    Q_INVOKABLE bool saveJson(QVariantMap test) const;

    //SLOTS
    public slots:
    void setData(const QVariantList& data);
    void setResult(bool result);
    void setLength(int length);

    void setStartCfg(const QVariantList& startCfg);
    void setCfgLength(int cfgLength);

    //SIGNALS
    signals:
    void JSONdataChanged(const QVariantList& data);
    void resultChanged(bool result);
    void lengthChanged(int length);

    void startCfgChanged(const QVariantList& startCfg);
    void lengthCfgChanged(int cfgLength);

    private:
    QVariantList  m_data;
    bool          m_result = {false};
    int           m_length = {0};

    QVariantList  m_startCfg;
    int           m_cfgLength = {0};
};