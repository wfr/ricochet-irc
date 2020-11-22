#include "QRegularExpressionValidator.h"

QT_BEGIN_NAMESPACE

/*!
    Sets up the validator. The \a parent parameter is
    passed on to the QObject constructor.
*/

QValidator::QValidator(QObject * parent)
    : QObject(parent)
{
}

/*!
    Destroys the validator, freeing any storage and other resources
    used.
*/

QValidator::~QValidator()
{
}


/*!
    Sets the \a locale that will be used for the validator. Unless
    setLocale has been called, the validator will use the default
    locale set with QLocale::setDefault(). If a default locale has not
    been set, it is the operating system's locale.
    \sa locale(), QLocale::setDefault()
*/
void QValidator::setLocale(const QLocale &locale)
{
    if (this->locale_ != locale) {
        this->locale_ = locale;
        emit changed();
    }
}

QLocale QValidator::locale() const
{
    return locale_;
}

/*!
    \fn void QValidator::fixup(QString & input) const
    This function attempts to change \a input to be valid according to
    this validator's rules. It need not result in a valid string:
    callers of this function must re-test afterwards; the default does
    nothing.
    Reimplementations of this function can change \a input even if
    they do not produce a valid string. For example, an ISBN validator
    might want to delete every character except digits and "-", even
    if the result is still not a valid ISBN; a surname validator might
    want to remove whitespace from the start and end of the string,
    even if the resulting string is not in the list of accepted
    surnames.
*/

void QValidator::fixup(QString &) const
{
}


/*!
    Constructs a validator with a \a parent object that accepts
    any string (including an empty one) as valid.
*/

QRegularExpressionValidator::QRegularExpressionValidator(QObject *parent)
    : QValidator(parent)
{
    // origRe in the private will be an empty QRegularExpression,
    // and therefore this validator will match any string.
}

/*!
    Constructs a validator with a \a parent object that
    accepts all strings that match the regular expression \a re.
*/

QRegularExpressionValidator::QRegularExpressionValidator(const QRegularExpression &re, QObject *parent)
    : QValidator(parent)
{
    setRegularExpression(re);
}


/*!
    Destroys the validator.
*/

QRegularExpressionValidator::~QRegularExpressionValidator()
{
}

/*!
    Returns \l Acceptable if \a input is matched by the regular expression for
    this validator, \l Intermediate if it has matched partially (i.e. could be
    a valid match if additional valid characters are added), and \l Invalid if
    \a input is not matched.
    In case the \a input is not matched, the \a pos parameter is set to
    the length of the \a input parameter; otherwise, it is not modified.
    For example, if the regular expression is \b{\\w\\d\\d} (word-character,
    digit, digit) then "A57" is \l Acceptable, "E5" is \l Intermediate, and
    "+9" is \l Invalid.
    \sa QRegularExpression::match()
*/

QValidator::State QRegularExpressionValidator::validate(QString &input, int &pos) const
{
    // We want a validator with an empty QRegularExpression to match anything;
    // since we're going to do an exact match (by using d->usedRe), first check if the rx is empty
    // (and, if so, accept the input).
    if (origRe.pattern().isEmpty())
        return QValidator::State::Acceptable;

    const QRegularExpressionMatch m = usedRe.match(input, 0, QRegularExpression::PartialPreferCompleteMatch);
    if (m.hasMatch()) {
        return QValidator::State::Acceptable;
    } else if (input.isEmpty() || m.hasPartialMatch()) {
        return QValidator::State::Intermediate;
    } else {
        pos = input.size();
        return QValidator::State::Invalid;
    }
}

/*!
    \property QRegularExpressionValidator::regularExpression
    \brief the regular expression used for validation
    By default, this property contains a regular expression with an empty
    pattern (which therefore matches any string).
*/

QRegularExpression QRegularExpressionValidator::regularExpression() const
{
    return origRe;
}

/*!
    \internal
    Sets \a re as the regular expression. It wraps the regexp that's actually used
    between \\A and \\z, therefore forcing an exact match.
*/
void QRegularExpressionValidator::setRegularExpression(const QRegularExpression &re)
{
    if (origRe != re) {
        usedRe = origRe = re; // copies also the pattern options
        usedRe.setPattern(QStringLiteral("\\A(?:") + re.pattern() + QStringLiteral(")\\z"));
        emit regularExpressionChanged(re);
        emit changed();
    }
}

QT_END_NAMESPACE
