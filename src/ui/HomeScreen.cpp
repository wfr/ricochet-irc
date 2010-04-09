#include "main.h"
#include "HomeScreen.h"
#include "tor/TorControlManager.h"
#include <QApplication>
#include <QBoxLayout>
#include <QLabel>
#include <QToolButton>
#include <QAction>

HomeScreen::HomeScreen(QWidget *parent)
	: QWidget(parent)
{
	QBoxLayout *layout = new QVBoxLayout(this);
	layout->setMargin(3);

	QBoxLayout *topLayout = new QHBoxLayout;
	layout->addLayout(topLayout);

	createActions();

	createAvatar();

	topLayout->addLayout(createButtons());

	topLayout->addStretch(1);
	topLayout->addWidget(avatar);
	layout->addStretch(1);

	QFrame *line = new QFrame;
	line->setFrameStyle(QFrame::HLine | QFrame::Plain);

	QPalette p = line->palette();
	p.setColor(QPalette::WindowText, p.color(QPalette::Mid));
	line->setPalette(p);

	layout->addWidget(line);

	layout->addLayout(createStatus());
}

void HomeScreen::createAvatar()
{
	avatar = new QLabel;
	avatar->setAlignment(Qt::AlignTop | Qt::AlignLeft);
	avatar->addAction(actChangeAvatar);
	avatar->setContextMenuPolicy(Qt::ActionsContextMenu);

	QImage image = config->value("core/avatar").value<QImage>();
	if (!image.isNull())
		avatar->setPixmap(QPixmap::fromImage(image));
	else
		avatar->setPixmap(QPixmap(":/graphics/avatar-placeholder.png"));
}

void HomeScreen::createActions()
{
	actAddContact = new QAction(QIcon(":/icons/user--plus.png"), tr("Add New Contact"), this);
	actChangeAvatar = new QAction(QIcon(":/icons/image--pencil.png"), tr("Change Avatar"), this);
	actOpenDownloads = new QAction(QIcon(":/icons/folder-open-image.png"), tr("Open Downloads Folder"), this);

	actTestConnection = new QAction(QIcon(":/icons/globe-green.png"), tr("Test Connection"), this);
	actOptions = new QAction(QIcon(":/icons/gear.png"), tr("Options"), this);
	actTorConfig = new QAction(QIcon(":/icons/wall--pencil.png"), tr("Configure Tor"), this);

	QAction *separator = new QAction(this);
	separator->setSeparator(true);

	buttonActions << actAddContact << actChangeAvatar << actOpenDownloads;
	buttonActions << separator;
	buttonActions << actTestConnection << actOptions << actTorConfig;
}

#include <QGridLayout>

QLayout *HomeScreen::createButtons()
{
	QGridLayout *layout = new QGridLayout;
	layout->setSpacing(0);
	layout->setHorizontalSpacing(12);

	int row = 0, column = 0;

	QLabel *heading = new QLabel;
	heading->setPixmap(QPixmap(":/graphics/logotext.png"));
	heading->setContentsMargins(0, 0, 0, 14);
	layout->addWidget(heading, row++, column, 1, 2, Qt::AlignTop | Qt::AlignHCenter);

	for (QList<QAction*>::ConstIterator it = buttonActions.begin(); it != buttonActions.end(); ++it)
	{
		if ((*it)->isSeparator())
		{
			//layout->addSpacing(8);
			//layout->addStretch();
			column++;
			row = 1;
			continue;
		}

		QToolButton *btn = new QToolButton;
		btn->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		btn->setFixedHeight(23);
		btn->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
		btn->setAutoRaise(true);
		btn->setDefaultAction(*it);
		layout->addWidget(btn, row++, column, 1, 1, Qt::AlignTop | Qt::AlignLeft);
	}

	layout->setRowStretch(layout->rowCount()-1, 1);

	return layout;
}

QLayout *HomeScreen::createStatus()
{
	QBoxLayout *layout = new QHBoxLayout;

	QFont font("Calibri");
	font.setPixelSize(13);

	QLabel *statusIcon = new QLabel;
	statusIcon->setPixmap(QPixmap(":/icons/tick-circle.png"));
	statusIcon->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	statusIcon->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
	layout->addWidget(statusIcon);

	torStatus = new QLabel;
	torStatus->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
	torStatus->setFont(font);
	torStatus->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
	layout->addWidget(torStatus);

	torInfo = new QLabel;
	torInfo->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
	font.setBold(true);
	torInfo->setFont(font);

	QPalette p = torInfo->palette();
	p.setColor(QPalette::WindowText, QColor(171, 171, 171));
	torInfo->setPalette(p);

	layout->addWidget(torInfo);

	connect(torManager, SIGNAL(statusChanged(Status,Status)), this, SLOT(updateTorStatus()));
	updateTorStatus();

	return layout;
}

void HomeScreen::updateTorStatus()
{
	QString infoText = QLatin1String("TorIM ") + QApplication::applicationVersion();

	QString torVersion = torManager->torVersion();
	if (!torVersion.isEmpty())
		infoText.append(QLatin1String("\nTor ") + torVersion);

	torInfo->setText(infoText);

	torStatus->setText(torManager->statusText());
}