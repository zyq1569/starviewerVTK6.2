/*************************************************************************************
  Copyright (C) 2014 Laboratori de Gràfics i Imatge, Universitat de Girona &
  Institut de Diagnòstic per la Imatge.
  Girona 2014. All rights reserved.
  http://starviewer.udg.edu

  This file is part of the Starviewer (Medical Imaging Software) open source project.
  It is subject to the license terms in the LICENSE file found in the top-level
  directory of this distribution and at http://starviewer.udg.edu/license. No part of
  the Starviewer (Medical Imaging Software) open source project, including this file,
  may be copied, modified, propagated, or distributed except according to the
  terms contained in the LICENSE file.
 *************************************************************************************/

#include "qapplicationmainwindow.h"

#include "extensionhandler.h"
#include "extensionworkspace.h"
#include "logging.h"
#include "qlogviewer.h"
#include "patient.h"
#include "qconfigurationdialog.h"
#include "volume.h"
#include "settings.h"
#include "extensionfactory.h"
#include "extensionmediatorfactory.h"
#include "starviewerapplication.h"
#include "statswatcher.h"
#include "databaseinstallation.h"
#include "interfacesettings.h"
#include "starviewerapplicationcommandline.h"
#include "risrequestwrapper.h"
#include "qaboutdialog.h"
#include "externalapplication.h"
#include "externalapplicationsmanager.h"
#include "queryscreen.h"
#include "risrequestmanager.h"

// Pel LanguageLocale
#include "coresettings.h"
#include "inputoutputsettings.h"
#include "applicationversionchecker.h"
#include "screenmanager.h"
#include "qscreendistribution.h"
#include "volumerepository.h"
#include "applicationstylehelper.h"
#include "qdiagnosistest.h"

// Amb starviewer lite no hi haurà hanging protocols, per tant no els carregarem
#ifndef STARVIEWER_LITE
#include "hangingprotocolsloader.h"
#include "customwindowlevelsloader.h"
#include "studylayoutconfigsloader.h"
#endif

#ifdef STARVIEWER_CE
#include "qmedicaldeviceinformationdialog.h"
#endif // STARVIEWER_CE

// Qt
#include <QAction>
#include <QSignalMapper>
#include <QMenuBar>
#include <QCloseEvent>
#include <QMessageBox>
#include <QApplication>
#include <QLocale>
#include <QProgressDialog>
#include <QDesktopServices>
#include <QPair>
#include <QWidgetAction>
#include <QShortcut>
#include <QToolBar>
// Shortucts
#include "shortcuts.h"
#include "shortcutmanager.h"

//------------------------------------
#include "imagethumbnaildockwidget.h"
#include "patientbrowsermenu.h"
//------------------------------------

namespace udg {

typedef SingletonPointer<QueryScreen> QueryScreenSingleton;

// Per processar les opcions entrades per línia de comandes hem d'utilitzar un Singleton de StarviewerApplicationCommandLine, això ve degut a que
// d'instàncies de QApplicationMainWindow en tenim tantes com finestres obertes d'Starviewer tinguem. Instàncies deQApplicationMainWindow es crees
// i es destrueixen a mesura que s'obre una nova finestra o es tanca una finestra d'Starviewer per tant no podem responsabilitzar a cap
// QApplicationMainWindow que s'encarregui de antendre les peticions rebudes via arguments o rebudes d'altres instàncies d'Starviewer a través
// de QtSingleApplication, perquè no podem garantir que cap QApplicationMainWindow estigui viva durant tota l'execució d'Starviewer, per encarregar-se
// de processar els arugments de línia de comandes.

// Per això el que s'ha fet és que totes les QApplicationMainWindow es connectin a un signal de la mateixa instància de
// StarviewerSingleApplicationCommandLineSingleton, aquest signal és newOptionsToRun() que s'emet cada vegada que es reben nous arguments ja
// procedeixin de la mateixa instància al iniciar-la o d'altres instàncies via QtSingleApplication. Una vegada s'ha emés el signal les instàncies
// de QApplicationMainWindow a mesura que responen al signal amb el mètode takeOptionToRun() van processan tots els arguments fins que no en
// queda cap per processar.

// L'opció que processa una instància de QApplicationMainWindow obtinguda a través del mètode takeOptionToRun() desapereix de la llista d'opcions
// per processar de StarviewerApplicationCommandLine, de manera que tot i que totes les instàncies de QApplicationMainWindow poden processar
// opcions rebuts, cada opció només serà processat per la primera instància que l'agafi a través del mètode takeOptionToRun().

typedef SingletonPointer<StarviewerApplicationCommandLine> StarviewerSingleApplicationCommandLineSingleton;

QApplicationMainWindow::QApplicationMainWindow(QWidget *parent)
    : QMainWindow(parent), m_patient(0), m_isBetaVersion(false)
{
    connect(StarviewerSingleApplicationCommandLineSingleton::instance(), SIGNAL(newOptionsToRun()), SLOT(newCommandLineOptionsToRun()));

    this->setAttribute(Qt::WA_DeleteOnClose);
    m_extensionWorkspace = new ExtensionWorkspace(this);
    this->setCentralWidget(m_extensionWorkspace);

    m_DockImageThumbnail = new ImageThumbnailDockWidget("",this);//("Thumbnail");
    addDockWidget(Qt::LeftDockWidgetArea,m_DockImageThumbnail);
    //m_DockImageThumbnail->setFeatures(QDockWidget::DockWidgetMovable);
    m_DockImageThumbnail->setObjectName("ImageThumbnail");

    DatabaseInstallation databaseInstallation;
    if (!databaseInstallation.checkStarviewerDatabase())
    {
        QString errorMessage = databaseInstallation.getErrorMessage();
        QMessageBox::critical(0, ApplicationNameString, tr("There have been some errors:") + "\n" + errorMessage + "\n\n" + 
                                                    tr("You can resolve this error at Tools > Configuration > Local Database."));
    }

    m_extensionHandler = new ExtensionHandler(this);

    m_logViewer = new QLogViewer(this);

    //createActions();
    //createMenus();

	//----------------------------------
	//add m_mainToolbar
	m_mainToolbar = new QToolBar(this);
	this->addToolBar(Qt::TopToolBarArea, m_mainToolbar);
	m_mainToolbar->setIconSize(QSize(30, 30));
	m_mainToolbar->layout()->setSpacing(10);
	m_mainToolbar->setFloatable(false);
	m_mainToolbar->setMovable(false);
	m_mainToolbar->setToolButtonStyle(Qt::ToolButtonIconOnly);
	//m_mainToolbar->setStyleSheet("background-color:rgb(150,150,150)}");
	//m_mainToolbar->setStyleSheet("background-color:lightgray;");
	//m_mainToolbar->setStyleSheet("background-color:rgb(128,128,128);");
	//m_mainToolbar->setStyleSheet("QToolButton:!hover {background-color:lightgray} QToolBar {background: rgb(150,150,150)}");
	m_mainToolbar->setStyleSheet("QToolButton:!hover {background-color:lightgray} QToolBar {background:lightgray}");
	//this->setStyleSheet("background-color:lightgray}");
	//QAction *actionHide = new QAction(QIcon(":/images/showhide.png"), "show or hide Thumbnail ...", this);
	//m_mainToolbar->addAction(actionHide);
	//connect(actionHide, SIGNAL(triggered()), SLOT(showhideDockImage()));//Open an existing DICOM folder

	QAction *actionFile = new QAction(QIcon(":/images/folderopen.png"), "Open Files from a Directory...", this);
	m_mainToolbar->addAction(actionFile);
	connect(actionFile, &QAction::triggered, [this] { m_extensionHandler->request(6); });//Open an existing DICOM folder
	m_mainToolbar->insertSeparator(actionFile);

    QAction *action3D = new QAction(QIcon(":/images/3D.svg"), "3D Viewer", this);
	m_mainToolbar->addAction(action3D);
	connect(action3D, &QAction::triggered, [this] { m_extensionHandler->request("Q3DViewerExtension"); });

	//QAction *actionMultScreens = new QAction(QIcon(":/images/icons/Monitor.svg"), "MultiScreens", this);
	//m_mainToolbar->addAction(actionMultScreens);
	//connect(actionMultScreens, SIGNAL(triggered(bool)), this, SLOT(maximizeMultipleScreens()));

	//QAction *actionNextScreens = new QAction(QIcon(":/images/icons/MonitorNext.svg"), "Next Desktop", this);
	//m_mainToolbar->addAction(actionNextScreens);
	//actionNextScreens->setShortcuts(ShortcutManager::getShortcuts(Shortcuts::MoveToNextDesktop));
	//actionNextScreens->setToolTip("Next Desktop|Ctrl + Shift + Right");
	//connect(actionNextScreens, SIGNAL(triggered(bool)), this, SLOT(moveToNextDesktop()));

	QMenu* menu = new QMenu("windows", this);
	QAction* nextScreens = menu->addAction("NextScreens");
	QMenu* menuSub = new QMenu("...", this); //创建第二个menu对象
	nextScreens->setMenu(menuSub);
    nextScreens->setIcon(QIcon(":/images/MonitorNext.svg"));
	nextScreens->setToolTip("Next Desktop | Ctrl + Shift + Right");
	nextScreens->setShortcuts(ShortcutManager::getShortcuts(Shortcuts::MoveToNextDesktop));
	connect(nextScreens, SIGNAL(triggered(bool)), this, SLOT(moveToNextDesktop()));
	QScreenDistribution *screen = new QScreenDistribution(this);
	QWidgetAction* subDesk = new QWidgetAction(this);
	subDesk->setDefaultWidget(screen);
	menuSub->addAction(subDesk);
	connect(screen, SIGNAL(screenClicked(int)), this, SLOT(moveToDesktop(int)));
	m_mainToolbar->addAction(nextScreens);

	//QAction *actionMPR = new QAction(QIcon(":/images/icons/MPR-2D.svg"), "MPR-2D Viewer", this);
	//m_mainToolbar->addAction(actionMPR);
	//connect(actionMPR, &QAction::triggered, [this] { m_extensionHandler->request("MPRExtension"); });

	QAction *actionPACS = new QAction(QIcon(":/images/pacsNodes"), "PACS Images", this);
	m_mainToolbar->addAction(actionPACS);
	connect(actionPACS, &QAction::triggered, [this] { m_extensionHandler->request(7); });

	QAction *actionConfig = new QAction(QIcon(":/images/preferences.png"), "&Configuration...", this);
	actionConfig->setShortcuts(ShortcutManager::getShortcuts(Shortcuts::Preferences));
	m_mainToolbar->addAction(actionConfig);
	connect(actionConfig, SIGNAL(triggered()), SLOT(showConfigurationDialog()));

	QMenu* menuhelp = new QMenu("AppHelp", this);
	QAction* aboutAction = menuhelp->addAction("&About");
	QMenu* menuhelpSub = new QMenu("...", this); //创建第二个menu对象
	aboutAction->setMenu(menuhelpSub);
	aboutAction->setIcon(QIcon(":/images/help.ico"));
	aboutAction->setToolTip("Show the application's About box");
	connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));

	QAction *sysInfoTest = new QAction(QIcon(":/images/help.ico"), "SysTest", this);
	menuhelpSub->addAction(sysInfoTest);
	connect(sysInfoTest, SIGNAL(triggered()), this, SLOT(showDiagnosisTestDialog()));
	m_mainToolbar->addAction(aboutAction);
	//---------------------------------------------------------------------------------------------------
	//----------------------------------

    m_applicationVersionChecker = new ApplicationVersionChecker(this);
    m_applicationVersionChecker->checkReleaseNotes();

    // Llegim les configuracions de l'aplicació, estat de la finestra, posicio,etc
    readSettings();
    // Icona de l'aplicació
    this->setWindowIcon(QIcon(":/images/starviewer.png"));
    this->setWindowTitle(ApplicationNameString);

// Amb starviewer lite no hi haurà hanging protocols, per tant no els carregarem
#ifndef STARVIEWER_LITE
    // Càrrega dels repositoris que necessitem tenir carregats durant tota l'aplicació
    // Només carregarem un cop per sessió/instància d'starviewer
    static bool repositoriesLoaded = false;
    if (!repositoriesLoaded)
    {
        HangingProtocolsLoader hangingProtocolsLoader;
        hangingProtocolsLoader.loadDefaults();

        CustomWindowLevelsLoader customWindowLevelsLoader;
        customWindowLevelsLoader.loadDefaults();

        StudyLayoutConfigsLoader layoutConfigsLoader;
        layoutConfigsLoader.load();

        repositoriesLoaded = true;
    }
#endif

    // Creem el progress dialog que notificarà la càrrega de volums
    //m_progressDialog = new QProgressDialog(this);
    //m_progressDialog->setModal(true);
    //m_progressDialog->setRange(0, 100);
    //m_progressDialog->setMinimumDuration(0);
    //m_progressDialog->setWindowTitle(tr("Loading"));
    //m_progressDialog->setLabelText(tr("Loading data, please wait..."));
    //m_progressDialog->setCancelButton(0);

#ifdef BETA_VERSION
    markAsBetaVersion();
    showBetaVersionDialog();
#endif

    computeDefaultToolTextSize();

    m_statsWatcher = new StatsWatcher("Menu triggering", this);
    m_statsWatcher->addTriggerCounter(m_fileMenu);
    m_statsWatcher->addTriggerCounter(m_visualizationMenu);
    m_statsWatcher->addTriggerCounter(m_toolsMenu);
    m_statsWatcher->addTriggerCounter(m_helpMenu);
    m_statsWatcher->addTriggerCounter(m_languageMenu);
    m_statsWatcher->addTriggerCounter(m_windowMenu);
}

QApplicationMainWindow::~QApplicationMainWindow()
{
    writeSettings();
    this->killBill();
    delete m_extensionWorkspace;
    delete m_extensionHandler;
}

void QApplicationMainWindow::createActions()
{
    m_signalMapper = new QSignalMapper(this);
    connect(m_signalMapper, SIGNAL(mapped(int)), m_extensionHandler, SLOT(request(int)));
    connect(m_signalMapper, SIGNAL(mapped(const QString)), m_extensionHandler, SLOT(request(const QString)));

    m_newAction = new QAction(this);
    m_newAction->setText(tr("&New Window"));
    m_newAction->setShortcuts(ShortcutManager::getShortcuts(Shortcuts::NewWindow));
    m_newAction->setStatusTip(tr("Open a new working window"));
    m_newAction->setIcon(QIcon(":/images/new.png"));
    connect(m_newAction, SIGNAL(triggered()), SLOT(openBlankWindow()));

    m_openAction = new QAction(this);
    m_openAction->setText(tr("&Open Files..."));
    m_openAction->setShortcuts(ShortcutManager::getShortcuts(Shortcuts::OpenFile));
    m_openAction->setStatusTip(tr("Open one or several existing volume files"));
    m_openAction->setIcon(QIcon(":/images/open.png"));
    m_signalMapper->setMapping(m_openAction, 1);
    connect(m_openAction, SIGNAL(triggered()), m_signalMapper, SLOT(map()));

    m_openDirAction = new QAction(this);
    m_openDirAction->setText(tr("Open Files from a Directory..."));
    m_openDirAction->setShortcuts(ShortcutManager::getShortcuts(Shortcuts::OpenDirectory));
    m_openDirAction->setStatusTip(tr("Open an existing DICOM folder"));
    m_openDirAction->setIcon(QIcon(":/images/openDicom.png"));
    m_signalMapper->setMapping(m_openDirAction, 6);
    connect(m_openDirAction, SIGNAL(triggered()), m_signalMapper, SLOT(map()));

    m_pacsAction = new QAction(this);
#ifdef STARVIEWER_LITE
    // El menú "PACS" es dirà "Exams"
    m_pacsAction->setText(tr("&Exams..."));
    m_pacsAction->setShortcuts(ShortcutManager::getShortcuts(Shortcuts::OpenExams));
    m_pacsAction->setStatusTip(tr("Browse exams"));
#else
    m_pacsAction->setText(tr("&PACS..."));
    m_pacsAction->setShortcuts(ShortcutManager::getShortcuts(Shortcuts::OpenPACS));
    m_pacsAction->setStatusTip(tr("Open PACS Query Screen"));

    m_localDatabaseAction = new QAction(this);
    m_localDatabaseAction->setText(tr("&Local Database Studies..."));
    m_localDatabaseAction->setShortcuts(ShortcutManager::getShortcuts(Shortcuts::OpenLocalDatabaseStudies));
    m_localDatabaseAction->setStatusTip(tr("Browse local database studies"));
    m_localDatabaseAction->setIcon(QIcon(":/images/database.png"));
    m_signalMapper->setMapping(m_localDatabaseAction, 10);
    connect(m_localDatabaseAction, SIGNAL(triggered()), m_signalMapper, SLOT(map()));
#endif
    // TODO potser almenys per la versió Lite caldria canviar la icona
    m_pacsAction->setIcon(QIcon(":/images/pacsQuery.png"));
    m_signalMapper->setMapping(m_pacsAction, 7);
    connect(m_pacsAction, SIGNAL(triggered()), m_signalMapper, SLOT(map()));

    m_openDICOMDIRAction = new QAction(this);
    m_openDICOMDIRAction->setText(tr("Open DICOMDIR..."));
    m_openDICOMDIRAction->setShortcuts(ShortcutManager::getShortcuts(Shortcuts::OpenDICOMDIR));
    m_openDICOMDIRAction->setStatusTip(tr("Open DICOMDIR from CD, DVD, USB flash drive or hard disk"));
    m_openDICOMDIRAction->setIcon(QIcon(":/images/openDICOMDIR.png"));
    m_signalMapper->setMapping(m_openDICOMDIRAction, 8);
    connect(m_openDICOMDIRAction, SIGNAL(triggered()), m_signalMapper, SLOT(map()));

    QStringList extensionsMediatorNames = ExtensionMediatorFactory::instance()->getFactoryIdentifiersList();
    foreach (const QString &name, extensionsMediatorNames)
    {
        ExtensionMediator *mediator = ExtensionMediatorFactory::instance()->create(name);

        if (mediator)
        {
            QAction *action = new QAction(this);
            action->setText(mediator->getExtensionID().getLabel());
            action->setStatusTip(tr("Open the %1 Application").arg(mediator->getExtensionID().getLabel()));
            action->setEnabled(false);
            m_signalMapper->setMapping(action, mediator->getExtensionID().getID());
            connect(action, SIGNAL(triggered()), m_signalMapper, SLOT(map()));
            m_actionsList.append(action);

            delete mediator;
        }
        else
        {
            ERROR_LOG("Error carregant el mediator de " + name);
        }
    }

    m_maximizeAction = new QAction(this);
    m_maximizeAction->setText(tr("Maximize to Multiple Screens"));
    m_maximizeAction->setStatusTip(tr("Maximize the window to as many screens as possible"));
    m_maximizeAction->setCheckable(false);
    m_maximizeAction->setShortcuts(ShortcutManager::getShortcuts(Shortcuts::MaximizeMultipleScreens));
    connect(m_maximizeAction, SIGNAL(triggered(bool)), this, SLOT(maximizeMultipleScreens()));

    m_moveToDesktopAction = new QWidgetAction(this);
    QScreenDistribution *screenDistribution = new QScreenDistribution(this);
    m_moveToDesktopAction->setDefaultWidget(screenDistribution);
    m_moveToDesktopAction->setText(tr("Move to Screen"));
    m_moveToDesktopAction->setStatusTip(tr("Move the window to the screen..."));
    m_moveToDesktopAction->setCheckable(false);
    connect(screenDistribution, SIGNAL(screenClicked(int)), this, SLOT(moveToDesktop(int)));
    
    m_moveToPreviousDesktopAction = new QAction(this);
    m_moveToPreviousDesktopAction->setText(tr("Move to previous screen"));
    m_moveToPreviousDesktopAction->setStatusTip(tr("Move the window to the previous screen"));
    m_moveToPreviousDesktopAction->setCheckable(false);
    m_moveToPreviousDesktopAction->setShortcuts(ShortcutManager::getShortcuts(Shortcuts::MoveToPreviousDesktop));
    connect(m_moveToPreviousDesktopAction, SIGNAL(triggered(bool)), SLOT(moveToPreviousDesktop()));

    m_moveToNextDesktopAction = new QAction(this);
    m_moveToNextDesktopAction->setText(tr("Move to next screen"));
    m_moveToNextDesktopAction->setStatusTip(tr("Move the window to the next screen"));
    m_moveToNextDesktopAction->setCheckable(false);
    m_moveToNextDesktopAction->setShortcuts(ShortcutManager::getShortcuts(Shortcuts::MoveToNextDesktop));
    connect(m_moveToNextDesktopAction, SIGNAL(triggered(bool)), SLOT(moveToNextDesktop()));

    m_openUserGuideAction = new QAction(this);
    m_openUserGuideAction->setText(tr("User Guide"));
    m_openUserGuideAction->setStatusTip(tr("Open user guide"));
    connect(m_openUserGuideAction, SIGNAL(triggered()), this, SLOT(openUserGuide()));

    m_openQuickStartGuideAction = new QAction(this);
    m_openQuickStartGuideAction->setText(tr("Quick Start Guide"));
    m_openQuickStartGuideAction->setStatusTip(tr("Open quick start guide"));
    connect(m_openQuickStartGuideAction, SIGNAL(triggered()), this, SLOT(openQuickStartGuide()));

    m_openShortcutsGuideAction = new QAction(this);
    m_openShortcutsGuideAction->setText(tr("Shortcuts Guide"));
    m_openShortcutsGuideAction->setStatusTip(tr("Open shortcuts guide"));
    connect(m_openShortcutsGuideAction, SIGNAL(triggered()), this, SLOT(openShortcutsGuide()));

#ifdef STARVIEWER_CE
    m_showMedicalDeviceInformationAction = new QAction(this);
    m_showMedicalDeviceInformationAction->setText(tr("Information about use as medical device"));
    connect(m_showMedicalDeviceInformationAction, &QAction::triggered, this, &QApplicationMainWindow::showMedicalDeviceInformationDialogUnconditionally);
#endif // STARVIEWER_CE

    m_logViewerAction = new QAction(this);
    m_logViewerAction->setText(tr("Show Log File"));
    m_logViewerAction->setStatusTip(tr("Show log file"));
    m_logViewerAction->setIcon(QIcon(":/images/logs.png"));
    connect(m_logViewerAction, SIGNAL(triggered()), m_logViewer, SLOT(updateData()));
    connect(m_logViewerAction, SIGNAL(triggered()), m_logViewer, SLOT(exec()));

    m_openReleaseNotesAction = new QAction(this);
    m_openReleaseNotesAction->setText(tr("&Release Notes"));
    m_openReleaseNotesAction->setStatusTip(tr("Show the application's release notes for current version"));
    connect(m_openReleaseNotesAction, SIGNAL(triggered()), SLOT(openReleaseNotes()));

    m_aboutAction = new QAction(this);
    m_aboutAction->setText(tr("&About"));
    m_aboutAction->setStatusTip(tr("Show the application's About box"));
    m_aboutAction->setIcon(QIcon(":/images/starviewer.png"));
    connect(m_aboutAction, SIGNAL(triggered()), SLOT(about()));

    m_closeAction = new QAction(this);
    m_closeAction->setText(tr("&Close"));
    m_closeAction->setShortcuts(ShortcutManager::getShortcuts(Shortcuts::CloseCurrentExtension));
    m_closeAction->setStatusTip(tr("Close current extension page"));
    m_closeAction->setIcon(QIcon(":/images/fileclose.png"));
    connect(m_closeAction, SIGNAL(triggered()), m_extensionWorkspace, SLOT(closeCurrentApplication()));

    m_exitAction = new QAction(this);
    m_exitAction->setText(tr("E&xit"));
    m_exitAction->setShortcuts(ShortcutManager::getShortcuts(Shortcuts::CloseApplication));
    m_exitAction->setStatusTip(tr("Exit the application"));
    m_exitAction->setIcon(QIcon(":/images/exit.png"));
    connect(m_exitAction, SIGNAL(triggered()), qApp, SLOT(closeAllWindows()));

    m_configurationAction = new QAction(this);
    m_configurationAction->setText(tr("&Configuration..."));
    m_configurationAction->setShortcuts(ShortcutManager::getShortcuts(Shortcuts::Preferences));
    m_configurationAction->setStatusTip(tr("Modify %1 configuration").arg(ApplicationNameString));
    m_configurationAction->setIcon(QIcon(":/images/preferences.png"));
    connect(m_configurationAction, SIGNAL(triggered()), SLOT(showConfigurationDialog()));

    m_runDiagnosisTestsAction = new QAction(this);
    m_runDiagnosisTestsAction->setText(tr("&Run Diagnosis Tests"));
    m_runDiagnosisTestsAction->setStatusTip(tr("Run %1 diagnosis tests").arg(ApplicationNameString));
    connect(m_runDiagnosisTestsAction, SIGNAL(triggered()), SLOT(showDiagnosisTestDialog()));}

void QApplicationMainWindow::maximizeMultipleScreens()
{
    ScreenManager screenManager;
    screenManager.maximize(this);
}

void QApplicationMainWindow::moveToDesktop(int screenIndex)
{
    ScreenManager screenManager;
    screenManager.moveToDesktop(this, screenIndex);
}

void QApplicationMainWindow::moveToPreviousDesktop()
{
    ScreenManager screenManager;
    screenManager.moveToPreviousDesktop(this);
}

void QApplicationMainWindow::moveToNextDesktop()
{
    ScreenManager screenManager;
    screenManager.moveToNextDesktop(this);
}

void QApplicationMainWindow::showConfigurationDialog()
{
    QConfigurationDialog configurationDialog;
    configurationDialog.exec();
}

void QApplicationMainWindow::createMenus()
{
    // Menú d'arxiu
    m_fileMenu = menuBar()->addMenu(tr("&File"));
    m_fileMenu->addAction(m_newAction);
#ifndef STARVIEWER_LITE
    m_fileMenu->addAction(m_localDatabaseAction);
#endif
    m_fileMenu->addAction(m_pacsAction);
    m_fileMenu->addAction(m_openDICOMDIRAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_openAction);
    m_fileMenu->addAction(m_openDirAction);
    m_fileMenu->addSeparator();
    m_fileMenu->addAction(m_closeAction);
    m_fileMenu->addAction(m_exitAction);

#ifdef STARVIEWER_LITE
    // No afegim els menús de visualització
#else
    // Accions relacionades amb la visualització
    m_visualizationMenu = menuBar()->addMenu(tr("&Visualization"));

    foreach (QAction *action, m_actionsList)
    {
        m_visualizationMenu->addAction(action);
    }
#endif

    // Menú tools
    m_toolsMenu = menuBar()->addMenu(tr("&Tools"));
    m_languageMenu = m_toolsMenu->addMenu(tr("&Language"));
    createLanguageMenu();
    m_toolsMenu->addAction(m_configurationAction);
    m_toolsMenu->addAction(m_runDiagnosisTestsAction);
    m_externalApplicationsMenu = 0;
    createExternalApplicationsMenu();
    connect(ExternalApplicationsManager::instance(), SIGNAL(onApplicationsChanged()), this, SLOT(createExternalApplicationsMenu()));

    // Menú 'window'
    m_windowMenu = menuBar()->addMenu(tr("&Window"));
    m_moveWindowToDesktopMenu = m_windowMenu->addMenu(tr("Move to Screen"));
    m_moveWindowToDesktopMenu->addAction(m_moveToDesktopAction);
    m_windowMenu->addAction(m_maximizeAction);
    m_windowMenu->addAction(m_moveToPreviousDesktopAction);
    m_windowMenu->addAction(m_moveToNextDesktopAction);
    
    menuBar()->addSeparator();

    // Menú d'ajuda i suport
    m_helpMenu = menuBar()->addMenu(tr("&Help"));
    m_helpMenu->addAction(m_openUserGuideAction);
    m_helpMenu->addAction(m_openQuickStartGuideAction);
    m_helpMenu->addAction(m_openShortcutsGuideAction);
    m_helpMenu->addSeparator();
#ifdef STARVIEWER_CE
    m_helpMenu->addAction(m_showMedicalDeviceInformationAction);
    m_helpMenu->addSeparator();
#endif // STARVIEWER_CE
    m_helpMenu->addAction(m_logViewerAction);
    m_helpMenu->addSeparator();
    m_helpMenu->addAction(m_openReleaseNotesAction);
    m_helpMenu->addSeparator();
    m_helpMenu->addAction(m_aboutAction);
}

void QApplicationMainWindow::createLanguageMenu()
{
    QMap<QString, QString> languages;
    languages.insert("ca_ES", tr("Catalan"));
    languages.insert("es_ES", tr("Spanish"));
    languages.insert("en_GB", tr("English"));

    QSignalMapper *signalMapper = new QSignalMapper(this);
    connect(signalMapper, SIGNAL(mapped(QString)), this, SLOT(switchToLanguage(QString)));

    QActionGroup *actionGroup = new QActionGroup(this);

    QMapIterator<QString, QString> i(languages);
    while (i.hasNext())
    {
        i.next();

        QAction *action = createLanguageAction(i.value(), i.key());
        signalMapper->setMapping(action, i.key());
        connect(action, SIGNAL(triggered()), signalMapper, SLOT(map()));

        actionGroup->addAction(action);
        m_languageMenu->addAction(action);
    }
}

void QApplicationMainWindow::createExternalApplicationsMenu()
{
    QList<ExternalApplication> externalApplications = ExternalApplicationsManager::instance()->getApplications();
    delete m_externalApplicationsMenu;

    if (externalApplications.length() == 0) //If no external applications are defined, do not create the menu;
    {
        m_externalApplicationsMenu = 0;
        return;
    }

    m_externalApplicationsMenu = m_toolsMenu->addMenu(tr("&External applications"));
    m_externalApplicationsMenu->setIcon(QIcon(":/images/system-run.svg"));

    QSignalMapper *signalMapper = new QSignalMapper(m_externalApplicationsMenu);
    connect(signalMapper, SIGNAL(mapped(int)), this, SLOT(launchExternalApplication(int)));

    QVector<QList<QKeySequence>> shortcutVector(12);
    shortcutVector[0] = ShortcutManager::getShortcuts(Shortcuts::ExternalApplication1);
    shortcutVector[1] = ShortcutManager::getShortcuts(Shortcuts::ExternalApplication2);
    shortcutVector[2] = ShortcutManager::getShortcuts(Shortcuts::ExternalApplication3);
    shortcutVector[3] = ShortcutManager::getShortcuts(Shortcuts::ExternalApplication4);
    shortcutVector[4] = ShortcutManager::getShortcuts(Shortcuts::ExternalApplication5);
    shortcutVector[5] = ShortcutManager::getShortcuts(Shortcuts::ExternalApplication6);
    shortcutVector[6] = ShortcutManager::getShortcuts(Shortcuts::ExternalApplication7);
    shortcutVector[7] = ShortcutManager::getShortcuts(Shortcuts::ExternalApplication8);
    shortcutVector[8] = ShortcutManager::getShortcuts(Shortcuts::ExternalApplication9);
    shortcutVector[9] = ShortcutManager::getShortcuts(Shortcuts::ExternalApplication10);
    shortcutVector[10] = ShortcutManager::getShortcuts(Shortcuts::ExternalApplication11);
    shortcutVector[11] = ShortcutManager::getShortcuts(Shortcuts::ExternalApplication12);

    QListIterator<ExternalApplication> i(externalApplications);
    int position = 0;
    while (i.hasNext())
    {
        const ExternalApplication& extApp = i.next();
        QAction* action = new QAction(extApp.getName(),0); //When added to a QMenu, that menu becomes the parent.
        if (position < shortcutVector.size())
        {
            action->setShortcuts(shortcutVector[position]);
        }

        m_externalApplicationsMenu->addAction(action);
        signalMapper->setMapping(action, position);
        connect(action, SIGNAL(triggered()), signalMapper, SLOT(map()));
        position++;
    }
}

QAction* QApplicationMainWindow::createLanguageAction(const QString &language, const QString &locale)
{
    Settings settings;
    QString defaultLocale = settings.getValue(CoreSettings::LanguageLocale).toString();

    QAction *action = new QAction(this);
    action->setText(language);
    action->setStatusTip(tr("Switch to %1 language").arg(language));
    action->setCheckable(true);
    action->setChecked(defaultLocale == locale);

    return action;
}

void QApplicationMainWindow::killBill()
{
    // Eliminem totes les extensions
    this->getExtensionWorkspace()->killThemAll();
    // TODO descarregar tots els volums que tingui el pacient en aquesta finestra
    // quan ens destruim alliberem tots els volums que hi hagi a memòria
    if (this->getCurrentPatient() != NULL)
    {
        foreach (Study *study, this->getCurrentPatient()->getStudies())
        {
            foreach (Series *series, study->getSeries())
            {
                foreach (Identifier id, series->getVolumesIDList())
                {
                    VolumeRepository::getRepository()->deleteVolume(id);
                }
            }
        }
    }
}

void QApplicationMainWindow::switchToLanguage(QString locale)
{
    Settings settings;
    settings.setValue(CoreSettings::LanguageLocale, locale);

    QMessageBox::information(this, tr("Language Switch"), tr("Changes will take effect the next time you start the application"));
}

void QApplicationMainWindow::launchExternalApplication(int i)
{
    QList<ExternalApplication> externalApplications = ExternalApplicationsManager::instance()->getApplications();
    if (i < 0 && i >= externalApplications.size())
    {
        ERROR_LOG("Trying to launch an unexistant external application");
    }
    const ExternalApplication &app = externalApplications.at(i);
    if (!ExternalApplicationsManager::instance()->launch(app))
    {
        //Launch failed.
        QMessageBox::critical(this, tr("External application launch error"), tr("There has been an error launching the external application."));
    }
}

QApplicationMainWindow* QApplicationMainWindow::setPatientInNewWindow(Patient *patient)
{
    QApplicationMainWindow *newMainWindow = openBlankWindow();
    newMainWindow->setPatient(patient);
    QList<Patient*> patientsList;
    patientsList << patient;
    newMainWindow->addPatientsThumbnail(patientsList);
    return newMainWindow;
}

QApplicationMainWindow* QApplicationMainWindow::openBlankWindow()
{
    QApplicationMainWindow *newMainWindow = new QApplicationMainWindow(0);
    newMainWindow->show();

    return newMainWindow;
}

void QApplicationMainWindow::setPatient(Patient *patient)
{
    // Si les dades de pacient són nules, no fem res
    if (!patient)
    {
        DEBUG_LOG("NULL Patient, maybe creating a blank new window");
        return;
    }

    if (this->getCurrentPatient())
    {
        // Primer ens carreguem el pacient
        this->killBill();
        delete m_patient;
        m_patient = NULL;
        DEBUG_LOG("Ja teníem un pacient, l'esborrem.");
    }

    m_patient = patient;
    connectPatientVolumesToNotifier(patient);

    this->setWindowTitle(m_patient->getID() + " : " + m_patient->getFullName());
    enableExtensions();
    m_extensionHandler->getContext().setPatient(patient);
    m_extensionHandler->openDefaultExtension();
}

Patient* QApplicationMainWindow::getCurrentPatient()
{
    return m_patient;
}

unsigned int QApplicationMainWindow::getCountQApplicationMainWindow()
{
    unsigned int count = 0;
    foreach (QWidget *widget, QApplication::topLevelWidgets())
    {
        if (qobject_cast<QApplicationMainWindow*>(widget))
        {
            ++count;
        }
    }

    return count;
}

QList<QApplicationMainWindow*> QApplicationMainWindow::getQApplicationMainWindows()
{
    QList<QApplicationMainWindow*> mainApps;
    foreach (QWidget *widget, qApp->topLevelWidgets())
    {
        QApplicationMainWindow *window = qobject_cast<QApplicationMainWindow*>(widget);
        if (window)
        {
            mainApps << window;
        }
    }
    return mainApps;
}

QApplicationMainWindow* QApplicationMainWindow::getActiveApplicationMainWindow()
{
    return qobject_cast<QApplicationMainWindow*>(QApplication::activeWindow());
}

ExtensionWorkspace* QApplicationMainWindow::getExtensionWorkspace()
{
    return m_extensionWorkspace;
}

void QApplicationMainWindow::closeEvent(QCloseEvent *event)
{
    // \TODO aquí hauríem de controlar si l'aplicació està fent altres tasques pendents que s'haurien de finalitzar abans de tancar
    // l'aplicació com per exemple imatges en descàrrega del PACS o similar.
    // Caldria fer-ho de manera centralitzada.
    event->accept();
}

void QApplicationMainWindow::resizeEvent(QResizeEvent *event)
{
    if (m_isBetaVersion)
    {
        updateBetaVersionTextPosition();
    }
    QMainWindow::resizeEvent(event);
}

void QApplicationMainWindow::showEvent(QShowEvent *event)
{
    m_applicationVersionChecker->showIfCorrect();
    QMainWindow::showEvent(event);
}

void QApplicationMainWindow::about()
{
    QAboutDialog *about = new QAboutDialog(this);
    about->exec();
}

void QApplicationMainWindow::writeSettings()
{
    Settings settings;
    settings.saveGeometry(InterfaceSettings::ApplicationMainWindowGeometry, this);
}

void QApplicationMainWindow::enableExtensions()
{
    foreach (QAction *action, m_actionsList)
    {
        action->setEnabled(true);
    }
}

void QApplicationMainWindow::markAsBetaVersion()
{
    m_isBetaVersion = true;
    m_betaVersionMenuText = new QLabel(menuBar());
    m_betaVersionMenuText->setText("<a href='beta'><img src=':/images/beta-warning.png'></a>&nbsp;<a href='beta'>Beta Version</a>");
    m_betaVersionMenuText->setAlignment(Qt::AlignVCenter);
    connect(m_betaVersionMenuText, SIGNAL(linkActivated(const QString&)), SLOT(showBetaVersionDialog()));
    updateBetaVersionTextPosition();
}

void QApplicationMainWindow::updateBetaVersionTextPosition()
{
    m_betaVersionMenuText->move(this->size().width() - (m_betaVersionMenuText->sizeHint().width() + 10), 5);
}

void QApplicationMainWindow::showBetaVersionDialog()
{
    QMessageBox::warning(this, tr("Beta Version"),
                         tr("<h2>%1</h2>"
                            "<p align='justify'>This is a preview release of %1 used exclusively for testing purposes.</p>"
                            "<p align='justify'>This version is intended for radiologists and our test-team members. "
                            "Users of this version should not expect extensions to function properly.</p>"
                            "<p align='justify'>If you want to help us to improve %1, please report any found bug or "
                            "any feature request you may have by sending an e-mail to: <a href=\"mailto:%2\">%2</a></p>"
                            "<h3>We really appreciate your feedback!</h3>").arg(ApplicationNameString).arg(OrganizationEmailString));
}

void QApplicationMainWindow::readSettings()
{
    Settings settings;
    if (!settings.contains(InterfaceSettings::ApplicationMainWindowGeometry))
    {
        this->showMaximized();
    }
    else
    {
        settings.restoreGeometry(InterfaceSettings::ApplicationMainWindowGeometry, this);
    }
}

void QApplicationMainWindow::connectPatientVolumesToNotifier(Patient *patient)
{
    foreach (Study *study, patient->getStudies())
    {
        foreach (Series *series, study->getSeries())
        {
            foreach (Volume *volume, series->getVolumesList())
            {
                connect(volume, SIGNAL(progress(int)), SLOT(updateVolumeLoadProgressNotification(int)));
            }
        }
    }
}

#ifdef STARVIEWER_CE
void QApplicationMainWindow::showMedicalDeviceInformationDialog()
{
    Settings settings;

    if (!settings.getValue(InterfaceSettings::DontShowMedicalDeviceInformationDialog).toBool())
    {
        showMedicalDeviceInformationDialogUnconditionally();
    }
}

void QApplicationMainWindow::showMedicalDeviceInformationDialogUnconditionally()
{
    QMedicalDeviceInformationDialog *dialog = new QMedicalDeviceInformationDialog(this);
    dialog->exec();
}
#endif // STARVIEWER_CE

void QApplicationMainWindow::newCommandLineOptionsToRun()
{
    QPair<StarviewerApplicationCommandLine::StarviewerCommandLineOption, QString> optionValue;

    // Mentre quedin opcions per processar
    while (StarviewerSingleApplicationCommandLineSingleton::instance()->takeOptionToRun(optionValue))
    {
        switch (optionValue.first)
        {
            case StarviewerApplicationCommandLine::OpenBlankWindow:
                INFO_LOG("Rebut argument de linia de comandes per obrir nova finestra");
                openBlankWindow();
                break;
            case StarviewerApplicationCommandLine::RetrieveStudyByUid:
                INFO_LOG("Received command line argument to retrieve a study by its UID");
                sendRequestRetrieveStudyByUidToLocalStarviewer(optionValue.second);
                break;
            case StarviewerApplicationCommandLine::RetrieveStudyByAccessionNumber:
                INFO_LOG("Rebut argument de linia de comandes per descarregar un estudi a traves del seu accession number");
                sendRequestRetrieveStudyWithAccessionNumberToLocalStarviewer(optionValue.second);
                break;
            default:
                INFO_LOG("Argument de linia de comandes invalid");
                break;
        }
    }
}

void QApplicationMainWindow::sendRequestRetrieveStudyByUidToLocalStarviewer(QString studyInstanceUid)
{
    Settings settings;
    if (settings.getValue(udg::InputOutputSettings::ListenToRISRequests).toBool())
    {
        // TODO Ugly shortcut for #2643. Major refactoring needed to clean this (see #2764).
        DicomMask mask;
        mask.setStudyInstanceUID(studyInstanceUid);
        QueryScreenSingleton::instance()->getRISRequestManager()->processRISRequest(mask);
    }
    else
    {
        QMessageBox::information(this, ApplicationNameString,
                                 tr("Please activate \"Listen to RIS requests\" option in %1 configuration to retrieve studies from SAP.")
                                 .arg(ApplicationNameString));
    }
}

void QApplicationMainWindow::sendRequestRetrieveStudyWithAccessionNumberToLocalStarviewer(QString accessionNumber)
{
    Settings settings;
    if (settings.getValue(udg::InputOutputSettings::ListenToRISRequests).toBool())
    {
        RISRequestWrapper().sendRequestToLocalStarviewer(accessionNumber);
    }
    else
    {
        // TODO:S'hauria de fer un missatge més genèric
        QMessageBox::information(this, ApplicationNameString,
                                 tr("Please activate \"Listen to RIS requests\" option in %1 configuration to retrieve studies from SAP.")
                               .arg(ApplicationNameString));
    }
}

void QApplicationMainWindow::updateVolumeLoadProgressNotification(int progress)
{
    //m_progressDialog->setValue(progress);
}

void QApplicationMainWindow::openUserGuide()
{
    Settings settings;
    QString defaultLocale = settings.getValue(CoreSettings::LanguageLocale).toString();
    QString prefix = defaultLocale.left(2).toUpper();
    QString userGuideFilePath = QCoreApplication::applicationDirPath() + "/" + prefix + "_Starviewer_User_guide.pdf";
    QDesktopServices::openUrl(QUrl::fromLocalFile(userGuideFilePath));
}

void QApplicationMainWindow::openQuickStartGuide()
{
    Settings settings;
    QString defaultLocale = settings.getValue(CoreSettings::LanguageLocale).toString();
    QString prefix = defaultLocale.left(2).toUpper();
    QString userGuideFilePath = QCoreApplication::applicationDirPath() + "/" + prefix + "_Starviewer_Quick_start_guide.pdf";
    QDesktopServices::openUrl(QUrl::fromLocalFile(userGuideFilePath));
}

void QApplicationMainWindow::openShortcutsGuide()
{
    Settings settings;
    QString defaultLocale = settings.getValue(CoreSettings::LanguageLocale).toString();
    QString prefix = defaultLocale.left(2).toUpper();
    QString userGuideFilePath = QCoreApplication::applicationDirPath() + "/" + prefix + "_Starviewer_Shortcuts_guide.pdf";
    QDesktopServices::openUrl(QUrl::fromLocalFile(userGuideFilePath));
}

void QApplicationMainWindow::showDiagnosisTestDialog()
{
    QDiagnosisTest qDiagnosisTest;
    qDiagnosisTest.execAndRunDiagnosisTest();
}

void QApplicationMainWindow::openReleaseNotes()
{
    m_applicationVersionChecker->showLocalReleaseNotes();
}

void QApplicationMainWindow::computeDefaultToolTextSize()
{
    ApplicationStyleHelper().recomputeStyleToScreenOfWidget(this);
}

void QApplicationMainWindow::addPatientsThumbnail(QList<Patient*> patientsList)
{
    m_DockImageThumbnail->addPatientsThumbmailList(patientsList);
#ifdef DOCKRIGHT
    m_DockImageThumbnailRight->addPatientsThumbmailList(patientsList);
#endif
}

void QApplicationMainWindow::closePatient()
{
    if (m_DockImageThumbnail)
    {
        m_DockImageThumbnail->mainAppclearThumbnail();
    }
#ifdef DOCKRIGHT
    if (m_DockImageThumbnailRight)
    {
        m_DockImageThumbnailRight->mainAppclearThumbnail();
    }
#endif
    this->killBill();
    this->setWindowTitle("NULL");
    if (m_patient)
    {
        m_patient->clearAllStudy();
        delete m_patient;
        m_patient = NULL;
    }
}

//ExtensionHandler* QApplicationMainWindow::getExtensionHandler()
//{
//    return  m_extensionHandler;
//}

QWidget *QApplicationMainWindow::currentWidgetOfExtensionWorkspace()
{
    return m_extensionWorkspace->currentWidget();
}

void QApplicationMainWindow::closeCurrentPatient()
{
    //connect(m_closeAction, SIGNAL(triggered()), m_extensionWorkspace, SLOT(closeCurrentApplication()));
    //----------20200921---------------------------------------------------------------------------------
    //connect(m_closeAction, SIGNAL(triggered()), m_DockImageThumbnail, SLOT(mainAppclearThumbnail()));
    //----------------------------------------------------------------------------------------------------
    if (m_extensionWorkspace->currentWidget())
    {
        m_extensionWorkspace->closeCurrentApplication();
        m_DockImageThumbnail->mainAppclearThumbnail();
#ifdef DOCKRIGHT
        m_DockImageThumbnailRight->mainAppclearThumbnail();
#endif
        this->killBill();
        this->setWindowTitle("NULL");
        //m_patient->setID("NULL");
        //m_patient->setFullName("NULL");
        if (m_patient)
        {
            //m_patient->clearAllStudy();
            delete m_patient;
            m_patient = NULL;
        }

    }
}
///------------------------------------------------------------------------------------------------------------

void QApplicationMainWindow::showhideDockImage()
{
    static bool flag = true;
    if (flag)
    {
        m_DockImageThumbnail->hide();
        flag = false;
    }
    else
    {
        m_DockImageThumbnail->show();
        flag = true;
    }
}

void QApplicationMainWindow::updateActiveFromStaticViewerMenu(const QList<Volume*> &volumes)
{
    QWidget* widget = currentWidgetOfExtensionWorkspace();
    QString className = widget->metaObject()->className();
    QString str = className.section("::", 1, 1);
    ExtensionMediator *mediator = ExtensionMediatorFactory::instance()->create(str);
    if (mediator)
    {
        if (widget)
        {
            Volume *volume = volumes.at(0);
            ExtensionWorkspace *extensionWorkspace = getExtensionWorkspace();
            int extensionIndex = extensionWorkspace->currentIndex();
            if (extensionWorkspace->tabText(extensionIndex).contains("3D Viewer"))
            {
                if (!volume)
                {
                    QMessageBox::warning(0, "3D-Viewer", ("3D-Viewer: No image is selected!!"));
                    delete mediator;
                    return;
                }
                //if (!volume->is3Dimage())
                {
                    QMessageBox::warning(0, "3D-Viewer", ("The selected item : 3D-Viewer fail!!! images < 5 or SliceThickness = 0.0"));
                    delete mediator;
                    return;
                }
                extensionWorkspace->setTabText(extensionIndex, "3D Viewer#Series:" + volume->getSeries()->getSeriesNumber());
            }
            //mediator->executionCommand(widget, volume);
        }
        delete mediator;
    }
}

void QApplicationMainWindow::openCommandDirDcm(QString rootPath)
{
    QStringList dirsList, filenames;
    QDir rootDir(rootPath);
    if (rootDir.exists())
    {
        // We add the current directory to the list
        dirsList << rootPath;
        foreach(const QString &dirName, dirsList)
        {
            filenames << generateFilenames(dirName);
        }
    }
    if (!filenames.isEmpty())
    {
        m_extensionHandler->closeCurrentPatient();
        m_extensionHandler->processCommandInput(filenames);
    }
}

QStringList QApplicationMainWindow::generateFilenames(const QString &dirPath)
{
    QStringList list;
    //We check that the directory has files
    QDir dir(dirPath);
    QFileInfoList fileInfoList = dir.entryInfoList(QDir::Files);

    QString suffix;
    //We add to the list each of the absolute paths of the files contained in the directory
    foreach(const QFileInfo &fileInfo, fileInfoList)
    {
        suffix = fileInfo.suffix();
        if ((suffix.length() > 0 && suffix.toLower() == "dcm") || suffix.length() == 0)
        {
            list << fileInfo.absoluteFilePath();
        }
    }

    return list;
}
}; // end namespace udg
