/* Duplicated from qtbase/src/gui/util/qvalidator.h */
#ifndef QVALIDATOR_H
#define QVALIDATOR_H

#include <QObject>
#include <QRegularExpression>
#include <QString>
#include <QLocale>

class QValidator : public QObject
{
    Q_OBJECT
public:
    explicit QValidator(QObject * parent = Q_NULLPTR);
    ~QValidator();

    enum State {
        Invalid,
        Intermediate,
        Acceptable
    };

    void setLocale(const QLocale &locale_);
    QLocale locale() const;

    virtual State validate(QString &, int &) const = 0;
    virtual void fixup(QString &) const;

Q_SIGNALS:
    void changed();

protected:

private:
    QLocale locale_;


    Q_DISABLE_COPY(QValidator)
};



class QRegularExpressionValidator : public QValidator
{
    Q_OBJECT
    Q_PROPERTY(QRegularExpression regularExpression READ regularExpression WRITE setRegularExpression NOTIFY regularExpressionChanged)

public:
    explicit QRegularExpressionValidator(QObject *parent = Q_NULLPTR);
    explicit QRegularExpressionValidator(const QRegularExpression &re, QObject *parent = Q_NULLPTR);
    ~QRegularExpressionValidator();

    virtual QValidator::State validate(QString &input, int &pos) const;

    QRegularExpression regularExpression() const;

public Q_SLOTS:
    void setRegularExpression(const QRegularExpression &re);

Q_SIGNALS:
    void regularExpressionChanged(const QRegularExpression &re);

private:
    QRegularExpression origRe; // the one set by the user
    QRegularExpression usedRe; // the one actually used

    Q_DISABLE_COPY(QRegularExpressionValidator)
};

#endif // QVALIDATOR_H
